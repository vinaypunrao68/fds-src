#include <SmObjDb.h>
#include <StorMgr.h>

namespace fds {

extern ObjectStorMgr *objStorMgr;

SmObjDb::SmObjDb(ObjectStorMgr *storMgr,
        std::string stor_prefix_,
        fds_log* _log) {
    SetLog(_log);
    objStorMgr = storMgr;
    stor_prefix = stor_prefix_;
    smObjDbMutex = new fds_mutex("SmObjDb Mutex");
}

SmObjDb::~SmObjDb() {
    delete smObjDbMutex;
}

fds_token_id SmObjDb::GetSmObjDbId(const fds_token_id &tokId) const
{
    return tokId & SM_TOKEN_MASK;
}

ObjectDB *SmObjDb::openObjectDB(fds_token_id tokId) {
    ObjectDB *objdb = NULL;
    fds_token_id dbId = GetSmObjDbId(tokId);

    smObjDbMutex->lock();
    if ( (objdb = tokenTbl[dbId]) == NULL ) {
        // Create leveldb
        std::string filename= stor_prefix + "SNodeObjIndex_" + std::to_string(dbId);
        objdb  = new ObjectDB(filename);
        tokenTbl[dbId] = objdb;
    }
    smObjDbMutex->unlock();

    return objdb;
}

void SmObjDb::closeObjectDB(fds_token_id tokId) {
    fds_token_id dbId = GetSmObjDbId(tokId);
    ObjectDB *objdb = NULL;

    smObjDbMutex->lock();
    if ( (objdb = tokenTbl[dbId]) == NULL ) {
        smObjDbMutex->unlock();
        return;
    }
    tokenTbl[dbId] = NULL;
    smObjDbMutex->unlock();
    delete objdb;
}

ObjectDB *SmObjDb::getObjectDB(fds_token_id tokId) {
    ObjectDB *objdb = NULL;
    fds_token_id dbId = GetSmObjDbId(tokId);

    smObjDbMutex->lock();
    objdb = tokenTbl[dbId];
    smObjDbMutex->unlock();
    return objdb;
}

fds::Error SmObjDb::Get(const ObjectID& obj_id, ObjectBuf& obj_buf) {
    fds_token_id tokId = objStorMgr->getDLT()->getToken(obj_id);
    fds::Error err = ERR_OK;
    ObjectDB *odb = getObjectDB(tokId);
    if (odb) {
        err =  odb->Get(obj_id, obj_buf);
    } else {
        odb = openObjectDB(tokId);
        err =  odb->Get(obj_id, obj_buf);
    }
    return err;
}

fds::Error SmObjDb::Put(const ObjectID& obj_id, ObjectBuf& obj_buf) {
    fds_token_id tokId = objStorMgr->getDLT()->getToken(obj_id);
    fds::Error err = ERR_OK;
    ObjectDB *odb = getObjectDB(tokId);
    if (odb) {
        err =  odb->Put(obj_id, obj_buf);
    } else {
        odb = openObjectDB(tokId);
        err =  odb->Put(obj_id, obj_buf);
    }
    DBG(LOGDEBUG << "token: " << tokId <<  " dbId: " << GetSmObjDbId(tokId)
            << " Obj id: " << obj_id);
    return err;
}

fds::Error SmObjDb::get(const ObjectID& obj_id, SmObjMetadata& md)
{
    Error err = ERR_OK;

    fds_token_id tokId = objStorMgr->getDLT()->getToken(obj_id);
    ObjectDB *odb = getObjectDB(tokId);
    if (!odb) {
        odb = openObjectDB(tokId);
    }

    OnDiskSmObjMetadata disk_md;
    ObjectBuf buf;
    err = odb->Get(obj_id, buf);
    if (err != ERR_OK) {
        /* Object not found. Return. */
        return err;
    }

    disk_md.unmarshall(const_cast<char*>(buf.data.data()), buf.data.size());

    if (objStorMgr->isTokenInSyncMode(tokId)) {
        if (disk_md.metadataExists() &&
            disk_md.metadataOlderThan(objStorMgr->getTokenSyncTimeStamp(tokId))) {
            /* Existing entry is older than sync time stamp.  Demote it
             * as sync entry
             */
            disk_md.setSyncMetaData(disk_md.getMetaData());
            disk_md.removeMetaData();

            /* Store the data back */
            buf.resize(disk_md.marshalledSize());
            size_t sz = disk_md.marshall(const_cast<char*>(buf.data.data()), buf.data.size());
            fds_assert(sz == buf.data.size());
            // TODO(Rao): Return merge of metadata and sync metadata.  For now returning
            // object not found
            err = ERR_SM_OBJ_METADATA_NOT_FOUND;
        } else if (disk_md.metadataExists()) {
            md = disk_md.getMetaData();
        } else {
            err = ERR_SM_OBJ_METADATA_NOT_FOUND;
        }
    }  else {
        fds_assert(disk_md.metadataExists() &&
                !disk_md.syncMetadataExists());
        md = disk_md.getMetaData();
    }
    return err;
}

fds::Error SmObjDb::put(const ObjectID& obj_id, const SmObjMetadata& md)
{
    Error err = ERR_OK;

    fds_token_id tokId = objStorMgr->getDLT()->getToken(obj_id);
    ObjectDB *odb = getObjectDB(tokId);
    if (!odb) {
        odb = openObjectDB(tokId);
    }

    OnDiskSmObjMetadata disk_md;
    ObjectBuf buf;
    if (objStorMgr->isTokenInSyncMode(tokId)) {
        err = odb->Get(obj_id, buf);
        if (err == ERR_OK) {
            disk_md.unmarshall(const_cast<char*>(buf.data.data()), buf.data.size());
            if (disk_md.metadataExists() &&
                disk_md.metadataOlderThan(objStorMgr->getTokenSyncTimeStamp(tokId))) {
                /* Existing entry is older than sync time stamp.  Demote it
                 * as sync entry
                 */
                disk_md.setSyncMetaData(disk_md.getMetaData());
                disk_md.removeMetaData();
            }
        }
    }
    disk_md.setMetaData(md);

    /* Store the data back */
    buf.resize(disk_md.marshalledSize());
    size_t sz = disk_md.marshall(const_cast<char*>(buf.data.data()), buf.data.size());
    fds_assert(sz == buf.data.size());
    err = odb->Put(obj_id, buf);
    return err;
}

fds::Error SmObjDb::putSyncEntry(const ObjectID& obj_id,
        const SmObjMetadata& md)
{
    Error err = ERR_OK;
    fds_token_id tokId = objStorMgr->getDLT()->getToken(obj_id);
    if (!objStorMgr->isTokenInSyncMode(tokId)) {
        LOGERROR << "Failed.  Not in sync mode.  id: " << obj_id;
        fds_assert(!"Not in sync mode");
        return ERR_SM_NOT_IN_SYNC_MODE;
    }

    ObjectDB *odb = getObjectDB(tokId);
    if (!odb) {
        odb = openObjectDB(tokId);
    }

    /* TODO: Version handling */
    OnDiskSmObjMetadata disk_md;
    ObjectBuf buf;
    err = odb->Get(obj_id, buf);
    if (err == ERR_OK) {
        disk_md.unmarshall(const_cast<char*>(buf.data.data()), buf.data.size());
        if (disk_md.metadataExists() &&
            disk_md.metadataOlderThan(md.get_modification_ts())) {
            /* Existing entry is outdated.  Purge it */
            disk_md.removeMetaData();
        }
    }
    /* Add sync entry */
    disk_md.setSyncMetaData(md);

    /* Store the data back */
    buf.resize(disk_md.marshalledSize());
    size_t sz = disk_md.marshall(const_cast<char*>(buf.data.data()), buf.data.size());
    fds_assert(sz == buf.data.size());
    err = odb->Put(obj_id, buf);
    return err;
}

void SmObjDb::iterRetrieveObjects(const fds_token_id &token,
        const size_t &max_size,
        FDSP_MigrateObjectList &obj_list,
        SMTokenItr &itr)
{
    fds_uint32_t tot_msg_len = 0;
    diskio::DataTier tierUsed;
    fds::Error err = ERR_OK;
    ObjectID objId;
    ObjectLess id_less;
    ObjectDB *odb = getObjectDB(token);

    if (odb == NULL ) {
        itr.objId = SMTokenItr::itr_end;
        return;
    }

    DBG(int obj_itr_cnt = 0);

    ObjectID start_obj_id, end_obj_id;
    objStorMgr->getDLT()->getTokenObjectRange(token, start_obj_id, end_obj_id);
    // If the iterator is non-zero then use that as a sarting point for the scan else make up a start from token
    if ( itr.objId != NullObjectID) {
        start_obj_id = itr.objId;
    }
    DBG(LOGDEBUG << "token: " << token << " begin: "
            << start_obj_id << " end: " << end_obj_id);

    leveldb::Slice startSlice((const char *)&start_obj_id, start_obj_id.GetLen());

    boost::shared_ptr<leveldb::Iterator> dbIter(odb->GetDB()->NewIterator(odb->GetReadOptions()));
    leveldb::Options options_ = odb->GetOptions();

    memcpy(&objId , &start_obj_id, start_obj_id.GetLen());
    // TODO(Rao): This iterator is very inefficient. We're always
    // iterating through all of the objects in this DB even if they
    // are not part of the token we care about.
    // Ideally, we can iterate sorted keys so that we can seek to
    // the object id range we care about.
    for(dbIter->Seek(startSlice); dbIter->Valid(); dbIter->Next())
    {
        ObjectBuf        objData;
        // Read the record
        memcpy(&objId , dbIter->key().data(), objId.GetLen());
        DBG(LOGDEBUG << "Checking objectId: " << objId << " for token range: " << token);

        // TODO: process the key/data
        if ((objId == start_obj_id || id_less(start_obj_id, objId)) &&
            (objId == end_obj_id || id_less(objId, end_obj_id))) {
            // Get the object buffer
            err = objStorMgr->readObject(objId, objData, tierUsed);
            if (err == ERR_OK ) {
                if ((max_size - tot_msg_len) >= objData.size) {
                    FDSP_MigrateObjectData mig_obj;
                    mig_obj.meta_data.token_id = token;
                    LOGDEBUG << "Adding a new objectId to objList" << objId;
                    mig_obj.meta_data.object_id.digest = std::string((const char *)objId.GetId(), (size_t)objId.GetLen());
                    mig_obj.meta_data.obj_len = objData.size;
                    mig_obj.data = objData.data;
                    obj_list.push_back(mig_obj);
                    tot_msg_len += objData.size;

                    objStorMgr->counters_.get_tok_objs.incr();
                    DBG(obj_itr_cnt++);
                } else {
                    itr.objId = objId;
                    DBG(LOGDEBUG << "token: " << token <<  " dbId: " << GetSmObjDbId(token)
                        << " cnt: " << obj_itr_cnt) << " token retrieve not completly with "
                        << " max size" << max_size << " and total msg len " << tot_msg_len;
                    return;
                }
            }
            fds_verify(err == ERR_OK);
        }

    } // Enf of for loop
    itr.objId = SMTokenItr::itr_end;

    DBG(LOGDEBUG << "token: " << token <<  " dbId: " << GetSmObjDbId(token)
        << " cnt: " << obj_itr_cnt) << " token retrieve complete";
}

}  // namespace fds
