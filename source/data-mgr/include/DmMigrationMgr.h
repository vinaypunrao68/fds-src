/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

#ifndef SOURCE_DATA_MGR_INCLUDE_DMMIGRATIONMGR_H_
#define SOURCE_DATA_MGR_INCLUDE_DMMIGRATIONMGR_H_

#include <fds_error.h>
#include <DmMigrationExecutor.h>
#include <DmMigrationClient.h>
#include <DmMigrationDest.h>
#include <DmMigrationSrc.h>
#include <condition_variable>
#include <MigrationUtility.h>
#include <counters.h>

namespace fds {

// Forward declaration
class DmIoReqHandler;

class DmMigrationMgr {
	using DmMigrationExecMap = std::map<std::pair<NodeUuid, fds_volid_t>, DmMigrationExecutor::shared_ptr>;
    using DmMigrationClientMap = std::map<std::pair<NodeUuid, fds_volid_t>, DmMigrationClient::shared_ptr>;
	using DmMigrationDestMap = std::map<std::pair<NodeUuid, fds_volid_t>, DmMigrationDest::shared_ptr>;
    using DmMigrationSrcMap = std::map<std::pair<NodeUuid, fds_volid_t>, DmMigrationSrc::shared_ptr>;

  public:
    explicit DmMigrationMgr(DmIoReqHandler* DmReqHandle, DataMgr& _dataMgr);
    ~DmMigrationMgr();

    void mod_shutdown();

    inline bool isMigrationEnabled() {
    	return enableMigrationFeature;
    }

    inline bool isMigrationAborted() {
    	return (migrationAborted.load(std::memory_order_relaxed));
    }

    /**
     * Migration State Machine related methods.
     */
    enum MigrationState {
        MIGR_IDLE,         // MigrationMgr is ready to receive work.
        MIGR_IN_PROGRESS,  // MigrationMgr has received a job and has dispatched work.
        MIGR_ABORTED       // If any of the jobs dispatched failed.
    };

    /**
     * The role of this current migration role
     */
    enum MigrationRole {
    	MIGR_EXECUTOR,
		MIGR_CLIENT
    };

    enum MigrationType {
        MIGR_DM_ADD_NODE, // If this migration is initiated by OM due to new DM node
        MIGR_DM_RESYNC	  // If this migration is peer-initiated between DMs
    };

    inline fds_bool_t isMigrationInProgress(MigrationRole role) const {
     	if (role == MIGR_EXECUTOR) {
     		return (atomic_load(&executorState) == MIGR_IN_PROGRESS);
    	} else if (role == MIGR_CLIENT) {
     		return (atomic_load(&clientState) == MIGR_IN_PROGRESS);
    	} else {
    		fds_assert(0);
    		return (false);
    	}
    }
    inline fds_bool_t isMigrationIdle(MigrationRole role) const {
     	if (role == MIGR_EXECUTOR) {
     		return (atomic_load(&executorState) == MIGR_IDLE);
    	} else if (role == MIGR_CLIENT) {
     		return (atomic_load(&clientState) == MIGR_IDLE);
    	} else {
    		fds_assert(0);
    		return (true);
    	}
    }
    inline fds_bool_t isMigrationAborted(MigrationRole role) const {
     	if (role == MIGR_EXECUTOR) {
     		return (atomic_load(&executorState) == MIGR_ABORTED);
    	} else if (role == MIGR_CLIENT) {
     		return (atomic_load(&clientState) == MIGR_ABORTED);
    	} else {
    		fds_assert(0);
    		return (true);
    	}
    }

    inline fds_uint64_t ongoingMigrationCnt(MigrationRole role) const {
    	if (role == MIGR_EXECUTOR) {
    		return (executorMap.size());
    	} else if (role == MIGR_CLIENT) {
    		return (clientMap.size());
    	} else {
    		return 0;
    	}
    }

    /**
     * Destination side DM:
     * Method to invoke migration operations requested by the OM.
     * This is the entry point and it'll decode the migration message. For each of the
     * volume that it needs to pull, it'll spawn off executors who are in charge of
     * contacting the correct source DM, which will then do handshaking between the
     * volume diffs, and then do the sync.
     * This method will spawn off an executor and return once all the executors have
     * been spawned successfully. In the case where more volume migrations are requestsed
     * than allowed parallely, then this will block.
     * Returns ERR_OK if the migrations specified in the migrationMsg has been
     * able to be dispatched for the executors.
     */
    Error startMigrationExecutor(DmRequest* dmRequest);

