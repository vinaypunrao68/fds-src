/*
 * Copyright 2013-2014 Formation Data Systems, Inc.
 */

#include <vector>
#include <object-store/SmDiskMap.h>
#include <MigrationMgr.h>
#include <fds_process.h>
#include <fdsp_utils.h>
#include "PerfTrace.h"

namespace fds {

MigrationMgr::MigrationMgr(SmIoReqHandler *dataStore)
        : smReqHandler(dataStore),
          omStartMigrCb(NULL),
          targetDltVersion(DLT_VER_INVALID),
          numBitsPerDltToken(0)
{
    migrState = ATOMIC_VAR_INIT(MIGR_IDLE);
    nextLocalExecutorId = ATOMIC_VAR_INIT(1);
    snapshotRequest.io_type = FDS_SM_SNAPSHOT_TOKEN;
    snapshotRequest.retryReq = false;
    snapshotRequest.smio_snap_resp_cb = std::bind(&MigrationMgr::smTokenMetadataSnapshotCb,
                                                  this,
                                                  std::placeholders::_1,
                                                  std::placeholders::_2,
                                                  std::placeholders::_3,
                                                  std::placeholders::_4,
                                                  std::placeholders::_5);

    enableMigrationFeature = g_fdsprocess->get_fds_config()->get<bool>("fds.sm.migration.enable_feature");
}

MigrationMgr::~MigrationMgr() {
    mTimer.destroy();
}

/**
 * Handles start migration message from OM.
 * Creates MigrationExecutor object for each <SM token, source SM> pair
 * which initiate token migration
 */
Error
MigrationMgr::startMigration(fpi::CtrlNotifySMStartMigrationPtr& migrationMsg,
                             OmStartMigrationCbType cb,
                             const NodeUuid& mySvcUuid,
                             fds_uint32_t bitsPerDltToken,
                             bool forResync)
{
    Error err(ERR_OK);

    // Check if the migraion feature is enabled or disabled.
    if (false == enableMigrationFeature) {
        LOGCRITICAL << "Migration is disabled! ignoring start migration msg";
        if (cb) {
            cb(ERR_OK);
        }
        return err;
    }

    // it's strange to receive empty message from OM, but ok we just ignore that
    if (migrationMsg->migrations.size() == 0) {
        LOGWARN << "We received empty migrations message from OM, nothing to do";
        if (cb) {
            cb(ERR_OK);
        }
        return err;
    }

    FdsTimerTaskPtr retryTokenMigrationTask(
                        new FdsTimerFunctionTask(mTimer,
                                                 std::bind(
                                                    &MigrationMgr::retryTokenMigrForFailedDltTokens,
                                                    this)));
    int retryTimePeriod = 2;
    mTimer.scheduleRepeated(retryTokenMigrationTask, std::chrono::seconds(retryTimePeriod));

    // We need to do migration, switch to 'in progress' state
    MigrationState expectState = MIGR_IDLE;
    if (!std::atomic_compare_exchange_strong(&migrState, &expectState, MIGR_IN_PROGRESS)) {
        // already in "in progress" state, but ok if for the same target DLT (if this SM
        // got request to be a source of the migration, before it started processing
        // startMigration request
        fds_uint32_t migrDltVersion = migrationMsg->DLT_version;
        LOGMIGRATE << "startMigration called in non-idle state " << migrState
                   << " for DLT version " << migrDltVersion
                   << ", DLT version of on-going migration " << targetDltVersion;
        if (migrDltVersion != targetDltVersion) {
            LOGERROR << "startMigration called while migration for a different target DLT "
                     << targetDltVersion << " is still in progress!";
            if (cb) {
                cb(ERR_NOT_READY);
            }
            return ERR_NOT_READY;
        }
    }
    resyncOnRestart = forResync;
    targetDltVersion = migrationMsg->DLT_version;
    numBitsPerDltToken = bitsPerDltToken;
    omStartMigrCb = cb;  // we will have to send ack to OM when we get second delta set

    // reset DLT tokens state to "not ready" for all DLT tokens
    if (dltTokenStates.size() == 0) {
        // first time migration/resync started
        fds_uint32_t numTokens = pow(2, bitsPerDltToken);
        dltTokenStates.clear();
        // if migration due to adding this SM first time to the domain
        // we are setting tokens to 'ready' right away, since migration code is designed
        // to propagate DLT only after all tokens are synced and ready -- we know we will
        // not get IO for these tokens while first two rounds of migration finish.  So for now
        // to make code simple and don't handle cases that currently do not
        // happen we just making those tokens 'ready' in SM right away.
        dltTokenStates.assign(numTokens, !resyncOnRestart);
    } else {
        // resync on restart should happen only once during on SM run between restarts
        fds_verify(!resyncOnRestart);
    }

    // create migration executors for each <SM token, source SM> pair
    for (auto migrGroup : migrationMsg->migrations) {
        // migrGroup is <source SM, set of DLT tokens> pair
        NodeUuid srcSmUuid(migrGroup.source);
        LOGMIGRATE << "Will migrate tokens from source SM " << std::hex
                   << srcSmUuid.uuid_get_val() << std::dec;
        // iterate over DLT tokens for this source SM
        for (auto dltTok : migrGroup.tokens) {
            fds_token_id smTok = SmDiskMap::smTokenId(dltTok);
            LOGNOTIFY << "Source SM " << std::hex << srcSmUuid.uuid_get_val() << std::dec
                       << " DLT token " << dltTok << " SM token " << smTok;
            // if we don't know about this SM token and source SM, create migration executor
            if ((migrExecutors.count(smTok) == 0) ||
                (migrExecutors.count(smTok) > 0 && migrExecutors[smTok].count(srcSmUuid) == 0)) {
                LOGMIGRATE << "Will create migration executor class";
                fds_uint32_t localExecId = std::atomic_fetch_add(&nextLocalExecutorId, (fds_uint32_t)1);
                fds_uint64_t globalExecId = getExecutorId(localExecId, mySvcUuid);
                LOGMIGRATE << "Will create migration executor class with executor ID "
                           << std::hex << globalExecId << std::dec;
                migrExecutors[smTok][srcSmUuid] = MigrationExecutor::unique_ptr(
                    new MigrationExecutor(smReqHandler,
                                          bitsPerDltToken,
                                          srcSmUuid,
                                          smTok, globalExecId, targetDltVersion,
                                          forResync,
                                          std::bind(
                                              &MigrationMgr::dltTokenMigrationFailedCb,
                                              this,
                                              std::placeholders::_1),
                                          std::bind(
                                              &MigrationMgr::migrationExecutorDoneCb, this,
                                              std::placeholders::_1, std::placeholders::_2,
                                              std::placeholders::_3, std::placeholders::_4, std::placeholders::_5)));
            }
            // tell migration executor that it is responsible for this DLT token
            migrExecutors[smTok][srcSmUuid]->addDltToken(dltTok);
        }
    }

    // for now we will do one SM token at a time
    // take first SM token from the migrExecutors map and queue qos msg to create
    // a snapshot of SM token metadata
    MigrExecutorMap::const_iterator cit = migrExecutors.cbegin();
    fds_verify(cit != migrExecutors.cend());
    startSmTokenMigration(cit->first);
    return err;
}

void
MigrationMgr::retryTokenMigrForFailedDltTokens()
{

    fds_mutex::scoped_lock l(migrSmTokenLock);
    if (!retryMigrSmTokenSet.empty()) {
        retrySmTokenInProgress = *(retryMigrSmTokenSet.begin());
        LOGMIGRATE << "Starting migration retry for SM token " << retrySmTokenInProgress;

        // enqueue snapshot work
        snapshotRequest.token_id = retrySmTokenInProgress;
        snapshotRequest.retryReq = true;
        Error err = smReqHandler->enqueueMsg(FdsSysTaskQueueId, &snapshotRequest);
        if (!err.ok()) {
            LOGERROR << "Failed to enqueue index db snapshot message ;" << err;
            // for now, we are failing the whole migration on any error
            abortMigration(err);
        }
    }
}
/**
 * Starts token resync for all DLT tokens assigned to this SM.
 * It identifies the source SMs for assigned tokens from the
 * new dlt version passed to it and starts the migration process.
 */
Error
MigrationMgr::startResync(const fds::DLT *dlt,
                          const NodeUuid& mySvcUuid,
                          fds_uint32_t bitsPerDltToken)
{
    fpi::CtrlNotifySMStartMigrationPtr resyncMsg(
                       new fpi::CtrlNotifySMStartMigration());
    resyncMsg->DLT_version = dlt->getVersion();
    DLT::SourceNodeMap srcSmTokensMap;
    bool forResync = true;

    dlt->getSourceForAllNodeTokens(mySvcUuid, srcSmTokensMap);
    for (auto &ptr: srcSmTokensMap) {
        fpi::SMTokenMigrationGroup grp;
        grp.source = ptr.first.toSvcUuid();
        grp.tokens = ptr.second;
        resyncMsg->migrations.push_back(grp);
    }

    return startMigration(resyncMsg, NULL, mySvcUuid, bitsPerDltToken, forResync);
}

void
MigrationMgr::startSmTokenMigration(fds_token_id smToken) {
    smTokenInProgress = smToken;
    LOGMIGRATE << "Starting migration for SM token " << smToken;

    // enqueue snapshot work
    snapshotRequest.token_id = smTokenInProgress;
    snapshotRequest.retryReq = false;
    Error err = smReqHandler->enqueueMsg(FdsSysTaskQueueId, &snapshotRequest);
    if (!err.ok()) {
        LOGERROR << "Failed to enqueue index db snapshot message ;" << err;
        // for now, we are failing the whole migration on any error
        abortMigration(err);
    }
}

/**
 * Callback whith SM token snapshot
 */
void
MigrationMgr::smTokenMetadataSnapshotCb(const Error& error,
                                        SmIoSnapshotObjectDB* snapRequest,
                                        leveldb::ReadOptions& options,
                                        leveldb::DB *db,
                                        bool retryMigrFailedTokens)
{
    Error err(ERR_OK);
    fds_token_id curSmTokenInProgress;

    if (atomic_load(&migrState) == MIGR_ABORTED) {
        LOGMIGRATE << "Migration was aborted, ignoring migration task";
        return;
    }

    if (retryMigrFailedTokens) {
        curSmTokenInProgress = retrySmTokenInProgress;
    } else {
        curSmTokenInProgress = smTokenInProgress;
    }

    // on error, we just stop the whole migration process
    if (!error.ok()) {
        LOGERROR << "Failed to get index db snapshot for SM token " << curSmTokenInProgress;
        abortMigration(error);
        return;
    }

    // must match sm token id that is currently in progress
    fds_verify(snapRequest->token_id == curSmTokenInProgress);
    fds_verify(migrExecutors.count(curSmTokenInProgress) > 0);

    // pass this snapshot to all migration executors that are responsible for
    // migrating DLT tokens that belong to this SM token
    for (SrcSmExecutorMap::const_iterator cit = migrExecutors[curSmTokenInProgress].cbegin();
         cit != migrExecutors[curSmTokenInProgress].cend();
         ++cit) {
        if (retryMigrFailedTokens) {
            err = cit->second->startObjectRebalanceAgain(options, db);
        } else {
            err = cit->second->startObjectRebalance(options, db);
        }

        if (!err.ok()) {
            LOGERROR << "Failed to start object rebalance for SM token "
                     << curSmTokenInProgress << ", source SM " << std::hex
                     << (cit->first).uuid_get_val() << std::dec << " " << err;
            break;  // we are going to abort migration
        }
    }

    // done with the snapshot
    db->ReleaseSnapshot(options.snapshot);

    if (!err.ok()) {
        abortMigration(err);
    }

    // At this point this SM (destination) sent rebalance set msgs to source SMs
    // We are waiting for response(s) from source SMs to continue with migration
    // or we will get abort migration from OM if we don't get response from SM
    // because it is down (SL will timeout on these requests and send msg to OM)

    // But in the case of token migration retry for dlt tokens, issue the snapshot
    // request for the next smToken for whom we want to retry migration because
    // it failed the first time with source SM not ready as the reason.
    if (retryMigrFailedTokens) {
        migrSmTokenLock.lock();
        retryMigrSmTokenSet.erase(retryMigrSmTokenSet.begin());
        migrSmTokenLock.unlock();
        retryTokenMigrForFailedDltTokens();
    }
}

/**
 * Handle start object rebalance from destination SM
 */
Error
MigrationMgr::startObjectRebalance(fpi::CtrlObjectRebalanceFilterSetPtr& rebalSetMsg,
                                   const fpi::SvcUuid &executorSmUuid,
                                   const NodeUuid& mySvcUuid,
                                   fds_uint32_t bitsPerDltToken,
                                   const DLT* dlt)
{
    Error err(ERR_OK);
    fds_bool_t srcAccepted = false;
    LOGMIGRATE << "Object Rebalance Initial Set executor SM Id " << std::hex
               << executorSmUuid.svc_uuid << " executor ID " << rebalSetMsg->executorID
               << std::dec << " seqNum " << rebalSetMsg->seqNum
               << " last " << rebalSetMsg->lastFilterSet;

    // check if this SM accept this SM to be a source for given DLT token
    srcAccepted = acceptSourceResponsibility(rebalSetMsg->tokenId, rebalSetMsg->forResync,
                                             executorSmUuid, mySvcUuid, dlt);

    // If this SM is just a source and does not get Start Migration from OM
    // make sure that we set the migration state in progress
    MigrationState expectState = MIGR_IDLE;
    if (!std::atomic_compare_exchange_strong(&migrState, &expectState, MIGR_IN_PROGRESS)) {
        // check if migration was aborted
        if (atomic_load(&migrState) == MIGR_ABORTED) {
            // Something happened, for now stopping migration on any error
            LOGWARN << "Migration was already aborted, not going to handle object rebalance msg";
            return ERR_SM_TOK_MIGRATION_ABORTED;
        }
        // else was already in progress
    }
    numBitsPerDltToken = bitsPerDltToken;
    targetDltVersion = rebalSetMsg->targetDltVersion;
    resyncOnRestart = rebalSetMsg->forResync;

    MigrationClient::shared_ptr migrClient;
    int64_t executorId = rebalSetMsg->executorID;
    {
        SCOPEDWRITE(clientLock);
        if (migrClients.count(executorId) == 0) {
            // first time we see a message for this executor ID
            NodeUuid executorNodeUuid(executorSmUuid);
	    migrClients[executorId] = std::make_shared<MigrationClient>(smReqHandler,
                                                                        executorNodeUuid,
                                                                        targetDltVersion,
                                                                        bitsPerDltToken,
                                                                        rebalSetMsg->forResync);
        }
        migrClient = migrClients[executorId];
    }

    // message contains DLTToken + {<objects + refcnt>} + seqNum + lastSetFlag.
    err = migrClient->migClientStartRebalanceFirstPhase(rebalSetMsg, srcAccepted);
    if (err == ERR_SM_RESYNC_SOURCE_DECLINE) {
        fds_verify(!srcAccepted);
        // migrClient received all filter sets from given executor, and all dlt
        // tokens were declined, we are finished with this migration client
        LOGMIGRATE << "This SM declined all DLT tokens for executor " << std::hex
                   << executorId << std::dec;
        SCOPEDWRITE(clientLock);
        migrClients[executorId]->waitForIOReqsCompletion(executorId);
        migrClients.erase(executorId);
    }
    if (!srcAccepted) {
        return ERR_SM_RESYNC_SOURCE_DECLINE;
    }
    return err;
}

/**
 * Ack from source SM when it receives the whole initial set of
 * objects.
 */
Error
MigrationMgr::startObjectRebalanceResp()
{
    Error err(ERR_OK);
    LOGMIGRATE << "";
    return err;
}

fds_bool_t
MigrationMgr::acceptSourceResponsibility(fds_token_id dltToken,
                                         fds_bool_t resyncOnRestart,
                                         const fpi::SvcUuid &executorSmUuid,
                                         const NodeUuid& mySvcUuid,
                                         const DLT* dlt)
{
    // If this SM is already a destination for this DLT token and it has a lower
    // responsibility for this DLT token compared to the destination SM,
    // decline the request -- this is to prevent circular resync between two SMs
    // that are both source and destination for the same SM token
    fds_token_id smTok = SmDiskMap::smTokenId(dltToken);
    if (migrExecutors.count(smTok) > 0) {
        for (SrcSmExecutorMap::const_iterator cit = migrExecutors[smTok].cbegin();
             cit != migrExecutors[smTok].cend();
             ++cit) {
            if (cit->second->responsibleForDltToken(dltToken)) {
                // this SM is already a destination for this token ID
                // this must not happen if this is migration triggered by OM!
                if (!resyncOnRestart) {
                    // we are going to return an error that will trigger abort migration
                    LOGERROR << "Add/remove SM migration process: SM is already a destination "
                             << " for the DLT token " << dltToken << " returning error";
                    return false;
                }

                // see if it has higher responsibility for the DLT token
                DltTokenGroupPtr smGroup = dlt->getNodes(dltToken);
                NodeUuid executorNodeUuid(executorSmUuid);
                // see which SM we find first
                for (uint i = 0; i < smGroup->getLength(); ++i) {
                    if (smGroup->get(i) == executorNodeUuid) {
                        LOGMIGRATE << "Token resync on restart: declining to be a source for DLT token "
                                   << dltToken << " for which this SM is already a destination"
                                   << " and has a lower responsibility then SM that initiated this sync";
                        return false;
                    } else if (smGroup->get(i) == mySvcUuid) {
                        // this SM has higher responsibility for DLT token -- accept source SM responsibility
                        LOGMIGRATE << "Token resync on restart: this SM is already a destination for DLT token "
                                   << dltToken << " but this SM has a higher responsibility for the "
                                   << " DLT token -- accepting source SM responsibility for this DLT token";
                        break;
                    }
                }
            }
        }
    }

    return true;
}

/**
 * Handle msg from destination SM to send data/metadata changes since the first delta set
 */
Error
MigrationMgr::startSecondObjectRebalance(fpi::CtrlGetSecondRebalanceDeltaSetPtr& msg,
                                         const fpi::SvcUuid &executorSmUuid)
{
    Error err(ERR_OK);
    LOGMIGRATE << "Request to receive the rebalance diff since the first rebalance from "
               << std::hex << executorSmUuid.svc_uuid << " executor ID " << std::dec
               << msg->executorID;

    if (atomic_load(&migrState) == MIGR_ABORTED) {
        // Something happened, for now stopping migration on any error
        LOGWARN << "Migration was already aborted, not going to handle second object rebalance msg";
        return ERR_SM_TOK_MIGRATION_ABORTED;
    }
    fds_verify(atomic_load(&migrState) == MIGR_IN_PROGRESS);

    SCOPEDREAD(clientLock);
    // we must have migration client if we are in progress state
    fds_verify(migrClients.count(msg->executorID) != 0);
    // TODO(Sean):  Need to reset the double sequence for executor on the destion SM
    //              before starting the second phase.
    migrClients[msg->executorID]->migClientStartRebalanceSecondPhase(msg);

    return err;
}

Error
MigrationMgr::finishClientResync(fds_uint64_t executorId)
{
    Error err(ERR_OK);
    fds_bool_t doneWithClients = false;

    fiu_do_on("sm.exit.before.client.erase", exit(1));
    if (atomic_load(&migrState) == MIGR_ABORTED) {
        // Something happened, for now stopping migration on any error
        LOGWARN << "Migration was already aborted, not going to handle second object rebalance msg";
        return ERR_SM_TOK_MIGRATION_ABORTED;
    }
    fds_verify(atomic_load(&migrState) == MIGR_IN_PROGRESS);

    // we must only receive this message when are resyncing on restart
    fds_verify(resyncOnRestart);

    {  // scope for client lock
        SCOPEDREAD(clientLock);
        // ok if migration client does not exist
        if (migrClients.count(executorId) > 0) {
            LOGDEBUG << "Remove migration client for executor " << std::hex << executorId
                     << std::dec << " which means that forwarding from this client will stop too";
            // the destination SM told us it does not need this client anymore
            // just remove it, which will also stop forwarding IO from this client
            migrClients[executorId]->waitForIOReqsCompletion(executorId);
            migrClients.erase(executorId);
            doneWithClients = (migrClients.size() == 0);
        }
    }

    // check if the whole resync on restart is finished
    if (doneWithClients) {
        checkResyncDoneAndCleanup();
    }

    return err;
}


/**
 * Handle rebalance delta set at destination from the source
 */
Error
MigrationMgr::recvRebalanceDeltaSet(fpi::CtrlObjectRebalanceDeltaSetPtr& deltaSet)
{
    Error err(ERR_OK);
    fds_uint64_t executorId = deltaSet->executorID;

    // if we receive this in IDLE or ABORTED state, just ignore
    if (atomic_load(&migrState) != MIGR_IN_PROGRESS) {
        LOGWARN << "Migration NOT in progress anymore, assuming was aborted!";
        return ERR_SM_TOK_MIGRATION_ABORTED;
    }

    // since we are doing one SM token at a time, search for executor in deltaSet
    fds_bool_t found = false;
    for (SrcSmExecutorMap::const_iterator cit = migrExecutors[smTokenInProgress].cbegin();
         cit != migrExecutors[smTokenInProgress].cend();
         ++cit) {
        if (cit->second->getId() == executorId) {
            found = true;
            err = cit->second->applyRebalanceDeltaSet(deltaSet);
            // After this method, migrExecutors may be empty if this is resync
            // and we are finished with all executors
            break;
        }
    }
    fds_verify(found);   // did we start doing more than one SM token at a time?
    return err;
}

/**
 * Ack from destination for rebalance delta set message
 */
Error
MigrationMgr::rebalanceDeltaSetResp()
{
    Error err(ERR_OK);
    LOGMIGRATE << "";
    return err;
}

void
MigrationMgr::migrationExecutorDoneCb(fds_uint64_t executorId,
                                      fds_token_id smToken,
                                      const std::set<fds_token_id>& dltTokens,
                                      fds_uint32_t round,
                                      const Error& error)
{
    fds_bool_t isFirstRound = (round == 1);

    LOGMIGRATE << "Migration executor " << std::hex << executorId << std::dec
               << " finished migration round " << round << " done? "
               << (round == 2) << error;

    fds_verify(smToken == smTokenInProgress);

    MigrationState curState = atomic_load(&migrState);
    if (curState == MIGR_ABORTED) {
        // migration already stopped, don't do anything..
        return;
    }
    fds_verify(curState == MIGR_IN_PROGRESS);

    // Currently DTL tokens may become active in the following cases:
    // 1) If this is NOT resync on restart, DLT tokens are active right away
    // 2) If this is resync on restart, DLT token becomes available when source
    // SM declines to be a source (because this SM has higher responsibility for
    // this DLT token, so we declare the DLT token ready on this SM): this is the
    // case when we just started round 1 of token resync (passed as round == 0) and
    // source SM returned ERR_SM_RESYNC_SOURCE_DECLINE error.
    // 3) If this is resync on restart, and we finished second round of resync (during
    // resync, we go to the second round right away). This is the case when round == 2
    // and no error happended (if error case, we are aborting the resync at the moment)
    if (resyncOnRestart &&
        (((round == 0) && (error == ERR_SM_RESYNC_SOURCE_DECLINE)) ||
         ((round == 2) && (error.ok())))) {
        // ok if source declined, we declare this DLT token ready
        fds_verify(dltTokenStates.size() > 0);
        for (auto dltTok : dltTokens) {
            dltTokenStates[dltTok] = true;
            LOGDEBUG << "DLT token " << dltTok << " is now ACTIVE";
        }
        // if round 0, nothing else to do
        if (round == 0) {
            return;
        }
    }
    fds_verify(round > 0);

    // beta2: stop the whole migration process on any error
    if (!error.ok()) {
        abortMigration(error);
        return;
    }

    // check if there are other executors for the same SM Token that need to start migration
    MigrExecutorMap::const_iterator it = migrExecutors.find(smTokenInProgress);
    fds_verify(it != migrExecutors.end());
    // if we are done migration for all executors migrating current SM token,
    // start executors for the next SM token (if any)
    fds_bool_t finished = true;
    for (SrcSmExecutorMap::const_iterator cit = migrExecutors[smTokenInProgress].cbegin();
         cit != migrExecutors[smTokenInProgress].cend();
         ++cit) {
        if (!cit->second->isRoundDone(isFirstRound)) {
            finished = false;
            break;
        }
    }

    // migrate next SM token or we are done
    if (finished) {
        ++it;
        if (it != migrExecutors.end()) {
            // we have more SM tokens to migrate
            if (isFirstRound || resyncOnRestart) {
                startSmTokenMigration(it->first);
            } else {
                startSecondRebalanceRound(it->first);
            }
        } else {
            // we are done migrating, reply to start migration msg from OM
            if (isFirstRound && !resyncOnRestart) {
                // start with first executor to do the second round
                // --> start of second round
                // --> incrememnt counter / marker of second round
                PerfTracer::incr(PerfEventType::SM_MIGRATION_SECOND_PHASE, 0);
                startSecondRebalanceRound(migrExecutors.begin()->first);
            } else {
                // done with second round -- all done
                if (omStartMigrCb) {
                    omStartMigrCb(ERR_OK);
                    omStartMigrCb = NULL;  // we replied, so reset
                }
                if (resyncOnRestart) {
                    // done with executors.  First check if there is any pending migration
                    // requests before clearing executors.  At this point, there shouldn't
                    // be any.
                    coalesceExecutors();
                    migrExecutors.clear();
                    // after resync on restart, migrating on DLT change does not
                    // need token readiness at the moments; so setting all DLT tokens
                    // in the vector as available
                    fds_uint32_t numTokens = dltTokenStates.size();
                    if (numTokens) {
                        dltTokenStates.assign(numTokens, true);
                    }
                    // see if clients are also done so we can cleanup
                    checkResyncDoneAndCleanup();
                }
            }
        }
    }
}

void
MigrationMgr::startSecondRebalanceRound(fds_token_id smToken)
{
    Error err(ERR_OK);
    smTokenInProgress = smToken;
    LOGNORMAL << "Starting second round of migration for SM token " << smToken;

    // notify all migration executors responsible for this SM token to start
    // second round of migration
    for (SrcSmExecutorMap::const_iterator cit = migrExecutors[smTokenInProgress].cbegin();
         cit != migrExecutors[smTokenInProgress].cend();
         ++cit) {
        err = cit->second->startSecondObjectRebalanceRound();
        if (!err.ok()) {
            LOGERROR << "Failed to start second round of object rebalance for SM token "
                     << smTokenInProgress << ", source SM " << std::hex
                     << (cit->first).uuid_get_val() << std::dec << " " << err;
            break;  // we are going to abort migration
        }
    }

    if (!err.ok()) {
        abortMigration(err);
    }
}

fds_uint64_t
MigrationMgr::getTargetDltVersion() const
{
    // this will be invalid if migration not in progress
    return targetDltVersion;
}

void
MigrationMgr::startForwarding(fds_uint64_t executorId, fds_token_id smTok)
{
    // ignore invalid executor id
    if (executorId == SM_INVALID_EXECUTOR_ID) {
        LOGDEBUG << "Invalid executor ID, ok if called when there is no migration";
        return;
    }
    if (!isMigrationInProgress()) {
        return;
    }

    SCOPEDREAD(clientLock);
    // since executorID is valid, this request must have come from
    // migration client; so we must have it
    fds_verify(migrClients.count(executorId) > 0);
    // Tell migration client responsible for migrating SM token
    // to set forwarding flag, if this is second snapshot
    migrClients[executorId]->setForwardingFlagIfSecondPhase(smTok);
}


// This request has a set of objects that's not grouped by DLT tokens.  We have to group
// it based on the DLT token and forward it.
fds_bool_t
MigrationMgr::forwardAddObjRefIfNeeded(FDS_IOType* req)
{
    fds_bool_t forwarded = false;
    std::map<fds_token_id, fpi::AddObjectRefMsgPtr> addObjRefMap;
    SmIoAddObjRefReq* addObjRefReq = static_cast<SmIoAddObjRefReq *>(req);

    for (auto objId : addObjRefReq->objIds()) {
        ObjectID oid = ObjectID(objId.digest);

        fds_verify(numBitsPerDltToken != 0);
        fds_token_id dltTok = DLT::getToken(oid, numBitsPerDltToken);

        // Check if the mapping already exists.
        std::map<fds_token_id, fpi::AddObjectRefMsgPtr>::const_iterator it = addObjRefMap.find(dltTok);
        if (addObjRefMap.end() == it) {
            // Mapping doesn't exist.  Create a new mapping
            fpi::AddObjectRefMsgPtr addObjRefMsgPtr(new fpi::AddObjectRefMsg());

            addObjRefMsgPtr->srcVolId = addObjRefReq->getSrcVolId();
            addObjRefMsgPtr->destVolId = addObjRefReq->getDestVolId();
            fpi::FDS_ObjectIdType objectId;
            assign(objectId, oid);
            addObjRefMsgPtr->objIds.push_back(objectId);

            addObjRefMap.emplace(dltTok, addObjRefMsgPtr);
        } else {
            // Mapping already exists.  Add object id to the map.
            fpi::FDS_ObjectIdType objectId;
            assign(objectId, oid);
            fds_verify(it->first == dltTok);
            it->second->objIds.push_back(objectId);
        }
    }

    // TODO(Sean):
    // This is really not optimal code, but since this code paths is likely (99.99%) to be
    // re-written, not investing too much time/effort to optimize the loop.
    // For every DLT=>{ObjIds}, we have to check with all migration clients to see
    // if foward is needed.
    // To optimized, we really need auxillary data struct to map the DLT to migration client,
    // but since this will be revisited/re-written by the DM team, not investing too much time
    // at this point.
    for (std::map<fds_token_id, fpi::AddObjectRefMsgPtr>::iterator mapIter = addObjRefMap.begin();
         mapIter != addObjRefMap.end();
         ++mapIter)
    {
         SCOPEDREAD(clientLock);
         for (MigrClientMap::iterator clientIter = migrClients.begin();
              clientIter != migrClients.end();
              ++clientIter) {
            forwarded |= clientIter->second->forwardAddObjRefIfNeeded(mapIter->first,
                                                                      mapIter->second);
        }
    }

    return forwarded;
}

fds_bool_t
MigrationMgr::forwardReqIfNeeded(const ObjectID& objId,
                                 fds_uint64_t reqDltVersion,
                                 FDS_IOType* req)
{
    fds_bool_t forwarded = false;

    // we only do forwarding if migration is in progress
    if (isMigrationInProgress()) {
        if (!resyncOnRestart && (reqDltVersion == targetDltVersion)) {
            // this request was also sent to the destination SM
            // since AM sending the request is already on new DLT version
            return forwarded;
        }
        // in resync on restart case we always forward if forward flag is set

        // This request has a set of object IDs that may belong to different
        // DLT tokens.  We need to forward a set of object IDs to correct
        // client by partitioning and grouping them before forwarding.
        if (FDS_SM_ADD_OBJECT_REF == req->io_type) {
            fds_verify(NullObjectID == objId);
            forwarded = forwardAddObjRefIfNeeded(req);
        } else {
            // in this state, we must know about bits per tok
            fds_verify(numBitsPerDltToken != 0);
            fds_token_id dltTok = DLT::getToken(objId, numBitsPerDltToken);
            // tell each migration client reponsible for migrating this DLT
            // token to forward the request to the destination
            SCOPEDREAD(clientLock);
            for (MigrClientMap::iterator it = migrClients.begin();
                 it != migrClients.end();
                 ++it) {
                forwarded |= it->second->forwardIfNeeded(dltTok, req);
            }
        }
    }
    return forwarded;
}

/**
 * Handles DLT close event. At this point IO should not arrive
 * with old DLT. Once we reply, we are done with token migration.
 * Assumes we are receiving DLT close event for the correct version,
 * caller should check this
 */
Error
MigrationMgr::handleDltClose()
{
    Error err(ERR_OK);

    // TODO(Anna) FS-1760 OM should not send DLT close to SM on restart
    // but it currently does; SM should just ignore it.. but better to fix
    // OM not to do this, in case we miss some other case like migration on
    // DLT change while we are still doing resync
    if (resyncOnRestart) {
        LOGWARN << "SM received DLT close while it is doing resync on restart."
                << " OM should not send DLT close on restart, so ignoring DLT "
                << "close msg from OM.";
        return err;
    }

    // for now, to make sure we can handle another migration...
    MigrationState expectState = MIGR_IN_PROGRESS;
    if (!std::atomic_compare_exchange_strong(&migrState, &expectState, MIGR_IDLE)) {
        LOGNOTIFY << "DLT Close called in non- in progress state " << migrState;
        return ERR_OK;  // this is ok
    }
    LOGMIGRATE << "Will cleanup executors and migr clients";
    // Wait for all pending IOs to complete on Executors.
    coalesceExecutors();
    migrExecutors.clear();

    {
        SCOPEDWRITE(clientLock);
        // Wait for all pending IOs to complete on Clieng.
        coalesceClients();
        migrClients.clear();
    }
    targetDltVersion = DLT_VER_INVALID;
    resyncOnRestart = false;
    return err;
}

void
MigrationMgr::notifyDltUpdate(fds_uint32_t bitsPerDltToken)
{
    if (!isMigrationInProgress()) {
        fds_verify(bitsPerDltToken > 0);
        numBitsPerDltToken = bitsPerDltToken;
        if (dltTokenStates.size() == 0) {
            fds_uint32_t numTokens = pow(2, bitsPerDltToken);
            dltTokenStates.assign(numTokens, true);
        }
    }
}

void
MigrationMgr::checkResyncDoneAndCleanup()
{
    if (!resyncOnRestart) {
        // not resync case
        return;
    }
    if (migrExecutors.size() == 0) {
        // we are done with executors
        SCOPEDWRITE(clientLock);
        if (migrClients.size() == 0) {
            // we are also done with clients = done with resync
            MigrationState expectState = MIGR_IN_PROGRESS;
            if (!std::atomic_compare_exchange_strong(&migrState, &expectState, MIGR_IDLE)) {
                LOGMIGRATE << "Already done or aborted, nothing to do, current state " << migrState;
                return;  // this is ok
            }
            LOGNOTIFY << "Token resync on restart / or being resync client completed for DLT version "
                      << targetDltVersion;
        }
    }
}

/**
 * Handles message from OM to abort migration
 */
Error
MigrationMgr::abortMigration()
{
    Error err(ERR_OK);
    LOGNOTIFY << "Will abort token migration per OM request";
    abortMigration(ERR_SM_TOK_MIGRATION_ABORTED);
    return err;
}

/// local method that actually aborts migration
void
MigrationMgr::abortMigration(const Error& error)
{
    LOGNOTIFY << "Aborting token migration " << error;

    // set migration state to aborted
    MigrationState expectState = MIGR_IN_PROGRESS;
    if (!std::atomic_compare_exchange_strong(&migrState, &expectState, MIGR_ABORTED)) {
        MigrationState curState = atomic_load(&migrState);
        if (curState == MIGR_IDLE) {
            // nothing to do, migration was not in progress
            LOGNOTIFY << "Migration was idle, nothing to abort";
        } else if (curState == MIGR_ABORTED) {
            LOGNOTIFY << "Migration already aborted, nothing else to do";
        }
        return;  // this is ok
    }

    // if we need to ack Start Migration from OM, we will have a cb to reply to
    if (omStartMigrCb) {
        omStartMigrCb(error);
        omStartMigrCb = NULL;
    }

    // There could be some pending IOs in flight.  We can't blindly call to clear
    // all executors.  Call to coalesce executors before blowing them away.
    coalesceExecutors();

    // Clear all migrationExecutors.
    migrExecutors.clear();

    // Clear all retry SM token set.
    retryMigrSmTokenSet.clear();
    targetDltVersion = DLT_VER_INVALID;

    resyncOnRestart = false;
}

void
MigrationMgr::dltTokenMigrationFailedCb(fds_token_id &smToken)
{
    fds_mutex::scoped_lock l(migrSmTokenLock);
    retryMigrSmTokenSet.insert(smToken);
}

fds_uint64_t
MigrationMgr::getExecutorId(fds_uint32_t localId,
                            const NodeUuid& smSvcUuid) const
{
    fds_uint64_t execId = smSvcUuid.uuid_get_val();
    // Keep most significant bits to read the uuid easier.
    return ((execId & (~0UL << 32)) | localId);
}

void
MigrationMgr::coalesceExecutors()
{
    for (auto citExec = migrExecutors.cbegin(); citExec != migrExecutors.cend(); ++citExec) {
        fds_token_id tok = citExec->first;
        for (auto citSrcExec = migrExecutors[tok].cbegin(); citSrcExec != migrExecutors[tok].cend(); ++citSrcExec) {
            citSrcExec->second->waitForIOReqsCompletion(tok, citSrcExec->first);
        }
    }
}

void
MigrationMgr::coalesceClients()
{
    for (auto citClient = migrClients.cbegin(); citClient != migrClients.cend(); ++citClient) {
        citClient->second->waitForIOReqsCompletion(citClient->first);
    }

}

}  // namespace fds

