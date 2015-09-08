/*
 * Copyright 2013-2014 Formation Data Systems, Inc.
 */

#ifndef SOURCE_STOR_MGR_INCLUDE_MIGRATIONMGR_H_
#define SOURCE_STOR_MGR_INCLUDE_MIGRATIONMGR_H_

#include <vector>
#include <SmIo.h>
#include <MigrationExecutor.h>
#include <MigrationClient.h>
#include <fds_timer.h>

namespace fds {

/**
 * Callback for Migration Start Ack
 */
typedef std::function<void (const Error&)> OmStartMigrationCbType;
typedef std::function<void (fds_bool_t, fds_bool_t)> ResyncDoneOrPendingCb;

/*
 * Class responsible for migrating tokens between SMs
 * For each migration process, it creates migrationExecutors
 * and migrationSource objects that actually handle migration
 * on source and destination; MigrationMgr mainly forwards
 * service layer messages to appropriate migrationExecutor or
 * migrationSource objects.
 *
 * Current protocol:
 * On start migration message from OM, this migration manager (remember
 * that this is a destination SM for given DLT tokens) will:
 *    1) create a set of per-<SM token, source SM> migration executor objects
 *    2) Picks the first SM token (for now we are doing one SM token at a time),
 *       and submits qos request to create snapshot of SM token metadata
 *    3) MigrationMgr receives a callback when SM token metadata snapshot
 *       is done with actual snapshot.
 *    4) For each source SM that will migrate this SM token (one SM token may
 *       contain multiple DLT tokens, for which different SMs are resposible to
 *       migrate data to this SM), MigrationMgr calls appropriate migration
 *       executor's startObjectRebalance()
 *    5) TBD
 *    6) When set of migration executors responsible for current SM token finish
 *       migration, SmTokenMigration picks another SM token and repeats steps 2-5
 */
class MigrationMgr {
  public:
    explicit MigrationMgr(SmIoReqHandler *dataStore);
    ~MigrationMgr();

    typedef std::unique_ptr<MigrationMgr> unique_ptr;
    /// source SM -> MigrationExecutor object map
    typedef std::unordered_map<NodeUuid, MigrationExecutor::shared_ptr, UuidHash> SrcSmExecutorMap;
    /// SM token id -> [ source SM -> MigrationExecutor ]
    typedef std::unordered_map<fds_token_id, SrcSmExecutorMap> MigrExecutorMap;
    /// executorId -> migrationClient
    typedef std::unordered_map<fds_uint64_t, MigrationClient::shared_ptr> MigrClientMap;

    typedef std::set<fds_token_id> RetrySmTokenSet;
    /**
     * Matches OM state, just for sanity checks that we are getting
     * messages from OM/SMs in the correct state...
     */
    enum MigrationState {
        MIGR_IDLE,         // state in after DLT close and before Start migration
        MIGR_IN_PROGRESS,  // state between Start Migration and DLT close
        MIGR_ABORTED       // If migration aborted due to error before DLT close
    };

    enum MigrationType {
        MIGR_SM_ADD_NODE,
        MIGR_SM_RESYNC
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
     * Handles start migration message from OM.
     * Creates MigrationExecutor object for each SM token, source SM
     * which initiate token migration
     */
    Error startMigration(fpi::CtrlNotifySMStartMigrationPtr& migrationMsg,
                         OmStartMigrationCbType cb,
                         const NodeUuid& mySvcUuid,
                         fds_uint32_t bitsPerDltToken,
                         MigrationType migrType,
                         bool onePhaseMigration);

    /**
     * Start resync process for SM tokens. Find the list of
     * SMTokenMigrationGroups and call startMigration.
     */
     Error startResync(const fds::DLT *dlt,
                       const NodeUuid& mySvcUuid,
                       fds_uint32_t bitsPerDltToken,
                       ResyncDoneOrPendingCb cb);

    /**
     * Handles message from OM to abort migration
     * Aborts migration if given target DLT version matches version
     * that this migration manager is doing migration; if version does not
     * match, the method does not do anything, and returns ERR_INVALID_ARG
     * @param tgtDltVersion DLT version for which we are aborting migration
     * @return ERR_OK if success; ERR_INVALID_ARG if targetDltVersion does not
     * match DLT version for which migration is in progress
     */
    Error abortMigrationFromOM(fds_uint64_t tgtDltVersion);

