#ifndef OBJECT_STORE_MOCK_H
#define OBJECT_STORE_MOCK_H

#include <unordered_map>
#include <fdsp/FDSP_types.h>
#include <hash/MurmurHash3.h>

#include <iostream>  // NOLINT(*)
#include <unordered_map>
#include <vector>
#include <string>
#include <set>
#include <atomic>
#include <boost/shared_ptr.hpp>

#include <util/Log.h>
#include <fds_assert.h>
#include <fdsp/FDSP_types.h>
#include <fdsp/FDSP_DataPathReq.h>
#include <fdsp/FDSP_DataPathResp.h>
#include <concurrency/Mutex.h>
#include <fds-probe/fds_probe.h>
#include <StorMgrVolumes.h>
#include <dlt.h>
#include <concurrency/ThreadPool.h>
#include <SmObjDb.h>
#include <StorMgr.h>
#include <fds_timestamp.h>

using namespace FDS_ProtocolInterface;  // NOLINT
namespace fds {

class MObjStore : public ObjectStorMgr {
 public:
    MObjStore(const std::string &prefix)
     : lock_("MObjStoreMutex"),
       dlt_(8, 3, 1)
    {
        threadpool_.reset(new fds_threadpool(2));
        smObjDb = new SmObjDb(this, prefix, g_fdslog);
        tokenStateDb_.reset(new kvstore::TokenStateDB());
        for (fds_token_id tok = 0; tok < 256; tok++) {
            tokenStateDb_->addToken(tok);
        }
        prefix_ = prefix;
    }
    ~MObjStore() {

    }
    void putObject(const std::string &data)
    {
        fds_mutex::scoped_lock l(lock_);
        ObjectID oid;
        MurmurHash3_x64_128(data.c_str(),
                data.size() + 1,
                0,
                &oid);

        writeObject(oid, data);
    }
    /**
     * This variant doens't do content hashing
     * @param oid
     * @param data
     */
    void putObject(const ObjectID& oid, const std::string &data)
    {
        writeObject(oid, data);
    }

    bool getObject(const ObjectID &oid, std::string &data) {

         fds_mutex::scoped_lock l(lock_);
         auto itr = object_db_.find(oid);
         if (itr == object_db_.end()) {
             return false;
         }
         data = itr->second;
         return true;
    }

    void writeObject(const ObjectID& oid, const std::string &data)
    {
        OpCtx opCtx(OpCtx::PUT, get_fds_timestamp_ms());
        obj_phy_loc_t obj_phy_loc;
        meta_vol_io_t vol;
        vol.vol_uuid = 1;
        obj_phy_loc.obj_tier = diskio::DataTier::diskTier;

        Error err = this->writeObjectMetaData(opCtx, oid, data.size(), &obj_phy_loc,
                false, diskio::DataTier::flashTier, &vol);
        fds_verify(err == ERR_OK);
        writeObjectData(oid, data);
    }

    void writeObjectData(const ObjectID& oid, const std::string &data)
    {
        object_db_[oid] = data;
        tokens_.insert(dlt_.getToken(oid));
        LOGDEBUG << prefix_ << " oid: " << oid << " data: " << data;
    }

    std::set<fds_token_id> getTokens() {
        fds_mutex::scoped_lock l(lock_);
        return tokens_;
    }

    virtual Error readObject(const SmObjDb::View& view,
            const ObjectID   &objId,
            ObjMetaData      &objMetadata,
            ObjectBuf        &objCompData,
            diskio::DataTier &tier) {
        auto itr = object_db_.find(objId);
        if (itr == object_db_.end()) {
            return ERR_DISK_READ_FAILED;
        }
        Error err = smObjDb->get(objId, objMetadata);
        if (err != ERR_OK) {
            return ERR_DISK_READ_FAILED;
        }
        boost::shared_ptr<std::string> str(new std::string(itr->second));
        objCompData = ObjectBuf(str);
        return ERR_OK;
    }

