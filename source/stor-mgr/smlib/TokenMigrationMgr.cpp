/*
 * Copyright 2013-2014 Formation Data Systems, Inc.
 */

#include <vector>
#include <TokenMigrationMgr.h>

namespace fds {

SmTokenMigrationMgr::SmTokenMigrationMgr() {
}

SmTokenMigrationMgr::~SmTokenMigrationMgr() {
}

/**
 * Handles start migration message from OM.
 * Creates MigrationExecutor object for each SM token, source SM
 * which initiate token migration
 */
Error
SmTokenMigrationMgr::startMigration(SmIoReqHandler *data_store,
                                    fpi::CtrlNotifySMStartMigrationPtr& migrationMsg) {
    Error err(ERR_OK);
    LOGDEBUG << "Start Migration";
    return err;
}

/**
 * Handle start object rebalance from destination SM
 */
Error
SmTokenMigrationMgr::startObjectRebalance(
                               fds_token_id tokenId,
                               std::vector<fpi::CtrlObjectMetaDataSync>& objToSync) {
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
    LOGDEBUG << "";
    return err;
}

}  // namespace fds

