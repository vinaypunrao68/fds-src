/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <string>
#include <dlt.h>
#include <odb.h>
#include <object-store/ObjectMetaDb.h>


namespace fds {

ObjectMetadataDb::ObjectMetadataDb()
        : bitsPerToken_(0) {
}

ObjectMetadataDb::~ObjectMetadataDb() {
}

void ObjectMetadataDb::setNumBitsPerToken(fds_uint32_t nbits) {
    bitsPerToken_ = nbits;
}

//
// returns object metadata DB, if it does not exist, creates it
//
osm::ObjectDB *ObjectMetadataDb::getObjectDB(fds_token_id tokId) {
    osm::ObjectDB *objdb = NULL;
    fds_token_id dbId = getDbId(tokId);

    read_synchronized(dbmapLock_) {
        TokenTblIter iter = tokenTbl.find(tokId);
        if (iter != tokenTbl.end()) return iter->second;
    }

    // Create leveldb
    std::string filename = "SNodeObjIndex_" + std::to_string(dbId);
    objdb = new(std::nothrow) osm::ObjectDB(filename);
    if (!objdb) {
        LOGERROR << "Failed to create ObjectDB " << filename;
        return NULL;
    }

    SCOPEDWRITE(dbmapLock_);
    tokenTbl[dbId] = objdb;
    return objdb;
}

void ObjectMetadataDb::closeObjectDB(fds_token_id tokId) {
    fds_token_id dbId = getDbId(tokId);
    osm::ObjectDB *objdb = NULL;

    SCOPEDWRITE(dbmapLock_);
    TokenTblIter iter = tokenTbl.find(dbId);
    if (iter == tokenTbl.end()) return;
    objdb = iter->second;
    tokenTbl.erase(iter);
    delete objdb;
}

Error ObjectMetadataDb::get(const ObjectID& objId,
                            ObjMetaData::ptr objMeta) {
    Error err = ERR_OK;
    ObjectBuf buf;

    fds_token_id tokId = DLT::getToken(objId, bitsPerToken_);
    osm::ObjectDB *odb = getObjectDB(tokId);
    if (!odb) {
        return ERR_OUT_OF_MEMORY;
    }

    // get meta from DB
    err = odb->Get(objId, buf);
    if (!err.ok()) {
        /* Object not found. Return. */
        return ERR_DISK_READ_FAILED;
    }

    objMeta->deserializeFrom(buf);

    // TODO(Anna) token sync code -- objMeta.checkAndDemoteUnsyncedData;

    return err;
}

Error ObjectMetadataDb::put(const ObjectID& objId,
                            ObjMetaData::const_ptr objMeta) {
    fds_token_id tokId = DLT::getToken(objId, bitsPerToken_);
    osm::ObjectDB *odb = getObjectDB(tokId);
    if (!odb) {
        return ERR_OUT_OF_MEMORY;
    }

    // TODO(Anna) update timestamp on data path (not migration)
    // in ObjectStore, not here, in case we put objMeta to cache
    /*
    if (opCtx.isClientIO()) {
        // Update timestamps.  Currenly only PUT and DELETE have an effect here
        if (md.obj_map.obj_create_time == 0) {
            md.obj_map.obj_create_time = opCtx.ts;
        }
        md.obj_map.assoc_mod_time = opCtx.ts;
    }
    */

    // store gata
    ObjectBuf buf;
    objMeta->serializeTo(buf);
    return odb->Put(objId, buf);
}

//
// delete object's metadata from DB
//
Error ObjectMetadataDb::remove(const ObjectID& objId) {
    fds_token_id tokId = DLT::getToken(objId, bitsPerToken_);
    osm::ObjectDB *odb = getObjectDB(tokId);
    if (!odb) {
        return ERR_OUT_OF_MEMORY;
    }

    return odb->Delete(objId);
}

}  // namespace fds