    /**
     * Source side DM:
     * Method to receive a request from a Destination DM.
     * Will spawn off a client to handle this specific request.
     * The reason why we're trying to emulate a non-blocking way of spawning off clients
     * is because we're planning ahead for the chance that we may allow 2 nodes to join
     * a cluster in one shot. If that happens, then it would be possible that a node may
     * act as a source node for multiple destination nodes. And it would be unwise to handle
     * only one client at a time.
     * TODO(Neil) - once we support 2 node additions, then we'll need to keep track of
     * multiple callback pointers, etc. For now, not doing it.
     */
    Error startMigrationClient(DmRequest* dmRequest);

    /**
     * Destination Side DM:
     * Handle deltaObject in Migration executor.
     */
    Error applyDeltaBlobs(DmIoMigrationDeltaBlobs* deltaBlobsReq);

    /**
     * Routes the DmIoMigrationDeltaBlobDesc request to the right executor
     */
    Error applyDeltaBlobDescs(DmIoMigrationDeltaBlobDesc* deltaBlobDescReq);

    /**
    * Routes DmIoFwdCat to right executor
    */
    Error handleForwardedCommits(DmIoFwdCat* fwdCatReq);

    /**
     * Destination DM:
     * Message with the in-flight transaction state (Commit Log) for a volume
     */
    Error applyTxState(DmIoMigrationTxState* txStateReq);

    /**
     * Public interface to check whether or not a I/O should be forwarded as part ofstion.
     * Params:
     * 1. volId - the volume in question.
     * 2. dmtVersion - dmtVersion of the commit to be sent.
     */
    fds_bool_t shouldForwardIO(fds_volid_t volId, fds_uint64_t dmtVersion);

    /**
     * When DMT Close is issued by the OM, DM should stop ALL I/O forwarding. This method goes
     * through the map of clients and turn off forwarding for each of them.
     */
    void stopAllClientForwarding();

    /**
     * Source side DM:
     * On any commit, the source DM will need to broadcast to other destination DMs
     * that are all receiving migraion regarding this volume ID.
     */
    Error forwardCatalogUpdate(fds_volid_t volId,
    						   DmIoCommitBlobTx *commitBlobReq,
    						   blob_version_t blob_version,
    						   const BlobObjList::const_ptr& blob_obj_list,
    						   const MetaDataList::const_ptr& meta_list);

    /**
     * Destination side DM:
     * In the case no forwards is sent, this will finish the migration
     */
    Error finishActiveMigration(NodeUuid destUuid, fds_volid_t volId, int64_t migrationId);


    /**
     * Used to clean up migration clients or executors
     */
    Error finishActiveMigration(MigrationRole role, int64_t migrationId);

    /**
     * Both DMs:
     * Abort and clean up current ongoing migration and reset state machine to IDLE
     * The Destination DM is in charge of telling the OM that DM Migration has failed.
     * If the Source DM has any error, it will first fail the migration, then tell the
     * Destination DM that an error has occurred. The Destination DM will then tell the OM.
     */
    void abortMigration();
    void abortMigrationReal();

    // Get timeout for messages between clients and executors
    inline uint32_t getTimeoutValue() {
    	return static_cast<uint32_t>(deltaBlobTimeout);
    }

    typedef std::unique_ptr<DmMigrationMgr> unique_ptr;
    typedef std::shared_ptr<DmMigrationMgr> shared_ptr;


    /**
     * DMT watermark is used to reject operations to a resyncing DM
     * until the DMT version is updated, which indicates the end of
     * forwarding of commits from another DM. This is to prevent
     * overwriting forwarded commits with commits directy from AM that
     * logically preceeded the forwarded writes.
     */

    // shouldFilterDmt() checks the wartermark and should be used by all writepath operations except forwarding
    bool shouldFilterDmt(fds_volid_t volId, fds_uint64_t dmt_version);

    /**
     * setDmtWatermark() is called by the executor before unblocking
     * the QoS queue. The watermark outlives the executor which may be
     * freed soon after.
     */
    void setDmtWatermark(fds_volid_t volId, fds_uint64_t dmt_version);

