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
        meta_obj_map_t loc;
        loc.obj_tier = diskio::DataTier::diskTier;
        Error err = smObjDb->writeObjectLocation(oid, &loc, false);
        fds_verify(err == ERR_OK);
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
            ObjectBuf        &objCompData,
            diskio::DataTier &tier) {
        fds_mutex::scoped_lock l(lock_);
        auto itr = object_db_.find(objId);
        if (itr == object_db_.end()) {
            return ERR_SM_OBJ_METADATA_NOT_FOUND;
        }
        objCompData = ObjectBuf(itr->second);
        return ERR_OK;
    }

    virtual fds_token_id getTokenId(const ObjectID& objId) override {
        return dlt_.getToken(objId);
    }
    virtual bool isTokenInSyncMode(const fds_token_id &tokId) override {
        // fds_assert(!"not implemented");
        return false;
    }
    virtual uint64_t getTokenSyncTimeStamp(const fds_token_id &tokId) override {
        fds_assert(!"not implemented");
        return 0;
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
        Error err(ERR_OK);
        SmIoPutTokObjectsReq *putTokReq = static_cast<SmIoPutTokObjectsReq*>(ioReq);
        FDSP_MigrateObjectList &objList = putTokReq->obj_list;

        for (auto obj : objList) {
            ObjectID objId(obj.meta_data.object_id.digest);
            /* TODO: Update obj metadata */

            /* Doing a look up before a commit a write */
            /* TODO:  Ideally if writeObject returns us an error for duplicate
             * write we don't need this check
             */
            diskio::MetaObjMap objMap;
            err = smObjDb->readObjectLocations(SmObjDb::NON_SYNC_MERGED, objId, objMap);
            if (err == ERR_OK) {
                continue;
            }
            /* NOTE: Not checking for other errors.  If read failed then write should also
             * fail
             */
            err = ERR_OK;

            /* Moving the data to not incur copy penalty */
            DBG(std::string temp_data = obj.data);
            ObjectBuf objData;
            objData.data = std::move(obj.data);
            objData.size = objData.data.size();
            fds_assert(temp_data == objData.data);
            obj.data.clear();

            writeObject(objId, objData.data);
        }
        putTokReq->response_cb(err, putTokReq);

    }

    void getTokenObjectsInternal(SmIoReq* ioReq)
    {
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
        SmIoApplySyncMetadata *applyMdReq =  static_cast<SmIoApplySyncMetadata*>(ioReq);

        LOGDEBUG << prefix_ << " oid: " << applyMdReq->md.object_id.digest;

        Error e = smObjDb->putSyncEntry(ObjectID(applyMdReq->md.object_id.digest),
                applyMdReq->md);
        if (e != ERR_OK) {
            fds_assert(!"error");
            LOGERROR << "Error in applying sync metadata.  Object Id: "
                    << applyMdReq->md.object_id.digest;
        }
        // TODO(Rao): return missing objects
        applyMdReq->smio_sync_md_resp_cb(e, applyMdReq, std::set<ObjectID>());
    }

    void
    resolveSyncEntryInternal(SmIoReq* ioReq)
    {
        SmIoResolveSyncEntry *resolve_entry =  static_cast<SmIoResolveSyncEntry*>(ioReq);
        Error e = smObjDb->resolveEntry(resolve_entry->object_id);
        if (e != ERR_OK) {
            fds_assert(!"error");
            LOGERROR << "Error in resolving metadata entry.  Object Id: "
                    << resolve_entry->object_id;
        }
        resolve_entry->smio_resolve_resp_cb(e, resolve_entry);
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
