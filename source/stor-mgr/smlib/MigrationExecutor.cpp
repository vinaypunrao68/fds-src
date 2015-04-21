/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

#include <map>
#include <vector>

#include <fds_process.h>
#include <ObjMeta.h>
#include <dlt.h>
#include <SMSvcHandler.h>
#include <net/SvcRequestPool.h>
#include <MigrationExecutor.h>

namespace fds {

MigrationExecutor::MigrationExecutor(SmIoReqHandler *_dataStore,
                                     fds_uint32_t bitsPerToken,
                                     const NodeUuid& srcSmId,
                                     fds_token_id smTokId,
                                     fds_uint64_t executorID,
                                     fds_uint64_t targetDltVer,
                                     MigrationExecutorDoneHandler doneHandler)
        : executorId(executorID),
          migrDoneHandler(doneHandler),
          dataStore(_dataStore),
          bitsPerDltToken(bitsPerToken),
          smTokenId(smTokId),
          sourceSmUuid(srcSmId),
          targetDltVersion(targetDltVer)
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
    if (!std::atomic_compare_exchange_strong(&state, &expectState, ME_FIRST_PHASE_REBALANCE_START)) {
        LOGNOTIFY << "startObjectRebalance called in non- ME_INIT state " << state;
        return ERR_NOT_READY;
    }

    LOGNORMAL << "Executor " << std::hex << executorId << " will send obj ids to source SM "
              << sourceSmUuid.uuid_get_val() << std::dec << " for SM token "
              << smTokenId << " (appropriate set of DLT tokens) "
              << " target DLT version " << targetDltVersion;

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
        msg->targetDltVersion = targetDltVersion;
        msg->tokenId = dltTok;
        msg->executorID = executorId;
        msg->seqNum = seqId++;
        msg->lastFilterSet = (seqId < dltTokens.size()) ? false : true;
        LOGMIGRATE << "Executor " << std::hex << executorId << std::dec
                   << "Filter Set Msg: token=" << dltTok << ", seqNum="
                   << msg->seqNum << ", last=" << msg->lastFilterSet;
        perTokenMsgs[dltTok] = msg;
    }

    /**
     * Iterate through the level db and add to set of objects to rebalance.
     */
    bool objAddedToFilterSet = false;
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

        // Copy object metadata ref count, including volume association.
        // If the source refcnt or volume assoction information has changed, then we
        // need to get that information from the source SM and overwrite it (since
        // for now we are going to blindly trust that source SM object meta data is
        // the correct one.
        //
        // TODO(Sean):  For now, we are dealing only with the object ref_cnt and
        //              per volume association volume ref_cnt.
        fpi::CtrlObjectMetaDataSync omdFilter;
        omd.syncObjectMetaData(omdFilter);


        /* TODO(Sean):  We should add to filter object set if the ref cnt is positive (???)
         *              Are there other checks before adding to the filter set?
         */
        if (omdFilter.objRefCnt > 0) {
            LOGMIGRATE << "Executor " << std::hex << executorId << std::dec
                       << " FilterSet add ObjId=" << id << ", dltToken=" << dltTokId
                       << " refcnt=" << omdFilter.objRefCnt << " to thrift msg to source SM "
                       << std::hex << sourceSmUuid.uuid_get_val() << std::dec;
            perTokenMsgs[dltTokId]->objectsToFilter.push_back(omdFilter);
            objAddedToFilterSet = true;
        }
    }
    delete it;

    if (objAddedToFilterSet) {
        LOGCRITICAL << "UNSUPPORTED CONFIGURATION: "
                    << "Executor " << std::hex << executorId << std::dec
                    << " added at least on objet to the filter set.";
    }

    // before sending rebalance msgs to source SM, move to next state, in case we
    // receive responses before finish sending all the messages...
    // we sent all the messages, go to next state
    expectState = ME_FIRST_PHASE_REBALANCE_START;
    if (!std::atomic_compare_exchange_strong(&state, &expectState, ME_FIRST_PHASE_APPLYING_DELTA)) {
        // this must not happen
        LOGERROR << "Executor " << std::hex << executorId << std::dec
                 << ": Unexpected migration executor state";
        fds_panic("Unexpected migration executor state!");
    }
    LOGMIGRATE << "Executor " << std::hex << executorId << std::dec
               << " ME_FIRST_PHASE_REBALANCE_START --> ME_FIRST_PHASE_APPLYING_DELTA state";

    // send rebalance set of objects to source SM
    for (auto tok : dltTokens) {
        LOGMIGRATE << "Executor " << std::hex << executorId << std::dec
                   << " sending rebalance initial set for DLT token "
                   << tok << " set size " << perTokenMsgs[tok]->objectsToFilter.size()
                   << " to source SM "
                   << std::hex << sourceSmUuid.uuid_get_val() << std::dec;
        if (!testMode) {
            try {
                auto asyncRebalSetReq = gSvcRequestPool->newEPSvcRequest(sourceSmUuid.toSvcUuid());
                asyncRebalSetReq->setPayload(FDSP_MSG_TYPEID(fpi::CtrlObjectRebalanceFilterSet),
                                       perTokenMsgs[tok]);
                asyncRebalSetReq->onResponseCb(RESPONSE_MSG_HANDLER(
                    MigrationExecutor::objectRebalanceFilterSetResp));
                asyncRebalSetReq->setTimeoutMs(5000);
                // we are not waiting for response, so not setting a callback
                asyncRebalSetReq->invoke();
            }
            /* TODO(Gurpreet): We should handle the exception and propogate the error to
             * Token Migration Manager.
             */
            catch (...) {
                LOGMIGRATE << "Async rebalance request failed for token " << tok << "to source SM "
                << std::hex << sourceSmUuid.uuid_get_val() << std::dec;
            }
        }
    }

    LOGMIGRATE << "Executor " << std::hex << executorId << std::dec
               << " sent rebalance initial set msgs to source SM"
               << std::hex << sourceSmUuid.uuid_get_val() << std::dec;
    return err;
}

