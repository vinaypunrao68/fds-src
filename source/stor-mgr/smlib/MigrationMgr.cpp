/*
 * Copyright 2013-2014 Formation Data Systems, Inc.
 */

#include <net/SvcMgr.h>
#include <vector>
#include <fiu-control.h>
#include <fiu-local.h>
#include <object-store/SmDiskMap.h>
#include <StorMgr.h>
#include <MigrationMgr.h>
#include <fds_process.h>
#include <fdsp_utils.h>
#include "PerfTrace.h"

namespace fds {
class ObjectStorMgr;

MigrationMgr::MigrationMgr(SmIoReqHandler *dataStore)
        : smReqHandler(dataStore),
          omStartMigrCb(NULL),
          targetDltVersion(DLT_VER_INVALID),
          numBitsPerDltToken(0),
          maxRetriesWithDifferentSources(3),
          abortError(ERR_OK),
          nextExecutor(migrExecutors),
          migrationTimeoutTimer(new FdsTimer())
{
    migrState = ATOMIC_VAR_INIT(MIGR_IDLE);
    nextLocalExecutorId = ATOMIC_VAR_INIT(1);
    uniqRestartId = ATOMIC_VAR_INIT(1);
    objStoreMgrUuid = (dynamic_cast<ObjectStorMgr *>(dataStore))->getUuid();
    LOGMIGRATE << "Object store manager uuid " << objStoreMgrUuid;

    snapshotRequests.reserve(SMTOKEN_COUNT);
    for (uint32_t i = 0; i < SMTOKEN_COUNT; ++i) {
        snapshotRequests.emplace_back(new SmIoSnapshotObjectDB());
        snapshotRequests.back()->io_type = FDS_SM_SNAPSHOT_TOKEN;
        snapshotRequests.back()->retryReq = false;
        snapshotRequests.back()->smio_snap_resp_cb = std::bind(&MigrationMgr::smTokenMetadataSnapshotCb,
                                                           this,
                                                           std::placeholders::_1,
                                                           std::placeholders::_2,
                                                           std::placeholders::_3,
                                                           std::placeholders::_4,
                                                           std::placeholders::_5,
                                                           std::placeholders::_6);
    }

    parallelMigration = g_fdsprocess->get_fds_config()->get<uint32_t>("fds.sm.migration.parallel_migration", 16);
    LOGMIGRATE << "Parallel migration - " << parallelMigration << " threads";
    enableMigrationFeature = g_fdsprocess->get_fds_config()->get<bool>("fds.sm.migration.enable_feature");

    // get migration timeout duration from the platform.conf file.
    migrationTimeoutSec =
            g_fdsprocess->get_fds_config()->get<uint32_t>("fds.sm.migration.migration_timeout", 300);
}

MigrationMgr::~MigrationMgr() {
    mTimer.destroy();
    migrationTimeoutTimer->destroy();
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
                             MigrationType migrationType,
                             bool onePhaseMigration)
{
    Error err(ERR_OK);
    LOGMIGRATE << "Going to start SM token migration for DLT version "
               << migrationMsg->DLT_version << "."
               << " It is a resync? " << (migrationType == SMMigrType::MIGR_SM_RESYNC);

    fiu_do_on("abort.sm.migration",\
              LOGNOTIFY << "abort.sm.migration fault point enabled";\
              sleep(1); if (cb) { cb(ERR_NOT_READY); } return ERR_NOT_READY;);

    // Check if the migraion feature is enabled or disabled.
    if (false == enableMigrationFeature) {
        LOGCRITICAL << "Migration is disabled! ignoring start migration msg";
        if (cb) {
            cb(ERR_OK);
        }
        return err;
    }

    if (migrationMsg->migrations.size() == 0) {
        LOGWARN << "We received empty migrations message, nothing to do";
        if (cb) {
            cb(ERR_OK);
        }
        return err;
    }

    retryTokenMigrationTask.reset(new FdsTimerFunctionTask(mTimer,
                                                           std::bind(
                                                             &MigrationMgr::retryTokenMigrForFailedDltTokens,
                                                             this)));
    int retryTimePeriod = 2;
    mTimer.scheduleRepeated(retryTokenMigrationTask, std::chrono::seconds(retryTimePeriod));

    // We need to do migration, switch to 'in progress' state
    MigrationState expectState = MIGR_IDLE;
    if (!std::atomic_compare_exchange_strong(&migrState, &expectState, MIGR_IN_PROGRESS)) {
        // if in "in progress" state, ok if for the same target DLT (if this SM
        // got request to be a source of the migration, before it started processing
        // startMigration request
        // if in 'aborted' state, return error, since migration manager is in unknown
        // state trying to abort the previous migration
        fds_uint32_t migrDltVersion = migrationMsg->DLT_version;
        Error retError(ERR_OK);
        LOGMIGRATE << "startMigration called in non-idle state " << migrState
                   << " for DLT version " << migrDltVersion
                   << ", DLT version of on-going migration " << targetDltVersion;
        if (migrState == MIGR_ABORTED) {
            LOGWARN << "startMigration called while previous migration aborting";
            retError = ERR_SM_TOK_MIGRATION_INPROGRESS;
        } else if (migrDltVersion != targetDltVersion) {
            LOGERROR << "startMigration called while migration for a different target DLT "
                     << targetDltVersion << " is still in progress!";
            retError = ERR_SM_TOK_MIGRATION_INPROGRESS;
        }
        if (!retError.ok()) {
            if (cb) {
                cb(retError);
            }
            return retError;
        }
    }
    resyncOnRestart = onePhaseMigration;
    targetDltVersion = migrationMsg->DLT_version;
    numBitsPerDltToken = bitsPerDltToken;
    omStartMigrCb = cb;  // we will have to send ack to OM when we get second delta set

    // reset DLT tokens state to "not ready" for all DLT tokens, if this is the first
    // migration or resync
    resetDltTokensStates(bitsPerDltToken);
 
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
            SCOPEDWRITE(migrExecutorLock);
            if ((migrExecutors.count(smTok) == 0) ||
                (migrExecutors.count(smTok) > 0 && migrExecutors[smTok].count(srcSmUuid) == 0)) {
                migrExecutors[smTok][srcSmUuid] = createMigrationExecutor(srcSmUuid,
                                                                          mySvcUuid,
                                                                          bitsPerDltToken,
                                                                          smTok,
                                                                          targetDltVersion,
                                                                          migrationType,
                                                                          onePhaseMigration);
            }
            // tell migration executor that it is responsible for this DLT token
            migrExecutors[smTok][srcSmUuid]->addDltToken(dltTok);
        }
    }

    // TODO: limit this and make it configurable
    LOGNORMAL << "Number of executors: " << migrExecutors.size();

    fds_verify(smTokenInProgress.size() == 0);
    // (Matteo) migrExecutorLock and smTokenInProgressMutex
    // are nested here. Please, mantain the order
    SCOPEDREAD(migrExecutorLock);
    nextExecutor.set(migrExecutors.begin());
    fds_verify(parallelMigration > 0);
    /**
     * TODO:(Gurpreet) Currently for parallel migrations we
     * are assuming a single executor-per smToken because
     * # of SM tokens = # of DLT tokens. Parallel migration
     * logic should be revisited when the above assumption
     * changes.
     */
    for (uint32_t issued = 0; issued < parallelMigration; issued++) {

        MigrExecutorMap::const_iterator next = 
            nextExecutor.fetch_and_increment_saturating(); 
        if (next == migrExecutors.cend())
            break;

        smTokenInProgressMutex.lock();
        auto cachedNextSMToken = next->first;
        smTokenInProgress.insert(next->first);
        smTokenInProgressMutex.unlock();        

        startSmTokenMigration(cachedNextSMToken);
    }
    return err;
}

