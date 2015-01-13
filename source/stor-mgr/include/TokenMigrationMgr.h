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

    /**
     * Matches OM state, just for sanity checks that we are getting
     * messages from OM/SMs in the correct state...
     */
    enum MigrationState {
        MIGR_IDLE,
        MIGR_IN_PROGRESS
    };

    /**
     * Handles start migration message from OM.
     * Creates MigrationExecutor object for each SM token, source SM
     * which initiate token migration
     */
    Error startMigration(fpi::CtrlNotifySMStartMigrationPtr& migrationMsg,
                         fds_uint32_t bitsPerDltToken);

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
    /**
     * Callback function from the metadata snapshot request for a particular SM token
     */
    void smTokenMetadataSnapshotCb(const Error& error,
                                   SmIoSnapshotObjectDB* snapRequest,
                                   leveldb::ReadOptions& options,
                                   leveldb::DB *db);

    /**
     * Stops migration and sends ack with error to OM
     */
    void abortMigration(const Error& error);

    /// state of migration manager
    std::atomic<MigrationState> migrState;
    /// next ID to assign to a migration executor
    std::atomic<fds_uint64_t> nextExecutorId;

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
};

}  // namespace fds
#endif  // SOURCE_STOR_MGR_INCLUDE_TOKENMIGRATIONMGR_H_