    virtual fds_token_id getTokenId(const ObjectID& objId) override {
        return dlt_.getToken(objId);
    }
    virtual const DLT* getDLT() override {
        return &dlt_;
    }

    int totalObjectCnt() {
        fds_mutex::scoped_lock l(lock_);
        return object_db_.size();
    }


    void putTokenObjectsInternal(SmIoReq* ioReq)
    {
        fds_mutex::scoped_lock l(lock_);

        Error err(ERR_OK);
        SmIoPutTokObjectsReq *putTokReq = static_cast<SmIoPutTokObjectsReq*>(ioReq);
        FDSP_MigrateObjectList &objList = putTokReq->obj_list;

        for (auto obj : objList) {
            ObjectID objId(obj.meta_data.object_id.digest);

            ObjMetaData objMetadata;
            err = smObjDb->get(objId, objMetadata);
            if (err != ERR_OK && err != ERR_DISK_READ_FAILED) {
                fds_assert(!"ObjMetadata read failed" );
                LOGERROR << "Failed to write the object: " << objId;
                break;
            }
            err = ERR_OK;

            /* Apply metadata */
            objMetadata.apply(obj.meta_data);

            LOGDEBUG << objMetadata.logString();

            if (objMetadata.dataPhysicallyExists()) {
                /* write metadata */
                smObjDb->put(OpCtx(OpCtx::COPY), objId, objMetadata);
                /* No need to write the object data.  It already exits */
                continue;
            }

            /* Moving the data to not incur copy penalty */
            DBG(std::string temp_data = obj.data);
            ObjectBuf objData;
            *(objData.data) = std::move(obj.data);
            fds_assert(temp_data == *(objData.data));
            obj.data.clear();

            /* write data to storage tier */
            writeObjectData(objId, *(objData.data));

            /* write the metadata */
            obj_phy_loc_t phy_loc;
            phy_loc.obj_tier = diskio::DataTier::diskTier;
            objMetadata.updatePhysLocation(&phy_loc);
            smObjDb->put_(objId, objMetadata);
        }
        putTokReq->response_cb(err, putTokReq);

    }

    void getTokenObjectsInternal(SmIoReq* ioReq)
    {
        fds_mutex::scoped_lock l(lock_);

        Error err(ERR_OK);
        SmIoGetTokObjectsReq *getTokReq = static_cast<SmIoGetTokObjectsReq*>(ioReq);

        smObjDb->iterRetrieveObjects(getTokReq->token_id,
                getTokReq->max_size, getTokReq->obj_list, getTokReq->itr);

        getTokReq->response_cb(err, getTokReq);
        /* NOTE: We expect the caller to free up ioReq */
    }

//    void putTokenObjectsInternal(SmIoReq* ioReq)
//    {
//        Error err(ERR_OK);
//        fds_mutex::scoped_lock l(lock_);
//
//        SmIoPutTokObjectsReq *putTokReq = static_cast<SmIoPutTokObjectsReq*>(ioReq);
//        FDSP_MigrateObjectList &objList = putTokReq->obj_list;
//
//        for (auto obj : objList) {
//            ObjectID objId(obj.meta_data.object_id.digest);
//
//            object_db_[objId] = obj.data;
//            token_db_[putTokReq->token_id].insert(objId);
//        }
//        putTokReq->response_cb(err, putTokReq);
//    }
//
//    void getTokenObjectsInternal(SmIoReq* ioReq)
//    {
//        Error err(ERR_OK);
//        fds_mutex::scoped_lock l(lock_);
//
//        SmIoGetTokObjectsReq *getTokReq = static_cast<SmIoGetTokObjectsReq*>(ioReq);
//        fds_token_id token_id = getTokReq->token_id;
//        auto obj_ids = token_db_[token_id];
//        for (auto obj_id : obj_ids) {
//            std::string obj_data = object_db_[obj_id];
//            FDSP_MigrateObjectData mig_obj;
//            mig_obj.meta_data.token_id = token_id;
//            mig_obj.meta_data.object_id.digest = std::string((const char *)obj_id.GetId(), (size_t)obj_id.GetLen());
////            mig_obj.meta_data.object_id.hash_high = obj_id.GetHigh();
////            mig_obj.meta_data.object_id.hash_low = obj_id.GetLow();
//            mig_obj.meta_data.obj_len = obj_data.size();
//            mig_obj.data = obj_data;
//            getTokReq->obj_list.push_back(mig_obj);
//        }
//        getTokReq->itr.objId = SMTokenItr::itr_end;
//
//        getTokReq->response_cb(err, getTokReq);
//    }

