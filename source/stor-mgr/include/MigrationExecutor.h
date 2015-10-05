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

class EPSvcRequest;

/**
 * Callback to notify that migration executor is done with migration
 * @param[in] round is the round that is finished:
 *            0 -- one DLT token finished during resync, but the whole
 *            executor haven't finished yet
 *            1 -- first round of migration finished for the executor
 *            2 -- second round of migration finished for the executor
 */
typedef std::function<void (fds_uint64_t executorId,
                            fds_token_id smToken,
                            const std::set<fds_token_id>& dltTokens,
                            fds_uint32_t round,
                            const Error& error)> MigrationExecutorDoneHandler;

typedef std::function<void (fds_token_id &dltToken)> MigrationDltFailedCb;

typedef std::function<void(fds_uint64_t,
                           fds_uint32_t,
                           const std::set<fds_uint32_t>&,
                           fds_uint32_t,
                           const fds::Error&)> TimeoutCb;

typedef std::function<void(fds_uint64_t&,
                           fds_token_id&)> MigrationAbortCb;

class MigrationExecutor {
  public:
    MigrationExecutor(SmIoReqHandler *_dataStore,
                      fds_uint32_t bitsPerToken,
                      const NodeUuid& srcSmId,
                      fds_token_id smTokId,
                      fds_uint64_t id,
                      fds_uint64_t targetDltVer,
                      fds_uint32_t migrationType,
                      bool onePhaseMigration,
                      MigrationDltFailedCb failedRetryHandler,
                      MigrationExecutorDoneHandler doneHandler,
                      FdsTimerPtr &timeoutTimer,
                      uint32_t timoutDuration,
                      TimeoutCb timeoutCb,
                      MigrationAbortCb abortMigrationCb,
                      fds_uint32_t uniqId = 0,
                      fds_uint16_t instanceNum = 1);
    ~MigrationExecutor();

    typedef std::unique_ptr<MigrationExecutor> unique_ptr;
    typedef std::shared_ptr<MigrationExecutor> shared_ptr;

    enum MigrationExecutorState {
        ME_INIT,
        ME_FIRST_PHASE_REBALANCE_START,
        ME_FIRST_PHASE_APPLYING_DELTA,
        ME_SECOND_PHASE_REBALANCE_START,
        ME_SECOND_PHASE_APPLYING_DELTA,
        ME_DONE,
        ME_DONE_WITH_ERROR,
        ME_ERROR
    };

    const fds_uint32_t INVALID_ROUND = 3;

    inline fds_uint64_t getId() const {
        return executorId;
    }
    inline MigrationExecutorState getState() const {
        return std::atomic_load(&state);
    }
    inline fds_uint16_t getInstanceNum() const {
        return instanceNum;
    }
    inline fds_uint32_t getUniqueId() const {
        return uniqueId;
    }
    inline std::set<fds_token_id> getTokens() const {
        return dltTokens;
    }
    inline fds_bool_t isRoundDone(fds_bool_t isFirstRound) const {
        MigrationExecutorState curState = std::atomic_load(&state);
        if (curState == ME_DONE_WITH_ERROR ||
            curState == ME_ERROR) {
            return true;
        }
        if (isFirstRound) {
            return (curState == ME_SECOND_PHASE_REBALANCE_START);
        }
        return (curState == ME_DONE);
    }
    inline fds_bool_t isDone() const {
        MigrationExecutorState curState = std::atomic_load(&state);
        return ((curState == ME_DONE) || (curState == ME_DONE_WITH_ERROR));
    }
    inline void setDoneWithError() {
        MigrationExecutorState newState = ME_DONE_WITH_ERROR;
        std::atomic_store(&state, newState);
    }

    inline bool inErrorState() {
        MigrationExecutorState curState = std::atomic_load(&state);
        if (curState == ME_ERROR || curState == ME_DONE_WITH_ERROR) {
            return true;
        } else {
            return false;
        }
    }

    inline bool filterRespAndStateMismatch() {
        MigrationExecutorState curState = std::atomic_load(&state);
        if (onePhaseMigration) {
            return (curState != ME_SECOND_PHASE_APPLYING_DELTA);
        } else {
            return (curState != ME_FIRST_PHASE_APPLYING_DELTA);
        }
    }

    inline fds_uint32_t migrationRound() {
        MigrationExecutorState curState = std::atomic_load(&state);
        switch (curState) {
            case ME_FIRST_PHASE_REBALANCE_START:
            case ME_FIRST_PHASE_APPLYING_DELTA:
                return 1;
            case ME_SECOND_PHASE_REBALANCE_START:
            case ME_SECOND_PHASE_APPLYING_DELTA:
                return 2;
            default:
                return INVALID_ROUND;
        }
    }

    inline void setAbortPending() {
        abortPending = true;
    }

    inline bool isAbortPending() {
        return abortPending;
    }

    /**
     * Adds DLT token to the list of DLT tokens for which this
     * MigrationExecutor is responsible for
     * Can only be called before startObjectRebalance
     */
    void addDltToken(fds_token_id dltTok);
    fds_bool_t responsibleForDltToken(fds_token_id dltTok) const;

    /**
     * Start the object rebalance.  The rebalance inintiated by the
     * destination SM.
     */
    Error startObjectRebalance(leveldb::ReadOptions& options,
                               std::shared_ptr<leveldb::DB> db);