    /**
     * Dumps all migration counters to the log NORMAL
     */
    void dumpStats();

    /*
     * Check if any of the migrations have been idle and abort migration if any migration
     * has been idle.
     */
    void migrationIdleTimeoutCheck();

    /**
     * Get the timeout in seconds between msgs
     */
    inline uint32_t getIdleTimeout() {
        return idleTimeoutSecs;
    }

    /**
     * Version 2: Uses volume group coordinator for peer migration.
     * This is the hook to start a migration.
     */
    Error startMigration(NodeUuid& srcDmUuid,
                         fpi::FDSP_VolumeDescType &vol,
                         int64_t migrationId,
                         migrationCb doneCb);

    /**
     * Not to be called by anyone else but internally by handler.
     */
    Error startMigrationSource(DmRequest *dmRequest);

  private:
    DmIoReqHandler* DmReqHandler;
    fds_rwlock migrExecutorLock;
    fds_rwlock migrClientLock;
    std::atomic<MigrationState> clientState;
    std::atomic<MigrationState> executorState;

    DataMgr& dataManager;

    /** check if the feature is enabled or not.
     */
    bool enableMigrationFeature;

    /**
     * Migration aborted?
     */
    std::atomic<bool> migrationAborted;

    /**
     * If OM issues a re-sync or start migration and there is ongoing migrations,
     * then the following is used to ensure that migration is fully finished/aborted
     * before restarting a new one.
     */
    std::mutex migrationBatchMutex;

    // This cv is used for both abort and success
    std::condition_variable migrationCV;

    /**
     * Seconds to sleep prior to starting migration
     */
    unsigned delayStart;

    /**
     * check if resync on restart feature is enabled.
     */
    bool enableResyncFeature;

    /**
     * maximum number of blobs per delta set sent from source DM.
     */
    uint64_t maxNumBlobs;

    /**
     * maximum number of blob desc per delta set sent from source DM.
     */
    uint64_t maxNumBlobDesc;

    /**
     * timeout for delta blob set
     */
    uint32_t deltaBlobTimeout;

    /**
    * If migration executor is idle for longer than this value migration is aborted.
    */
    uint32_t idleTimeoutSecs;

    /**
     * DMT version undergoing migration
     */
    int64_t DMT_version;

    /**
     * Throttles the number of max concurrent migrations
     * Below are protected by migrExecutorLock.
     */
    fds_uint32_t maxConcurrency;
    fds_uint32_t firedMigrations;
    // Bookmark for last fired executor
    DmMigrationExecMap::iterator mit;

    /**
     * Destination side DM:
     * Create an executor instance. Does bookkeeping.
     * Returns ERR_OK if the executor instance was created successfully.
     * Uses the executorMap to store the created instances.
     */
    Error createMigrationExecutor(const NodeUuid& srcDmUuid,
							      fpi::FDSP_VolumeDescType &vol,
                                  int64_t migrationId,
							      MigrationType& migrationType,
								  const fds_bool_t& autoIncrement = false);

    /**
     * Destination side DM:
     * Gets an ptr to the migration executor. Used internally.
     */
    DmMigrationExecutor::shared_ptr getMigrationExecutor(std::pair<NodeUuid, fds_volid_t> uniqueId);

    /**
     * Source side DM:
     * Gets an ptr to the migration client. Used as part of forwarding, etc.
     */
    DmMigrationClient::shared_ptr getMigrationClient(std::pair<NodeUuid, fds_volid_t> uniqueId);

   /**
     * Destination side DM:
     * Map of ongoing migration executor instances index'ed by vol ID (uniqueKey)
     */
    DmMigrationExecMap executorMap;

    /**
     * Destination side DM:
     * Wrapper around calling OmStartMigrCb with a ERR_OK
     */
    void ackStaticMigrationComplete(const Error &status, int64_t migrationId);

	/*
     * Destination side DM:
     * Ack back to DM start migration from the Destination DM to OM.
     * This is called only when the migration completes or aborts.  The error
     * stuffed in the asynchdr determines if the migration completed successfully or not.
     */
    migrationCb OmStartMigrCb;

    /**
     * Destination side DM:
     * Callback for migrationExecutor. Not the callback from client.
     */
    void migrationExecutorDoneCb(NodeUuid srcNode,
                                 fds_volid_t volId,
                                 int64_t migrationId,
                                 const Error &result);