    void
    snapshotTokenInternal(SmIoReq* ioReq)
    {
        Error err(ERR_OK);
        SmIoSnapshotObjectDB *snapReq = static_cast<SmIoSnapshotObjectDB*>(ioReq);

        leveldb::DB *db;
        leveldb::ReadOptions options;

        smObjDb->snapshot(snapReq->token_id, db, options);

        snapReq->smio_snap_resp_cb(err, snapReq, options, db);
    }

    void
    applySyncMetadataInternal(SmIoReq* ioReq)
    {
        fds_mutex::scoped_lock l(lock_);
        SmIoApplySyncMetadata *applyMdReq =  static_cast<SmIoApplySyncMetadata*>(ioReq);

        LOGDEBUG << prefix_ << " oid: " << applyMdReq->md.object_id.digest;

        Error e = smObjDb->putSyncEntry(ObjectID(applyMdReq->md.object_id.digest),
                applyMdReq->md, applyMdReq->dataExists);
        if (e != ERR_OK) {
            fds_assert(!"error");
            LOGERROR << "Error in applying sync metadata.  Object Id: "
                    << applyMdReq->md.object_id.digest;
        }
        applyMdReq->smio_sync_md_resp_cb(e, applyMdReq);
    }

    void
    resolveSyncEntryInternal(SmIoReq* ioReq)
    {
        fds_mutex::scoped_lock l(lock_);
        SmIoResolveSyncEntry *resolve_entry =  static_cast<SmIoResolveSyncEntry*>(ioReq);
        Error e = smObjDb->resolveEntry(resolve_entry->object_id);
        if (e != ERR_OK) {
            fds_assert(!"error");
            LOGERROR << "Error in resolving metadata entry.  Object Id: "
                    << resolve_entry->object_id;
        }
        resolve_entry->smio_resolve_resp_cb(e, resolve_entry);
    }

    void
    applyObjectDataInternal(SmIoReq* ioReq)
    {
        fds_mutex::scoped_lock l(lock_);
        SmIoApplyObjectdata *applydata_entry =  static_cast<SmIoApplyObjectdata*>(ioReq);
        ObjectID objId = applydata_entry->obj_id;
        ObjMetaData objMetadata;

        Error err = smObjDb->get(objId, objMetadata);
        if (err != ERR_OK && err != ERR_DISK_READ_FAILED) {
            fds_assert(!"ObjMetadata read failed" );
            LOGERROR << "Failed to write the object: " << objId;
            applydata_entry->smio_apply_data_resp_cb(err, applydata_entry);
        }

        err = ERR_OK;

        if (objMetadata.dataPhysicallyExists()) {
            /* No need to write the object data.  It already exits */
            LOGDEBUG << "Object already exists. Id: " << objId;
            applydata_entry->smio_apply_data_resp_cb(err, applydata_entry);
        }

        /* Moving the data to not incur copy penalty */
        DBG(std::string temp_data = applydata_entry->obj_data);
        ObjectBuf objData;
        *(objData.data) = std::move(applydata_entry->obj_data);
        fds_assert(temp_data == *(objData.data));
        applydata_entry->obj_data.clear();

        /* write data to storage tier */
        /* Makeup some data */
        obj_phy_loc_t phys_loc;
        phys_loc.obj_stor_offset = 10;
        phys_loc.obj_file_id = 1;
        phys_loc.obj_tier = diskio::diskTier;
        writeObjectData(objId, *(objData.data));

        /* write the metadata */
        objMetadata.updatePhysLocation(&phys_loc);
        smObjDb->put(OpCtx(OpCtx::SYNC), objId, objMetadata);

        applydata_entry->smio_apply_data_resp_cb(err, applydata_entry);
    }

