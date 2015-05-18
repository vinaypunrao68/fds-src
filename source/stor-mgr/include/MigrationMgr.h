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

/*
 * Class responsible for migrating tokens between SMs
 * For each migration process, it creates migrationExecutors
 * and migrationSource objects that actually handle migration
 * on source and destination; SmTokenMigrationMgr mainly forwards
 * service layer messages to appropriate migrationExecutor or
 * migrationSource objects.
 *
 * Current protocol:
 * On start migration message from OM, this migration manager (remember
 * that this is a destination SM for given DLT tokens) will:
 *    1) create a set of per-<SM token, source SM> migration executor objects
 *    2) Picks the first SM token (for now we are doing one SM token at a time),
 *       and submits qos request to create snapshot of SM token metadata
 *    3) SmTokenMigrationMgr receives a callback when SM token metadata snapshot
 *       is done with actual snapshot.
 *    4) For each source SM that will migrate this SM token (one SM token may
 *       contain multiple DLT tokens, for which different SMs are resposible to
 *       migrate data to this SM), SmTokenMigrationMgr calls appropriate migration
 *       executor's startObjectRebalance()
 *    5) TBD
 *    6) When set of migration executors responsible for current SM token finish
 *       migration, SmTokenMigration picks another SM token and repeats steps 2-5
 */
class SmTokenMigrationMgr {
  public:
    explicit SmTokenMigrationMgr(SmIoReqHandler *dataStore);
    ~SmTokenMigrationMgr();

    typedef std::unique_ptr<SmTokenMigrationMgr> unique_ptr;
    /// source SM -> MigrationExecutor object map
    typedef std::unordered_map<NodeUuid, MigrationExecutor::unique_ptr, UuidHash> SrcSmExecutorMap;
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
                         bool forResync);

    /**
     * Start resync process for SM tokens. Find the list of
     * SMTokenMigrationGroups and call startMigration.
     */
     Error startResync(const fds::DLT *dlt,
                       const NodeUuid& mySvcUuid,
                       fds_uint32_t bitsPerDltToken);

    /**
     * Handles message from OM to abort migration
     */
    Error abortMigration();

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
    Error handleDltClose();

    inline fds_bool_t isDltTokenReady(const ObjectID& objId) const {
        if (dltTokenStates.size() > 0) {
            fds_verify(numBitsPerDltToken > 0);
            fds_token_id dltTokId = DLT::getToken(objId, numBitsPerDltToken);
            return dltTokenStates[dltTokId];
        }
        return false;
    }
    /**
     * If migration not in progress and DLT tokens active/not active
     * states are not assigned, activate DLT tokens (this SM did not need
     * to resync or get data from other SMs).
     */
    void notifyDltUpdate(fds_uint32_t bitsPerDltToken);

  private:
    /**
     * Callback function from the metadata snapshot request for a particular SM token
     */
    void smTokenMetadataSnapshotCb(const Error& error,
                                   SmIoSnapshotObjectDB* snapRequest,
                                   leveldb::ReadOptions& options,
                                   leveldb::DB *db, bool retry);

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

    /// enqueues snapshot message to qos
    void startSmTokenMigration(fds_token_id smToken);

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
     * Stops migration and sends ack with error to OM
     */
    void abortMigration(const Error& error);

    /**
     * If all executors and clients are done, moves migration to IDLE state
     * and resets the state
     */
    void checkResyncDoneAndCleanup();

    /// state of migration manager
    std::atomic<MigrationState> migrState;
    /// target DLT version for which current migration is happening
    /// does not mean anything if mgr in IDLE state
    fds_uint64_t targetDltVersion;
    fds_uint32_t numBitsPerDltToken;

    std::vector<fds_bool_t> dltTokenStates;

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

    /// callback to svc handler to ack back to OM for Start Migration
    OmStartMigrationCbType omStartMigrCb;

    /// set of SM tokens that are currently in progress of migrating
    std::unordered_set<fds_token_id> smTokenInProgress;
    /// mutex for smTokenInProgress
    fds_rwlock smTokenInProgressRWLock;

    /// thread-safe iterator over MigrExecutorMap
    /// provides - constructor
    ///          - fetch_and_increment_saturating
    ///          - set
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
    fds_bool_t resyncOnRestart;  // true if resyncing tokens without DLT change

    /// SM token for which retry token migration is going on.
    fds_token_id retrySmTokenInProgress;

    /**
     * pointer to SmIoReqHandler so we can queue work to QoS queues
     * passed in constructor, does not own
     */
    SmIoReqHandler *smReqHandler;

    /**
     * Map of the Qos request to snapshot index db
     * This is not protected. Assuming no aditions/removals to the 
     * map after initialization
     */

    // std::unordered_map<fds_token_id, SmIoSnapshotObjectDB> snapshotRequest;
    std::vector<SmIoSnapshotObjectDB> snapshotRequests;
    std::atomic<int> nextSnapshotRequest;
    SmIoSnapshotObjectDB* getSnapshotRequest() {
        return &snapshotRequests[nextSnapshotRequest++ % parallelMigration];    
    }
    
    /// SM token id -> [ source SM -> MigrationExecutor ]
    MigrExecutorMap migrExecutors;
    /// so far we don't need a lock for the above map, because the actions
    /// to update this map are serialized, and protected by mirgState

    /// executorId -> MigrationClient
    MigrClientMap migrClients;
    fds_rwlock clientLock;

    /// maximum number of items in the delta set.
    fds_uint32_t maxDeltaSetSize;

    /// enable/disable token migration feature -- from platform.conf
    fds_bool_t enableMigrationFeature;
    /// number of parallel thread -- from platform.conf
    int parallelMigration;

    /**
     * SM tokens for which token migration of atleast 1 dlt token failed
     * because the source SM was not ready.
     */
     RetrySmTokenSet retryMigrSmTokenSet;

     // For synchronization between the timer thread and token migration thread.
     fds_mutex migrSmTokenLock;

    /**
     * FDS timer. Currently used for resending the CtrlObjectRebalanceFilerSet
     * requests to the source SM which failed the first time because source SM
     * was not ready.
     */
     FdsTimer mTimer;
};

}  // namespace fds
#endif  // SOURCE_STOR_MGR_INCLUDE_MIGRATIONMGR_H_