    Error startSecondObjectRebalanceRound();

    /**
     * Start object rebalance for sm tokens whose token migration
     * failed in the previous try with error that source SM was
     * not ready to become source.
     */
    Error startObjectRebalanceAgain(leveldb::ReadOptions& options,
                                    std::shared_ptr<leveldb::DB> db);
    /**
     * Handles message from Source SM to apply delta set to this SM
     */
    Error applyRebalanceDeltaSet(fpi::CtrlObjectRebalanceDeltaSetPtr& deltaSet);

    /**
     * Wait for all pending Executor requests to complete.
     */
    void waitForIOReqsCompletion(fds_token_id tok, NodeUuid nodeUuid);

    /**
     * Abort migration for this executor.
     */
    void abortMigration(const Error &err);

    /**
     * Erase the dlt tokens set for which migration was supposed
     * to be retried from the same source SM.
     */
    void clearRetryDltTokenSet();

  private:
    /**
     * Callback when apply delta set QoS message execution is completed
     */
    void objDeltaAppliedCb(const Error& error,
                           SmIoApplyObjRebalDeltaSet* req);

    /**
     * Called to finish up (abort with error or complete) first round
     * or second round of migration; finishing second round finishes
     * migration; error aborts migration
     */
    void handleMigrationRoundDone(const Error& error);

    /// callback from SL on rebalance filter set msg
    void objectRebalanceFilterSetResp(fds_token_id dltToken,
                                      uint64_t seqId,
                                      EPSvcRequest* req,
                                      const Error& error,
                                      boost::shared_ptr<std::string> payload);

    /// callback from SL on second rebalance delta set msg response
    void getSecondRebalanceDeltaResp(EPSvcRequest* req,
                                     const Error& error,
                                     boost::shared_ptr<std::string> payload);


    // send finish resync msg to source SM for corresponding client
    void sendFinishResyncToClient();

    // callback from SL on response for finish client resync message
    void finishResyncResp(EPSvcRequest* req,
                          const Error& error,
                          boost::shared_ptr<std::string> payload);

    /// Id of this executor, used for communicating with source SM
    fds_uint64_t executorId;

    /**
     * For a given SM token, the instance number of executor
     * created to migrate token data.
     */
    fds_uint16_t instanceNum;

    /// state of this migration executor
    std::atomic<MigrationExecutorState> state;

    /// callback to notify that migration finished
    MigrationExecutorDoneHandler migrDoneHandler;

    /// callback to update failed dlt token migration set
    MigrationDltFailedCb migrFailedRetryHandler;

    /**
     * Object data store handler.  Set during the initialization.
     */
    SmIoReqHandler *dataStore;

    /**
     * Source SM id
     */
    NodeUuid sourceSmUuid;

    /**
     * Unique id for the executor. This will be used to identify
     * migration executors for which to restart migration from a
     * new source SM.
     */
     fds_uint32_t uniqueId;

    /**
     * Migration Type: Is it a add new SM node or a SM resync?
     */
    fds_uint32_t migrationType;

    /**
     * SM Token to migration from the source SM node.
     */
    fds_token_id smTokenId;

    /**
     * Target DLT version for this executor
     */
    fds_uint64_t targetDltVersion;

    /**
     * Flag to signify if abort is pending for this migration executor.
     */
    bool abortPending;

    /**
     * To be called when abortPending is set by the Migration Manager indicating
     * that migration needs to be aborted.
     */
    MigrationAbortCb abortMigrationCb;

    /**
     * Set of DLT tokens that needs to be migrated from source SM
     * SM token contains one or more DLT tokens
     */
    std::set<fds_token_id> dltTokens;
    fds_uint32_t bitsPerDltToken;

    /**
     * Set of DLT tokens that failed to be migrated from source SM
     * because the source was not ready.
     * And the lock protecting the DLT tokens map
     */
    std::unordered_map<fds_token_id, uint64_t> retryDltTokens;
    std::unordered_map<fds_token_id, uint32_t> dltTokRetryCount;
    fds_mutex retryDltTokensLock;

    /**
     * Maintain messages from the source SM, so we don't lose it.  Each async message
     * from source SM has a unique sequence number.
     */
    MigrationDoubleSeqNum seqNumDeltaSet;

    /**
     * Callback for the seqNumDeltaSet timeout
     */
    void handleTimeout();

    /**
     * Callback for the timeout handler
     */

    TimeoutCb timeoutCb;

    /**
     * Keep track of outstanding IO requests.  This is used to prevent MigrationMgr from
     * deleting the Executor.
     *
     * TODO(Sean):  This doesn't eliminate 100% of the race condition that exist between the abort
     *              migration and the executor.  The effectiveness of the approach is
     *              determined how how early we can increment the "reference count."
     *              It is possible to call unordered map of excutors.clear() before
     *              reference count is incremented.
     *              Other way is to hold a mutex per executor, this causes bigger issues
     *              with recursive locking and easiness of deadlocking.  This approach
     *              was explored, but gave up due to "code litttering" all over the migration
     *              path with mutex ops.
     */
    MigrationTrackIOReqs trackIOReqs;

    /**
     * Will this migration have only one phase?
     */
    bool onePhaseMigration;
};

}  // namespace fds

#endif  // SOURCE_STOR_MGR_INCLUDE_MIGRATIONEXECUTOR_H_