    /**
     * Source side DM:
     * Create a client instance. Does book keeping.
     * Returns ERR_OK if the client instance was created successfully.
     * Uses the clientMap to store the created instances.
     */
    Error createMigrationClient(NodeUuid& srcDmUuid,
    								const NodeUuid& mySvcUuid,
									fpi::CtrlNotifyInitialBlobFilterSetMsgPtr& rvmp,
									migrationCb cleanUp);

    /**
     * Source side DM:
     * Map of ongoing migration client instances index'ed by vol ID (uniqueKey)
     */
    DmMigrationClientMap clientMap;

    /**
     * Source side DM:
     * Callback for migrationClient.
     */
    void migrationClientDoneCb(fds_volid_t uniqueId,
                               int64_t migrationId,
                               const Error &result);

    void migrationClientDoneCbInternal(fds_volid_t uniqueId,
                                       int64_t migrationId,
                                       const Error &result,
                                       bool volumeGroupMode);



    /**
     * For debugging
     */
    void dumpDmIoMigrationDeltaBlobs(DmIoMigrationDeltaBlobs *deltaBlobReq);
    void dumpDmIoMigrationDeltaBlobs(fpi::CtrlNotifyDeltaBlobsMsgPtr &msg);
    void dumpDmIoMigrationDeltaBlobDesc(DmIoMigrationDeltaBlobDesc *deltaBlobReq);
    void dumpDmIoMigrationDeltaBlobDesc(fpi::CtrlNotifyDeltaBlobDescMsgPtr &msg);


    /**
     * The followings are used to ensure we do correct accounting for
     * the number of accesses to executors and clients.
     * So that we don't blow away things when there are still threads accessing them.
     */
    fds_rwlock executorAccessLock;
    fds_rwlock clientAccessLock;


    /**
     * Both DMs
     * If a migration batch is ongoing, this method waits for that batch to finish
     */
    void waitForMigrationBatchToFinish(MigrationRole role);

    /**
     * DMT watermark map - for rejecting messages from AM during resync that duplicate forwarded commits
     */
    std::unordered_map<fds_volid_t, fds_uint64_t> dmt_watermark;

    /**
     * the thread used to manage waiting for all piece of migration to complete before cleaning up when we abort
     */
    std::thread *abort_thread;

    // Clear clientmap and other related stats
    void clearClients();

    // Clear executorMap and other related stats
    void clearExecutors();

    void startMigrationStopWatch();
    void stopMigrationStopWatch();
    /**
     * Used for keeping time stats
     */
    std::atomic<bool> timerStarted;
    util::StopWatch migrationStopWatch;


    /**
     *  Version 2 of source and destinations
     */
    Error createMigrationSource(NodeUuid &destDmUuid,
                                const NodeUuid& MySvcUuid,
                                fpi::CtrlNotifyInitialBlobFilterSetMsgPtr rvmp,
                                migrationCb cleanUp);

    Error createMigrationDest(NodeUuid &srcDmUuid,
                              fpi::FDSP_VolumeDescType &vol,
                              int64_t migrationId,
                              migrationCb doneCb);

    /**
     * Source side DM:
     * Map of ongoing migration client instances index'ed by vol ID (uniqueKey)
     */
    DmMigrationSrcMap srcMap;

   /**
     * Destination side DM:
     * Map of ongoing migration executor instances index'ed by vol ID (uniqueKey)
     */
    DmMigrationDestMap destMap;

    /**
     * Locks for the above maps
     */
    fds_rwlock migrSrcLock;
    fds_rwlock migrDestLock;

    // Get a destination ptr
    DmMigrationDest::shared_ptr getMigrationDest(std::pair<NodeUuid, fds_volid_t> uniqueId);

    // Get a source ptr
    DmMigrationSrc::shared_ptr getMigrationSrc(std::pair<NodeUuid, fds_volid_t> uniqueId);

    // Simply prints out msgs for now
    void migrationSourceDoneCb(fds_volid_t uniqueId, int64_t migrationId, const Error &result);

};  // DmMigrationMgr
}  // namespace fds


#endif  // SOURCE_DATA_MGR_INCLUDE_DMMIGRATIONMGR_H_
