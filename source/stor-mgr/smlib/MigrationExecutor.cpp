/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

#include <ObjMeta.h>
#include <dlt.h>
#include <MigrationExecutor.h>

namespace fds {

MigrationExecutor::MigrationExecutor(SmIoReqHandler *_dataStore,
                                     fds_uint32_t bitsPerToken,
                                     const NodeUuid& srcSmId,
                                     fds_token_id smTokId)
    : dataStore(_dataStore),
      bitsPerDltToken(bitsPerToken),
      smTokenId(smTokId),
      sourceSmUuid(srcSmId)
{
}

MigrationExecutor::~MigrationExecutor()
{
}

void
MigrationExecutor::addDltToken(fds_token_id dltTok) {
    fds_verify(dltTokens.count(dltTok) == 0);
    dltTokens.insert(dltTok);
}

// DO NOT release snapshot here, becuase it maybe passed to other
// migration executors
Error
MigrationExecutor::startObjectRebalance(leveldb::ReadOptions& options,
                                        leveldb::DB *db)
{
    Error err(ERR_OK);
    ObjMetaData omd;
    LOGNORMAL << "Will send obj ids to source SM " << std::hex
              << sourceSmUuid.uuid_get_val() << std::dec << " for SM token "
              << smTokenId << " (appropriate set of DLT tokens)";

    /**
     * Iterate through the level db and add to set of objects to rebalance.
     */
    leveldb::Iterator* it = db->NewIterator(options);
    fpi::CtrlObjectRebalanceInitialSetPtr msg(new fpi::CtrlObjectRebalanceInitialSet());
    for (it->SeekToFirst(); it->Valid(); it->Next()) {
        ObjectID id(it->key().ToString());
        // send objects that belong to DLT tokens that need to be migrated from src SM
        fds_token_id dltTokId = DLT::getToken(id, bitsPerDltToken);
        if (dltTokens.count(dltTokId) == 0) {
            // ignore this object
            continue;
        }

        // add object id to the thrift paired set of object ids and ref count
        omd.deserializeFrom(it->value());
        fpi::CtrlObjectMetaDataSync omdSync;
        omdSync.objectID.digest = it->key().ToString();
        omdSync.objRefCnt = omd.getRefCnt();
        LOGDEBUG << "Will add object " << id << ", dltToken " << dltTokId
                 << " refcnt " << omdSync.objRefCnt << " to thrift msg to source SM "
                 << std::hex << sourceSmUuid.uuid_get_val() << std::dec;
        msg->objectsToSync.push_back(omdSync);
    }
    delete it;

    /**
     * TODO(Sean):  To support active IO on both the source and destination SMs,
     *              we need to keep this snapshot until initial set of objects
     *              are relanced.  After that, we need to take another snapshot
     *              to determine if active IOs have changed the state of the existing
     *              objects (i.e. ref cnt) or additional object are written.
     */

    /**
     * TODO(Sean): Send the set to the source SM.
     */

    LOGDEBUG << "Generated destination SM rebalance set of objects.";
    return err;
}

}  // namespace fds
