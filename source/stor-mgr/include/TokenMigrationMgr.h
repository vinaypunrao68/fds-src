/*
 * Copyright 2013-2014 Formation Data Systems, Inc.
 */

#ifndef SOURCE_STOR_MGR_INCLUDE_TOKENMIGRATIONMGR_H_
#define SOURCE_STOR_MGR_INCLUDE_TOKENMIGRATIONMGR_H_

#include <vector>
#include <SmIo.h>

namespace fds {

/*
 * Class responsible for migrating tokens between SMs
 * For each migration process, it creates migrationExecutors
 * and migrationSource objects that actually handle migration
 * on source and destination; SmTokenMigrationMgr mainly forwards
 * service layer messages to appropriate migrationExecutor or
 * migrationSource objects.
 */
class SmTokenMigrationMgr {
  public:
    SmTokenMigrationMgr();
    ~SmTokenMigrationMgr();

    typedef std::unique_ptr<SmTokenMigrationMgr> unique_ptr;

    /**
     * Handles start migration message from OM.
     * Creates MigrationExecutor object for each SM token, source SM
     * which initiate token migration
     */
    Error startMigration(SmIoReqHandler *data_store,
                         fpi::CtrlNotifySMStartMigrationPtr& migrationMsg);

    /**
     * Handle start object rebalance from destination SM
     */
    Error startObjectRebalance(fds_token_id tokenId,
                               std::vector<fpi::CtrlObjectMetaDataSync>& objToSync);


    /**
     * Ack from source SM when it receives the whole initial set of
     * objects.
     */
    Error startObjectRebalanceResp();

    /**
     * Handle rebalance delta set at destination from the source
     */
    Error recvRebalanceDeltaSet(fpi::CtrlObjectRebalanceDeltaSetPtr& deltaSet);

    /**
     * Ack from destination for rebalance delta set message
     */
    Error rebalanceDeltaSetResp();

    /**
     * Handles DLT close event. At this point IO should not arrive
     * with old DLT. Once we reply, we are done with token migration.
     * Assumes we are receiving DLT close event for the correct version,
     * caller should check this
     */
    Error handleDltClose();

  private:
    /// <source SM uuid, SM token> map
};

}  // namespace fds
#endif  // SOURCE_STOR_MGR_INCLUDE_TOKENMIGRATIONMGR_H_
