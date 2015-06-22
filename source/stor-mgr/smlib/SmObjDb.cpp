/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <string>
#include <SmObjDb.h>
#include <StorMgr.h>

using osm::ObjectDB;

namespace fds {

OpCtx::OpCtx(const OpCtx::OpType &t)
: OpCtx(t, 0)
{
}

OpCtx::OpCtx(const OpCtx::OpType &t, const uint64_t &timestamp)
{
    type = t;
    ts = timestamp;
}
bool OpCtx::isClientIO() const
{
    return (type == GET || type == PUT || type == DELETE);
}

SmObjDb::SmObjDb(ObjectStorMgr *storMgr,
        std::string stor_prefix_,
        fds_log* _log)
{
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

    FDSGUARD(*smObjDbMutex);
    TokenTblIter iter = tokenTbl.find(tokId);
    if (iter != tokenTbl.end()) return iter->second;

    // Create leveldb
    std::string filename = std::string(diskio::gl_dataIOMod.disk_path(tokId, diskio::diskTier)) +
             "//SNodeObjIndex_" + std::to_string(dbId);
    // std::string filename= stor_prefix + "SNodeObjIndex_" + std::to_string(dbId);
    try
    {
        // not reading from config, because SmObjDb is deprecated
        objdb = new osm::ObjectDB(filename, true);
    }
    catch(const osm::OsmException& e)
    {
        LOGERROR << "Failed to create ObjectDB " << filename;
        LOGERROR << e.what();

        /*
         * TODO(Greg): We need to end this process at this point, but we need
         * a more controlled and graceful way of doing it. I suggest that in
         * such cases we throw another exception to be caught by the mainline
         * method which can then perform any cleanup, log a useful message, and
         * shutdown.
         */
        LOGNORMAL << "SM shutting down with a failure condition.";
        exit(EXIT_FAILURE);
    }

    tokenTbl[dbId] = objdb;

    return objdb;
}

void SmObjDb::closeObjectDB(fds_token_id tokId) {
    fds_token_id dbId = GetSmObjDbId(tokId);
    ObjectDB *objdb = NULL;

    FDSGUARD(*smObjDbMutex);
    TokenTblIter iter = tokenTbl.find(dbId);
    if (iter == tokenTbl.end()) return;
    objdb = iter->second;
    tokenTbl.erase(iter);
    delete objdb;
}

ObjectDB *SmObjDb::getObjectDB(fds_token_id tokId) {
    ObjectDB *objdb = NULL;
    fds_token_id dbId = GetSmObjDbId(tokId);

    FDSGUARD(*smObjDbMutex);
    TokenTblIter iter = tokenTbl.find(dbId);
    if (iter != tokenTbl.end()) return iter->second;

    // Create leveldb
    std::string filename = std::string(diskio::gl_dataIOMod.disk_path(tokId, diskio::diskTier)) +
             "//SNodeObjIndex_" + std::to_string(dbId);
    // std::string filename= stor_prefix + "SNodeObjIndex_" + std::to_string(dbId);
    try
    {
        // not using config because SmObjDb is deprecated
        objdb = new osm::ObjectDB(filename, true);
    }
    catch(const osm::OsmException& e)
    {
        LOGERROR << "Failed to create ObjectDB " << filename;
        LOGERROR << e.what();

        /*
         * TODO(Greg): We need to end this process at this point, but we need
         * a more controlled and graceful way of doing it. I suggest that in
         * such cases we throw another exception to be caught by the mainline
         * method which can then perform any cleanup, log a useful message, and
         * shutdown.
         */
        LOGNORMAL << "SM shutting down with a failure condition.";
        exit(EXIT_FAILURE);
    }

    tokenTbl[dbId] = objdb;
    return objdb;
}

void SmObjDb::lock(const ObjectID& objId) {
    fds_token_id tokId = getTokenId_(objId);
    ObjectDB *objdb = getObjectDB(tokId);
    objdb->wrLock();
}


void SmObjDb::unlock(const ObjectID& objId) {
    fds_token_id tokId = getTokenId_(objId);
    ObjectDB *objdb = getObjectDB(tokId);
    objdb->wrUnlock();
}

/**
 * Takes snapshot of db identified tokId
 * @param tokId
 * @param db - returns a pointer to snapshotted db
 * @param options - level db specifics for snapshot
 */

void SmObjDb::snapshot(const fds_token_id& tokId,
        std::shared_ptr<leveldb::DB>& db, leveldb::ReadOptions& options)
{
    ObjectDB *odb = getObjectDB_(tokId);
    db = odb->GetDB();
    options.snapshot = db->GetSnapshot();
    return;
}
bool SmObjDb::dataPhysicallyExists(const ObjectID& objId) {
    ObjMetaData md;
    Error err = get(objId, md);
    if (err != ERR_OK) return false;
    return md.dataPhysicallyExists();
}

fds::Error SmObjDb::get(const ObjectID& objId, ObjMetaData& md) {
    Error err = ERR_OK;

    fds_token_id tokId = getTokenId_(objId);
    ObjectDB *odb = getObjectDB_(tokId);
    ObjectBuf buf;
    err = odb->Get(objId, buf);
    if (err != ERR_OK) {
        /* Object not found. Return. */
        return err;
    }

    md.deserializeFrom(buf);

    /* We need to treat any existing metadata that is prior to sync start
     * timestamp as not up-to-data metada.  We'll demote that as sync entry.
     */
    if (isTokenInSyncMode_(tokId)) {
        uint64_t syncStartTs = 0;
        objStorMgr->getTokenStateDb()->getTokenSyncStartTS(tokId, syncStartTs);
        md.checkAndDemoteUnsyncedData(syncStartTs);
    }
    return err;
}

fds::Error SmObjDb::put(const OpCtx &opCtx,
        const ObjectID& objId, ObjMetaData& md)
{
    Error err = ERR_OK;

    if (opCtx.isClientIO()) {
        /* Update timestamps.  Currenly only PUT and DELETE have an effect here */
        if (md.obj_map.obj_create_time == 0) {
            md.obj_map.obj_create_time = opCtx.ts;
        }
        md.obj_map.assoc_mod_time = opCtx.ts;
    }

    LOGDEBUG << "ctx: " << opCtx.type << " " << md.logString();

    return put_(objId, md);
}

fds::Error SmObjDb::put_(const ObjectID& objId, const ObjMetaData& md)
{
    Error err = ERR_OK;

    fds_token_id tokId = getTokenId_(objId);
    ObjectDB *odb = getObjectDB_(tokId);

    /* Store the data back */
    ObjectBuf buf;
    md.serializeTo(buf);

    err = odb->Put(objId, buf);

    return err;
}

fds::Error SmObjDb::remove(const ObjectID& objId) {
    Error err(ERR_OK);
    fds_token_id tokId = getTokenId_(objId);
    ObjectDB *odb = getObjectDB_(tokId);

    // delete object's metadata from DB
    err = odb->Delete(objId);

    return err;
}

inline fds_token_id SmObjDb::getTokenId_(const ObjectID& objId)
{
    return objStorMgr->getTokenId(objId);
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

/**
 * Resolves sync metadata with metadata accumulated after syncpoint by
 * merging them both.
 * @param objId
 * @return
 */
Error SmObjDb::resolveEntry(const ObjectID& objId)
{
    Error err(ERR_OK);
    ObjMetaData md;
    err = get(objId, md);

    if (err != ERR_OK && err != ERR_NOT_FOUND) {
        LOGERROR << "Error: " << err << " objId: " << objId;
        return err;
    }

    LOGDEBUG << " Object id: " << objId;
    md.mergeNewAndUnsyncedData();

    return put(OpCtx(OpCtx::SYNC), objId, md);
}

/**
 * Retries metadata from snapshot iterator
 * @param itr
 * @param md
 * @return
 */
Error SmObjDb::get_from_snapshot(leveldb::Iterator* itr, ObjMetaData& md)
{
    md.deserializeFrom(itr->value());
    return ERR_OK;
}

}  // namespace fds
