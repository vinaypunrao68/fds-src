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
#include <MigrationMgr.h>

namespace fds {

MigrationExecutor::MigrationExecutor(SmIoReqHandler *_dataStore,
                                     fds_uint32_t bitsPerToken,
                                     const NodeUuid& srcSmId,
                                     fds_token_id smTokId,
                                     fds_uint64_t executorID,
                                     fds_uint64_t targetDltVer,
                                     fds_uint32_t migrType,
                                     bool resync,
                                     MigrationDltFailedCb failedRetryHandler,
                                     MigrationExecutorDoneHandler doneHandler,
                                     FdsTimerPtr& timeoutTimer,
                                     uint32_t timeoutDuration,
                                     TimeoutCb timeoutCb,
                                     MigrationAbortCb abortCallback,
                                     fds_uint32_t uid,
                                     fds_uint16_t iNum)
        : timeoutCb(timeoutCb),
          executorId(executorID),
          migrDoneHandler(doneHandler),
          migrFailedRetryHandler(failedRetryHandler),
          dataStore(_dataStore),
          bitsPerDltToken(bitsPerToken),
          smTokenId(smTokId),
          sourceSmUuid(srcSmId),
          targetDltVersion(targetDltVer),
          migrationType(migrType),
          onePhaseMigration(resync),
          seqNumDeltaSet(timeoutTimer, timeoutDuration, std::bind(&MigrationExecutor::handleTimeout, this)),
          abortMigrationCb(abortCallback),
          uniqueId(uid),
          instanceNum(iNum)
{
    state = ATOMIC_VAR_INIT(ME_INIT);
    abortPending = false;
}

MigrationExecutor::~MigrationExecutor()
{
}

void MigrationExecutor::handleTimeout() {
    this->timeoutCb(this->executorId, this->smTokenId,
                    this->dltTokens, migrationRound(),
                    ERR_SM_TOK_MIGRATION_TIMEOUT);
}

void
MigrationExecutor::addDltToken(fds_token_id dltTok) {
    MigrationExecutorState curState = atomic_load(&state);
    fds_verify(curState == ME_INIT);
    fds_verify(dltTokens.count(dltTok) == 0);
    dltTokens.insert(dltTok);
}

fds_bool_t
MigrationExecutor::responsibleForDltToken(fds_token_id dltTok) const {
    return (dltTokens.count(dltTok) > 0);
}

// DO NOT release snapshot here, because it maybe passed to other
// migration executors
Error
MigrationExecutor::startObjectRebalanceAgain(leveldb::ReadOptions& options,
                                             std::shared_ptr<leveldb::DB> db)
{
    Error err(ERR_OK);
    ObjMetaData omd;

    /**
     * If abort is pending for this Executor. Exit.
     */
    if (isAbortPending()) {
        LOGMIGRATE << "Aborting migration for Executor " << std::hex << executorId
                   << " from source SM " << sourceSmUuid.uuid_get_val() << std::dec
                   << " for SM token " << smTokenId << " target DLT version "
                   << targetDltVersion;
        abortMigrationCb(executorId, smTokenId);
        return ERR_SM_TOK_MIGRATION_ABORTED;
    }

    /**
     * Take a scoped lock for retryDltTokens map as it's accessed several
     * times in the method.
     */
    fds_mutex::scoped_lock l(retryDltTokensLock);

    /**
     * Return if no dlt tokens to retry migration for from the same source SM.
     */
    if (retryDltTokens.empty()) {
        return err;
    }

    // Track IO request for startObjectRebalance.
    // If we can successfully start tracking IO request, then proceed with tracking it.
    // If we can't start trakcing IO request, then terminate this request.
    if (!trackIOReqs.startTrackIOReqs()) {
        // For now, just return an error that migration is aborted.
        return ERR_SM_TOK_MIGRATION_ABORTED;
    }

    if (!onePhaseMigration) {
        MigrationExecutorState expectState = ME_FIRST_PHASE_APPLYING_DELTA;
        if (!std::atomic_compare_exchange_strong(&state,
                                                 &expectState,
                                                 ME_FIRST_PHASE_APPLYING_DELTA)) {
            LOGNOTIFY << "Non-ME_INIT state " << state;

            // Since the state transition failed, stop tracking this IO.
            trackIOReqs.finishTrackIOReqs();

            return ERR_NOT_READY;
        }
    } else {
        MigrationExecutorState expectState = ME_SECOND_PHASE_APPLYING_DELTA;
        if (!std::atomic_compare_exchange_strong(&state,
                                                 &expectState,
                                                 ME_SECOND_PHASE_APPLYING_DELTA)) {
            LOGNOTIFY << "Non-ME_SECOND_PHASE_APPLYING_DELTA state " << state;

            // Since the state transition failed, stop tracking this IO.
            trackIOReqs.finishTrackIOReqs();

            return ERR_NOT_READY;
        }
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
    for (auto &dltTok : retryDltTokens) {
        // for now packing all objects per one DLT token into one message
        fpi::CtrlObjectRebalanceFilterSetPtr msg(new fpi::CtrlObjectRebalanceFilterSet());
        msg->targetDltVersion = targetDltVersion;
        msg->tokenId = dltTok.first;
        msg->executorID = executorId;
        msg->seqNum = dltTok.second;
        msg->lastFilterSet = ((dltTok.second + 1) < dltTokens.size()) ? false : true;
        msg->onePhaseMigration = onePhaseMigration;
        LOGMIGRATE << "Executor " << std::hex << executorId << std::dec
                   << "Filter Set Msg: token=" << msg->tokenId << ", seqNum="
                   << msg->seqNum << ", last=" << msg->lastFilterSet;
        perTokenMsgs[dltTok.first] = msg;
    }

    /**
     * Iterate through the level db and add to set of objects to rebalance.
     */
    bool objAddedToFilterSet = false;
    for (it->SeekToFirst(); it->Valid(); it->Next()) {
        ObjectID id(it->key().ToString());
        // send objects that belong to DLT tokens that need to be migrated from src SM
        fds_token_id dltTokId = DLT::getToken(id, bitsPerDltToken);
        if (retryDltTokens.find(dltTokId) == retryDltTokens.end()) {
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

    for (auto &dltTok : retryDltTokens) {
        LOGMIGRATE << "Executor " << std::hex << executorId << std::dec
                   << " sending rebalance initial set for DLT token "
                   << dltTok.first << " set size "
                   << perTokenMsgs[dltTok.first]->objectsToFilter.size()
                   << " to source SM "
                   << std::hex << sourceSmUuid.uuid_get_val() << std::dec;
        try {
            auto asyncRebalSetReq = gSvcRequestPool->newEPSvcRequest(sourceSmUuid.toSvcUuid());
            asyncRebalSetReq->setPayload(FDSP_MSG_TYPEID(fpi::CtrlObjectRebalanceFilterSet),
                                   perTokenMsgs[dltTok.first]);
            asyncRebalSetReq->onResponseCb(RESPONSE_MSG_HANDLER(
                MigrationExecutor::objectRebalanceFilterSetResp,
                dltTok.first,
                dltTok.second));
            asyncRebalSetReq->setTimeoutMs(5000);
            asyncRebalSetReq->invoke();
        }
        /* TODO(Gurpreet): We should handle the exception and propogate the error to
         * Token Migration Manager.
         */
        catch (...) {
            LOGMIGRATE << "Async rebalance request failed for token " << dltTok.first << "to source SM "
                       << std::hex << sourceSmUuid.uuid_get_val() << std::dec;
        }
    }

    retryDltTokens.clear();

    LOGMIGRATE << "Executor " << std::hex << executorId << std::dec
               << " sent rebalance initial set msgs to source SM"
               << std::hex << sourceSmUuid.uuid_get_val() << std::dec;

    // Completed this IO request.  Stop tracking this IO.
    trackIOReqs.finishTrackIOReqs();

    return err;
}

// DO NOT release snapshot here, becuase it maybe passed to other
// migration executors
Error
MigrationExecutor::startObjectRebalance(leveldb::ReadOptions& options,
                                        std::shared_ptr<leveldb::DB> db)
{
    /**
     * If abort is pending for this Executor. Exit.
     */
    if (isAbortPending()) {
        LOGMIGRATE << "Aborting migration for Executor " << std::hex << executorId
                   << " from source SM " << sourceSmUuid.uuid_get_val() << std::dec
                   << " for SM token " << smTokenId << " target DLT version "
                   << targetDltVersion;
        abortMigrationCb(executorId, smTokenId);
        return ERR_SM_TOK_MIGRATION_ABORTED;
    }

    LOGMIGRATE << "startObjectRebalance - Executor " << std::hex << executorId << std::dec
               << " instanceNum = " << instanceNum << " uniqueId = " << uniqueId;
    Error err(ERR_OK);
    ObjMetaData omd;

    // Track IO request for startObjectRebalance.
    // If we can successfully start tracking IO request, then proceed with tracking it.
    // If we can't start trakcing IO request, then terminate this request.
    if (!trackIOReqs.startTrackIOReqs()) {
        // For now, just return an error that migration is aborted.
        return ERR_SM_TOK_MIGRATION_ABORTED;
    }

    MigrationExecutorState expectState = ME_INIT;
    MigrationExecutorState nextState = ME_INIT;
    if (!std::atomic_compare_exchange_strong(&state,
                                             &expectState,
                                             ME_FIRST_PHASE_REBALANCE_START)) {
        LOGNOTIFY << "startObjectRebalance called in non- ME_INIT state " << state;

        // Since the state transition failed, stop tracking this IO.
        trackIOReqs.finishTrackIOReqs();

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
        msg->onePhaseMigration = onePhaseMigration;
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

    // before sending rebalance msgs to source SM, move to next state, in case we
    // receive responses before finish sending all the messages...
    // we sent all the messages, go to next state
    expectState = ME_FIRST_PHASE_REBALANCE_START;
    if (onePhaseMigration) {
        nextState = ME_SECOND_PHASE_APPLYING_DELTA;
    } else {
        nextState = ME_FIRST_PHASE_APPLYING_DELTA;
    }
    if (!std::atomic_compare_exchange_strong(&state, &expectState, nextState)) {
        // this must not happen
        LOGERROR << "Executor " << std::hex << executorId << std::dec
                 << ": Unexpected migration executor state";
        fds_panic("Unexpected migration executor state!");
    }
    LOGMIGRATE << "Executor " << std::hex << executorId << std::dec
               << " ME_FIRST_PHASE_REBALANCE_START --> " << nextState;

    // send rebalance set of objects to source SM
    // since we may start receiving responses with declines, we may change
    // dltTokens (remove tokens that we are resyncing). So work on copy
    // of token set, in case dltTokens start changing...
    std::set<fds_token_id> dltTokensCopy;
    dltTokensCopy.insert(dltTokens.begin(), dltTokens.end());
    for (auto tok : dltTokensCopy) {
        LOGMIGRATE << "Executor " << std::hex << executorId << std::dec
                   << " sending rebalance initial set for DLT token "
                   << tok << " set size " << perTokenMsgs[tok]->objectsToFilter.size()
                   << " to source SM "
                   << std::hex << sourceSmUuid.uuid_get_val() << std::dec;
        try {
            auto asyncRebalSetReq = gSvcRequestPool->newEPSvcRequest(sourceSmUuid.toSvcUuid());
            asyncRebalSetReq->setPayload(FDSP_MSG_TYPEID(fpi::CtrlObjectRebalanceFilterSet),
                                         perTokenMsgs[tok]);
            asyncRebalSetReq->onResponseCb(RESPONSE_MSG_HANDLER(MigrationExecutor::objectRebalanceFilterSetResp,
                                                                tok,
                                                                perTokenMsgs[tok]->seqNum));
            asyncRebalSetReq->setTimeoutMs(5000);
            asyncRebalSetReq->invoke();

            // for each msg we sent, start tracking this as IO -- because we want callback still exist
            // when reply happens on abort path
            if (!trackIOReqs.startTrackIOReqs()) {
                // For now, just return an error that migration is aborted.
                return ERR_SM_TOK_MIGRATION_ABORTED;
            }
        }
        /* TODO(Gurpreet): We should handle the exception and propogate the error to
         * Token Migration Manager.
         */
        catch (...) {
            LOGMIGRATE << "Async rebalance request failed for token " << tok << "to source SM "
                       << std::hex << sourceSmUuid.uuid_get_val() << std::dec;
        }

        // TODO(Gurpreet): Add proper error code during integration of
        // failure handling for migration.
        fiu_do_on("fail.sm.migration.sending.filter.set",
                  LOGDEBUG << "fault fail.sm.migration.sending.filter.set enabled"; \
                  if (executorId % 20 == 0) { trackIOReqs.finishTrackIOReqs(); \
                                              return ERR_SM_TOK_MIGRATION_ABORTED;});
    }

    LOGMIGRATE << "Executor " << std::hex << executorId << std::dec
               << " sent rebalance initial set msgs to source SM"
               << std::hex << sourceSmUuid.uuid_get_val() << std::dec;

    // Completed this IO request.  Stop tracking this IO.
    trackIOReqs.finishTrackIOReqs();

    return err;
}

void
MigrationExecutor::objectRebalanceFilterSetResp(fds_token_id dltToken,
                                                uint64_t seqId,
                                                EPSvcRequest* req,
                                                const Error& error,
                                                boost::shared_ptr<std::string> payload)
{
    LOGDEBUG << "Received CtrlObjectRebalanceFilterSet response for executor "
             << std::hex << executorId << std::dec << " DLT token " << dltToken
             << " " << error;

    // Completed this IO request.  Stop tracking this IO.
    trackIOReqs.finishTrackIOReqs();

    /**
     * If abort is pending for this Executor. Exit.
     */
    if (isAbortPending()) {
        LOGMIGRATE << "Aborting migration for Executor " << std::hex << executorId
                   << " from source SM " << sourceSmUuid.uuid_get_val() << std::dec
                   << " for SM token " << smTokenId << " target DLT version "
                   << targetDltVersion;
        abortMigrationCb(executorId, smTokenId);
        return;
    }

    if (inErrorState()) {
        LOGDEBUG << "Ignoring CtrlObjectRebalanceFilterSet response for executor "
                 << std::hex << executorId << std::dec << " DLT token " << dltToken
                 << " " << error << " since Migration Executor is in " << getState()
                 << " state";
        return;
    }

    // here we just check for errors
    if (!error.ok()) {
        switch (error.GetErrno()) {
            case ERR_SM_RESYNC_SOURCE_DECLINE:
                // SM declined to be a source for DLT token
                LOGMIGRATE << "CtrlObjectRebalanceFilterSet declined for dlt token " << dltToken
                           << " ; ok will stop resync for this dlt token, executor "
                           << std::hex << executorId << std::dec;
                dltTokens.erase(dltToken);
                // notify that this particular DLT token is not going to resync = ready
                if (migrDoneHandler) {
                    std::set<fds_token_id> oneTokSet;
                    oneTokSet.insert(dltToken);
                    fds_uint32_t round = 0;
                    LOGMIGRATE << "Migration finished for executor " << std::hex << executorId
                               << " src SM " << sourceSmUuid.uuid_get_val() << std::dec
                               << " instanceNum = " << instanceNum << " uniqueId = " << uniqueId
                               << ", SM token " << smTokenId
                               << " Round " << round
                               << " isResync? " << onePhaseMigration;
                    migrDoneHandler(executorId, smTokenId, oneTokSet, round, error);
                }
                if (dltTokens.size() == 0) {
                    // resync was declined for all DLT tokens of this executor, we are done
                    LOGMIGRATE << "Resync was declined for all DLT tokens for executor "
                               << std::hex << executorId << std::dec;
                    handleMigrationRoundDone(ERR_OK);
                }
                break;
            case ERR_SM_NOT_READY_AS_MIGR_SRC:
                LOGMIGRATE << "CtrlObjectRebalanceFilterSet declined for dlt token " << dltToken
                           << " source SM " << sourceSmUuid << " not ready";
                migrFailedRetryHandler(smTokenId);
                {
                    fds_mutex::scoped_lock l(retryDltTokensLock);
                    retryDltTokens[dltToken] = seqId;
                }
                break;
            default:
                LOGERROR << "CtrlObjectRebalanceFilterSet for token " << dltToken
                         << " executor " << std::hex << executorId << std::dec
                         << " response " << error;
                // if this is network error, don't return it to OM, because it may
                // look like this node is unreachable
                Error migrErr(error);
                if ((error == ERR_SVC_REQUEST_TIMEOUT) || (error == ERR_SVC_REQUEST_INVOCATION)) {
                    migrErr = ERR_SM_TOK_MIGRATION_SRC_SVC_REQUEST;
                }
                handleMigrationRoundDone(migrErr);
        }
    }
}

Error
MigrationExecutor::applyRebalanceDeltaSet(fpi::CtrlObjectRebalanceDeltaSetPtr& deltaSet)
{
    Error err(ERR_OK);

    /**
     * If abort is pending for this Executor. Exit.
     */
    if (isAbortPending()) {
        LOGMIGRATE << "Aborting migration for Executor " << std::hex << executorId
                   << " from source SM " << sourceSmUuid.uuid_get_val() << std::dec
                   << " for SM token " << smTokenId << " target DLT version "
                   << targetDltVersion;
        abortMigrationCb(executorId, smTokenId);
        return ERR_SM_TOK_MIGRATION_ABORTED;
    }

    if (inErrorState()) {
        LOGDEBUG << "Ignoring delta set for executor " << std::hex << executorId
                 << " since Migration Executor is in " << getState() << " state";
        return ERR_SM_TOK_MIGRATION_ABORTED;
    }

    // Track apply delta set.  If we can't track IO, then this MigrationExecutor is
    // being coalesced.
    if (!trackIOReqs.startTrackIOReqs()) {
        return ERR_SM_TOK_MIGRATION_ABORTED;
    }

    fds_verify((fds_uint64_t)deltaSet->executorID == executorId);
    MigrationExecutorState curState = atomic_load(&state);
    fds_verify((curState == ME_FIRST_PHASE_APPLYING_DELTA) ||
               (curState == ME_SECOND_PHASE_APPLYING_DELTA));

    LOGMIGRATE << "Sync Delta Object Set: " << deltaSet->objectToPropagate.size()
               << " objects, executor ID=" << std::hex << deltaSet->executorID << std::dec
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
	    trackIOReqs.finishTrackIOReqs();
            handleMigrationRoundDone(err);
	    return ERR_OK;
        }

        // Empty delta set, so no need to track this IO.
        trackIOReqs.finishTrackIOReqs();

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
        applyReq->smioObjdeltaRespCb = std::bind(&MigrationExecutor::objDeltaAppliedCb,
                                                 this,
                                                 std::placeholders::_1,
                                                 std::placeholders::_2);

        LOGMIGRATE << "Enqueue QoS Delta Set: "
                   << " qosSeq=" << applyReq->qosSeqNum
                   << " qosLastSet=" << applyReq->qosLastSet
                   << " size of qos set=" << applyReq->deltaSet.size()
                   << " first rebalance round=" << (curState == ME_FIRST_PHASE_APPLYING_DELTA);

        // Before enqueuing applyDelta, start tracking IO...  It will be untracked in
        // objDeltaAppliedCb() function.
        if (!trackIOReqs.startTrackIOReqs()) {
            // If can't track, then no need to enqueue IO, since this MigrationExecutor is being
            // coalesced.
            //
            // TODO(Sean):  Uhm...  Wonder if this will have side effects.
            break;
        }

        // enqueue to QoS queue
        err = dataStore->enqueueMsg(FdsSysTaskQueueId, applyReq);
        fds_verify(err.ok());   // TODO(Anna) need to save the work and try later

        // prepare for creation of next QoS request
        itFirst = itLast;
        ++qosSeqNum;
    }

    // Completed with enqueueing delta set request to QoS.
    trackIOReqs.finishTrackIOReqs();

    return err;
}

/// callback when apply delta set QoS message got executed
void
MigrationExecutor::objDeltaAppliedCb(const Error& error,
                                     SmIoApplyObjRebalDeltaSet* req) {
    fds_verify(req != NULL);

    // if we are in error state, do not do anything anymore...
    MigrationExecutorState curState = atomic_load(&state);
    if (inErrorState() || isAbortPending()) {
        LOGNORMAL << "MigrationExecutor in error state, ignoring the callback";

        // Stop tracking this IO.
        trackIOReqs.finishTrackIOReqs();
        abortMigrationCb(executorId, smTokenId);
        return;
    }

    // beta2: if error happened, stop migration
    if (!error.ok()) {
        LOGERROR << "Failed to apply a set of objects " << error;
        handleMigrationRoundDone(error);

        // Stop tracking this IO.
        trackIOReqs.finishTrackIOReqs();
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

        // Stop tracking this IO.
        trackIOReqs.finishTrackIOReqs();

        handleMigrationRoundDone(error);
        return;
    }

    // Stop tracking this IO.
    trackIOReqs.finishTrackIOReqs();

}

Error
MigrationExecutor::startSecondObjectRebalanceRound() {
    Error err(ERR_OK);

    /**
     * If abort is pending for this Executor. Exit.
     */
    if (isAbortPending()) {
        LOGMIGRATE << "Aborting migration for Executor " << std::hex << executorId
                   << " from source SM " << sourceSmUuid.uuid_get_val() << std::dec
                   << " for SM token " << smTokenId << " target DLT version "
                   << targetDltVersion;
        abortMigrationCb(executorId, smTokenId);
        return ERR_SM_TOK_MIGRATION_ABORTED;
    }

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
    bool stateChanged = std::atomic_compare_exchange_strong(&state,
                                                            &expectState,
                                                            ME_SECOND_PHASE_APPLYING_DELTA);
    if (!stateChanged) {
        //On error, expectState now has the current state of Migration Executor.
        if (expectState != ME_ERROR && expectState != ME_DONE_WITH_ERROR) {
            // this must not happen
            LOGERROR << "Executor " << std::hex << executorId << std::dec
                     << ": Unexpected migration executor state " << state;
            fds_panic("Unexpected migration executor state!");
        } else {
            LOGNOTIFY << "Executor " << std::hex << executorId << std::dec
                      << " is in error state. Do not start it's second phase.";
            handleMigrationRoundDone(ERR_SM_TOK_MIGRATION_ABORTED);
            return err;
        }
    }
    LOGMIGRATE << "Executor " << std::hex << executorId << std::dec
               << " ME_SECOND_PHASE_REBALANCE_START --> ME_SECOND_PHASE_APPLYING_DELTA state";

    // send msg to the source SM to start second phase of the delta set.
    fpi::CtrlGetSecondRebalanceDeltaSetPtr msg(new fpi::CtrlGetSecondRebalanceDeltaSet());
    msg->executorID = executorId;

    auto async2RebalSetReq = gSvcRequestPool->newEPSvcRequest(sourceSmUuid.toSvcUuid());
    async2RebalSetReq->setPayload(FDSP_MSG_TYPEID(fpi::CtrlGetSecondRebalanceDeltaSet),
                                  msg);
    async2RebalSetReq->onResponseCb(RESPONSE_MSG_HANDLER(MigrationExecutor::getSecondRebalanceDeltaResp));
    async2RebalSetReq->setTimeoutMs(5000);
    async2RebalSetReq->invoke();

    // TODO(Gurpreet): Add proper error code during integration of
    // failure handling for migration.
    fiu_do_on("fail.sm.migration.second.rebalance.req",
              LOGDEBUG << "fault fail.sm.migration.second.rebalance.req enabled"; \
              if (executorId % 20 == 0) return ERR_SM_TOK_MIGRATION_ABORTED;);

    return err;
}

void
MigrationExecutor::getSecondRebalanceDeltaResp(EPSvcRequest* req,
                                               const Error& error,
                                               boost::shared_ptr<std::string> payload)
{
    LOGDEBUG << "Received second rebalance delta response for executor "
             << std::hex << executorId << std::dec << " " << error;

    /**
     * If abort is pending for this Executor. Exit.
     */
    if (isAbortPending()) {
        LOGMIGRATE << "Aborting migration for Executor " << std::hex << executorId
                   << " from source SM " << sourceSmUuid.uuid_get_val() << std::dec
                   << " for SM token " << smTokenId << " target DLT version "
                   << targetDltVersion;
        abortMigrationCb(executorId, smTokenId);
        return;
    }

    if (inErrorState()) {
        LOGDEBUG << "Ignoring CtrlGetSecondRebalanceDeltaSet response for executor "
                 << std::hex << executorId << std::dec << " " << error
                 << " since Migration Executor is in " << getState() << " state";
        return;
    }

    // TODO(Gurpreet): Add proper error code during integration of
    // failure handling for migration.
    fiu_do_on("fail.sm.migration.second.rebalance.resp",
              LOGDEBUG << "fault fail.sm.migration.second.rebalance.resp enabled"; \
              return;);

    // here we just check for errors
    if (!error.ok()) {
        handleMigrationRoundDone(error);
    }
}

void
MigrationExecutor::handleMigrationRoundDone(const Error& error) {
    fds_uint32_t roundNum = 2;
    LOGMIGRATE << "handleMigrationRoundDone " << error;
    // check and set the state
    if (error.ok()) {
        // if no error, we must be in one of the apply delta states

        // if we were in first round of migration, go to second round
        MigrationExecutorState expect = ME_FIRST_PHASE_APPLYING_DELTA;
        if (!std::atomic_compare_exchange_strong(&state,
                                                 &expect,
                                                 ME_SECOND_PHASE_REBALANCE_START)) {
            LOGMIGRATE << "ok, see if we are in the second round of migration";
            // ok, see if we are in the second round of migration
            expect = ME_SECOND_PHASE_APPLYING_DELTA;
            if (!std::atomic_compare_exchange_strong(&state, &expect, ME_DONE)) {
                MigrationExecutorState curState = atomic_load(&state);
                // check if we are in done with error state for a sm resync
                if (!((migrationType == SMMigrType::MIGR_SM_RESYNC) &&
                    (curState == ME_DONE_WITH_ERROR))) {
                    // must be a bug somewhere...
                    fds_panic("Unexpected migration executor state!");
                }            
            }
            // we finished second phase of migration. If this is resync after restart
            // send finish client resync message to the source.
            if (migrationType == SMMigrType::MIGR_SM_RESYNC) {
                sendFinishResyncToClient();
            }
        } else {
            LOGMIGRATE << "we just finished first round and started second round";
            roundNum = 1;
            // we just finished first round and started second round
        }
    } else {
        MigrationExecutorState newState = ME_ERROR;
        roundNum = migrationRound();
        if (std::atomic_exchange(&state, newState) == ME_ERROR) {
            /**
             * Ignore handling of migration round because migration executor
             * is already in error state. Error handling would have been done
             * the first time this executor saw an error and it's state was set
             * to ME_DONE_WITH_ERROR.
             */
            return;
        }

        if (migrationType == SMMigrType::MIGR_SM_RESYNC) {
            // in case the source started forwarding, we don't want it to continue
            // on error; so just send stop client resync message to source SM so
            // it can cleanup and stop forwarding
            sendFinishResyncToClient();
        }
    }

    LOGMIGRATE << "Migration finished for executor " << std::hex << executorId 
               << " src SM " << sourceSmUuid.uuid_get_val() << std::dec
               << " instanceNum = " << instanceNum << " uniqueId = " << uniqueId
               << ", SM token " << smTokenId
               << " Round " << roundNum
               << " isResync? " << onePhaseMigration;

    // notify the requester that this executor done with migration
    if (migrDoneHandler) {
        migrDoneHandler(executorId, smTokenId, dltTokens, roundNum, error);
    }
}

void
MigrationExecutor::sendFinishResyncToClient()
{
    LOGMIGRATE << "Executor " << std::hex << executorId << std::dec
               << " sending finish resync msg to Client";

    // send message to source SM to finish resync for this executor
    fpi::CtrlFinishClientTokenResyncMsgPtr msg(new fpi::CtrlFinishClientTokenResyncMsg());
    msg->executorID = executorId;

    auto asyncFinishClientReq = gSvcRequestPool->newEPSvcRequest(sourceSmUuid.toSvcUuid());
    asyncFinishClientReq->setPayload(FDSP_MSG_TYPEID(fpi::CtrlFinishClientTokenResyncMsg), msg);
    asyncFinishClientReq->onResponseCb(RESPONSE_MSG_HANDLER(MigrationExecutor::finishResyncResp));
    asyncFinishClientReq->setTimeoutMs(5000);
    asyncFinishClientReq->invoke();
}

void
MigrationExecutor::finishResyncResp(EPSvcRequest* req,
                                    const Error& error,
                                    boost::shared_ptr<std::string> payload)
{
    LOGMIGRATE << "Received finish resync response from client for executor "
               << std::hex << executorId << std::dec << " " << error;

    // here we just print an error if that happened... but nothing
    // we can do on destination since we already either finished sync or
    // aborted with error
    if (!error.ok()) {
        LOGWARN << "Received error for finish resync from client " << error
                << ", not changing any state, source should be able to deal with it";
    }
}


/**
 * We will wait for all pending IOs to complete before cleaning up executor.
 * Once the ABORT state is set at the MigrationMgr, IOs are not directed to
 * the MigrationExecutor.  However, there can be pending IOs in flight that we
 * we need to track.  Here is the list of IO operations we track for MigrationExecutor.
 */
void
MigrationExecutor::waitForIOReqsCompletion(fds_token_id tok, NodeUuid nodeUuid)
{
    LOGMIGRATE << "Waiting for pending requests to complete: "
               << " tok=" << tok
               << " NodeUuid=" << std::hex << nodeUuid.uuid_get_val() << std::dec;

    trackIOReqs.waitForTrackIOReqs();

    LOGMIGRATE << "Completed all pending requests: "
               << " tok=" << tok
               << " NodeUuid=" << std::hex << nodeUuid.uuid_get_val() << std::dec;
}

void
MigrationExecutor::abortMigration(const Error &err) {
    setDoneWithError();
    if (migrationType == SMMigrType::MIGR_SM_RESYNC) {
        sendFinishResyncToClient();
    }
}

void
MigrationExecutor::clearRetryDltTokenSet()
{
    fds_mutex::scoped_lock l(retryDltTokensLock);
    retryDltTokens.clear();
}

}  // namespace fds