    /**
     * Abort migration for a given SM token. Currently
     * this is used in case of errors seen during
     * migration on the destination side.
     */
    Error abortMigrationForSMToken(fds_token_id &smToken, const Error &err);

    /**
     * Callback notifying Migration Manager that Executor has acknowledged
     * abort migration request from the Manager and is ready to be aborted.
     */
    void abortMigrationCb(fds_uint64_t& executorId, fds_token_id& smToken);

    /**
     * Set abort pending for all Migration Executors. This is called in
     * response to CtrlNotifyAbortMigration message received from OM.
     */
    void setAbortPendingForExecutors();

    /**
     * Start migration for the next executor or if none found
     * move on to the next phase.
     */
    void startNextSMTokenMigration(fds_token_id &smToken, bool isFirstRound);

    /**
     * Handle a timeout error from executor or client.
     */
    void timeoutAbortMigration();

    /**
     * Handle start object rebalance from destination SM
     */
    Error startObjectRebalance(fpi::CtrlObjectRebalanceFilterSetPtr& rebalSetMsg,
                               const fpi::SvcUuid &executorSmUuid,
                               const NodeUuid& mySvcUuid,
                               fds_uint32_t bitsPerDltToken,
                               const DLT* dlt);

    /**
     * Ack from source SM when it receives the whole filter set of
     * objects.
     */
    Error startObjectRebalanceResp();

    /**
     * Handle msg from destination SM to send data/metadata changes since the first
     * delta set.
     */
    Error startSecondObjectRebalance(fpi::CtrlGetSecondRebalanceDeltaSetPtr& msg,
                                     const fpi::SvcUuid &executorSmUuid);

    /**
     * Handle rebalance delta set at destination from the source
     */
    Error recvRebalanceDeltaSet(fpi::CtrlObjectRebalanceDeltaSetPtr& deltaSet);

    /**
     * Ack from destination for rebalance delta set message
     */
    Error rebalanceDeltaSetResp();

    /**
     * Handle message from destination SM to finish token resync.
     * Migration client corresponding to given executorId will stop forwarding IO
     * and migration manager will remove the corresponding migration client
     */
    Error finishClientResync(fds_uint64_t executorId);

    /**
     * Forwards object to destination SM if needed
     * Forwarding will happen if the following conditions are true:
     *    - migration is in progress
     *    - reqDltVersion does not match targetDltVersion of this
     *      migration
     *    - Forwarding flag is set for DLT token this object ID belongs to;
     *      forwarding flag is set on migration Client when it receives
     *      message to start the second migration round (before doing second
     *      levelDB snapshot)
     *
     * @param objId object ID of the request
     * @param reqDltVersion dlt version passed with this request
     */
    fds_bool_t forwardReqIfNeeded(const ObjectID& objId,
                                  fds_uint64_t reqDltVersion,
                                  FDS_IOType* req);
    fds_bool_t forwardAddObjRefIfNeeded(FDS_IOType* req);

    fds_uint64_t getTargetDltVersion() const;

    /**
     * Sets start forwarding flag on migration client responsible
     * for migrating tokens to given executor
     * @param executorId executor ID that uniquely identifies destination
     * SM and DLT tokens that are being migrated to destination SM
     * @param smTok SM token id, used for sanity check
     */
    void startForwarding(fds_uint64_t executorId, fds_token_id smTok);

    /**
     * Handles DLT close event. At this point IO should not arrive
     * with old DLT. Once we reply, we are done with token migration.
     * Assumes we are receiving DLT close event for the correct version,
     * caller should check this
     */
    Error handleDltClose(const DLT* dlt,
                         const NodeUuid& mySvcUuid);

    fds_bool_t isDltTokenReady(const ObjectID& objId);

    /**
     * Returns true if all DLT tokens for which is SM is responsible for
     * are in 'ready' state (resync successed); otherwise returns false
     */
    fds_bool_t primaryTokensReady(const fds::DLT *dlt,
                                  const NodeUuid& mySvcUuid);

    /**
     * Reset all the dlt tokens assigned to this SM.
     */
    void resetDltTokensStates(fds_uint32_t& bitsPerDltToken);

