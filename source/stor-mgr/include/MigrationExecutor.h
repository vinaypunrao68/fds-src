/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

#ifndef SOURCE_STOR_MGR_INCLUDE_MIGRATIONEXECUTOR_H_
#define SOURCE_STOR_MGR_INCLUDE_MIGRATIONEXECUTOR_H_

#include <memory>
#include <mutex>
#include <set>

#include <fds_types.h>
#include <SmIo.h>

#include <MigrationUtility.h>

namespace fds {

/**
 * Callback to notify that migration executor is done with migration
 */
typedef std::function<void (fds_uint64_t executorId,
                            fds_token_id smToken,
                            const Error& error)> MigrationExecutorDoneHandler;

class MigrationExecutor {
  public:
    MigrationExecutor(SmIoReqHandler *_dataStore,
                      fds_uint32_t bitsPerToken,
                      const NodeUuid& srcSmId,
                      fds_token_id smTokId,
                      fds_uint64_t id,
                      MigrationExecutorDoneHandler doneHandler);
    ~MigrationExecutor();

    typedef std::unique_ptr<MigrationExecutor> unique_ptr;

    enum MigrationExecutorState {
        ME_INIT,
        ME_REBALANCE_START,
        ME_APPLYING_DELTA,
        ME_DONE,
        ME_ERROR
    };

    inline fds_uint64_t getId() const {
        return executorId;
    }
    inline MigrationExecutorState getState() const {
        return std::atomic_load(&state);
    }
    inline fds_bool_t isDone() const {
        return (std::atomic_load(&state) == ME_DONE);
    }

    /**
     * Adds DLT token to the list of DLT tokens for which this
     * MigrationExecutor is responsible for
     * Can only be called before startObjectRebalance
     */
    void addDltToken(fds_token_id dltTok);

    /**
     * Start the object rebalance.  The rebalance inintiated by the
     * destination SM.
     */
    Error startObjectRebalance(leveldb::ReadOptions& options,
                               leveldb::DB *db);

    /**
     * Handles message from Source SM to apply delta set to this SM
     */
    Error applyRebalanceDeltaSet(fpi::CtrlObjectRebalanceDeltaSetPtr& deltaSet);

  private:
    /**
     * Callback when apply delta set QoS message execution is completed
     */
    void objDeltaAppliedCb(const Error& error,
                           SmIoApplyObjRebalDeltaSet* req);

    /// Called to finish up (abort with error or complete) migration
    void handleMigrationDone(const Error& error);

    /// Id of this executor, used for communicating with source SM
    fds_uint64_t executorId;

    /// state of this migration executor
    std::atomic<MigrationExecutorState> state;

    /// callback to notify that migration finished
    MigrationExecutorDoneHandler migrDoneHandler;

    /**
     * Object data store handler.  Set during the initialization.
     */
    SmIoReqHandler *dataStore;

    /**
     * Source SM id
     */
    NodeUuid sourceSmUuid;

    /**
     * SM Token to migration from the source SM node.
     */
    fds_token_id smTokenId;

    /**
     * Set of DLT tokens that needs to be migrated from source SM
     * SM token contains one or more DLT tokens
     */
    std::set<fds_token_id> dltTokens;
    fds_uint32_t bitsPerDltToken;

    /**
     * Maintain messages from the source SM, so we don't lose it.  Each async message
     * from source SM has a unique sequence number.
     */
    MigrationSeqNumReceiver seqNumDeltaSet;

    /// true if standalone (no rpc sent)
    fds_bool_t testMode;
};

}  // namespace fds

#endif  // SOURCE_STOR_MGR_INCLUDE_MIGRATIONEXECUTOR_H_
