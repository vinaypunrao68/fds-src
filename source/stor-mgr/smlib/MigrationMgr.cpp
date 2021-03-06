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
#include <json/json.h>

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

    parallelMigration = CONFIG_UINT32("fds.sm.migration.parallel_migration", 2);
    LOGMIGRATE << "Parallel migration - " << parallelMigration << " threads";
    enableMigrationFeature = CONFIG_BOOL("fds.sm.migration.enable_feature", true);
    numPrimaries = CONFIG_UINT32("fds.sm.number_of_primary", 0);
    maxRetryCyclesWithDifferentSources = CONFIG_UINT32("fds.sm.migration.migration_retry_cycles", 32);

    // get migration timeout duration from the platform.conf file.
    migrationTimeoutSec = CONFIG_UINT32("fds.sm.migration.migration_timeout", 300);

    stateProviderId = "migrationmgr";
    g_fdsprocess->get_cntrs_mgr()->add_for_export(this);
}

MigrationMgr::~MigrationMgr() {
    g_fdsprocess->get_cntrs_mgr()->remove_from_export(this);
    mTimer.destroy();
    migrationTimeoutTimer->destroy();
}

fds_uint32_t
MigrationMgr::getMigrationMsgsTimeout() const {
    ObjectStorMgr* osm = dynamic_cast<ObjectStorMgr*>(smReqHandler);
    return osm->getInterStorMgrTimeout();
}

/**
 * Handles start migration message from OM.
 * Creates MigrationExecutor object for each <SM token, source SM> pair
 * which initiate token migration
 */
