/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

#include <map>

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
    testMode = g_fdsprocess->get_fds_config()->get<bool>("fds.sm.testing.standalone");
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

    // we are going to send rebalance initial set msg(s) per DLT token
    // even if there are no objects in level DB, we are sending one msg per
    // DLT token so that the source knows there are no objects for a given
    // DLT token
    leveldb::Iterator* it = db->NewIterator(options);
    std::map<fds_token_id, fpi::CtrlObjectRebalanceInitialSetPtr> perTokenMsgs;
    fds_uint64_t seqId = 0;
    for (auto tok : dltTokens) {
        // for now packing all objects per one DLT token into one message
        fpi::CtrlObjectRebalanceInitialSetPtr msg(new fpi::CtrlObjectRebalanceInitialSet());
        msg->tokenId = tok;
        msg->seqNum = ++seqId;
        msg->last = (seqId < dltTokens.size()) ? false : true;
        LOGNORMAL << "Initial Set Msg: token " << tok << ", seqNum "
                  << msg->seqNum << ", last " << msg->last;
        perTokenMsgs[tok] = msg;
    }

    /**
     * Iterate through the level db and add to set of objects to rebalance.
     */
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
        LOGNORMAL << "Will add object " << id << ", dltToken " << dltTokId
                 << " refcnt " << omdSync.objRefCnt << " to thrift msg to source SM "
                 << std::hex << sourceSmUuid.uuid_get_val() << std::dec;
        perTokenMsgs[dltTokId]->objectsToSync.push_back(omdSync);
    }
    delete it;

    // send rebalance set of objects to source SM
    for (auto tok : dltTokens) {
        LOGNORMAL << "Sending rebalance initial set for DLT token "
                  << tok << " set size " << perTokenMsgs[tok]->objectsToSync.size()
                  << " to source SM "
                  << std::hex << sourceSmUuid.uuid_get_val() << std::dec;
        if (!testMode) {
            auto asyncRebalSetReq = gSvcRequestPool->newEPSvcRequest(sourceSmUuid.toSvcUuid());
            asyncRebalSetReq->setPayload(FDSP_MSG_TYPEID(fpi::CtrlObjectRebalanceInitialSet),
                                       perTokenMsgs[tok]);
            asyncRebalSetReq->setTimeoutMs(5000);
            // we are not waiting for response, so not setting a callback
            asyncRebalSetReq->invoke();
        }
    }

    /**
     * TODO(Sean):  To support active IO on both the source and destination SMs,
     *              we need to keep this snapshot until initial set of objects
     *              are relanced.  After that, we need to take another snapshot
     *              to determine if active IOs have changed the state of the existing
     *              objects (i.e. ref cnt) or additional object are written.
     */
    return err;
}

}  // namespace fds