MigrationExecutor::unique_ptr
MigrationMgr::createMigrationExecutor(NodeUuid& srcSmUuid,
                                      const NodeUuid& mySvcUuid,
                                      fds_uint32_t bitsPerDltToken,
                                      fds_token_id& smTok,
                                      fds_uint64_t& targetDltVersion,
                                      MigrationType& migrationType,
                                      bool onePhaseMigration,
                                      fds_uint32_t uniqueId,
                                      fds_uint16_t instanceNum) {
    fds_uint32_t localExecId = std::atomic_fetch_add(&nextLocalExecutorId,
                                                     (fds_uint32_t)instanceNum);
    fds_uint64_t globalExecId = getExecutorId(localExecId, mySvcUuid);
    LOGMIGRATE << "Will create migration executor class with executor ID "
               << std::hex << globalExecId << std::dec;
    return MigrationExecutor::unique_ptr(
            new MigrationExecutor(smReqHandler,
                                  bitsPerDltToken,
                                  srcSmUuid,
                                  smTok, globalExecId, targetDltVersion,
                                  migrationType, onePhaseMigration,
                                  std::bind(&MigrationMgr::dltTokenMigrationFailedCb,
                                            this,
                                            std::placeholders::_1),
                                  std::bind(&MigrationMgr::migrationExecutorDoneCb,
                                            this,
                                            std::placeholders::_1,
                                            std::placeholders::_2,
                                            std::placeholders::_3,
                                            std::placeholders::_4,
                                            std::placeholders::_5),
                                  migrationTimeoutTimer,
                                  migrationTimeoutSec,
                                  std::bind(&MigrationMgr::timeoutAbortMigration,
                                            this),
                                  std::bind(&MigrationMgr::abortMigrationCb,
                                            this,
                                            std::placeholders::_1,
                                            std::placeholders::_2),
                                  uniqueId, instanceNum));
}

void
MigrationMgr::retryTokenMigrForFailedDltTokens()
{
    fds_mutex::scoped_lock l(migrSmTokenLock);
    if (!retryMigrSmTokenSet.empty()) {
        retrySmTokenInProgress = *(retryMigrSmTokenSet.begin());
        LOGNORMAL << "Starting migration retry for SM token " << retrySmTokenInProgress;

        // enqueue snapshot work
        snapshotRequests[retrySmTokenInProgress]->token_id = retrySmTokenInProgress;
        snapshotRequests[retrySmTokenInProgress]->retryReq = true;
        Error err = smReqHandler->enqueueMsg(FdsSysTaskQueueId, snapshotRequests[retrySmTokenInProgress].get());
        if (!err.ok()) {
            LOGERROR << "Failed to enqueue index db snapshot message ;" << err;
            // for now, we are failing the whole migration on any error
            abortMigrationForSMToken(retrySmTokenInProgress, err);
        }
    }
}

/**
 * Remove the sm tokens from the retry migration set. Retry here was for case
 * where source SM was not ready.
 */
