/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <string>
#include <dlt.h>
#include <PerfTrace.h>
#include <fds_process.h>
#include <object-store/SmDiskMap.h>
#include <object-store/ObjectMetaDb.h>
#include <sys/statvfs.h>
#include <fiu-control.h>
#include <fiu-local.h>

namespace fds {

ObjectMetadataDb::ObjectMetadataDb(UpdateMediaTrackerFnObj fn)
        : bitsPerToken_(0),
          mediaTrackerFn(fn) {
}

ObjectMetadataDb::~ObjectMetadataDb() {
}

void ObjectMetadataDb::setNumBitsPerToken(fds_uint32_t nbits) {
    bitsPerToken_ = nbits;
}

void
ObjectMetadataDb::closeMetadataDb() {
    LOGDEBUG << "Will close all open Metadata DBs";

    SCOPEDWRITE(dbmapLock_);
    tokenTbl.clear();
}

Error
ObjectMetadataDb::openMetadataDb(SmDiskMap::ptr& diskMap) {
    // open object metadata DB for each token that this SM owns
    // if metadata DB already open, no error
    SmTokenSet smToks = diskMap->getSmTokens();
    return openMetadataDb(diskMap, smToks);
}

Error
ObjectMetadataDb::openMetadataDb(SmDiskMap::ptr& diskMap,
                                 const SmTokenSet& smToks) {
    Error err(ERR_OK);
    smDiskMap = diskMap;
    fds_uint32_t ssdCount = diskMap->getTotalDisks(diskio::flashTier);
    fds_uint32_t hddCount = diskMap->getTotalDisks(diskio::diskTier);
    if ((ssdCount == 0) && (hddCount == 0)) {
        LOGCRITICAL << "No disks (no SSDs and no HDDs)";
        return ERR_SM_EXCEEDED_DISK_CAPACITY;
    }

    // tier we are using to store metadata
    metaTier = diskio::diskTier;

    // if we have SSDs, use SSDs; however, there is currently no way
    // for SM to tell if discovered SSDs are real or simulated
    // so we are using config for that (use SSDs at your own risk, because
    // SSDs may be just one single HDD device ;) )
    fds_bool_t useSsd = g_fdsprocess->get_fds_config()->get<bool>("fds.sm.testing.useSsdForMeta");
    if (useSsd) {
        // currently, we always have SSDs (simulated if no SSDs), so below check
        // is redundant, but just in case platform changes
        if (ssdCount > 0) {
            metaTier = diskio::flashTier;
        }
    } else {
        // if we don't have any HDDs, but have SSDs, still use SSDs for meta
        if (hddCount == 0) {
            metaTier = diskio::flashTier;
        }
    }

    // check in platform if we should do sync writes
    fds_bool_t syncW = g_fdsprocess->get_fds_config()->get<bool>("fds.sm.testing.syncMetaWrite");
    LOGDEBUG << "Will do sync? " << syncW << " (metadata) writes to object DB";

    // open object metadata DB for each token in the set
    // if metadata DB already open, no error
    for (SmTokenSet::const_iterator cit = smToks.cbegin();
         cit != smToks.cend();
         ++cit) {
        std::string diskPath = diskMap->getDiskPath(*cit, metaTier);
        err = openObjectDb(*cit, diskPath, syncW);
        if (!err.ok()) {
            LOGERROR << "Failed to open Object Meta DB for SM token " << *cit
                     << ", disk path " << diskPath << " " << err;
            break;
        }
    }
    return err;
}

Error
ObjectMetadataDb::deleteMetadataDb(const std::string& diskPath,
                                   const fds_token_id& smToken) {
    Error err(ERR_OK);
    std::string file = ObjectMetadataDb::getObjectMetaFilename(diskPath, smToken);
    leveldb::Status status = leveldb::DestroyDB(file, leveldb::Options());
    if (!status.ok()) {
        LOGNOTIFY << "Could not delete metadataDB for smToken = " << smToken;
    }
    return err;
}

Error
ObjectMetadataDb::closeAndDeleteMetadataDbs(const SmTokenSet& smTokensLost) {
    Error err(ERR_OK);
    for (SmTokenSet::const_iterator cit = smTokensLost.cbegin();
         cit != smTokensLost.cend();
         ++cit) {
        Error tmpErr = closeObjectDB(*cit, true);
        if (!tmpErr.ok()) {
            LOGERROR << "Failed to close ObjectDB for SM token " << *cit
                     << " " << tmpErr;
            err = tmpErr;
        } else {
            LOGNOTIFY << "Closed ObjectDB for SM token " << *cit;
        }
    }
    return err;
}


Error
ObjectMetadataDb::openObjectDb(fds_token_id smTokId,
                               const std::string& diskPath,
                               fds_bool_t syncWrite) {
    std::string filename = ObjectMetadataDb::getObjectMetaFilename(diskPath, smTokId);
    // LOGDEBUG << "SM Token " << smTokId << " MetaDB: " << filename;

    SCOPEDWRITE(dbmapLock_);
    // check whether this DB is already open
    TokenTblIter iter = tokenTbl.find(smTokId);
    if (iter != tokenTbl.end()) return ERR_OK;

    // create leveldb
    std::shared_ptr<osm::ObjectDB> objdb;
    try
    {
        objdb = std::make_shared<osm::ObjectDB>(filename, syncWrite);
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
std::shared_ptr<osm::ObjectDB> ObjectMetadataDb::getObjectDB(const ObjectID& objId) {
    fds_token_id smTokId = SmDiskMap::smTokenId(objId, bitsPerToken_);

    SCOPEDREAD(dbmapLock_);
    TokenTblIter iter = tokenTbl.find(smTokId);
    if (iter != tokenTbl.end()) {
        return iter->second;
    }

    // else did not find it
    return nullptr;
}

Error
ObjectMetadataDb::snapshot(fds_token_id smTokId,
                           std::shared_ptr<leveldb::DB>& db,
                           leveldb::ReadOptions& opts) {
    std::shared_ptr<osm::ObjectDB> odb;

    read_synchronized(dbmapLock_) {
        TokenTblIter iter = tokenTbl.find(smTokId);
        if (iter != tokenTbl.end()) {
            odb = iter->second;
        }
    }
    if (!odb) {
        return ERR_NOT_FOUND;
    }

    db = odb->GetDB();
    opts.snapshot = db->GetSnapshot();
    return ERR_OK;
}

Error
ObjectMetadataDb::snapshot(fds_token_id smTokId,
                           std::string &snapDir,
                           leveldb::CopyEnv **env) {
    std::shared_ptr<osm::ObjectDB> odb = nullptr;
    read_synchronized(dbmapLock_) {
        TokenTblIter iter = tokenTbl.find(smTokId);
        if (iter != tokenTbl.end()) {
            odb = iter->second;
        }
    }

    if (odb == nullptr) {
        return ERR_NOT_FOUND;
    } else {
        return odb->PersistentSnap(snapDir, env);
    }
}

Error ObjectMetadataDb::closeObjectDB(fds_token_id smTokId,
                                      fds_bool_t destroy) {
    std::shared_ptr<osm::ObjectDB> objdb = nullptr;

    SCOPEDWRITE(dbmapLock_);
    TokenTblIter iter = tokenTbl.find(smTokId);
    if (iter == tokenTbl.end()) return ERR_NOT_FOUND;
    objdb = iter->second;
    tokenTbl.erase(iter);
    if (destroy) {
        objdb->closeAndDestroy();
    }
    return ERR_OK;
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

    std::shared_ptr<osm::ObjectDB> odb = getObjectDB(objId);
    if (!odb) {
        LOGWARN << "ObjectDB probably not open, is this expected?";
        err = ERR_NOT_READY;
        return NULL;
    }

    // get meta from DB
    PerfContext tmp_pctx(PerfEventType::SM_OBJ_METADATA_DB_READ, volId);
    SCOPED_PERF_TRACEPOINT_CTX(tmp_pctx);
    err = odb->Get(objId, buf);
    fiu_do_on("sm.persist.meta_readfail", err = ERR_NO_BYTES_READ; );
    if (!err.ok()) {
        // Object not found. Return.
        fds_token_id smTokId = SmDiskMap::smTokenId(objId, bitsPerToken_);
        mediaTrackerFn(smTokId, metaTier, err);
        return NULL;
    }

    ObjMetaData::const_ptr objMeta(new ObjMetaData(buf));
    return objMeta;
}

Error ObjectMetadataDb::put(fds_volid_t volId,
                            const ObjectID& objId,
                            ObjMetaData::const_ptr objMeta) {
    Error err(ERR_OK);
    std::shared_ptr<osm::ObjectDB> odb = getObjectDB(objId);
    if (!odb) {
        LOGWARN << "ObjectDB probably not open, is this expected?";
        return ERR_NOT_READY;
    }

    // store gata
    PerfContext tmp_pctx(PerfEventType::SM_OBJ_METADATA_DB_WRITE, volId);
    SCOPED_PERF_TRACEPOINT_CTX(tmp_pctx);
    ObjectBuf buf;
    objMeta->serializeTo(buf);
    err = odb->Put(objId, buf);
    if (!err.ok()) {
        fds_token_id smTokId = SmDiskMap::smTokenId(objId, bitsPerToken_);
        mediaTrackerFn(smTokId, metaTier, err);
    }
    return err;
}

//
// delete object's metadata from DB
//
Error ObjectMetadataDb::remove(fds_volid_t volId,
                               const ObjectID& objId) {
    std::shared_ptr<osm::ObjectDB> odb = getObjectDB(objId);
    if (!odb) {
        LOGWARN << "ObjectDB probably not open, is this expected?";
        return ERR_NOT_READY;
    }

    PerfContext tmp_pctx(PerfEventType::SM_OBJ_METADATA_DB_REMOVE, volId);
    SCOPED_PERF_TRACEPOINT_CTX(tmp_pctx);
    return odb->Delete(objId);
}


std::string ObjectMetadataDb::getObjectMetaFilename(const std::string& diskPath, fds_token_id smTokId) {

    std::string filename = diskPath + "//SNodeObjIndex_" + std::to_string(smTokId);
    return filename;
}
}  // namespace fds
