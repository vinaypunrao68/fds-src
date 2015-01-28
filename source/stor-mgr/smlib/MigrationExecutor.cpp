/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

#include <map>
#include <vector>

#include <fds_process.h>
#include <ObjMeta.h>
#include <dlt.h>
#include <SMSvcHandler.h>
#include <MigrationExecutor.h>

namespace fds {

MigrationExecutor::MigrationExecutor(SmIoReqHandler *_dataStore,
                                     fds_uint32_t bitsPerToken,
                                     const NodeUuid& srcSmId,
                                     fds_token_id smTokId,
                                     fds_uint64_t executorID,
                                     MigrationExecutorDoneHandler doneHandler)
        : executorId(executorID),
          migrDoneHandler(doneHandler),
          dataStore(_dataStore),
          bitsPerDltToken(bitsPerToken),
          smTokenId(smTokId),
          sourceSmUuid(srcSmId)
{
    state = ATOMIC_VAR_INIT(ME_INIT);
    testMode = g_fdsprocess->get_fds_config()->get<bool>("fds.sm.testing.standalone");
}

MigrationExecutor::~MigrationExecutor()
{
}

void
MigrationExecutor::addDltToken(fds_token_id dltTok) {
    MigrationExecutorState curState = atomic_load(&state);
    fds_verify(curState == ME_INIT);
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

    MigrationExecutorState expectState = ME_INIT;
    if (!std::atomic_compare_exchange_strong(&state, &expectState, ME_REBALANCE_START)) {
        LOGNOTIFY << "startObjectRebalance called in non- ME_INIT state " << state;
        return ERR_NOT_READY;
    }

    LOGNORMAL << "Will send obj ids to source SM " << std::hex
              << sourceSmUuid.uuid_get_val() << std::dec << " for SM token "
              << smTokenId << " (appropriate set of DLT tokens)";

    // we are going to send rebalance initial set msg(s) per DLT token
    // even if there are no objects in level DB, we are sending one msg per
    // DLT token so that the source knows there are no objects for a given
    // DLT token
    leveldb::Iterator* it = db->NewIterator(options);
    std::map<fds_token_id, fpi::CtrlObjectRebalanceFilterSetPtr> perTokenMsgs;
    uint64_t seqId = 0UL;
    fds_verify(dltTokens.size() > 0);   // we must have at least one token
    for (auto dltTok : dltTokens) {
        // for now packing all objects per one DLT token into one message
        fpi::CtrlObjectRebalanceFilterSetPtr msg(new fpi::CtrlObjectRebalanceFilterSet());
        msg->tokenId = dltTok;
        msg->executorID = executorId;
        msg->seqNum = seqId++;
        msg->lastFilterSet = (seqId < dltTokens.size()) ? false : true;
        LOGMIGRATE << "Filter Set Msg: token=" << dltTok << ", seqNum="
                    << msg->seqNum << ", last=" << msg->lastFilterSet;
        perTokenMsgs[dltTok] = msg;
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


        fpi::CtrlObjectMetaDataSync omdFilter;
        omdFilter.objectID.digest = it->key().ToString();
        omdFilter.objRefCnt = omd.getRefCnt();

        /* TODO(Sean):  We should add to filter object set if the ref cnt is positive (???)
         *              Are there other checks before adding to the filter set?
         */
        if (omdFilter.objRefCnt > 0) {
            LOGMIGRATE << "FilterSet add ObjId=" << id << ", dltToken=" << dltTokId
                        << " refcnt=" << omdFilter.objRefCnt << " to thrift msg to source SM "
                        << std::hex << sourceSmUuid.uuid_get_val() << std::dec;
            perTokenMsgs[dltTokId]->objectsToFilter.push_back(omdFilter);
        }
    }
    delete it;

    // send rebalance set of objects to source SM
    for (auto tok : dltTokens) {
        LOGMIGRATE << "Sending rebalance initial set for DLT token "
                  << tok << " set size " << perTokenMsgs[tok]->objectsToFilter.size()
                  << " to source SM "
                  << std::hex << sourceSmUuid.uuid_get_val() << std::dec;
        if (!testMode) {
            auto asyncRebalSetReq = gSvcRequestPool->newEPSvcRequest(sourceSmUuid.toSvcUuid());
            asyncRebalSetReq->setPayload(FDSP_MSG_TYPEID(fpi::CtrlObjectRebalanceFilterSet),
                                       perTokenMsgs[tok]);
            asyncRebalSetReq->setTimeoutMs(0);
            // we are not waiting for response, so not setting a callback
            asyncRebalSetReq->invoke();
        }
    }

    // we sent all the messages, go to next state
    expectState = ME_REBALANCE_START;
    if (!std::atomic_compare_exchange_strong(&state, &expectState, ME_APPLYING_DELTA)) {
        // this must not happen
        fds_panic("Unexpected migration executor state!");
    }

    return err;
}

Error
MigrationExecutor::applyRebalanceDeltaSet(fpi::CtrlObjectRebalanceDeltaSetPtr& deltaSet) {
    Error err(ERR_OK);

    LOGMIGRATE << "Sync Delta Object Set " << deltaSet->objectToPropagate.size()
             << " objects, executor ID " << deltaSet->executorID
             << " seqNum " << deltaSet->seqNum
             << " lastSet " << deltaSet->lastDeltaSet;

    fds_verify((fds_uint64_t)deltaSet->executorID == executorId);
    MigrationExecutorState curState = atomic_load(&state);
    fds_verify(curState == ME_APPLYING_DELTA);

    // if the obj data+meta list is empty, and lastDeltaSet == true,
    // nothing to apply, but have to check if we are done with migration
    if (deltaSet->objectToPropagate.size() == 0) {
        // we should't receive empty set if that's not the last message
        fds_verify(deltaSet->lastDeltaSet);
        bool completeDeltaSetReceived = seqNumDeltaSet.setDoubleSeqNum(deltaSet->seqNum,
                                                                       deltaSet->lastDeltaSet,
                                                                       0,
                                                                       true);
        if (completeDeltaSetReceived) {
            LOGNORMAL << "All DeltaSet and QoS requests accounted for executor "
                      << executorId;
            handleFirstMigrationRoundDone(ERR_OK);
        }
        // we will get more delta sets for this executor (out-of-order)
        return ERR_OK;
    }

    // if objectToPropagate set is large, break down into smaller QoS work items
    // TODO(Anna) make configurable?, dynamic?, etc
    fds_uint32_t maxSize = 10;
    fds_uint32_t totalCnt = deltaSet->objectToPropagate.size() / maxSize + 1;
    fds_uint32_t qosSeqNum = 0;

    std::vector<fpi::CtrlObjectMetaDataPropagate>::iterator itFirst, itLast;
    itFirst = deltaSet->objectToPropagate.begin();
    for (fds_uint32_t i = 0; i < totalCnt; ++i) {
        SmIoApplyObjRebalDeltaSet* applyReq =
                new(std::nothrow) SmIoApplyObjRebalDeltaSet(executorId,
                                                            deltaSet->seqNum,
                                                            deltaSet->lastDeltaSet,
                                                            qosSeqNum,
                                                            (qosSeqNum == (totalCnt - 1)));
        fds_verify(applyReq != NULL);
        applyReq->io_type = FDS_SM_APPLY_DELTA_SET;

        if (i < (totalCnt - 1)) {
            itLast = itFirst + maxSize;
        } else {
            itLast = deltaSet->objectToPropagate.end();
        }
        applyReq->deltaSet.assign(itFirst, itLast);
        applyReq->smioObjdeltaRespCb = std::bind(
            &MigrationExecutor::objDeltaAppliedCb, this,
            std::placeholders::_1, std::placeholders::_2);

        LOGMIGRATE << "Enqueue QoS Delta Set: "
                 << " qosSeq=" << applyReq->qosSeqNum
                 << " qosLastSet=" << applyReq->qosLastSet
                 << " size of qos set=" << applyReq->deltaSet.size();

        // enqueue to QoS queue
        err = dataStore->enqueueMsg(FdsSysTaskQueueId, applyReq);
        fds_verify(err.ok());   // TODO(Anna) need to save the work and try later

        // prepare for creation of next QoS request
        itFirst = itLast;
        ++qosSeqNum;
    }

    return err;
}

/// callback when apply delta set QoS message got executed
void
MigrationExecutor::objDeltaAppliedCb(const Error& error,
                                     SmIoApplyObjRebalDeltaSet* req) {
    fds_verify(req != NULL);

    // if we are in error state, do not do anything anymore...
    MigrationExecutorState curState = atomic_load(&state);
    if (curState == ME_ERROR) {
        LOGNORMAL << "MigrationExecutor in error state, ignoring the callback";
        return;
    }

    // beta2: if error happened, stop migration
    if (!error.ok()) {
        LOGERROR << "Failed to apply a set of objects " << error;
        handleMigrationDone(error);
        return;
    }

    bool completeDeltaSetReceived = seqNumDeltaSet.setDoubleSeqNum(req->seqNum,
                                                                   req->lastSet,
                                                                   req->qosSeqNum,
                                                                   req->qosLastSet);
    if (completeDeltaSetReceived) {
        // this executor finished the first round of migration
        LOGNORMAL << "All DeltaSet and QoS requests accounted for executor " << req->executorId;
        handleFirstMigrationRoundDone(error);
        return;
    }
}

Error
MigrationExecutor::startSecondObjectRebalanceRound() {
    Error err(ERR_OK);

    // send message to source SM to request second delta set
    // just one message containing executor ID
    LOGMIGRATE << "Sending request for second delta set to source SM "
               << std::hex << sourceSmUuid.uuid_get_val() << std::dec
               << " Executor ID " << executorId;

    // Reset sequence number for the second phase delta set.
    seqNumDeltaSet.resetDoubleSeqNum();

    // send msg to the source SM to start second phase of the delta set.
    if (!testMode) {
        fpi::CtrlGetSecondRebalanceDeltaSetPtr msg(new fpi::CtrlGetSecondRebalanceDeltaSet());
        msg->executorID = executorId;

        auto async2RebalSetReq = gSvcRequestPool->newEPSvcRequest(sourceSmUuid.toSvcUuid());
        async2RebalSetReq->setPayload(FDSP_MSG_TYPEID(fpi::CtrlGetSecondRebalanceDeltaSet),
                                      msg);
        async2RebalSetReq->onResponseCb(RESPONSE_MSG_HANDLER(
            MigrationExecutor::getSecondRebalanceDeltaResp));
        async2RebalSetReq->setTimeoutMs(5000);
        async2RebalSetReq->invoke();
    }

    // we sent all start second round  messages from this executor, go to next state
    MigrationExecutorState expectState = ME_SECOND_REBALANCE_ROUND;
    if (!std::atomic_compare_exchange_strong(&state, &expectState, ME_APPLYING_SECOND_DELTA)) {
        // this must not happen
        fds_panic("Unexpected migration executor state!");
    }

    return err;
}

void
MigrationExecutor::getSecondRebalanceDeltaResp(EPSvcRequest* req,
                                               const Error& error,
                                               boost::shared_ptr<std::string> payload)
{
    LOGDEBUG << "Received second rebalance delta response for executor"
             << executorId << " " << error;
    // here we just check if there is no error
    if (!error.ok()) {
        handleMigrationDone(error);
    }

    // TODO(Anna) Since we do not have second rebalance set implemented yet on source
    // SM, we are finishing migration here; remove that when source implements sending
    // second rebalance set.
    LOGMIGRATE << "Finishing migration for executor " << executorId;
    handleMigrationDone(error);
}

void
MigrationExecutor::handleFirstMigrationRoundDone(const Error& error) {
    // move to next state
    if (error.ok()) {
        MigrationExecutorState expect = ME_APPLYING_DELTA;
        if (!std::atomic_compare_exchange_strong(&state, &expect, ME_SECOND_REBALANCE_ROUND)) {
            fds_panic("Unexpected migration executor state!");
        }

        LOGMIGRATE << "First round of migration finished for executor " << executorId
                   << " src SM " << std::hex << sourceSmUuid.uuid_get_val() << std::dec
                   << ", SM token " << smTokenId;

        // notify the requested that this executor done with first round of migration
        if (migrDoneHandler) {
            migrDoneHandler(executorId, smTokenId, true, error);
        }
    } else {
        // for now we are stopping migration on any error
        handleMigrationDone(error);
    }
}

void
MigrationExecutor::handleMigrationDone(const Error& error) {
    // check and set the state
    if (error.ok()) {
        MigrationExecutorState expect = ME_APPLYING_SECOND_DELTA;
        if (!std::atomic_compare_exchange_strong(&state, &expect, ME_DONE)) {
            fds_panic("Unexpected migration executor state!");
        }
    } else {
        // beta2: any error will stop the whole migration process
        // the error state, which will stop handling any other messages for
        // this executor
        MigrationExecutorState newState = ME_ERROR;
        std::atomic_store(&state, newState);
    }

    LOGMIGRATE << "Migration finished for executor " << executorId << " src SM "
               << std::hex << sourceSmUuid.uuid_get_val() << std::dec
               << ", SM token " << smTokenId;

    // notify the requester that this executor done with migration
    if (migrDoneHandler) {
        migrDoneHandler(executorId, smTokenId, false, error);
    }
}

}  // namespace fds
