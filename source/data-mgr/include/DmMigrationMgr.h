/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

#ifndef SOURCE_DATA_MGR_INCLUDE_DMMIGRATIONMGR_H_
#define SOURCE_DATA_MGR_INCLUDE_DMMIGRATIONMGR_H_

#include <fds_error.h>
#include <DmMigrationExecutor.h>
#include <condition_variable>

namespace fds {

// Forward declaration
class DmIoReqHandler;

// callback for start migration ack back to OM.
typedef std::function<void (const Error&)> OmStartMigrationCBType;

class DmMigrationMgr {

	using DmMigrationExecMap = std::unordered_map<fds_volid_t, DmMigrationExecutor::unique_ptr>;

  public:
    // explicit DmMigrationMgr(DmIoReqHandler* DmReqHandle);
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
        MIGR_DM_ADD_NODE, // If this migration is initiated by OM due to new DM node
        MIGR_DM_RESYNC	  // If this migration is peer-initiated between DMs
    };

    inline fds_bool_t isMigrationInProgress() const {
        MigrationState curState = atomic_load(&migrState);
        return (curState == MIGR_IN_PROGRESS);
    }
    inline fds_bool_t isMigrationIdle() const {
        MigrationState curState = atomic_load(&migrState);
        return (curState == MIGR_IDLE);
    }
    inline fds_bool_t isMigrationAborted() const {
        MigrationState curState = atomic_load(&migrState);
        return (curState == MIGR_ABORTED);
    }

    inline fds_uint64_t ongoingMigrationCnt() const {
    	return (executorMap.size());
    }

    /**
     * Method to invoke migration operations.
     * Returns ERR_OK if the migrations specified in the migrationMsg has been
     * able to be dispatched for the executors.
     */
    Error startMigration(fpi::CtrlNotifyDMStartMigrationMsgPtr &inMigrationMsg);

    typedef std::unique_ptr<DmMigrationMgr> unique_ptr;
    typedef std::shared_ptr<DmMigrationMgr> shared_ptr;

  protected:
  private:
    DmIoReqHandler* DmReqHandler;
    std::atomic<MigrationState> migrState;
    std::atomic<fds_bool_t> cleanUpInProgress;

    /**
     * Create an executor instance. Does bookkeeping.
     * Returns ERR_OK if the executor instance was created successfully.
     */
    Error createMigrationExecutor(const NodeUuid& srcDmUuid,
							      fpi::FDSP_VolumeDescType &vol,
							      MigrationType& migrationType);

    /**
     * Makes sure that the state machine is idle, and activate it.
     * Returns ERR_OK if that's the case, otherwise returns something else.
     */
    Error activateStateMachine();

    /**
     * Map of ongoing migration executor instances index'ed by vol ID (uniqueKey)
     */
	DmMigrationExecMap executorMap;
    fds_rwlock migrExecutorLock;

    // Ack back to DM start migration from the Destination DM to OM.
    OmStartMigrationCBType OmStartMigrCb;

    /**
     * Callback for migrationExecutor. Not the callback from client.
     */
    void migrationExecutorDoneCb(fds_volid_t volId, const Error &result);

};  // DmMigrationMgr

}  // namespace fds


#endif  // SOURCE_DATA_MGR_INCLUDE_DMMIGRATIONMGR_H_