    void
    readObjectDataInternal(SmIoReq* ioReq)
    {
        fds_mutex::scoped_lock l(lock_);
        SmIoReadObjectdata *read_entry =  static_cast<SmIoReadObjectdata*>(ioReq);
        ObjectBuf        objData;
        ObjMetaData objMetadata;
        diskio::DataTier tierUsed;

        Error err = readObject(SmObjDb::NON_SYNC_MERGED, read_entry->getObjId(),
                objMetadata, objData, tierUsed);
        if (err != ERR_OK) {
            fds_assert(!"failed to read");
            LOGERROR << "Failed to read object id: " << read_entry->getObjId();
        }
        // TODO(Rao): use std::move here
        read_entry->obj_data.data = *(objData.data);
        fds_assert(read_entry->obj_data.data.size() > 0);

        read_entry->smio_readdata_resp_cb(err, read_entry);
    }
    void
    readObjectMetadataInternal(SmIoReq* ioReq)
    {
        fds_mutex::scoped_lock l(lock_);
        SmIoReadObjectMetadata *read_entry =  static_cast<SmIoReadObjectMetadata*>(ioReq);
        ObjMetaData objMetadata;

        Error err = smObjDb->get(read_entry->getObjId(), objMetadata);
        if (err != ERR_OK) {
            fds_assert(!"failed to read");
            LOGERROR << "Failed to read object metadata id: " << read_entry->getObjId();
            read_entry->smio_readmd_resp_cb(err, read_entry);
        }

        objMetadata.extractSyncData(read_entry->meta_data);
        read_entry->smio_readmd_resp_cb(err, read_entry);
    }

    virtual Error enqueueMsg(fds_volid_t volId, SmIoReq* io)
    {
        switch (io->io_type) {
        case FDS_SM_WRITE_TOKEN_OBJECTS:
            threadpool_->schedule(&MObjStore::putTokenObjectsInternal, this, io);
            break;
        case FDS_SM_READ_TOKEN_OBJECTS:
            threadpool_->schedule(&MObjStore::getTokenObjectsInternal, this, io);
            break;
        case FDS_SM_SNAPSHOT_TOKEN:
        {
            threadpool_->schedule(&MObjStore::snapshotTokenInternal, this, io);
            break;
        }
        case FDS_SM_SYNC_APPLY_METADATA:
        {
            threadpool_->schedule(&MObjStore::applySyncMetadataInternal, this, io);
            break;
        }
        case FDS_SM_SYNC_RESOLVE_SYNC_ENTRY:
        {
            threadpool_->schedule(&MObjStore::resolveSyncEntryInternal, this, io);
            break;
        }
        case FDS_SM_APPLY_OBJECTDATA:
        {
            threadpool_->schedule(&MObjStore::applyObjectDataInternal, this, io);
            break;
        }
        case FDS_SM_READ_OBJECTDATA:
        {
            threadpool_->schedule(&MObjStore::readObjectDataInternal, this, io);
            break;
        }
        case FDS_SM_READ_OBJECTMETADATA:
        {
            threadpool_->schedule(&MObjStore::readObjectMetadataInternal, this, io);
            break;
        }
        default:
            fds_assert(!"Unknown message");
            break;
        }
        return ERR_OK;
    }

 private:
    fds_mutex  lock_;
    fds_threadpoolPtr threadpool_;
    DLT dlt_;
    std::unordered_map<ObjectID, std::string, ObjectHash> object_db_;
    std::set<fds_token_id> tokens_;
    std::string prefix_;
};

typedef boost::shared_ptr<MObjStore> MObjStorePtr;
}   // namespace fds
#endif
