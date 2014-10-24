/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <string>
#include <dlt.h>
#include <odb.h>
#include <PerfTrace.h>
#include <object-store/SmDiskMap.h>
#include <object-store/ObjectMetaDb.h>

namespace fds {

ObjectMetadataDb::ObjectMetadataDb()
        : bitsPerToken_(0) {
}

ObjectMetadataDb::~ObjectMetadataDb() {
    std::unordered_map<fds_token_id, osm::ObjectDB *>::iterator it;
    for (it = tokenTbl.begin();
         it != tokenTbl.end();
         ++it) {
        osm::ObjectDB * objDb = it->second;
        delete objDb;
        it->second = NULL;
    }
}

void ObjectMetadataDb::setNumBitsPerToken(fds_uint32_t nbits) {
    bitsPerToken_ = nbits;
}

Error
ObjectMetadataDb::openMetadataDb(const SmDiskMap::const_ptr& diskMap) {
    Error err(ERR_OK);
    // open object metadata DB for each token that this SM owns
    // if metadata DB already open, no error
    SmTokenSet smToks = diskMap->getSmTokens();
    for (SmTokenSet::const_iterator cit = smToks.cbegin();
         cit != smToks.cend();
         ++cit) {
        std::string diskPath = diskMap->getDiskPath(*cit, diskio::diskTier);
        err = openObjectDb(*cit, diskPath);
        if (!err.ok()) {
            LOGERROR << "Failed to open Object Meta DB for SM token " << *cit
                     << ", disk path " << diskPath << " " << err;
            break;
        }
    }
    return err;
}

Error
ObjectMetadataDb::openObjectDb(fds_token_id smTokId,
                               const std::string& diskPath) {
    osm::ObjectDB *objdb = NULL;
    std::string filename = diskPath + "//SNodeObjIndex_" + std::to_string(smTokId);
    // LOGDEBUG << "SM Token " << smTokId << " MetaDB: " << filename;

    SCOPEDWRITE(dbmapLock_);
    // check whether this DB is already open
    TokenTblIter iter = tokenTbl.find(smTokId);
    if (iter != tokenTbl.end()) return ERR_OK;

    // create leveldb
    try
    {
        objdb = new osm::ObjectDB(filename);
    }
    catch(const osm::OsmException& e)
    {
        LOGERROR << "Failed to create ObjectDB " << filename;
        LOGERROR << e.what();
        return ERR_NOT_READY;
    }

    tokenTbl[smTokId] = objdb;
    return ERR_OK;
}

//
// returns object metadata DB, if it does not exist, creates it
//
osm::ObjectDB *ObjectMetadataDb::getObjectDB(const ObjectID& objId) {
    fds_token_id smTokId = SmDiskMap::smTokenId(objId, bitsPerToken_);

    SCOPEDREAD(dbmapLock_);
    TokenTblIter iter = tokenTbl.find(smTokId);
    fds_verify(iter != tokenTbl.end());
    return iter->second;
}

void
ObjectMetadataDb::snapshot(fds_token_id smTokId,
                           leveldb::DB*& db,
                           leveldb::ReadOptions& opts) {
    osm::ObjectDB *odb = NULL;
    read_synchronized(dbmapLock_) {
        TokenTblIter iter = tokenTbl.find(smTokId);
        fds_verify(iter != tokenTbl.end());
        odb = iter->second;
    }
    fds_verify(odb != NULL);

    db = odb->GetDB();
    opts.snapshot = db->GetSnapshot();
}

void ObjectMetadataDb::closeObjectDB(fds_token_id smTokId) {
    osm::ObjectDB *objdb = NULL;

    SCOPEDWRITE(dbmapLock_);
    TokenTblIter iter = tokenTbl.find(smTokId);
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

    osm::ObjectDB *odb = getObjectDB(objId);
    fds_verify(odb != NULL);

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
    osm::ObjectDB *odb = getObjectDB(objId);
    fds_verify(odb != NULL);

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
    osm::ObjectDB *odb = getObjectDB(objId);
    fds_verify(odb != NULL);

    PerfContext tmp_pctx(SM_OBJ_METADATA_DB_REMOVE, volId, PerfTracer::perfNameStr(volId));
    SCOPED_PERF_TRACEPOINT_CTX(tmp_pctx);
    return odb->Delete(objId);
}

}  // namespace fds
