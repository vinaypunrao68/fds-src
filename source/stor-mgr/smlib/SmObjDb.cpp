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

fds::Error SmObjDb::get_(const View &view,
        const ObjectID& objId, OnDiskSmObjMetadata& md)
{
    Error err = ERR_OK;

    fds_token_id tokId = getTokenId_(objId);
    ObjectDB *odb = getObjectDB_(tokId);
    ObjectBuf buf;
    err = odb->Get(objId, buf);
    if (err != ERR_OK) {
        /* Object not found. Return. */
        return ERR_SM_OBJ_METADATA_NOT_FOUND;
    }

    md.unmarshall(const_cast<char*>(buf.data.data()), buf.data.size());

    if (isTokenInSyncMode_(tokId)) {
        md.checkAndDemoteUnsyncedData(objStorMgr->getTokenSyncTimeStamp(tokId));
        if (view == SYNC_MERGED) {
            md.mergeNewAndUnsyncedData();
        }
    }
    return err;
}

fds::Error SmObjDb::put_(const ObjectID& objId, const OnDiskSmObjMetadata& md)
{
    Error err = ERR_OK;

    fds_token_id tokId = getTokenId_(objId);
    ObjectDB *odb = getObjectDB_(tokId);

    /* Store the data back */
    ObjectBuf buf;
    buf.resize(md.marshalledSize());
    size_t sz = md.marshall(const_cast<char*>(buf.data.data()), buf.data.size());
    fds_assert(sz == buf.data.size());
    err = odb->Put(objId, buf);

    return err;
}
inline fds_token_id SmObjDb::getTokenId_(const ObjectID& objId)
{
    return objStorMgr->getDLT()->getToken(objId);
}

inline ObjectDB* SmObjDb::getObjectDB_(const fds_token_id& tokId)
{
    ObjectDB *odb = getObjectDB(tokId);
    if (!odb) {
        odb = openObjectDB(tokId);
    }
    return odb;
}

inline bool SmObjDb::isTokenInSyncMode_(const fds_token_id& tokId)
{
    return objStorMgr->isTokenInSyncMode(tokId);
}

fds::Error SmObjDb::putSyncEntry(const ObjectID& objId,
        const FDSP_MigrateObjectMetadata& data)
{
    OnDiskSmObjMetadata md;
    Error err = get_(NON_SYNC_MERGED, objId, md);

    if (err != ERR_OK && err != ERR_SM_OBJ_METADATA_NOT_FOUND) {
        LOGERROR << "Error while applying sync entry.  objId: " << objId;
        return err;
    }
    md.applySyncData(data);

    return put_(objId, md);
}

Error
SmObjDb::writeObjectLocation(const ObjectID& objId,
        meta_obj_map_t *obj_map,
        fds_bool_t      append) {

    Error err(ERR_OK);

    diskio::MetaObjMap objMap;
    ObjectBuf          objData;

    if (append == true) {
        LOGDEBUG << "Appending new location for object " << objId;

        /*
         * Get existing object locations
         * TODO: We need a better way to update this
         * location DB with a new location. This requires
         * reading the existing locations, updating the entry,
         * and re-writing it. We often just want to append.
         */
        err = readObjectLocations(NON_SYNC_MERGED, objId, objMap);
        if (err != ERR_OK && err != ERR_DISK_READ_FAILED) {
            LOGERROR << "Failed to read existing object locations"
                    << " during location write";
            return err;
        } else if (err == ERR_DISK_READ_FAILED) {
            /*
             * Assume this error means the key just did not exist.
             * TODO: Add an err to differention "no key" from "failed read".
             */
            LOGDEBUG << "Not able to read existing object locations"
                    << ", assuming no prior entry existed";
            err = ERR_OK;
        }
    }

    /*
     * Add new location to existing locations
     */
    objMap.updateMap(*obj_map);

    objData.size = objMap.marshalledSize();
    objData.data = std::string(objMap.marshalling(), objMap.marshalledSize());
    err = Put(objId, objData);
    if (err == ERR_OK) {
        LOGDEBUG << "Updating object location for object "
                << objId << " to " << objMap;
    } else {
        LOGERROR << "Failed to put object " << objId
                << " into odb with error " << err;
    }

    return err;
}

/*
 * Reads all object locations
 */
Error
SmObjDb::readObjectLocations(const View &view,
        const ObjectID     &objId,
        diskio::MetaObjMap &objMaps) {
    Error     err(ERR_OK);
    ObjectBuf objData;
    // TODO(Rao): Take the view in to account
    objData.size = 0;
    objData.data = "";
    err = Get(objId, objData);
    if (err == ERR_OK) {
        objData.size = objData.data.size();
        objMaps.unmarshalling(objData.data, objData.size);
    }

    return err;
}

Error
SmObjDb::deleteObjectLocation(const ObjectID& objId) {

    Error err(ERR_OK);
    // NOTE !!!
    meta_obj_map_t *obj_map = new meta_obj_map_t();

    diskio::MetaObjMap objMap;
    ObjectBuf          objData;

    /*
     * Get existing object locations
     */
    err = readObjectLocations(NON_SYNC_MERGED, objId, objMap);
    if (err != ERR_OK && err != ERR_DISK_READ_FAILED) {
        LOGERROR << "Failed to read existing object locations"
                << " during location write";
        return err;
    } else if (err == ERR_DISK_READ_FAILED) {
        /*
         * Assume this error means the key just did not exist.
         * TODO: Add an err to differention "no key" from "failed read".
         */
        LOGDEBUG << "Not able to read existing object locations"
                << ", assuming no prior entry existed";
        err = ERR_OK;
    }

    /*
     * Set the ref_cnt to 0, which will be the delete marker for this object and Garbage collector feeds on these objects
     */
    obj_map->obj_refcnt = -1;
    objData.size = objMap.marshalledSize();
    objData.data = std::string(objMap.marshalling(), objMap.marshalledSize());
    err = Put(objId, objData);
    if (err == ERR_OK) {
        LOGDEBUG << "Setting the delete marker for object "
                << objId << " to " << objMap;
    } else {
        LOGERROR << "Failed to put object " << objId
                << " into odb with error " << err;
    }

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
            err = objStorMgr->readObject(NON_SYNC_MERGED, objId, objData, tierUsed);
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