    void changeDltTokensState(const std::set<fds_token_id>& dltTokens,
                              const bool& state);

    /**
     * If SM lost some tokens, just mark them unavailable.
     */
    void markUnownedTokensUnavailable(const std::set<fds_token_id>& tokSet);

    bool dltTokenStatesEmpty();

    /**
     * If migration not in progress and DLT tokens active/not active
     * states are not assigned, activate DLT tokens (this SM did not need
     * to resync or get data from other SMs).
     * This method must be called only when this SM is a part of DLT
     */
    void notifyDltUpdate(const fds::DLT *dlt,
                         fds_uint32_t bitsPerDltToken,
                         const NodeUuid& mySvcUuid);

    /**
     * Coalesce all migration executor.
     */
    void coalesceExecutors();
    void coalesceExecutorsNoLock();

    /**
     * Coalesce all migration client.
     */
    void coalesceClients();

    /**
     * Unique restart id that will be used to restart migration
     * with new source SMs.
     */
     fds_uint32_t getUniqueRestartId()
     { return std::atomic_fetch_add(&uniqRestartId, (fds_uint32_t)1); }

    /**
     * Check if all the Migration Executors for a given sm token
     * are is error state.
     */
     bool allExecutorsInErrorState(const fds_token_id &sm_token);

  private:

    /**
     * Create Migration executor for token migration.
     */
   MigrationExecutor::unique_ptr createMigrationExecutor(NodeUuid& srcSmUuid,
                                                         const NodeUuid& mySvcUuid,
                                                         fds_uint32_t bitsPerDltToken,
                                                         fds_token_id& smTok,
                                                         fds_uint64_t& targetDltVersion,
                                                         MigrationType& migrationType,
                                                         bool onePhaseMigration,
                                                         fds_uint32_t uniqueId = 0,
                                                         fds_uint16_t instanceNum = 1);
    /**
     * Callback function from the metadata snapshot request for a particular SM token
     */
    void smTokenMetadataSnapshotCb(const Error& error,
                                   SmIoSnapshotObjectDB* snapRequest,
                                   leveldb::ReadOptions& options,
                                   std::shared_ptr<leveldb::DB> db, bool retry,
                                   fds_uint32_t uniqueId);

    /**
     * Callback for a migration executor that it finished migration
     *
     * @param[in] round is the round that is finished:
     *            0 -- one DLT token finished during resync, but the whole
     *            executor haven't finished yet
     *            1 -- first round of migration finished for the executor
     *            2 -- second round of migration finished for the executor
     */
    void migrationExecutorDoneCb(fds_uint64_t executorId,
                                 fds_token_id smToken,
                                 const std::set<fds_token_id>& dltTokens,
                                 fds_uint32_t round,
                                 const Error& error);
    /**
     * This callback basically inserts the dlt token to the
     * retry migration set.
     */
    void dltTokenMigrationFailedCb(fds_token_id &smToken);

    void retryTokenMigrForFailedDltTokens();

    void removeTokensFromRetrySet(std::vector<fds_token_id>& tokens);

    /// enqueues snapshot message to qos
    void startSmTokenMigration(fds_token_id smToken, fds_uint32_t uid=0);

    // send msg to source SM to send second round of delta sets
    void startSecondRebalanceRound(fds_token_id smToken);

    /**
     * Returns executor ID for this destination SM based on localId
     * 0...31 bits: localId
     * 32..64 bits: half of smSvcUuid  service UUID
     */
    fds_uint64_t getExecutorId(fds_uint32_t localId,
                               const NodeUuid& smSvcUuid) const;

    /**
     * If this is migration due to DLT change, decline to be a source
     * if this SM is already a destination for the given dltToken;
     * If this is resync on restart, decline to be a source if this SM is
     * already a destination for the given SM token AND it has lower
     * responsibility for this DLT token
     */
    fds_bool_t acceptSourceResponsibility(fds_token_id dltToken,
                                          fds_bool_t resyncOnRestart,
                                          const fpi::SvcUuid &executorSmUuid,
                                          const NodeUuid& mySvcUuid,
                                          const DLT* dlt);