void
MigrationExecutor::objectRebalanceFilterSetResp(EPSvcRequest* req,
                                                const Error& error,
                                                boost::shared_ptr<std::string> payload)
{
    LOGDEBUG << "Received CtrlObjectRebalanceFilterSet response for executor "
             << std::hex << executorId << std::dec << " " << error;
    // here we just check for errors
    if (!error.ok()) {
        LOGERROR << "CtrlObjectRebalanceFilterSet response " << error;
    }
}

Error
MigrationExecutor::applyRebalanceDeltaSet(fpi::CtrlObjectRebalanceDeltaSetPtr& deltaSet) {
    Error err(ERR_OK);

    fds_verify((fds_uint64_t)deltaSet->executorID == executorId);
    MigrationExecutorState curState = atomic_load(&state);
    fds_verify((curState == ME_FIRST_PHASE_APPLYING_DELTA) ||
               (curState == ME_SECOND_PHASE_APPLYING_DELTA));

    LOGMIGRATE << "Sync Delta Object Set: " << deltaSet->objectToPropagate.size()
               << " objects, executor ID=" << deltaSet->executorID
               << " seqNum=" << deltaSet->seqNum
               << " lastSet=" << deltaSet->lastDeltaSet
               << " first rebalance round=" << (curState == ME_FIRST_PHASE_APPLYING_DELTA);

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
                      << std::hex << executorId << std::dec;
            handleMigrationRoundDone(err);
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
                   << " size of qos set=" << applyReq->deltaSet.size()
                   << " first rebalance round=" << (curState == ME_FIRST_PHASE_APPLYING_DELTA);

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
        handleMigrationRoundDone(error);
        return;
    }

    // here no error happened so far, so we must be in one of
    // the applying delta states
    fds_verify((curState == ME_FIRST_PHASE_APPLYING_DELTA) ||
               (curState == ME_SECOND_PHASE_APPLYING_DELTA));

    bool completeDeltaSetReceived = seqNumDeltaSet.setDoubleSeqNum(req->seqNum,
                                                                   req->lastSet,
                                                                   req->qosSeqNum,
                                                                   req->qosLastSet);
    if (completeDeltaSetReceived) {
        // this executor finished the first or second round of migration, based on state
        LOGNORMAL << "All DeltaSet and QoS requests accounted for executor "
                  << std::hex << req->executorId << std::dec;
        handleMigrationRoundDone(error);
        return;
    }
}