void MigrationMgr::removeTokensFromRetrySet(std::vector<fds_token_id>& tokens)
{
    fds_mutex::scoped_lock l(migrSmTokenLock);
    for (auto token: tokens) {
        retryMigrSmTokenSet.erase(token);
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
                          fds_uint32_t bitsPerDltToken,
                          PendingResyncCb pendingResyncFn)
{
    LOGNORMAL << "Starting resync for dlt version " << dlt->getVersion();
    if (pendingResyncFn) {
        cachedPendingResyncCb = pendingResyncFn;
    }

    if (!isMigrationIdle()) {
        isResyncPending = true;
        LOGMIGRATE << "A migration still active. Making this migration request as pending.";
        return ERR_OK;
    }

    fpi::CtrlNotifySMStartMigrationPtr resyncMsg(
                       new fpi::CtrlNotifySMStartMigration());
    resyncMsg->DLT_version = dlt->getVersion();
    DLT::SourceNodeMap srcSmTokensMap;
    bool onePhaseMigration = true;
    numBitsPerDltToken = bitsPerDltToken;
    fds_verify(dlt->getDepth() != 0);   // otherwise this SM will not be in DLT
    maxRetriesWithDifferentSources = dlt->getDepth() - 1;
    resetDltTokensStates(bitsPerDltToken);

    dlt->getSourceForAllNodeTokens(mySvcUuid, srcSmTokensMap);
    for (auto &ptr: srcSmTokensMap) {
        fpi::SMTokenMigrationGroup grp;
        grp.source = ptr.first.toSvcUuid();
        grp.tokens = ptr.second;

        /**
         * Could not find any other SM from the dlt table to act as source
         * for migration of these dlt tokens. So mark them as available.
         */
        if (INVALID_RESOURCE_UUID == ptr.first.toSvcUuid()) {
            const TokenList tokens(ptr.second.begin(), ptr.second.end());
            changeDltTokensAvailability(tokens, true);
        } else {
            resyncMsg->migrations.push_back(grp);
        }
    }

    // set migration type to resync.
    SMMigrType migrType = MIGR_SM_RESYNC;

    return startMigration(resyncMsg, NULL, mySvcUuid, bitsPerDltToken,
                          migrType, onePhaseMigration);
}

void
MigrationMgr::startSmTokenMigration(fds_token_id smToken,
                                    fds_uint32_t uid) {
    Error err(ERR_OK);
    LOGMIGRATE << "Starting migration for SM token " << smToken;

    // enqueue snapshot work
    snapshotRequests[smToken]->token_id = smToken;
    snapshotRequests[smToken]->unique_id = uid;
    snapshotRequests[smToken]->retryReq = false;
    fiu_do_on("abort.sm.migration.at.start",
        LOGDEBUG << "fault abort.sm.migration.at.start enabled for SM token " << smToken; \
        if (smToken % 20 == 0) err = ERR_SM_TOK_MIGRATION_ABORTED;);
    if (err.ok()) {
        err = smReqHandler->enqueueMsg(FdsSysTaskQueueId, snapshotRequests[smToken].get());
    }
    if (!err.ok()) {
        LOGERROR << "Failed to enqueue index db snapshot message ;" << err;
        abortMigrationForSMToken(smToken, err);
    }
}

/**
 * Callback with SM token snapshot
 */
void
MigrationMgr::smTokenMetadataSnapshotCb(const Error& error,
                                        SmIoSnapshotObjectDB* snapRequest,
                                        leveldb::ReadOptions& options,
                                        std::shared_ptr<leveldb::DB> db,
                                        bool retryMigrFailedTokens,
                                        fds_uint32_t uniqueId)
{
    Error err(ERR_OK);
    fds_token_id curSmTokenInProgress;

    MigrationState curState = atomic_load(&migrState);
    if (curState == MIGR_ABORTED) {
        LOGMIGRATE << "Migration was aborted, ignoring migration task";
        err = ERR_SM_TOK_MIGRATION_ABORTED;
    } else if (curState == MIGR_IDLE) {
        LOGNOTIFY << "Migration is in idle state, probably was aborted, ignoring";
        err = ERR_SM_TOK_MIGRATION_ABORTED;
    }
    if (!err.ok()) {
        // we are done, if snapshot was taken successfully, release it
        if (error.ok()) {
            db->ReleaseSnapshot(options.snapshot);
        }
        return;
    }

    if (retryMigrFailedTokens) {
        curSmTokenInProgress = retrySmTokenInProgress;
    } else {
        curSmTokenInProgress = snapRequest->token_id;
    }
    LOGMIGRATE << "smTokenMetadataSnapshotCb - current token: " << curSmTokenInProgress;

    fiu_do_on("abort.sm.migration.at.snap.cb",
        LOGDEBUG << "fault abort.sm.migration.at.snap.cb enabled"; \
        if (curSmTokenInProgress % 20 == 0) err = ERR_SM_TOK_MIGRATION_ABORTED;);
    // on error, we just stop the whole migration process
    if (!error.ok() || !err.ok()) {
        LOGERROR << "Failed to get index db snapshot for SM token: " << curSmTokenInProgress
                 << "primary error: " << err << " secondary error: " << error;
        abortMigrationForSMToken(curSmTokenInProgress, error);
        return;
    }

    {
        SCOPEDREAD(migrExecutorLock);
        // must be currently in progress
        fds_verify(snapRequest->token_id == curSmTokenInProgress);
        fds_verify(migrExecutors.count(curSmTokenInProgress) > 0);

        // pass this snapshot to all migration executors that are responsible for
        // migrating DLT tokens that belong to this SM token
        for (SrcSmExecutorMap::const_iterator cit = migrExecutors[curSmTokenInProgress].cbegin();
             cit != migrExecutors[curSmTokenInProgress].cend();
             ++cit) {
            if (retryMigrFailedTokens) {
                err = cit->second->startObjectRebalanceAgain(options, db);
            } else if (uniqueId == cit->second->getUniqueId()) {
                err = cit->second->startObjectRebalance(options, db); 
            }

            if (!err.ok()) {
                LOGERROR << "Failed to start object rebalance for SM token "
                         << curSmTokenInProgress << ", source SM " << std::hex
                         << (cit->first).uuid_get_val() << std::dec << " " << err;
                break;  // we are going to abort migration
            }
        }
    }

    // done with the snapshot
    db->ReleaseSnapshot(options.snapshot);

    if (!err.ok()) {
        abortMigrationForSMToken(curSmTokenInProgress, err);
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
               << " last " << rebalSetMsg->lastFilterSet
               << " onePhaseMigration ? " << rebalSetMsg->onePhaseMigration
	       << " Target DLT version " << rebalSetMsg->targetDltVersion
	       << " tokenId " << rebalSetMsg->tokenId;

    // if migration already in progress, make sure that rebalance message is for
    // the same DLT version
    if (atomic_load(&migrState) == MIGR_IN_PROGRESS) {
        if ((fds_uint64_t)rebalSetMsg->targetDltVersion != targetDltVersion) {
            LOGWARN << "Migration IN PROGRESS for DLT version " << targetDltVersion
                    << " but received migration request from destination for DLT version "
                    << rebalSetMsg->targetDltVersion << " -- not ready";
            return ERR_SM_TOK_MIGRATION_INPROGRESS;
        }
    }

    // check if this SM accept this SM to be a source for given DLT token
    srcAccepted = acceptSourceResponsibility(rebalSetMsg->tokenId, rebalSetMsg->onePhaseMigration,
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
    resyncOnRestart = rebalSetMsg->onePhaseMigration;

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
                                                                        rebalSetMsg->onePhaseMigration);
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
    SCOPEDREAD(migrExecutorLock);
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

    fiu_do_on("sm.exit.before.client.erase", LOGNOTIFY << "sm.exit.before.client.erase fault point enabled"; \
              exit(1));
    if (atomic_load(&migrState) == MIGR_ABORTED) {
        // Something happened, for now stopping migration on any error
        LOGWARN << "Migration was already aborted, not going to handle second object rebalance msg";
        return ERR_SM_TOK_MIGRATION_ABORTED;
    } else if (atomic_load(&migrState) == MIGR_IDLE) {
        // possible to receive stray finish resync msg, if SM e.g. failed/restarted
        // and the destination still thinks previous SM is up
        LOGWARN << "Received finishClientResync in IDLE state for executor "
                << std::hex << executorId << std::dec;
        return ERR_NOT_FOUND;
    }

    // we must only receive this message when are resyncing on restart
    fds_verify(resyncOnRestart);

    {  // scope for client lock
        SCOPEDWRITE(clientLock);
        // ok if migration client does not exist
        if (migrClients.count(executorId) > 0) {
            LOGDEBUG << "Remove migration client for executor " << std::hex << executorId
                     << std::dec << " which means that forwarding from this client will stop too"
		     << ". Migration clients " << migrClients.size();
            // the destination SM told us it does not need this client anymore
            // just remove it, which will also stop forwarding IO from this client
            migrClients[executorId]->waitForIOReqsCompletion(executorId);
            migrClients.erase(executorId);
            doneWithClients = (migrClients.size() == 0);
	    LOGMIGRATE << "Removed one client, clients left so far " << migrClients.size();
        }
    }

    // check if the whole resync on restart is finished
    if (doneWithClients) {
        checkResyncDoneAndCleanup(false);
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

    // called for second round as well?
    LOGMIGRATE << "recvRebalanceDeltaSet: " << smTokenInProgress.size();
    // TODO(matteo): investigate more this function. smTokenInProgress is
    // being snapshotted and then iterated over here. It is unclear whether
    // this might lead to a race. Possibly investigate alternative solutions 
    // here
    std::unordered_set<fds_token_id> curSmTokenInProgress;
    {
        FDSGUARD(smTokenInProgressMutex);
        curSmTokenInProgress = smTokenInProgress;
    }

    for (auto token : curSmTokenInProgress) {
        LOGMIGRATE << "token: " << token;
        MigrationExecutor::shared_ptr executor(nullptr);
        {
            SCOPEDREAD(migrExecutorLock);
            for (SrcSmExecutorMap::const_iterator cit = migrExecutors[token].cbegin();
                 cit != migrExecutors[token].cend();
                 ++cit) {
                if (cit->second->getId() == executorId) {
                    executor = cit->second;
                    // After this method, migrExecutors may be empty if this is resync
                    // and we are finished with all executors
                    break;
                }
            }
        }
        if (executor) {
            err = executor->applyRebalanceDeltaSet(deltaSet);
        }
    }
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
               << " smToken=" << smToken
               << " finished migration round " << round << " done? "
               << (round == 2) << error;

    MigrationState curState = atomic_load(&migrState);
    if (curState == MIGR_ABORTED) {
        // migration already stopped, don't do anything..
        return;
    }
    fds_verify(curState == MIGR_IN_PROGRESS);

    // Currently DLT tokens may become active in the following cases:
    // 1) DLT token becomes available when source
    // SM declines to be a source (because this SM has higher responsibility for
    // this DLT token, so we declare the DLT token ready on this SM): this is the
    // case when we just started round 1 of token resync (passed as round == 0) and
    // source SM returned ERR_SM_RESYNC_SOURCE_DECLINE error.
    // 2) We finished second round of resync (during resync, we go to the second round right away).
    // This is the case when round == 2 and no error happended
    if ((((round == 0) && (error == ERR_SM_RESYNC_SOURCE_DECLINE)) ||
         ((round == 2) && (error.ok())))) {
        // ok if source declined, we declare this DLT token ready
        changeDltTokensState(dltTokens, true);

        // if round 0, nothing else to do
        if (round == 0) {
            return;
        }
    }

    fds_verify(round > 0);

    // beta2: stop the whole migration process on any error
    if (!error.ok()) {
        if (resyncOnRestart) {
            retryWithNewSMs(executorId, smToken, dltTokens, round, error);
        } else {
            // If we are changing DLT, just abort migration, OM will retry later
            abortMigration(error);
            // if we already synced some DLT tokens, they will be set to unavail
            // when DLT commit comes for the previousle committed DLT (to which OM
            // will revert to).
        }
    }

    fds_bool_t finished = true;
    {
        SCOPEDREAD(migrExecutorLock);
        // check if there are other executors for the same SM Token that need to start migration
        MigrExecutorMap::const_iterator it = migrExecutors.find(smToken);
        fds_verify(it != migrExecutors.end());
        /**
         * If we are done migration for all executors migrating current SM token,
         * start executors for the next SM token (if any)
         */
        for (SrcSmExecutorMap::const_iterator cit = migrExecutors[smToken].cbegin();
             cit != migrExecutors[smToken].cend();
             ++cit) {
            if (!cit->second->isRoundDone(isFirstRound)) {
                finished = false;
                break;
            }
        }
    }

    // migrate next SM token or we are done
    if (finished) {
        startNextSMTokenMigration(smToken, isFirstRound);
    }
}

void
MigrationMgr::startNextSMTokenMigration(fds_token_id &smToken,
                                        bool isFirstRound) {
    /**
     * Matteo: migrExecutorLock is nested with smTokenInProgressMutex, 
     * please be consistent with the ordering
     * fetch_and_increment_saturating uses a reference to migrExecutors, 
     * so it needs to be protected
     * migrExecutorLock is taken as upgradable lock, then upgraded when needed
     */
    migrExecutorLock.cond_write_lock();
    smTokenInProgressMutex.lock();

    LOGMIGRATE << "Erase " << smToken
               << " and fetch next executor for first round? " << isFirstRound;

    smTokenInProgress.erase(smToken);
    auto next = nextExecutor.fetch_and_increment_saturating();

    if (next != migrExecutors.end()) {
        // we have more SM tokens to migrate
        if (isFirstRound || resyncOnRestart) {
            smTokenInProgress.insert(next->first);
            LOGMIGRATE << "call startSmTokenMigration for " << next->first;
            auto cachedNextSMToken = next->first;

            smTokenInProgressMutex.unlock();
            migrExecutorLock.cond_write_unlock();

            startSmTokenMigration(cachedNextSMToken);
        } else {
            // coming in here during second phase when calling the ExecutorDone callback
            LOGMIGRATE << "starting second round for " << (next->first);

            // For a given SM token, if all executors are in error state.
            // No point calling second phase of migration.
            if (allExecutorsInErrorState(next->first)) {
                smTokenInProgressMutex.unlock();
                migrExecutorLock.cond_write_unlock();
            } else {
                smTokenInProgress.insert(next->first);
                auto cachedNextSMToken = next->first;

                smTokenInProgressMutex.unlock();
                migrExecutorLock.cond_write_unlock();

                startSecondRebalanceRound(cachedNextSMToken);
            }
        }
    } else {
        if (smTokenInProgress.size() > 0) {
            LOGMIGRATE << "Executor(s) still active from current phase."
                       << "Don't start next phase."
                       << "IsFirstPhase = " << isFirstRound;
            for (auto tok: smTokenInProgress) {
                LOGMIGRATE << "SM token in progress: " << tok;
            }

            smTokenInProgressMutex.unlock();
            migrExecutorLock.cond_write_unlock();
            return;
        }
        if (isFirstRound && !resyncOnRestart) {
            // --> start of second round
            // --> incrememnt counter / marker of second round
            PerfTracer::incr(PerfEventType::SM_MIGRATION_SECOND_PHASE, invalid_vol_id);
            LOGMIGRATE << "Starting second round for SM token " << (migrExecutors.begin()->first);
            LOGMIGRATE << "migrExecutors.size()=" << migrExecutors.size();
            nextExecutor.set(migrExecutors.begin());
            for (uint32_t issued = 0; issued < parallelMigration; ++issued) {
                    auto next = nextExecutor.fetch_and_increment_saturating();
                    if (next == migrExecutors.cend()) {
                        break;
                    }
                    if (!allExecutorsInErrorState(next->first)) {
                        smTokenInProgress.insert(next->first);
                        auto cachedNextSMToken = next->first;

                        smTokenInProgressMutex.unlock();
                        migrExecutorLock.cond_write_unlock();

                        startSecondRebalanceRound(cachedNextSMToken);

                        migrExecutorLock.cond_write_lock();
                        smTokenInProgressMutex.lock();
                    }
            }
            smTokenInProgressMutex.unlock();
            migrExecutorLock.cond_write_unlock();
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
                LOGNORMAL << "SM Resync data migration completed. Cleaninup up clients and executors";
                LOGMIGRATE << "ResyncOnRestart: done with executors; wait for clients to complete";
                coalesceExecutorsNoLock();
                LOGMIGRATE << "ResyncOnRestart: coalesced executors";
                migrExecutorLock.upgrade();
                migrExecutors.clear();
                LOGMIGRATE << "ResyncOnRestart: cleared executors";
                // see if clients are also done so we can cleanup
                checkResyncDoneAndCleanup(true);

                smTokenInProgressMutex.unlock();
                migrExecutorLock.write_unlock();
                LOGNOTIFY << "SM Resync process done!";
                checkAndStartPendingResync();
            } else {
                smTokenInProgressMutex.unlock();
                migrExecutorLock.cond_write_unlock();
            }
        }
    }
}

void
MigrationMgr::checkAndStartPendingResync() {
    bool resync = false;
    resync = std::atomic_exchange(&isResyncPending, resync);
    if (resync) {
        cachedPendingResyncCb();
    }
}

bool
MigrationMgr::allExecutorsInErrorState(const fds_token_id &smToken) {
    for (SrcSmExecutorMap::const_iterator cit = migrExecutors[smToken].cbegin();
         cit != migrExecutors[smToken].cend();
         ++cit) {
         if (!cit->second->inErrorState()) {
            return false;
         }
    }
    return true;
}

void
MigrationMgr::startSecondRebalanceRound(fds_token_id smToken)
{
    Error err(ERR_OK);
    LOGNORMAL << "Starting second round of migration for SM token " << smToken;

    // notify all migration executors responsible for this SM token to start
    // second round of migration
    SCOPEDREAD(migrExecutorLock);
    for (SrcSmExecutorMap::const_iterator cit = migrExecutors[smToken].cbegin();
         cit != migrExecutors[smToken].cend();
         ++cit) {
        if (!cit->second->inErrorState()) {
            err = cit->second->startSecondObjectRebalanceRound();
            if (!err.ok()) {
                LOGERROR << "Failed to start second round of object rebalance for SM token "
                         << smToken << ", source SM " << std::hex
                         << (cit->first).uuid_get_val() << std::dec << " " << err;
                break;  // we are going to abort migration
            }
        }
    }

    if (!err.ok()) {
        abortMigrationForSMToken(smToken, err);
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

            addObjRefMsgPtr->srcVolId = addObjRefReq->getSrcVolId().get();
            addObjRefMsgPtr->destVolId = addObjRefReq->getDestVolId().get();
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
MigrationMgr::handleDltClose(const DLT* dlt,
                             const NodeUuid& mySvcUuid)
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

    // make all DLT tokens that this SM does not own anymore "not ready"
    const TokenList& tokList = dlt->getTokens(mySvcUuid);
    std::set<fds_token_id> tokSet;
    for (auto tok : tokList) {
        tokSet.insert(tok);
    }

    markUnownedTokensUnavailable(tokSet);

    // for now, to make sure we can handle another migration...
    MigrationState expectState = MIGR_IN_PROGRESS;
    if (!std::atomic_compare_exchange_strong(&migrState, &expectState, MIGR_IDLE)) {
        LOGNOTIFY << "DLT Close called in non- in progress state " << migrState;
        return ERR_OK;  // this is ok
    }

    // Cancel retryTokenMigration Timer Task since we are done with migration.
    if (retryTokenMigrationTask != nullptr) {
        mTimer.cancel(retryTokenMigrationTask);
    }

    LOGMIGRATE << "Will cleanup executors and migr clients";
    // Wait for all pending IOs to complete on Executors.
    coalesceExecutors();
    LOGMIGRATE << "Done coalescing executors";
    {
        SCOPEDWRITE(migrExecutorLock);
        migrExecutors.clear();
    }

    {
        SCOPEDWRITE(clientLock);
        // Wait for all pending IOs to complete on Client.
        coalesceClients();
        migrClients.clear();
    }
    LOGMIGRATE << "Done coalescing clients";
    targetDltVersion = DLT_VER_INVALID;
    resyncOnRestart = false;

    /**
     * Current migration is complete. Now check if there is any pending
     * resync request is pending. If that's so, ask Object Store Manager
     * to start a fresh resync.
     */
    checkAndStartPendingResync();

    return err;
}

void
MigrationMgr::notifyDltUpdate(const DLT *dlt,
                              fds_uint32_t bitsPerDltToken,
                              const NodeUuid& mySvcUuid)
{
    if (!isMigrationInProgress()) {
        fds_verify(bitsPerDltToken > 0);
        numBitsPerDltToken = bitsPerDltToken;
        if (dltTokenStatesEmpty() &&
            dlt->getVersion() == 1) {
            // The case where SM starts up and there was no DLT before,
            // so this SM is up and does not resync or migration
            // Initialize DLT tokens that this SM owns to ready
            resetDltTokensStates(bitsPerDltToken);
            const TokenList& tokens = dlt->getTokens(mySvcUuid);
            changeDltTokensAvailability(tokens, true);
        }
    }
}

template<typename T>
void
MigrationMgr::changeDltTokensAvailability(const T &tokens, bool availability) {
    FDSGUARD(dltTokenStatesMutex);
    for (auto token : tokens) {
        dltTokenStates[token] = availability;
        LOGNOTIFY << "DLT token " << token << " availability = " << availability;
    }
}

fds_bool_t
MigrationMgr::isDltTokenReady(const ObjectID& objId) {
    FDSGUARD(dltTokenStatesMutex);
    if (dltTokenStates.size() > 0) {
        fds_verify(numBitsPerDltToken > 0);
        fds_token_id dltTokId = DLT::getToken(objId, numBitsPerDltToken);
        return dltTokenStates[dltTokId];
    }
    return false;
}

/**
 * Reset all the dlt tokens assigned to this SM.
 */
void
MigrationMgr::resetDltTokensStates(fds_uint32_t& bitsPerDltToken) {
    FDSGUARD(dltTokenStatesMutex);
    if (dltTokenStates.size() == 0) {
        // first time migration/resync started
        fds_uint32_t numTokens = pow(2, bitsPerDltToken);
        dltTokenStates.clear();
        dltTokenStates.assign(numTokens, false);
    } else {
    // resync on restart should happen only once during on SM run between restarts
    //fds_verify(!resyncOnRestart);
    // if this is not a first migration msgs (= SM is gaining additional DLT tokens),
    // nothing to do here, because these DLT tokens are alrady marked not ready
    }
}

void
MigrationMgr::changeDltTokensState(const std::set<fds_token_id>& dltTokens,
                                   const bool& state) {
    FDSGUARD(dltTokenStatesMutex);
    fds_verify(dltTokenStates.size() > 0);
    for (auto dltTok : dltTokens) {
        dltTokenStates[dltTok] = state;
        LOGNOTIFY << "DLT token " << dltTok << " availability = " << state;
    }
}

void
MigrationMgr::markUnownedTokensUnavailable(const std::set<fds_token_id>& tokSet) {
    FDSGUARD(dltTokenStatesMutex);
    for (fds_uint32_t i = 0; i < dltTokenStates.size(); ++i) {
        if (dltTokenStates[i] == true) {
            if (tokSet.count(i) == 0) {
                LOGNOTIFY << "DLT token " << i << " is not owned by this SM anymore --> INACTIVE";
                dltTokenStates[i] = false;
            }
        }
    }
}

bool
MigrationMgr::dltTokenStatesEmpty() {
    FDSGUARD(dltTokenStatesMutex);
    return (dltTokenStates.size() == 0);
}

void
MigrationMgr::checkResyncDoneAndCleanup(fds_bool_t checkedExecutorsDone)
{
    if (!resyncOnRestart) {
        // not resync case
        return;
    }
    LOGMIGRATE << "Checking if Resync is done";
    fds_bool_t executorsDone = checkedExecutorsDone ? true : false;
    if (!checkedExecutorsDone) {
        SCOPEDREAD(migrExecutorLock);
        if (migrExecutors.size() == 0) {
            executorsDone = true;
        }
    }
    LOGMIGRATE << "Done with executors ? " << executorsDone;
    if (!executorsDone) {
        return;  // executors not done, migration still in progress
    }

    // we are done with executors
    SCOPEDWRITE(clientLock);
    if (migrClients.size() == 0) {
        // we are also done with clients = done with resync
        MigrationState expectState = MIGR_IN_PROGRESS;
        if (!std::atomic_compare_exchange_strong(&migrState, &expectState, MIGR_IDLE)) {
            LOGMIGRATE << "Already done or aborted, nothing to do, current state " << migrState;
            return;  // this is ok
        }

        // Cancel retryTokenMigration Timer Task since we are done with migration.
        if (retryTokenMigrationTask != nullptr) {
            mTimer.cancel(retryTokenMigrationTask);
        }

        LOGNOTIFY << "Token resync on restart / or being resync client completed for DLT version "
                  << targetDltVersion;
    }
}

/**
 * Handle timeout error message from either Executor or Client's
 * message sequence number tracker.
 */
void
MigrationMgr::timeoutAbortMigration()
{
    LOGNOTIFY << "Will abort tokenmigration due to timeout";
    abortMigration(ERR_SM_TOK_MIGRATION_TIMEOUT);
}

/**
 * Abort migration for a given SM token.
 * Currently this is called when any error is seen on destination
 * of SM migration.
 */
Error
MigrationMgr::abortMigrationForSMToken(fds_token_id &smToken, const Error &err)
{
    LOGNOTIFY <<"Aborting migrations for SM Token " << smToken;
    for (SrcSmExecutorMap::const_iterator cit = migrExecutors[smToken].cbegin();
         cit != migrExecutors[smToken].cend(); ++cit) {
        bool isFirstRound = !(cit->second->migrationRound() == 2);
        cit->second->abortMigration(err);
        changeDltTokensAvailability(cit->second->getTokens(), false);
        startNextSMTokenMigration(smToken, isFirstRound);
    }
    return err;
}

/**
 * Handles message from OM to abort migration
 */
Error
MigrationMgr::abortMigrationFromOM(fds_uint64_t tgtDltVersion)
{
    Error err(ERR_OK);
    MigrationState curState = atomic_load(&migrState);
    if (curState == MIGR_IN_PROGRESS) {
        if (tgtDltVersion == targetDltVersion) {
            LOGNOTIFY << "Will abort token migration per OM request";
            abortMigration(ERR_SM_TOK_MIGRATION_ABORTED);
        } else {
            LOGNOTIFY << "Ignoring abort migration request from OM "
                      << " target DLT version from OM " << tgtDltVersion
                      << " does not match version of this migration " << targetDltVersion;
            return ERR_INVALID_ARG;
        }
    } else if (curState == MIGR_IDLE) {
        LOGNOTIFY << "Abort migration called in idle state, will send not found error, "
                  << "so that OM can act on it if needed";
        return ERR_NOT_FOUND;
    }
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
    abortError = error;

    // Cancel retryTokenMigration Timer Task since migration is aborting
    if (retryTokenMigrationTask != nullptr) {
        mTimer.cancel(retryTokenMigrationTask);
    }
    setAbortPendingForExecutors();

    /**
     * Create a timer task that will check and wait for all currently
     * running executors to exit and then abort the migration.
     */
     tryAbortingMigrationTask.reset(new FdsTimerFunctionTask(
                                                    abortTimer,
                                                    std::bind(&MigrationMgr::tryAbortingMigration,
                                                              this)));
    int tryAbortInterval = 1;
    abortTimer.scheduleRepeated(tryAbortingMigrationTask, std::chrono::seconds(tryAbortInterval));
}

void
MigrationMgr::tryAbortingMigration() {
    LOGMIGRATE << "";
    /**
     * Cancel the try abort timer task.
     */
    abortTimer.cancel(tryAbortingMigrationTask);

    // if we need to ack Start Migration from OM, we will have a cb to reply to
    if (omStartMigrCb) {
        omStartMigrCb(abortError);
        omStartMigrCb = NULL;
    }

    // There could be some pending IOs in flight.  We can't blindly call to clear
    // all executors.  Call to coalesce executors before blowing them away.
    LOGMIGRATE << "Will coalesce executors";
    coalesceExecutors();
    LOGMIGRATE << "Finished coalescing executors, will clear executors";

    // Clear all migrationExecutors.
    {
        SCOPEDWRITE(migrExecutorLock);
        migrExecutors.clear();
    }
    smTokenInProgressMutex.lock();
    smTokenInProgress.clear();
    smTokenInProgressMutex.unlock();

    // Clear all retry SM token set.
    retryMigrSmTokenSet.clear();
    targetDltVersion = DLT_VER_INVALID;

    resyncOnRestart = false;

    LOGNOTIFY << "Done cleanup; migration aborted";
    MigrationState expectState = MIGR_ABORTED;
    if (!std::atomic_compare_exchange_strong(&migrState, &expectState, MIGR_IDLE)) {
        LOGERROR << "Unexpected migration state " << migrState;
    }
}

void
MigrationMgr::setAbortPendingForExecutors() {

    SCOPEDWRITE(migrExecutorLock);
    for (auto &executors : migrExecutors) {
        for (auto &executor : executors.second) {
            executor.second->setAbortPending();
        }
    }
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
    SCOPEDREAD(migrExecutorLock);
    coalesceExecutorsNoLock();
}

void
MigrationMgr::coalesceExecutorsNoLock()
{
    for (auto citExec = migrExecutors.cbegin(); citExec != migrExecutors.cend(); ++citExec) {
        fds_token_id tok = citExec->first;
        for (auto citSrcExec = migrExecutors[tok].cbegin(); citSrcExec != migrExecutors[tok].cend(); ++citSrcExec) {
            citSrcExec->second->waitForIOReqsCompletion(tok, citSrcExec->first);
        }
        LOGMIGRATE << "Finished coalescing executors for token " << tok;
    }
}

void
MigrationMgr::coalesceClients()
{
    for (auto citClient = migrClients.cbegin(); citClient != migrClients.cend(); ++citClient) {
        citClient->second->waitForIOReqsCompletion(citClient->first);
    }
}

void MigrationMgr::retryWithNewSMs(fds_uint64_t executorId,
                                   fds_token_id smToken,
                                   const std::set<fds_token_id>& dltTokens,
                                   fds_uint32_t round,
                                   const Error& error) {
    fds_bool_t createdAtLeastOneExecutor = false;
    NodeUuid sourceSmUuid;   // source SM for executor with id executorId
    MigrationExecutor::shared_ptr migrExecutor;   // executor that failed to sync
    fds_uint32_t uniqueId = getUniqueRestartId();
    {
        SCOPEDREAD(migrExecutorLock);
        for (SrcSmExecutorMap::const_iterator cit = migrExecutors[smToken].cbegin();
             cit != migrExecutors[smToken].cend();
             ++cit) {
            if (cit->second->getId() == executorId) {
                // found executor
                sourceSmUuid = cit->first;
                migrExecutor = cit->second;
                break;
            }
        }
    }

    if (migrExecutor->getInstanceNum() >= maxRetriesWithDifferentSources) {
        LOGCRITICAL << "Executor " << std::hex << executorId
                    << " failed to sync DLT tokens from source SM "
                    << sourceSmUuid.uuid_get_val()
                    << " and exhausted number of retries. ";
        migrExecutor->setDoneWithError();
        migrExecutor->clearRetryDltTokenSet();
        return;
    }

    // on error, executor sends stop resync msg to client, so that if client is
    // still alive, it will stop sending any sync related msgs to this SM

    // it only makes sense to retry if error happened on source SM side
    // or we could't reach SM
    if ((error == ERR_SVC_REQUEST_TIMEOUT) ||
        (error == ERR_SVC_REQUEST_INVOCATION) ||
        (error == ERR_SM_TOK_MIGRATION_SRC_SVC_REQUEST) ||
        (error == ERR_SM_TOK_MIGRATION_TIMEOUT) ||
        /// we get this error from source SM which failed to start
        (error == ERR_NODE_NOT_ACTIVE)) {
        LOGMIGRATE << "Executor " << std::hex << executorId
                   << " failed to sync DLT tokens from source SM "
                   << sourceSmUuid.uuid_get_val() << std::dec
                   << " SM token " << smToken << " " << error
                   << " will find new source SM(s) to sync from";

        const DLT* dlt = MODULEPROVIDER()->getSvcMgr()->getDltManager()->getDLT();
        NodeTokenMap newTokenGroups = dlt->getNewSourceSMs(sourceSmUuid,
                                                           dltTokens,
                                                           migrExecutor->getInstanceNum(),
                                                           failedSMsAsSource);

        for (auto const& tokenGroup : newTokenGroups) {
            NodeUuid srcSmUuid(tokenGroup.first);
            LOGMIGRATE << "Will migrate DLT tokens from source SM " << std::hex
                       << srcSmUuid.uuid_get_val() << std::dec
                       << " for SM token " << smToken;
            for (auto const& dltToken : tokenGroup.second) {
                fds_token_id smToken = SmDiskMap::smTokenId(dltToken);
                LOGNOTIFY << "Source SM " << std::hex << srcSmUuid.uuid_get_val() << std::dec
                           << " DLT token " << dltToken << " SM token " << smToken;
                SCOPEDWRITE(migrExecutorLock);
                if ((migrExecutors.count(smToken) == 0) ||
                    (migrExecutors.count(smToken) > 0 && migrExecutors[smToken].count(srcSmUuid) == 0)) {
                    fds_uint8_t curInstanceNum = migrExecutor->getInstanceNum() + 1;
                    MigrationType migrType = MIGR_SM_RESYNC;
                    migrExecutors[smToken][srcSmUuid] = createMigrationExecutor(srcSmUuid,
                                                                                objStoreMgrUuid,
                                                                                numBitsPerDltToken,
                                                                                smToken,
                                                                                targetDltVersion,
                                                                                migrType,
                                                                                true, //one phase migration
                                                                                uniqueId,
                                                                                curInstanceNum);
                    createdAtLeastOneExecutor = true;
                }

                if (migrExecutors[smToken][srcSmUuid]->getUniqueId() == uniqueId) {
                    // tell migration executor that it is responsible for this DLT token
                    migrExecutors[smToken][srcSmUuid]->addDltToken(dltToken);
                }
            }
        }
    }

    // DLT tokens that we failed to retry will remain unavailable
    // set "done with error" state for the failed executor, we will clean it
    // when the whole resync/migration is finished
    migrExecutor->setDoneWithError();
    migrExecutor->clearRetryDltTokenSet();

    /**
     * Now we are going to actually start migration only for these newly created
     * migration executors. To enable that, we will pass a unique restart id along
     * with the smToken for which we are issuing startMigration. This unique id
     * will be checked when snapshot callback tries to handover the newly taken
     * smToken snapshot to the relevant migration executors.
     */
    if (createdAtLeastOneExecutor) {
        startSmTokenMigration(smToken, uniqueId);
    } else {
        LOGCRITICAL << "Executor " << std::hex << executorId
                    << " failed to sync DLT tokens from source SM "
                    << sourceSmUuid.uuid_get_val()
                    << " and couldn't find any other SMs to sync from. ";
    }
}

void
MigrationMgr::abortMigrationCb(fds_uint64_t& executorId,
                               fds_token_id& smToken) {
    LOGMIGRATE << "executorID " << std::hex << executorId << std::dec;
    for (SrcSmExecutorMap::const_iterator cit = migrExecutors[smToken].cbegin();
         cit != migrExecutors[smToken].cend(); ++cit) {
        if (cit->second->getId() == executorId) {
            cit->second->abortMigration(ERR_SM_TOK_MIGRATION_ABORTED);
            changeDltTokensAvailability(cit->second->getTokens(), false);
        }
    }

    // Erase the smToken from inProgress data structure.
    if (allExecutorsInErrorState(smToken)) {
        FDSGUARD(smTokenInProgressMutex);
        smTokenInProgress.erase(smToken);
    }
}
}  // namespace fds

