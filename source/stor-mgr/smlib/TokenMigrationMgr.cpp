/*
 * Copyright 2013-2014 Formation Data Systems, Inc.
 */

#include <vector>
#include <object-store/SmDiskMap.h>
#include <TokenMigrationMgr.h>

namespace fds {

SmTokenMigrationMgr::SmTokenMigrationMgr(SmIoReqHandler *dataStore)
        : smReqHandler(dataStore) {
    migrState = ATOMIC_VAR_INIT(MIGR_IDLE);

    snapshotRequest.io_type = FDS_SM_SNAPSHOT_TOKEN;
    snapshotRequest.smio_snap_resp_cb = std::bind(&SmTokenMigrationMgr::smTokenMetadataSnapshotCb,
                                                  this,
                                                  std::placeholders::_1,
                                                  std::placeholders::_2,
                                                  std::placeholders::_3,
                                                  std::placeholders::_4);
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
                                    fds_uint32_t bitsPerDltToken) {
    Error err(ERR_OK);
    // it's strange to receive empty message from OM, but ok we just ignore that
    if (migrationMsg->migrations.size() == 0) {
        LOGWARN << "We received empty migrations message from OM, nothing to do";
        return err;
    }

    // we need to do migration, switch to 'in progress' state
    MigrationState expectState = MIGR_IDLE;
    if (!std::atomic_compare_exchange_strong(&migrState, &expectState, MIGR_IN_PROGRESS)) {
        LOGNOTIFY << "startMigration called in non-idle state " << migrState;
        return ERR_NOT_READY;
    }

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
                migrExecutors[smTok][srcSmUuid] = MigrationExecutor::unique_ptr(
                    new MigrationExecutor(smReqHandler, bitsPerDltToken, srcSmUuid, smTok));
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
    smTokenInProgress = cit->first;
    LOGNORMAL << "Starting migration for SM token " << smTokenInProgress;

    // enqueue snapshot work
    snapshotRequest.token_id = smTokenInProgress;
    err = smReqHandler->enqueueMsg(FdsSysTaskQueueId, &snapshotRequest);
    if (!err.ok()) {
        LOGERROR << "Failed to enqueue index db snapshot message ;" << err;
        // for now, we are failing the whole migration on any error
        abortMigration(err);
    }

    return err;
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

    // TODO(Anna) start snapshot of the next SM token when we finish
    // migration of all DLT tokens belonging to this SM token
}

/**
 * Handle start object rebalance from destination SM
 */
Error
SmTokenMigrationMgr::startObjectRebalance(fpi::CtrlObjectRebalanceInitialSetPtr& rebalSetMsg,
                                          const fpi::SvcUuid &executorSmUuid,
                                          fds_uint32_t bitsPerDltToken) {
    Error err(ERR_OK);
    LOGDEBUG << "";
    return err;
}

/**
 * Ack from source SM when it receives the whole initial set of
 * objects.
 */
Error
SmTokenMigrationMgr::startObjectRebalanceResp() {
    Error err(ERR_OK);
    LOGDEBUG << "";
    return err;
}

/**
 * Handle rebalance delta set at destination from the source
 */
Error
SmTokenMigrationMgr::recvRebalanceDeltaSet(fpi::CtrlObjectRebalanceDeltaSetPtr& deltaSet) {
    Error err(ERR_OK);
    LOGDEBUG << "";
    return err;
}

/**
 * Ack from destination for rebalance delta set message
 */
Error
SmTokenMigrationMgr::rebalanceDeltaSetResp() {
    Error err(ERR_OK);
    LOGDEBUG << "";
    return err;
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
        LOGNOTIFY << "startMigration called in non- in progress state " << migrState;
        return ERR_OK;  // this is ok
    }
    LOGDEBUG << "";
    return err;
}

void
SmTokenMigrationMgr::abortMigration(const Error& error) {
    LOGNOTIFY << "Aborting token migration " << error;

    // TODO(Anna) depending on current state, send ack with error to OM
    // and do any necessary cleanup here...

    migrExecutors.clear();
}

}  // namespace fds
