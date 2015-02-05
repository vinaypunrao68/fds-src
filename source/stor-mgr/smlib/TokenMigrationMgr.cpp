/*
 * Copyright 2013-2014 Formation Data Systems, Inc.
 */

#include <vector>
#include <object-store/SmDiskMap.h>
#include <TokenMigrationMgr.h>
#include <fds_process.h>

namespace fds {

SmTokenMigrationMgr::SmTokenMigrationMgr(SmIoReqHandler *dataStore)
        : smReqHandler(dataStore),
          omStartMigrCb(NULL),
          targetDltVersion(DLT_VER_INVALID),
          numBitsPerDltToken(0),
          clientLock("Migration Client Map Lock") {
    migrState = ATOMIC_VAR_INIT(MIGR_IDLE);
    nextLocalExecutorId = ATOMIC_VAR_INIT(0);

    snapshotRequest.io_type = FDS_SM_SNAPSHOT_TOKEN;
    snapshotRequest.smio_snap_resp_cb = std::bind(&SmTokenMigrationMgr::smTokenMetadataSnapshotCb,
                                                  this,
                                                  std::placeholders::_1,
                                                  std::placeholders::_2,
                                                  std::placeholders::_3,
                                                  std::placeholders::_4);

    enableMigrationFeature = g_fdsprocess->get_fds_config()->get<bool>("fds.sm.migration.enable_feature");
}

SmTokenMigrationMgr::~SmTokenMigrationMgr() {
}

/**
 * Handles start migration message from OM.
 * Creates MigrationExecutor object for each <SM token, source SM> pair
 * which initiate token migration
 */
Error
SmTokenMigrationMgr::startMigration(fpi::CtrlNotifySMStartMigrationPtr& migrationMsg,
                                    OmStartMigrationCbType cb,
                                    const NodeUuid& mySvcUuid,
                                    fds_uint32_t bitsPerDltToken) {
    Error err(ERR_OK);

    // it's strange to receive empty message from OM, but ok we just ignore that
    if (migrationMsg->migrations.size() == 0) {
        LOGWARN << "We received empty migrations message from OM, nothing to do";
        return err;
    }

    // Check if the migraion feature is enabled or disabled.
    if (false == enableMigrationFeature) {
        LOGCRITICAL << "Migration is disabled! ignoring start migration msg";
        cb(ERR_OK);
        return err;
    }

    // We need to do migration, switch to 'in progress' state
    MigrationState expectState = MIGR_IDLE;
    if (!std::atomic_compare_exchange_strong(&migrState, &expectState, MIGR_IN_PROGRESS)) {
        LOGNOTIFY << "startMigration called in non-idle state " << migrState;
        return ERR_NOT_READY;
    }
    targetDltVersion = migrationMsg->DLT_version;
    numBitsPerDltToken = bitsPerDltToken;
    omStartMigrCb = cb;  // we will have to send ack to OM when we get second delta set

    // create migration executors for each <SM token, source SM> pair
    for (auto migrGroup : migrationMsg->migrations) {
        // migrGroup is <source SM, set of DLT tokens> pair
        NodeUuid srcSmUuid(migrGroup.source);
        LOGNORMAL << "Will migrate tokens from source SM " << std::hex
                  << srcSmUuid.uuid_get_val() << std::dec;
        // iterate over DLT tokens for this source SM
        for (auto dltTok : migrGroup.tokens) {
            fds_token_id smTok = SmDiskMap::smTokenId(dltTok);
            LOGNORMAL << "Source SM " << std::hex << srcSmUuid.uuid_get_val() << std::dec
                      << " DLT token " << dltTok << " SM token " << smTok;
            // if we don't know about this SM token and source SM, create migration executor
            if ((migrExecutors.count(smTok) == 0) ||
                (migrExecutors.count(smTok) > 0 && migrExecutors[smTok].count(srcSmUuid) == 0)) {
                LOGNORMAL << "Will create migration executor class";
                fds_uint32_t localExecId = std::atomic_fetch_add(&nextLocalExecutorId, (fds_uint32_t)1);
                fds_uint64_t globalExecId = getExecutorId(localExecId, mySvcUuid);
                LOGMIGRATE << "Will create migration executor class with executor ID "
                           << std::hex << globalExecId << std::dec;
                migrExecutors[smTok][srcSmUuid] = MigrationExecutor::unique_ptr(
                    new MigrationExecutor(smReqHandler,
                                          bitsPerDltToken,
                                          srcSmUuid,
                                          smTok, globalExecId, targetDltVersion,
                                          std::bind(
                                              &SmTokenMigrationMgr::migrationExecutorDoneCb, this,
                                              std::placeholders::_1, std::placeholders::_2,
                                              std::placeholders::_3, std::placeholders::_4)));
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
SmTokenMigrationMgr::startSmTokenMigration(fds_token_id smToken) {
    smTokenInProgress = smToken;
    LOGNORMAL << "Starting migration for SM token " << smToken;

    // enqueue snapshot work
    snapshotRequest.token_id = smTokenInProgress;
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
SmTokenMigrationMgr::smTokenMetadataSnapshotCb(const Error& error,
                                               SmIoSnapshotObjectDB* snapRequest,
                                               leveldb::ReadOptions& options,
                                               leveldb::DB *db) {
    Error err(ERR_OK);
    if (atomic_load(&migrState) == MIGR_ABORTED) {
        LOGNORMAL << "Migration was aborted, ignoring migration task";
        return;
    }

    // must match sm token id that is currently in progress
    fds_verify(snapRequest->token_id == smTokenInProgress);
    fds_verify(migrExecutors.count(smTokenInProgress) > 0);

    // on error, we just stop the whole migration process
    if (!error.ok()) {
        LOGERROR << "Failed to get index db snapshot for SM token " << smTokenInProgress;
        abortMigration(error);
        return;
    }

    // pass this snapshot to all migration executors that are responsible for
    // migrating DLT tokens that belong to this SM token
    for (SrcSmExecutorMap::const_iterator cit = migrExecutors[smTokenInProgress].cbegin();
         cit != migrExecutors[smTokenInProgress].cend();
         ++cit) {
        err = cit->second->startObjectRebalance(options, db);
        if (!err.ok()) {
            LOGERROR << "Failed to start object rebalance for SM token "
                     << smTokenInProgress << ", source SM " << std::hex
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
}

/**
 * Handle start object rebalance from destination SM
 */
Error
SmTokenMigrationMgr::startObjectRebalance(fpi::CtrlObjectRebalanceFilterSetPtr& rebalSetMsg,
                                          const fpi::SvcUuid &executorSmUuid,
                                          fds_uint32_t bitsPerDltToken) {
    Error err(ERR_OK);
    LOGMIGRATE << "Object Rebalance Initial Set executor SM Id " << std::hex
               << executorSmUuid.svc_uuid << " executor ID " << rebalSetMsg->executorID
               << std::dec << " seqNum " << rebalSetMsg->seqNum
               << " last " << rebalSetMsg->lastFilterSet;

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

    MigrationClient::shared_ptr migrClient;
    int64_t executorId = rebalSetMsg->executorID;
    {
        fds_mutex::scoped_lock l(clientLock);
        if (migrClients.count(executorId) == 0) {
            // first time we see a message for this executor ID
            NodeUuid executorNodeUuid(executorSmUuid);
            migrClient.reset(new MigrationClient(smReqHandler,
                                                 executorNodeUuid,
                                                 bitsPerDltToken));
            migrClients[executorId] = migrClient;
            // The assumption here that all SMs do the same mapping of
            // dlt token to SM token.. This is currently true; but if
            // this behavior changes, we need to change the code below
            fds_token_id smToken = SmDiskMap::smTokenId(rebalSetMsg->tokenId);
            // TODO(Anna) we need to implement multiple migr clients responsible
            // for migrating the same SM token (happens when multiple SMs are
            // added to the domain at the same time)
            fds_verify(smtokExecutorIdMap.count(smToken) == 0);
            smtokExecutorIdMap[smToken] = executorId;
        } else {
            migrClient = migrClients[executorId];
        }
    }
    // message contains DLTToken + {<objects + refcnt>} + seqNum + lastSetFlag.
    migrClient->migClientStartRebalanceFirstPhase(rebalSetMsg);
    return err;
}

/**
 * Ack from source SM when it receives the whole initial set of
 * objects.
 */
Error
SmTokenMigrationMgr::startObjectRebalanceResp() {
    Error err(ERR_OK);
    LOGMIGRATE << "";
    return err;
}

/**
 * Handle msg from destination SM to send data/metadata changes since the first delta set
 */
Error
SmTokenMigrationMgr::startSecondObjectRebalance(fpi::CtrlGetSecondRebalanceDeltaSetPtr& msg,
                                                const fpi::SvcUuid &executorSmUuid) {
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

    fds_mutex::scoped_lock l(clientLock);
    // we must have migration client if we are in progress state
    fds_verify(migrClients.count(msg->executorID) != 0);
    // TODO(Sean):  Need to reset the double sequence for executor on the destion SM
    //              before starting the second phase.
    migrClients[msg->executorID]->migClientStartRebalanceSecondPhase(msg);

    return err;
}

/**
 * Handle rebalance delta set at destination from the source
 */
Error
SmTokenMigrationMgr::recvRebalanceDeltaSet(fpi::CtrlObjectRebalanceDeltaSetPtr& deltaSet) {
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
        }
    }
    fds_verify(found);   // did we start doing more than one SM token at a time?
    return err;
}

/**
 * Ack from destination for rebalance delta set message
 */
Error
SmTokenMigrationMgr::rebalanceDeltaSetResp() {
    Error err(ERR_OK);
    LOGMIGRATE << "";
    return err;
}

void
SmTokenMigrationMgr::migrationExecutorDoneCb(fds_uint64_t executorId,
                                             fds_token_id smToken,
                                             fds_bool_t isFirstRound,
                                             const Error& error) {
    LOGNORMAL << "Migration executor " << std::hex << executorId << std::dec
              << " finished migration round first? " << isFirstRound << " done? "
              << !isFirstRound << error;

    fds_verify(smToken == smTokenInProgress);

    MigrationState curState = atomic_load(&migrState);
    if (curState == MIGR_ABORTED) {
        // migration already stopped, don't do anything..
        return;
    }
    fds_verify(curState == MIGR_IN_PROGRESS);

    // beta2: stop the whole migration process on any error
    if (!error.ok()) {
        abortMigration(error);
        return;
    }

    // check if we there are other executors that need to start migration
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
            if (isFirstRound) {
                startSmTokenMigration(it->first);
            } else {
                startSecondRebalanceRound(it->first);
            }
        } else {
            // we are done migrating, reply to start migration msg from OM
            if (isFirstRound) {
                // start with first executor to do the second round
                startSecondRebalanceRound(migrExecutors.begin()->first);
            } else {
                // done with second round -- all done
                omStartMigrCb(ERR_OK);
                omStartMigrCb = NULL;  // we replied, so reset
            }
        }
    }
}

void
SmTokenMigrationMgr::startSecondRebalanceRound(fds_token_id smToken) {
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
SmTokenMigrationMgr::getTargetDltVersion() const {
    // this will be invalid if migration not in progress
    return targetDltVersion;
}

Error
SmTokenMigrationMgr::startForwarding(fds_token_id smTok) {
    if (!isMigrationInProgress()) {
        return ERR_NOT_READY;
    }
    // in this state, we must know about bits per tok
    SmTokExecutorIdMap::iterator it = smtokExecutorIdMap.find(smTok);
    if (it == smtokExecutorIdMap.end()) {
        // we cannot start forwarding because there is no
        // migration client (on source) that is migration this
        // SM token
        return ERR_NOT_READY;
    }
    fds_verify(migrClients.count(it->second) > 0);
    // Tell migration client responsible for migrating SM token
    LOGMIGRATE << "Setting forwarding flag for SM token " << smTok;
    migrClients[it->second]->setForwardingFlag();
    return ERR_OK;
}

fds_bool_t
SmTokenMigrationMgr::forwardReqIfNeeded(const ObjectID& objId,
                                        fds_uint64_t reqDltVersion,
                                        FDS_IOType* req) {
    // we only do forwarding if migration is in progress
    if (isMigrationInProgress()) {
        if (reqDltVersion == targetDltVersion) {
            // this request was also sent to the destination SM
            // since AM sending the request is already on new DLT version
            return false;
        }
        // in this state, we must know about bits per tok
        fds_verify(numBitsPerDltToken != 0);
        fds_token_id dltTok = DLT::getToken(objId, numBitsPerDltToken);
        fds_token_id smTok = SmDiskMap::smTokenId(dltTok);
        SmTokExecutorIdMap::iterator it = smtokExecutorIdMap.find(smTok);
        if (it != smtokExecutorIdMap.end()) {
            // migration client tells if we need to forward
            fds_verify(migrClients.count(it->second) > 0);
            return migrClients[it->second]->forwardIfNeeded(dltTok, req);
        }
        // else we don't have an executor so we are not doing
        // migration of this DLT token
    }
    return false;
}

/**
 * Handles DLT close event. At this point IO should not arrive
 * with old DLT. Once we reply, we are done with token migration.
 * Assumes we are receiving DLT close event for the correct version,
 * caller should check this
 */
Error
SmTokenMigrationMgr::handleDltClose() {
    Error err(ERR_OK);
    // for now, to make sure we can handle another migration...
    MigrationState expectState = MIGR_IN_PROGRESS;
    if (!std::atomic_compare_exchange_strong(&migrState, &expectState, MIGR_IDLE)) {
        LOGNOTIFY << "DLT Close called in non- in progress state " << migrState;
        return ERR_OK;  // this is ok
    }
    LOGMIGRATE << "Will cleanup executors and migr clients";
    migrExecutors.clear();
    {
        fds_mutex::scoped_lock l(clientLock);
        migrClients.clear();
        smtokExecutorIdMap.clear();
    }
    targetDltVersion = DLT_VER_INVALID;
    return err;
}

/**
 * Handles message from OM to abort migration
 */
Error
SmTokenMigrationMgr::abortMigration() {
    Error err(ERR_OK);
    LOGNOTIFY << "Will abort token migration per OM request";
    abortMigration(ERR_SM_TOK_MIGRATION_ABORTED);
    return err;
}

/// local method that actually aborts migration
void
SmTokenMigrationMgr::abortMigration(const Error& error) {
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

    // if we need to ack Start  Migration from OM, we will have a cb to reply to
    if (omStartMigrCb) {
        omStartMigrCb(error);
        omStartMigrCb = NULL;
    }

    migrExecutors.clear();
    targetDltVersion = DLT_VER_INVALID;
}

fds_uint64_t
SmTokenMigrationMgr::getExecutorId(fds_uint32_t localId,
                                   const NodeUuid& smSvcUuid) const {
    fds_uint64_t execId = smSvcUuid.uuid_get_val();
    return ((execId << 32) | localId);
}

}  // namespace fds