    /**
     * This method is called when executor failed to sync tokens on the
     * first or second round of migration/resync. For some set of errors,
     * new sources will be selected for a given set of dltTokens and token
     * sync/migration will restart with a new set of source SMs for this tokens.
     * On some errors (e.g., if failure happened on the destination side) or
     * if we tried with all source SMs, the sync will fail for the given set
     * of DLT tokens.
     * @return true if at least one retry started, otherwise return false
     */
    void retryWithNewSMs(fds_uint64_t executorId,
                         fds_token_id smToken,
                         const std::set<fds_token_id>& dltTokens,
                         fds_uint32_t round,
                         const Error& error);

    /**
     * Check if a resync is pending and start the resync if required.
     * Calls back StorMgr if this is resync on restart
     */
    void reportMigrationCompleted(fds_bool_t isResyncOnRestart);

    /**
     * Stops migration and sends ack with error to OM
     */
    void abortMigration(const Error& error);

    /**
     * Try aborting migration. This method will check if
     * any smToken migration is in progress. If not, then
     * abort the migration. Otherwise wait till all migration
     * executors for smTokens in progress reply back
     * acknowledging migration abort.
     */
    void tryAbortingMigration();

    /**
     * Set a given list of DLT tokens to available
     * for data operations.
     */
    template<typename T>
    void changeDltTokensAvailability(const T& tokens, bool availability);

    /**
     * If all executors and clients are done, moves migration to IDLE state
     * and resets the state
     * @param checkedExecutorsDone true if executors already checked and they are done
     *        so only check clients, and if they are also done, move to IDLE state
     */
    void checkResyncDoneAndCleanup(fds_bool_t checkedExecutorsDone);

    /// state of migration manager
    std::atomic<MigrationState> migrState;
    /// target DLT version for which current migration is happening
    /// does not mean anything if mgr in IDLE state
    fds_uint64_t targetDltVersion;
    fds_uint32_t numBitsPerDltToken;
    Error abortError;

    /**
     * Indexes this vector is a DLT token, and boolean value is true if
     * DLT token is available, and false if DLT token is unavailable.
     * Initialization on SM startup:
     *   Case 1: New SM, no previous DLT, so that no migration is necessary.
     *           SM will receive DLT update from OM and set all DLT tokens that this
     *           SM owns to available.
     *   Case 2: New SM added to the domain where there is an existing DLT.
     *           SM will receive StartMigration message from OM. All DLT tokens will
     *           be initialized to unavailable.
     *   Case 3: SM restarts and it was part of DLT before the shutdown.
     *           MigrationMgr will be called to start resync. All DLT tokens will be
     *           initialized to unavailable.
     *   Case 4: In one node SM cluster, SM restarts and it was part of DLT before
     *           shutdown. Since this is the only SM in the domain, mark all DLT
     *           tokens to available.
     *   Case 5: In case of a disk failure, all the tokens residing on that disk
     *           will be marked unavailable and their state will change once resync
     *           of those tokens complete.
     *
     *  dltTokenStatesMutex is meant for mutual exclusion to dltTokenStates.
     */
    std::vector<fds_bool_t> dltTokenStates;
    fds_mutex dltTokenStatesMutex;

    /// next ID to assign to a migration executor
    /**
     * TODO(Anna) executor ID must be a more complex type that encodes
     * destination SM uuid and unique id local to destination SM
     * Otherwise, source SMs that are responsible for migrating same SM
     * tokens to multiple destinations may get same executor id from
     * multiple destination SMs.
     * For now we encode first 32 bits of destination SM + uint32 number
     * local to destination SM; we need 64bit + 64bit type for executor ID
     */
    std::atomic<fds_uint32_t> nextLocalExecutorId;

    /// max number of times we will retry to sync DLT token
    fds_uint8_t maxRetriesWithDifferentSources;

    /// callback to svc handler to ack back to OM for Start Migration
    OmStartMigrationCbType omStartMigrCb;

    /// set of SM tokens that are currently in progress of migrating
    std::unordered_set<fds_token_id> smTokenInProgress;
    /// mutex for smTokenInProgress
    fds_mutex smTokenInProgressMutex;

