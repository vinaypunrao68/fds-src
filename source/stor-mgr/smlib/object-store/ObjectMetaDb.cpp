/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <string>
#include <dlt.h>
#include <odb.h>
#include <PerfTrace.h>
#include <object-store/ObjectMetaDb.h>

namespace fds {

ObjectMetadataDb::ObjectMetadataDb(const std::string& dir)
        : dir_(dir), bitsPerToken_(0) {
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
        TokenTblIter iter = tokenTbl.find(dbId);
        if (iter != tokenTbl.end()) return iter->second;
    }

    // Create leveldb
    std::string filename = dir_ + "SNodeObjIndex_" + std::to_string(dbId);
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

/**
 * Gets metadata from the db. If the metadata is located in the db
 * the shared ptr is allocated with the associated metadata being set.
 */
ObjMetaData::const_ptr
ObjectMetadataDb::get(fds_volid_t volId,
                      const ObjectID& objId,
                      Error &err) {
    err = ERR_OK;
    ObjectBuf buf;

    fds_token_id tokId = DLT::getToken(objId, bitsPerToken_);
    osm::ObjectDB *odb = getObjectDB(tokId);
    if (!odb) {
        err = ERR_OUT_OF_MEMORY;
        return NULL;
    }

    // get meta from DB
    PerfContext tmp_pctx(SM_OBJ_METADATA_DB_READ, volId, PerfTracer::perfNameStr(volId));
    SCOPED_PERF_TRACEPOINT_CTX(tmp_pctx);
    err = odb->Get(objId, buf);
    if (!err.ok()) {
        // Object not found. Return.
        return NULL;
    }

    ObjMetaData::const_ptr objMeta(new ObjMetaData(buf));
    // objMeta->deserializeFrom(buf);

    // TODO(Anna) token sync code -- objMeta.checkAndDemoteUnsyncedData;

    return objMeta;
}

Error ObjectMetadataDb::put(fds_volid_t volId,
                            const ObjectID& objId,
                            ObjMetaData::const_ptr objMeta) {
    fds_token_id tokId = DLT::getToken(objId, bitsPerToken_);
    osm::ObjectDB *odb = getObjectDB(tokId);
    if (!odb) {
        return ERR_OUT_OF_MEMORY;
    }

    // store gata
    PerfContext tmp_pctx(SM_OBJ_METADATA_DB_WRITE, volId, PerfTracer::perfNameStr(volId));
    SCOPED_PERF_TRACEPOINT_CTX(tmp_pctx);
    ObjectBuf buf;
    objMeta->serializeTo(buf);
    return odb->Put(objId, buf);
}

//
// delete object's metadata from DB
//
Error ObjectMetadataDb::remove(fds_volid_t volId,
                               const ObjectID& objId) {
    fds_token_id tokId = DLT::getToken(objId, bitsPerToken_);
    osm::ObjectDB *odb = getObjectDB(tokId);
    if (!odb) {
        return ERR_OUT_OF_MEMORY;
    }

    PerfContext tmp_pctx(SM_OBJ_METADATA_DB_REMOVE, volId, PerfTracer::perfNameStr(volId));
    SCOPED_PERF_TRACEPOINT_CTX(tmp_pctx);
    return odb->Delete(objId);
}

}  // namespace fds
