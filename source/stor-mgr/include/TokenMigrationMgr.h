/*
 * Copyright 2013-2014 Formation Data Systems, Inc.
 */

#ifndef SOURCE_STOR_MGR_INCLUDE_TOKENMIGRATIONMGR_H_
#define SOURCE_STOR_MGR_INCLUDE_TOKENMIGRATIONMGR_H_

#include <vector>
#include <SmIo.h>
#include <MigrationExecutor.h>
#include <MigrationClient.h>

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
    explicit SmTokenMigrationMgr(SmIoReqHandler *dataStore,
                                 const NodeUuid& myUuid);
    ~SmTokenMigrationMgr();

    typedef std::unique_ptr<SmTokenMigrationMgr> unique_ptr;
    /// source SM -> MigrationExecutor object map
    typedef std::unordered_map<NodeUuid, MigrationExecutor::unique_ptr, UuidHash> SrcSmExecutorMap;
    /// SM token id -> [ source SM -> MigrationExecutor ]
    typedef std::unordered_map<fds_token_id, SrcSmExecutorMap> MigrExecutorMap;
    /// executorId -> migrationClient
    typedef std::unordered_map<fds_uint64_t, MigrationClient::shared_ptr> MigrClientMap;
    /// SM token id -> executor ID map
    typedef std::unordered_map<fds_token_id, fds_uint64_t> SmTokExecutorIdMap;

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
                               fds_uint32_t bitsPerDltToken);

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
    fds_uint64_t getTargetDltVersion() const;

    /**
     * Sets start forwarding flag on migration client responsible
     * for given SM token
     */
    Error startForwarding(fds_token_id smTok);

    /**
     * Handles DLT close event. At this point IO should not arrive
     * with old DLT. Once we reply, we are done with token migration.
     * Assumes we are receiving DLT close event for the correct version,
     * caller should check this
     */
    Error handleDltClose();

  private:
    /**
     * Callback function from the metadata snapshot request for a particular SM token
     */
    void smTokenMetadataSnapshotCb(const Error& error,
                                   SmIoSnapshotObjectDB* snapRequest,
                                   leveldb::ReadOptions& options,
                                   leveldb::DB *db);

    /**
     * Callback for a migration executor that it finished migration
     */
    void migrationExecutorDoneCb(fds_uint64_t executorId,
                                 fds_token_id smToken,
                                 fds_bool_t isFirstRound,
                                 const Error& error);

    /// enqueues snapshot message to qos
    void startSmTokenMigration(fds_token_id smToken);

    // send msg to source SM to send second round of delta sets
    void startSecondRebalanceRound(fds_token_id smToken);

    /**
     * Returns executor ID for this destination SM based on localId
     * 0...31 bits: localId
     * 32..64 bits: half of this SM service UUID
     */
    fds_uint64_t getExecutorId(fds_uint32_t localId);

    /**
     * Stops migration and sends ack with error to OM
     */
    void abortMigration(const Error& error);

    /// state of migration manager
    std::atomic<MigrationState> migrState;
    /// target DLT version for which current migration is happening
    /// does not mean anything if mgr in IDLE state
    fds_uint64_t targetDltVersion;
    fds_uint32_t numBitsPerDltToken;

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
    NodeUuid localSMSvcUuid;

    /// callback to svc handler to ack back to OM for Start Migration
    OmStartMigrationCbType omStartMigrCb;

    /// SM token token that is currently in progress of migrating
    /// TODO(Anna) make it more general if we want to migrate several
    /// tokens at a time
    fds_token_id smTokenInProgress;

    /**
     * pointer to SmIoReqHandler so we can queue work to QoS queues
     * passed in constructor, does not own
     */
    SmIoReqHandler *smReqHandler;

    /**
     * Qos request to snapshot index db
     */
    SmIoSnapshotObjectDB snapshotRequest;

    /// SM token id -> [ source SM -> MigrationExecutor ]
    MigrExecutorMap migrExecutors;
    /// so far we don't need a lock for the above map, because the actions
    /// to update this map are serialized, and protected by mirgState

    /// executorId -> MigrationClient
    MigrClientMap migrClients;
    fds_mutex clientLock;
    /// to be able to get migrClient from SM token on src side
    SmTokExecutorIdMap smtokExecutorIdMap;

    /// maximum number of items in the delta set.
    fds_uint32_t maxDeltaSetSize;

    /// enable/disable token migration feature -- from platform.conf
    fds_bool_t enableMigrationFeature;
};

}  // namespace fds
#endif  // SOURCE_STOR_MGR_INCLUDE_TOKENMIGRATIONMGR_H_