Error
MigrationExecutor::startSecondObjectRebalanceRound() {
    Error err(ERR_OK);

    // send message to source SM to request second delta set
    // just one message containing executor ID
    LOGMIGRATE << "Sending request for second delta set to source SM "
               << std::hex << sourceSmUuid.uuid_get_val()
               << " Executor ID " << executorId << std::dec;

    // Reset sequence number for the second phase delta set.
    seqNumDeltaSet.resetDoubleSeqNum();

    // move to the next state before sending the message, in case we get the reply
    // while still in this method
    MigrationExecutorState expectState = ME_SECOND_PHASE_REBALANCE_START;
    if (!std::atomic_compare_exchange_strong(&state, &expectState, ME_SECOND_PHASE_APPLYING_DELTA)) {
        // this must not happen
        LOGERROR << "Executor " << std::hex << executorId << std::dec
                 << ": Unexpected migration executor state";
        fds_panic("Unexpected migration executor state!");
    }
    LOGMIGRATE << "Executor " << std::hex << executorId << std::dec
               << " ME_SECOND_PHASE_REBALANCE_START --> ME_SECOND_PHASE_APPLYING_DELTA state";

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

    return err;
}

void
MigrationExecutor::getSecondRebalanceDeltaResp(EPSvcRequest* req,
                                               const Error& error,
                                               boost::shared_ptr<std::string> payload)
{
    LOGDEBUG << "Received second rebalance delta response for executor "
             << std::hex << executorId << std::dec << " " << error;
    // here we just check for errors
    if (!error.ok()) {
        handleMigrationRoundDone(error);
    }
}

void
MigrationExecutor::handleMigrationRoundDone(const Error& error) {
    fds_bool_t firstRoundFinished = false;
    // check and set the state
    if (error.ok()) {
        // if no error, we must be in one of the apply delta states

        // if we were in first round of migration, go to second round
        MigrationExecutorState expect = ME_FIRST_PHASE_APPLYING_DELTA;
        if (!std::atomic_compare_exchange_strong(&state,
                                                 &expect,
                                                 ME_SECOND_PHASE_REBALANCE_START)) {
            // ok, see if we are in the second round of migration
            expect = ME_SECOND_PHASE_APPLYING_DELTA;
            if (!std::atomic_compare_exchange_strong(&state, &expect, ME_DONE)) {
                // must be a bug somewhere...
                fds_panic("Unexpected migration executor state!");
            }
            // we finished second phase of migration. If this is resync after restart
            // send finish client resync message to the source.
        } else {
            firstRoundFinished = true;
            // we just finished first round and started second round
        }
    } else {
        // beta2: any error will stop the whole migration process
        // the error state, which will stop handling any other messages for
        // this executor
        MigrationExecutorState newState = ME_ERROR;
        std::atomic_store(&state, newState);

        // in case the source started forwarding, we don't want it to continue
        // on error; so just send stop client resync message to source SM so
        // it can cleanup and stop forwarding
    }

    LOGMIGRATE << "Migration finished for executor " << std::hex << executorId
               << " src SM " << sourceSmUuid.uuid_get_val() << std::dec
               << ", SM token " << smTokenId
               << " firstRound? " << firstRoundFinished;

    // notify the requester that this executor done with migration
    if (migrDoneHandler) {
        migrDoneHandler(executorId, smTokenId, firstRoundFinished, error);
    }
}

}  // namespace fds