    /// thread-safe iterator over MigrExecutorMap
    /// provides - constructor
    ///          - fetch_and_increment_saturating
    ///          - set
    /// Needs to be here since it uses MigrExecutorMap
    struct NextExecutor {
        /// constructor
        explicit NextExecutor(const MigrExecutorMap& ex) : exMap(ex) {}
        /// return current iterator and increment atomically
        /// if at the end of the vector don't increment
        MigrExecutorMap::const_iterator fetch_and_increment_saturating() {
            FDSGUARD(m);
            auto tmp = it;
            if (it != exMap.cend())
                it++;
            return  tmp;
        }
        /// atomic setter
        void set(MigrExecutorMap::iterator i) {
            FDSGUARD(m);
            it = i;
        }
        const MigrExecutorMap& exMap;
        MigrExecutorMap::const_iterator it;
        fds_mutex m;
        NextExecutor() = delete;
    } nextExecutor;

    /// SM token token that is currently in the second round
    fds_token_id smTokenInProgressSecondRound;
    fds_bool_t resyncOnRestart {false};  // true if resyncing tokens without DLT change

    /// SM token for which retry token migration is going on.
    fds_token_id retrySmTokenInProgress;

    boost::shared_ptr<FdsTimerTask> retryTokenMigrationTask;

    /**
     * pointer to SmIoReqHandler so we can queue work to QoS queues
     * passed in constructor, does not own
     */
    SmIoReqHandler *smReqHandler;

    /**
     * Uuid of object Store Manager's for which this migration manager is created.
     * TODO(Gurpreet): Replace all the places in migration code where we pass mySvcUuid
     * as parameter and use objStoreMgrUuid instead.
     */
    NodeUuid objStoreMgrUuid;

    /**
     * Vector of requests (static). Equals to the number of tokens.
     * TODO(matteo): it seems I cannot create this dynamically and destroy it
     * later in ObjectStorMgr::snapshotTokenInternal. I suspect the snapshotRequest is
     * actually reused after it is popped out the QoS queue. Likely need more investigation
     */
    std::vector<std::unique_ptr<SmIoSnapshotObjectDB>> snapshotRequests;

    /**
     * Timer to detect if there is no activities on the Executors.
     */
    FdsTimerPtr migrationTimeoutTimer;

    /**
     * abort migration after this duration of inactivities.
     */
    uint32_t migrationTimeoutSec;

    /// SM token id -> [ source SM -> MigrationExecutor ]
    //
    MigrExecutorMap migrExecutors;
    fds_rwlock migrExecutorLock;

    /// executorId -> MigrationClient
    MigrClientMap migrClients;
    fds_rwlock clientLock;

    /// enable/disable token migration feature -- from platform.conf
    fds_bool_t enableMigrationFeature;
    /// number of parallel thread -- from platform.conf
    uint32_t parallelMigration;
    /// number of primary SMs
    fds_uint32_t numPrimaries;

    /**
     * SM tokens for which token migration of atleast 1 dlt token failed
     * because the source SM was not ready.
     */
     RetrySmTokenSet retryMigrSmTokenSet;

    /**
     * Pending resync
     */
    std::atomic<bool> isResyncPending = {false};
    ResyncDoneOrPendingCb cachedResyncDoneOrPendingCb = ResyncDoneOrPendingCb();

    /**
     * Source SMs which are marked as failed for some executors during migration.
     * We also keep uuid of this SM (destination) in the list with flag false, so that
     * we don't pick this SM as a source as well.
     * Protected by migrExecutorLock
     */
    std::map<NodeUuid, bool> failedSMsAsSource;

    // For synchronization between the timer thread and token migration thread.
     fds_mutex migrSmTokenLock;

    /**
     * FDS timer. Currently used for resending the CtrlObjectRebalanceFilerSet
     * requests to the source SM which failed the first time because source SM
     * was not ready.
     */
     FdsTimer mTimer;

    /**
     * Timer for handling abort migration gracefully.
     */
    FdsTimer abortTimer;

    /**
     * Try abortMigration task.
     */
    boost::shared_ptr<FdsTimerTask> tryAbortingMigrationTask;

    /**
     * Unique ID that will be used to identify what migrations to restart
     * in case of an election of new source SMs.
     */
     std::atomic<fds_uint32_t>  uniqRestartId;
};

typedef MigrationMgr::MigrationType SMMigrType;

}  // namespace fds
#endif  // SOURCE_STOR_MGR_INCLUDE_MIGRATIONMGR_H_
