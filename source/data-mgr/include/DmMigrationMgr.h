/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

#ifndef SOURCE_DATA_MGR_INCLUDE_DMMIGRATIONMGR_H_
#define SOURCE_DATA_MGR_INCLUDE_DMMIGRATIONMGR_H_

#include <fds_error.h>

namespace fds {

// Forward declaration
class DmIoReqHandler;

// callback for start migration ack back to OM.
typedef std::function<void (const Error&)> OmStartMigrationCBType;

class DmMigrationMgr {
  public:
    explicit DmMigrationMgr(DmIoReqHandler* DmReqHandle);
    ~DmMigrationMgr();

    /**
     * Migration State Machine related methods.
     */
    enum MigrationState {
        MIGR_IDLE,         // MigrationMgr is ready to receive work.
        MIGR_IN_PROGRESS,  // MigrationMgr has received a job and has dispatched work.
        MIGR_ABORTED       // If any of the jobs dispatched failed.
    };

    enum MigrationType {
        MIGR_DM_ADD_NODE,
        MIGR_DM_RESYNC
    };

    inline fds_bool_t isMigrationInProgress() const {
        MigrationState curState = atomic_load(&migrState);
        return (curState == MIGR_IN_PROGRESS);
    }
    inline fds_bool_t isMigrationIdle() const {
        MigrationState curState = atomic_load(&migrState);
        return (curState == MIGR_IDLE);
    }

    /**
     * Methods to invoke migration operations
     */
    Error startMigration(fpi::CtrlNotifyDMStartMigrationMsgPtr &migrationMsg);

    typedef std::unique_ptr<DmMigrationMgr> unique_ptr;
    typedef std::shared_ptr<DmMigrationMgr> shared_ptr;

  private:
    DmIoReqHandler* DmReqHandler;
    std::atomic<MigrationState> migrState;


    // Ack back to DM start migration from the Destination DM to OM.
    OmStartMigrationCBType OmStartMigrCb;

};  // DmMigrationMgr

}  // namespace fds


#endif  // SOURCE_DATA_MGR_INCLUDE_DMMIGRATIONMGR_H_