Error
MigrationMgr::startMigration(const fpi::CtrlNotifySMStartMigrationPtr& migrationMsg,
                             OmStartMigrationCbType cb,
                             const NodeUuid& mySvcUuid,
                             fds_uint32_t bitsPerDltToken,
                             MigrationType migrationType,
                             bool onePhaseMigration)
{
    Error err(ERR_OK);
    LOGNOTIFY << "Starting SM migration for DLT version "
               << migrationMsg->DLT_version << "."
               << " It is a resync? " << (migrationType == SMMigrType::MIGR_SM_RESYNC);

    //reset last execution outcome to false.
    lastExecutionOutcome = false;

    fiu_do_on("abort.sm.migration",\
              LOGDEBUG << "abort.sm.migration fault point enabled";\
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

    retryTokenMigrationTask.reset(new FdsTimerFunctionTask(std::bind(
                                                             &MigrationMgr::checkAndRetryMigration,
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

    // reset token state to "not ready" for all tokens.
    // **Only for rebalance(add/remove node) migration.**
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
    LOGNORMAL << "Number of executors(sm tokens) going to do data migration: " << migrExecutors.size();

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
                                      fds_uint16_t instanceNum,
                                      fds_uint16_t retryCycleNum) {
    fds_uint32_t localExecId = std::atomic_fetch_add(&nextLocalExecutorId,
                                                     (fds_uint32_t)instanceNum);
    fds_uint64_t globalExecId = getExecutorId(localExecId, mySvcUuid);
    LOGNOTIFY << "Creating executor: " << std::hex << globalExecId
              << " for sm token: " << std::dec << smTok
              << std::hex << " source sm(uuid): "
              << srcSmUuid.uuid_get_val() << std::dec
              << srcSmUuid.uuid_get_val() << std::dec << " instanceNum: "
              << instanceNum << " retryCycleNum: " << retryCycleNum;
    return MigrationExecutor::unique_ptr(
            new MigrationExecutor(smReqHandler,
                                  bitsPerDltToken,
                                  srcSmUuid,
                                  smTok, globalExecId, targetDltVersion,
                                  migrationType, onePhaseMigration,
                                  std::bind(&MigrationMgr::dltTokenMigrationFailedCb,
                                            this,
                                            std::placeholders::_1,
                                            std::placeholders::_2),
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
                                  uniqueId, instanceNum, retryCycleNum));
}

void
MigrationMgr::checkAndRetryMigration() {
    {
        fds_mutex::scoped_lock l(migrSmTokenLock);
        if (retryMigrSameSrcInProg) { return; }
    }
    retryTokenMigrForFailedDltTokens();
}

void
MigrationMgr::retryTokenMigrForFailedDltTokens()
{
    fds_mutex::scoped_lock l(migrSmTokenLock);

    if (!retryMigrSmTokenMap.empty()) {
        auto smTok = retryMigrSmTokenMap.begin();
        retrySmTokenInProgress = smTok->first;
        LOGNORMAL << "Starting migration retry for SM token " << retrySmTokenInProgress;

        // enqueue snapshot work
        snapshotRequests[retrySmTokenInProgress]->token_id = retrySmTokenInProgress;
        snapshotRequests[retrySmTokenInProgress]->retryReq = true;
        Error err = smReqHandler->enqueueMsg(FdsSysTaskQueueId, snapshotRequests[retrySmTokenInProgress].get());
        if (!err.ok()) {
            LOGERROR << "Failed to enqueue index db snapshot message ;" << err;
            // for now, we are failing the whole migration on any error
            abortMigrationForSMToken(retrySmTokenInProgress, err);
            return;
        }
        retryMigrSameSrcInProg = true;
    } else {
        retryMigrSameSrcInProg = false;
    }
}

/**
 * Remove the sm tokens from the retry migration set. Retry here was for case
 * where source SM was not ready.
 */
void MigrationMgr::removeTokensFromRetryMap(std::vector<fds_token_id>& tokens)
{
    fds_mutex::scoped_lock l(migrSmTokenLock);
    for (auto token: tokens) {
        retryMigrSmTokenMap.erase(token);
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
                          ResyncDoneOrPendingCb doneOrPendingResyncFn)
{
    LOGNORMAL << "Starting resync for dlt version " << dlt->getVersion();
    if (doneOrPendingResyncFn) {
        cachedResyncDoneOrPendingCb = doneOrPendingResyncFn;
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
    LOGNOTIFY << "Starting migration for SM token " << smToken;

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
MigrationMgr::smTokenMetadataSnapshotCb(const Error& snapErr,
                                        SmIoSnapshotObjectDB* snapRequest,
                                        leveldb::ReadOptions& options,
                                        std::shared_ptr<leveldb::DB> db,
                                        bool retryMigrFailedTokens,
                                        fds_uint32_t uniqueId)
{
    Error err(ERR_OK);
    fds_token_id curSmTokenInProgress;

    //Quickly check to see if there are any migration executors
    {
        SCOPEDREAD(migrExecutorLock);
        // Check the map to see if there are any migration executors
        if (migrExecutors.size() == 0) {
            LOGERROR << "There are no migration executors, aborting migration";
            abortMigrationForSMToken(curSmTokenInProgress, ERR_SM_TOK_MIGRATION_ABORTED);
            return;
        }
    }

    MigrationState curState = atomic_load(&migrState);
    if (curState == MIGR_ABORTED) {
        LOGDEBUG << "Migration was aborted, ignoring migration task";
        err = ERR_SM_TOK_MIGRATION_ABORTED;
    } else if (curState == MIGR_IDLE) {
        LOGNOTIFY << "Migration is in idle state, probably was aborted, ignoring";
        err = ERR_SM_TOK_MIGRATION_ABORTED;
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
    if (!snapErr.ok() || !err.ok()) {
        LOGERROR << "Failed to get index db snapshot for SM token: " << curSmTokenInProgress
                 << " primary error: " << err << " snapshot error: " << snapErr;
        smTokenMetadataSnapshotCbErrorHandler(snapErr, err, curSmTokenInProgress,
                                              options, db, retryMigrFailedTokens);
        return;
    }

    {
        SCOPEDREAD(migrExecutorLock);
        // must be currently in progress, otherwise abort gracefully
        if (snapRequest->token_id != curSmTokenInProgress) {
            LOGERROR << "Migration snap token id does not match current SM token in progress --"
                     << "SnapToken: " << snapRequest->token_id << " curSmTokenInProgress:" << curSmTokenInProgress;
            abortMigrationForSMToken(snapRequest->token_id, ERR_SM_TOK_MIGRATION_ABORTED);
            return;
        }

        // If there are no executors for current SM token, abort migration - something went wrong
        if (migrExecutors.count(snapRequest->token_id) < 1) {
            LOGERROR << "Migration did not have any executors for current SM token: " << curSmTokenInProgress
                     << " SnapToken: " << snapRequest->token_id;
            abortMigrationForSMToken(curSmTokenInProgress, ERR_SM_TOK_MIGRATION_ABORTED);
            return;
        }

        // pass this snapshot to all migration executors that are responsible for
        // migrating DLT tokens that belong to this SM token
        for (SrcSmExecutorMap::const_iterator cit = migrExecutors[curSmTokenInProgress].cbegin();
             cit != migrExecutors[curSmTokenInProgress].cend();
             ++cit) {
            if (cit->second->inErrorState()) {
                // This is a stale executor that failed migration. We are now here because we
                // created a new executor for the same source.
                continue;
            } else if (retryMigrFailedTokens) {
                auto seqId = retryMigrSmTokenMap.find(curSmTokenInProgress)->second;
                err = cit->second->startObjectRebalanceAgain(options,
                                                             db,
                                                             curSmTokenInProgress,
                                                             seqId,
                                                             std::bind(&MigrationMgr::removeTokenFromRetryMap,
                                                                       this,
                                                                       curSmTokenInProgress));
                snapshotCleanupAndErrorHandler(snapErr, err,
                                               curSmTokenInProgress,
                                               options, db);
                if (!err.ok()) {
                    LOGERROR << "Failed to start rebalance for sm token: "
                             << curSmTokenInProgress << " source sm: " << std::hex
                             << (cit->first).uuid_get_val() << std::dec << " " << err;
                }
                return;
            } else if (uniqueId == cit->second->getUniqueId()) {
                err = cit->second->startObjectRebalance(options, db); 
            }

            if (!err.ok()) {
                LOGERROR << "Failed to start rebalance for sm token: "
                         << curSmTokenInProgress << " source sm: " << std::hex
                         << (cit->first).uuid_get_val() << std::dec << " " << err;
            }
        }
    }

    smTokenMetadataSnapshotCbErrorHandler(snapErr, err, curSmTokenInProgress,
                                          options, db, retryMigrFailedTokens);
}

void
MigrationMgr::removeTokenFromRetryMap(fds_token_id& smTokenId) {
    fds_mutex::scoped_lock l(migrSmTokenLock);
    retryMigrSameSrcInProg = false;
    retryMigrSmTokenMap.erase(smTokenId);
}

void
MigrationMgr::snapshotCleanupAndErrorHandler(const Error& snapErr,
                                             const Error& err,
                                             fds_token_id& smTokenId,
                                             leveldb::ReadOptions& options,
                                             std::shared_ptr<leveldb::DB> db) {
    if (options.snapshot) {
        db->ReleaseSnapshot(options.snapshot);
    }

    if (!err.ok() || !snapErr.ok()) {
        abortMigrationForSMToken(smTokenId, err);
    }
}

void
MigrationMgr::smTokenMetadataSnapshotCbErrorHandler(const Error& snapErr,
                                                    const Error& err,
                                                    fds_token_id& smTokenId,
                                                    leveldb::ReadOptions& options,
                                                    std::shared_ptr<leveldb::DB> db,
                                                    bool retryMigrFailedTokens) {
    if (retryMigrFailedTokens) {
        removeTokenFromRetryMap(smTokenId);
    }
    snapshotCleanupAndErrorHandler(snapErr, err, smTokenId, options, db);
}

void
MigrationMgr::initiateClientForMigration(const fpi::CtrlObjectRebalanceFilterSetPtr& rebalSetMsg,
                                         const fpi::SvcUuid &executorSmUuid,
                                         const NodeUuid& mySvcUuid,
                                         fds_uint32_t bitsPerDltToken,
                                         const DLT* dlt,
                                         std::function<void(Error)> cb) {
    auto err = startObjectRebalance(rebalSetMsg,
                                    executorSmUuid,
                                    mySvcUuid,
                                    bitsPerDltToken, dlt);
    cb(err);
}

/**
 * Handle start object rebalance from destination SM
 */
Error
MigrationMgr::startObjectRebalance(const fpi::CtrlObjectRebalanceFilterSetPtr& rebalSetMsg,
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

    numBitsPerDltToken = bitsPerDltToken;
    targetDltVersion = rebalSetMsg->targetDltVersion;
    resyncOnRestart = rebalSetMsg->onePhaseMigration;

    MigrationClient::shared_ptr migrClient;
    int64_t executorId = rebalSetMsg->executorID;
    {
        SCOPEDWRITE(clientLock);
        if (migrClients.count(executorId) == 0) {
            auto clientDoneCb = std::bind(&MigrationMgr::handleClientDone, this, std::placeholders::_1);
            // first time we see a message for this executor ID
            NodeUuid executorNodeUuid(executorSmUuid);
            migrClients[executorId] = std::make_shared<MigrationClient>(smReqHandler,
                                                                        executorNodeUuid,
                                                                        targetDltVersion,
                                                                        bitsPerDltToken,
                                                                        rebalSetMsg->onePhaseMigration,
                                                                        clientDoneCb);
        }
        migrClient = migrClients[executorId];
    }

    if (migrClients[executorId] == nullptr || migrClients.empty()) {
        return ERR_OUT_OF_MEMORY;
    }

    // If this SM is just a source and does not get Start Migration from OM
    // make sure that we set the migration state in progress
    MigrationState expectState = MIGR_IDLE;
    if (!std::atomic_compare_exchange_strong(&migrState, &expectState, MIGR_IN_PROGRESS)) {
        // check if migration was aborted
        if (atomic_load(&migrState) == MIGR_ABORTED) {
            LOGWARN << "Migration was already aborted, not going to handle object rebalance msg";
            return ERR_SM_TOK_MIGRATION_ABORTED;
        }
        // else was already in progress
    }

    // message contains DLTToken + {<objects + refcnt>} + seqNum + lastSetFlag.
    err = migrClient->migClientStartRebalanceFirstPhase(rebalSetMsg, srcAccepted);
    if (!err.ok()) {
        handleClientDone(executorId);
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
                NodeUuid executorNodeUuid(executorSmUuid);

                if (cit->first == executorNodeUuid) {
                    return false;
                }

                // see if it has higher responsibility for the DLT token
                DltTokenGroupPtr smGroup = dlt->getNodes(dltToken);
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
MigrationMgr::startSecondObjectRebalance(const fpi::CtrlGetSecondRebalanceDeltaSetPtr& msg,
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
    if (!(atomic_load(&migrState) == MIGR_IN_PROGRESS)) {
        LOGWARN << "Unexpected migration state. Aborting migration.";
        return ERR_SM_TOK_MIGRATION_ABORTED;
    }
    SCOPEDREAD(clientLock);
    // we must have migration client if we are in progress state
    if (migrClients.count(msg->executorID) == 0) {
        LOGERROR << "Lost migration client for executor: " << msg->executorID
                 << " destination SM: " << executorSmUuid.svc_uuid;
        return ERR_SM_TOK_MIGRATION_ABORTED;
    }
    // TODO(Sean):  Need to reset the double sequence for executor on the destion SM
    //              before starting the second phase.
    migrClients[msg->executorID]->migClientStartRebalanceSecondPhase(msg);

    return err;
}

void
MigrationMgr::handleClientDone(fds_uint64_t executorId) {

    fds_bool_t doneWithClients = false;
    clientLock.read_lock();
    if (migrClients.count(executorId) > 0) {
        // Get reference to the client so we can release the read lock
        MigrationClient::shared_ptr migrClient = migrClients[executorId];
        clientLock.read_unlock();
        LOGDEBUG << "Will remove client for executor " << std::hex << executorId;
        migrClient->waitForIOReqsCompletion(executorId);

        clientLock.write_lock();
        migrClients.erase(executorId);
        clientLock.write_unlock();
        doneWithClients = (migrClients.size() == 0);
        LOGMIGRATE << "Removed one client, clients left so far " << migrClients.size();
    } else {
        // Failed the if, still need to release the lock
        clientLock.read_unlock();
    }

    if (doneWithClients) {
        setWasSrcTimestamp();
        checkMigrationDoneAndCleanup(false);
    }
}


/**
 * Handle rebalance delta set at destination from the source
 */
Error
MigrationMgr::recvRebalanceDeltaSet(const fpi::CtrlObjectRebalanceDeltaSetPtr& deltaSet)
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

    if (!error.ok()) {
        retryWithNewSMs(executorId, smToken, dltTokens, round, error);
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
            LOGNOTIFY << "start migration for token: " << next->first
                      << " first round?: " << isFirstRound;
            auto cachedNextSMToken = next->first;

            smTokenInProgressMutex.unlock();
            migrExecutorLock.cond_write_unlock();

            startSmTokenMigration(cachedNextSMToken);
        } else {
            // coming in here during second phase when calling the ExecutorDone callback
            LOGNOTIFY << "starting second round for " << (next->first);

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
            LOGNOTIFY << "Starting second round for SM token " << (migrExecutors.begin()->first);
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
                LOGNOTIFY << "OM triggered token migration completed. Notifying OM";
                omStartMigrCb(ERR_OK);
                omStartMigrCb = NULL;  // we replied, so reset
            }
            if (resyncOnRestart) {
                // done with executors.  First check if there is any pending migration
                // requests before clearing executors.  At this point, there shouldn't
                // be any.
                LOGNOTIFY << "SM Resync data migration completed. Cleaninup up clients and executors";
                LOGMIGRATE << "ResyncOnRestart: done with executors; wait for clients to complete";
                if (!migrExecutors.empty()) {
                    setWasDstTimestamp();
                }
                coalesceExecutorsNoLock();
                LOGMIGRATE << "ResyncOnRestart: coalesced executors";
                migrExecutorLock.upgrade();
                migrExecutors.clear();
                LOGMIGRATE << "ResyncOnRestart: cleared executors";
                // see if clients are also done so we can cleanup
                checkMigrationDoneAndCleanup(true);
                smTokenInProgressMutex.unlock();
                migrExecutorLock.write_unlock();
                reportMigrationCompleted(true);
            } else {
                smTokenInProgressMutex.unlock();
                migrExecutorLock.cond_write_unlock();
            }
        }
    }
}

void
MigrationMgr::reportMigrationCompleted(fds_bool_t isResync) {

    LOGNOTIFY << "SM migration completed for dlt version: " << targetDltVersion
              << " resync: " << resyncOnRestart;
    // Reset resync indicator flag in Migration Manager.
    targetDltVersion = DLT_VER_INVALID;
    resyncOnRestart = false;
    lastExecutionOutcome = true;

    bool resyncPending = false;
    resyncPending = std::atomic_exchange(&isResyncPending, resyncPending);
    if (resyncPending || isResync) {
        if (cachedResyncDoneOrPendingCb) {
            cachedResyncDoneOrPendingCb(resyncPending, isResync);
        }
    } else {
        LOGNOTIFY << "No pending resync";
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
    /**
     * Info(Gurpreet): Forwarding io logic from Migration Client to Executor will only
     * execute for rebalance(add-node) migration case. Not for resync. (Although
     * I would love to remove forwarding logic altogether, but removing it for
     * rebalance case will require changes to dlt close handling, migration callback
     * to OM plus other changes, which is a work item in itself.
     * I've created FS-6229 for it.
     * For Resync, I've removed the constraint of not accepting puts during token
     * migration for a give token. IO forwarding was required when SM was not accepting
     * io from AMs during token migration. Now all tokens will accept io independent of
     * the state of resync.
     */
    if (resyncOnRestart) {
        return;
    }
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
    fds_assert(migrClients.count(executorId) > 0);
    if (migrClients.count(executorId) == 0) {
        LOGERROR << "Invalid migration client for token: " << smTok
                 << " executorId: " << std::hex << executorId;
        return;
    }
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

std::string
MigrationMgr::setWasSrcTimestamp() {
    wasSrcAtTime = util::getLocalTimeString(util::getTimeStampSeconds());
    return wasSrcAtTime;
}

std::string
MigrationMgr::setWasDstTimestamp() {
    wasDstAtTime = util::getLocalTimeString(util::getTimeStampSeconds());
    return wasDstAtTime;
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

    cleanUpClientsAndExecutors();

    // set last migration execution outcome to successfull
    lastExecutionOutcome = true;
    return err;
}

void
MigrationMgr::cleanUpClientsAndExecutors()
{
    // for now, to make sure we can handle another migration...
    MigrationState expectState = MIGR_IN_PROGRESS;
    if (!std::atomic_compare_exchange_strong(&migrState, &expectState, MIGR_IDLE)) {
        LOGNOTIFY << "cleanUpClientsAndExecutors() called in non- in progress state " << migrState;
        return;
    }

    // Cancel retryTokenMigration Timer Task since we are done with migration.
    if (retryTokenMigrationTask != nullptr) {
        mTimer.cancel(retryTokenMigrationTask);
    }

    LOGMIGRATE << "Will cleanup executors and migr clients";

    if (!migrExecutors.empty()) {
        setWasDstTimestamp();
    }

    // Wait for all pending IOs to complete on Executors.
    coalesceExecutors();
    LOGMIGRATE << "Done coalescing executors";
    {
        SCOPEDWRITE(migrExecutorLock);
        migrExecutors.clear();
    }

    if (!migrClients.empty()) {
        setWasSrcTimestamp();
    }

    {
        SCOPEDWRITE(clientLock);
        // Wait for all pending IOs to complete on Client.
        coalesceClients();
        migrClients.clear();
    }
    LOGMIGRATE << "Done coalescing clients";

    /**
     * Current migration is complete. Now check if there is any pending
     * resync request is pending. If that's so, ask Object Store Manager
     * to start a fresh resync.
     */
    reportMigrationCompleted(false);
}

void
MigrationMgr::makeTokensAvailable(const DLT *dlt,
                                  fds_uint32_t bitsPerDltToken,
                                  const NodeUuid& mySvcUuid)
{
    if (!isMigrationInProgress()) {
        fds_verify(bitsPerDltToken > 0);
        numBitsPerDltToken = bitsPerDltToken;

        // The case where SM starts up and there was no DLT before,
        // so this SM is up and does not resync or migration
        // Initialize DLT tokens that this SM owns to ready
        resetDltTokensStates(bitsPerDltToken);
        const TokenList& tokens = dlt->getTokens(mySvcUuid);
        LOGNOTIFY << "Total tokens owned: " << tokens.size();
        changeDltTokensAvailability(tokens, true);
    }
}

// Change state for a bunch of tokens
template<typename T>
void
MigrationMgr::changeDltTokensAvailability(const T &tokens, bool availability) {
    for (auto token : tokens) {
        changeTokenState(token, availability);
    }
}

// Change state for single token
void
MigrationMgr::changeTokenState(fds_token_id &token, bool availability) {
    {
        FDSGUARD(dltTokenStatesMutex);
        dltTokenStates[token] = availability;
    }
    LOGNOTIFY << "DLT token " << token << " availability = " << availability;
}

fds_bool_t
MigrationMgr::isDltTokenReady(const ObjectID& objId) {
    FDSGUARD(dltTokenStatesMutex);
    if (dltTokenStates.size() > 0) {
        fds_verify(numBitsPerDltToken > 0);
        fds_token_id dltTokId = DLT::getToken(objId, numBitsPerDltToken);
        return dltTokenStates[dltTokId];
    }
    return true;
}

fds_bool_t
MigrationMgr::primaryTokensReady(const fds::DLT *dlt,
                                 const NodeUuid& mySvcUuid) {
    if (numPrimaries == 0) {
        // not using consistency model, ok if resync failed, so returnung true
        return true;
    }
    TokenList tokList;   // list of DLT tokens for which this SM is primary
    fds_uint32_t rows = dlt->getDepth();
    if (numPrimaries < rows) {
        rows = numPrimaries;
    }
    for (fds_uint32_t i = 0; i < rows; ++i) {
        dlt->getTokens(&tokList, mySvcUuid, i);
    }

    // if at least one primary token is not ready, return false
    FDSGUARD(dltTokenStatesMutex);
    for (auto tok : tokList) {
        if (!dltTokenStates[tok]) {
            return false;
        }
    }
    // all primary tokens are ready
    return true;
}

/**
 * Reset all the dlt tokens assigned to this SM.
 */
void
MigrationMgr::resetDltTokensStates(fds_uint32_t& bitsPerDltToken) {
    FDSGUARD(dltTokenStatesMutex);
    if (dltTokenStates.size() == 0) {
        // first time migration/resync started
        LOGNOTIFY << "Resetting all tokens to unavailable";
        fds_uint32_t numTokens = pow(2, bitsPerDltToken);
        dltTokenStates.clear();
        dltTokenStates.assign(numTokens, false);
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
MigrationMgr::checkMigrationDoneAndCleanup(fds_bool_t checkedExecutorsDone)
{
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

        LOGNOTIFY << "SM migration process done! (Token resync on restart / or being resync client"
                  << " completed for DLT version " << targetDltVersion << ")";
    }
}

/**
 * Handle timeout error message from either Executor or Client's
 * message sequence number tracker.
 */
void
MigrationMgr::timeoutAbortMigration()
{
    LOGNOTIFY << "Will abort token migration due to timeout";
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
    LOGNOTIFY << "Aborting migrations for SM Token " << smToken
              << ". Error: " << err;
    SrcSmExecutorMap::const_iterator cit = migrExecutors[smToken].cbegin();
    if (cit == migrExecutors[smToken].cend()) {
        LOGWARN << "Migration executor not found for SM token: " << smToken;
        return err;
    }

    bool isFirstRound = !(cit->second->migrationRound() == 2);
    cit->second->abortMigration(err);
    /**
     * Even if data migration did not complete for this token.
     * Make the token available. IOs should never stop.
     */
    changeDltTokensAvailability(cit->second->getTokens(), true);
    startNextSMTokenMigration(smToken, isFirstRound);
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
    if (!migrExecutors.empty()) {
        setWasDstTimestamp();
    }
    if (!migrClients.empty()) {
        setWasSrcTimestamp();
    }
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
    retryMigrSmTokenMap.clear();
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
MigrationMgr::dltTokenMigrationFailedCb(fds_token_id &smToken, uint64_t seqId)
{
    fds_mutex::scoped_lock l(migrSmTokenLock);
    retryMigrSmTokenMap.insert(std::make_pair(smToken, seqId));
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
    fds_uint16_t curRetryCycleNum;
    fds_uint16_t curInstanceNum;

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

    curRetryCycleNum = migrExecutor->getRetryCycleNum();
    curInstanceNum = migrExecutor->getInstanceNum();

    if (curInstanceNum >= maxRetriesWithDifferentSources) {
        if (curInstanceNum % maxRetriesWithDifferentSources == 0) {
            curRetryCycleNum= 1;
        }
    }

    if (curRetryCycleNum > maxRetryCyclesWithDifferentSources) {
        LOGCRITICAL << "Executor " << std::hex << executorId
                    << " failed to sync DLT tokens from source SM "
                    << sourceSmUuid.uuid_get_val()
                    << " and exhausted number of retries. "
                    << "curRetryCycleNum :" << curRetryCycleNum;
        migrExecutor->setDoneWithError();
        // Retries exhausted, still make token available so that it can serve
        // existing data and accept new writes.
        changeTokenState(smToken, true);
        return;
    }

    // on error, executor sends stop resync msg to client, so that if client is
    // still alive, it will stop sending any sync related msgs to this SM

    // it only makes sense to retry if error happened on source SM side
    // or we could't reach SM
    if (retryWithNewSmErrors(error)) {
        LOGNOTIFY << "Executor " << std::hex << executorId
                  << " failed to sync DLT tokens from source SM "
                  << sourceSmUuid.uuid_get_val() << std::dec
                  << " SM token " << smToken << " " << error
                  << " will find new source SM(s) to sync from";

        const DLT* dlt = MODULEPROVIDER()->getSvcMgr()->getDltManager()->getDLT();
        NodeTokenMap newTokenGroups = dlt->getNewSourceSMs(sourceSmUuid,
                                                           dltTokens,
                                                           migrExecutor->getInstanceNum(),
                                                           objStoreMgrUuid);

        for (auto const& tokenGroup : newTokenGroups) {
            NodeUuid srcSmUuid(tokenGroup.first);
            LOGMIGRATE << "Will migrate DLT tokens from source SM " << std::hex
                       << srcSmUuid.uuid_get_val() << std::dec
                       << " for SM token " << smToken;
            for (auto const& dltToken : tokenGroup.second) {
                if (tokenGroup.first == INVALID_RESOURCE_UUID) {
                    LOGWARN << "Failed to sync DLT token " << dltToken << ", token will remain UNAVAILABLE";
                    continue;
                }
                fds_token_id smToken = SmDiskMap::smTokenId(dltToken);
                LOGMIGRATE << "Source SM " << std::hex << srcSmUuid.uuid_get_val() << std::dec
                           << " DLT token " << dltToken << " SM token " << smToken;
                SCOPEDWRITE(migrExecutorLock);

                curInstanceNum += 1;

                // If all the sources have been tried once, create new executor and retry with
                // a previously failed source SM until all sources have been tried
                // maxRetryCyclesWithDifferentSources times.

                MigrationType migrType = MIGR_SM_RESYNC;
                migrExecutors[smToken][srcSmUuid] = createMigrationExecutor(srcSmUuid,
                                                                            objStoreMgrUuid,
                                                                            numBitsPerDltToken,
                                                                            smToken,
                                                                            targetDltVersion,
                                                                            migrType,
                                                                            true, //one phase migration
                                                                            uniqueId,
                                                                            curInstanceNum,
                                                                            curRetryCycleNum);

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

    /**
     * Now we are going to actually start migration only for these newly created
     * migration executors. To enable that, we will pass a unique restart id along
     * with the smToken for which we are issuing startMigration. This unique id
     * will be checked when snapshot callback tries to handover the newly taken
     * smToken snapshot to the relevant migration executors.
     */
    startSmTokenMigration(smToken, uniqueId);
}

void
MigrationMgr::abortMigrationCb(fds_uint64_t& executorId,
                               fds_token_id& smToken) {
    LOGMIGRATE << "executorID " << std::hex << executorId << std::dec;
    for (SrcSmExecutorMap::const_iterator cit = migrExecutors[smToken].cbegin();
         cit != migrExecutors[smToken].cend(); ++cit) {
        if (cit->second->getId() == executorId) {
            cit->second->abortMigration(ERR_SM_TOK_MIGRATION_ABORTED);
            changeDltTokensAvailability(cit->second->getTokens(), true);
        }
    }

    // Erase the smToken from inProgress data structure.
    if (allExecutorsInErrorState(smToken)) {
        FDSGUARD(smTokenInProgressMutex);
        smTokenInProgress.erase(smToken);
    }
}

void MigrationMgr::getMigExecStateInfo(std::vector<fds_token_id>&done,
                                       std::vector<fds_token_id>& pending,
                                       std::vector<fds_token_id>& inprog) {
    SCOPEDREAD(migrExecutorLock);
    for (auto srcSmTokenMap : migrExecutors) {
        for (auto executor : srcSmTokenMap.second) {
            if (executor.second->inInitState()) {
                pending.push_back(srcSmTokenMap.first);
            } else if (executor.second->isDone() || executor.second->inErrorState()) {
                done.push_back(srcSmTokenMap.first);
            } else {
                inprog.push_back(srcSmTokenMap.first);
            }
        }
    }
}


//Provides info about migration state and token availability
std::string MigrationMgr::getStateInfo() {
    Json::Value unavailableTokens;
    Json::Value availableTokens;
    Json::Value state;
    fds_uint64_t dltV = getTargetDltVersion();

    const DLT* dlt = MODULEPROVIDER()->getSvcMgr()->getDltManager()->getDLT();
    if (getTargetDltVersion() != DLT_VER_INVALID) {
        dlt = MODULEPROVIDER()->getSvcMgr()->getDltManager()->getDLT(getTargetDltVersion());
    }
    if (dlt) {
        dltV = dlt->getVersion();
    } else {
        LOGWARN << "Target dlt: " << dltV << " copy not present with sm";
    }

    /* Lock dltTokenStates to make sure there is no change in dlt while checking*/
    if (dlt && !dltTokenStatesEmpty()) {
        auto myTokens = dlt->getTokens(objStoreMgrUuid);
        std::sort(myTokens.begin(), myTokens.end());
        synchronized(dltTokenStatesMutex) {
            for (const auto &tok : myTokens) {
                if (dltTokenStates[tok]) {
                    availableTokens.append(tok);
                } else {
                    unavailableTokens.append(tok);
                }
            }
        }
        state["owned"] = static_cast<Json::Value::Int64>(myTokens.size());
        state["available"] = availableTokens;
        state["unavailable"] = unavailableTokens;
    }

    Json::Value edone, epending, einprog;
    std::vector<fds_token_id> execs_done, execs_pending, execs_inprog;

    getMigExecStateInfo(execs_done, execs_pending, execs_inprog);

    for (const auto &token: execs_done) { edone.append(token); }
    for (const auto &token: execs_pending) { epending.append(token); }
    for (const auto &token: execs_inprog) { einprog.append(token); }

    /* Return the available and unavailable token counts as well as the unavailable token list */
    state["dlt_version"] = static_cast<Json::Value::Int64>(dltV);
    std::string outcome;
    (lastExecutionOutcome) ? outcome = "(completed successfully)" : outcome = "(completed with errors)";
    auto wasDstAtInfo = wasDstAtTime;
    if (!wasDstAtInfo.empty()) { wasDstAtInfo.append(outcome); }
    state["was_dst_at"] = wasDstAtInfo;
    state["was_src_at"] = wasSrcAtTime;
    state["rebal_inprog"] = (isMigrationInProgress() ? (isResync() ? false: true) : false);
    state["resync_inprog"] = (isMigrationInProgress() ? (isResync() ? true: false) : false);
    state["mig_done"] = edone;
    state["mig_pending"] = epending;
    state["mig_in_prog"] = einprog;
    state["num_execs"] = static_cast<Json::Value::Int64>(migrExecutors.size());
    state["num_clients"] = static_cast<Json::Value::Int64>(migrClients.size());
    std::stringstream ss;
    ss << state;
    return ss.str();
}

/* Provides stateProviderId */
std::string MigrationMgr::getStateProviderId()
{
    return stateProviderId;
}


}  // namespace fds

