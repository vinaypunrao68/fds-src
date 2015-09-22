/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

#ifndef SOURCE_DATA_MGR_INCLUDE_DMMIGRATIONMGR_H_
#define SOURCE_DATA_MGR_INCLUDE_DMMIGRATIONMGR_H_

#include <fds_error.h>
#include <DmMigrationExecutor.h>
#include <DmMigrationClient.h>
#include <condition_variable>
#include <MigrationUtility.h>

namespace fds {

// Forward declaration
class DmIoReqHandler;

class DmMigrationMgr : public DmMigrationBase {
	using DmMigrationExecMap = std::unordered_map<fds_volid_t, DmMigrationExecutor::shared_ptr>;
    using DmMigrationClientMap = std::unordered_map<fds_volid_t, DmMigrationClient::shared_ptr>;

  public:
    explicit DmMigrationMgr(DmIoReqHandler* DmReqHandle, DataMgr& _dataMgr);
    ~DmMigrationMgr();

    inline bool isMigrationEnabled() {
    	return enableMigrationFeature;
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
    	MIGR_UNKNOWN,
    	MIGR_EXECUTOR,
		MIGR_CLIENT
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
     * Public interface to check whether or not a I/O should be forwarded as part of
     * active migration.
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
     * Sends the finishVolResync msg to show that there's no more forwarding.
     * We want it done ASAP because volume I/O is quiesced on the dest side.
     */
    Error sendFinishFwdMsg(fds_volid_t volId);

    Error forwardCatalogUpdate(fds_volid_t volId,
    						   DmIoCommitBlobTx *commitBlobReq,
    						   blob_version_t blob_version,
    						   const BlobObjList::const_ptr& blob_obj_list,
    						   const MetaDataList::const_ptr& meta_list);

    /**
     * Destination side DM:
     * In the case no forwards is sent, this will finish the migration
     */
    Error finishActiveMigration(fds_volid_t volId);
    Error finishActiveMigration();

    /**
     * Both DMs:
     * Abort and clean up current ongoing migration and reset state machine to IDLE
     * The Destination DM is in charge of telling the OM that DM Migration has failed.
     * If the Source DM has any error, it will first fail the migration, then tell the
     * Destination DM that an error has occurred. The Destination DM will then tell the OM.
     */
    void abortMigration();
    void abortMigrationReal();

    void asyncMsgPassed();
    void asyncMsgFailed();
    void asyncMsgIssued();

    // Get timeout for messages between clients and executors
    inline uint32_t getTimeoutValue() {
    	return static_cast<uint32_t>(deltaBlobTimeout);
    }

    typedef std::unique_ptr<DmMigrationMgr> unique_ptr;
    typedef std::shared_ptr<DmMigrationMgr> shared_ptr;

  protected:
  private:
    DmIoReqHandler* DmReqHandler;
    fds_rwlock migrExecutorLock;
    fds_rwlock migrClientLock;
    std::atomic<MigrationState> migrState;
    MigrationRole myRole;

    DataMgr& dataManager;

    /** check if the feature is enabled or not.
     */
    bool enableMigrationFeature;

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
							      MigrationType& migrationType,
								  const fds_bool_t& autoIncrement = false);

    /**
     * Destination side DM:
     * Gets an ptr to the migration executor. Used internally.
     */
    DmMigrationExecutor::shared_ptr getMigrationExecutor(fds_volid_t uniqueId);

    /**
     * Source side DM:
     * Gets an ptr to the migration client. Used as part of forwarding, etc.
     */
    DmMigrationClient::shared_ptr getMigrationClient(fds_volid_t uniqueId);

    /**
     * Destination side DM:
     * Makes sure that the state machine is idle, and activate it.
     * Returns ERR_OK if that's the case, otherwise returns something else.
     */
    Error activateStateMachine(MigrationRole role);

   /**
     * Destination side DM:
     * Map of ongoing migration executor instances index'ed by vol ID (uniqueKey)
     */
    DmMigrationExecMap executorMap;

    /**
     * Destination side DM:
     * Wrapper around calling OmStartMigrCb with a ERR_OK
     */
    void ackStaticMigrationComplete(const Error &status);

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
    void migrationExecutorDoneCb(fds_volid_t volId, const Error &result);


    /**
     * Source side DM:
     * Create a client instance. Does book keeping.
     * Returns ERR_OK if the client instance was created successfully.
     * Uses the clientMap to store the created instances.
     */
    Error createMigrationClient(NodeUuid& srcDmUuid,
    								const NodeUuid& mySvcUuid,
									fpi::CtrlNotifyInitialBlobFilterSetMsgPtr& rvmp);

    /**
     * Source side DM:
     * Map of ongoing migration client instances index'ed by vol ID (uniqueKey)
     */
    DmMigrationClientMap clientMap;

    /**
     * Source side DM:
     * Callback for migrationClient.
     */
    void migrationClientDoneCb(fds_volid_t uniqueId, const Error &result);

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
     * Scoped tracking - how it works:
     * Normally, the migrationMgr gets a read lock on the respective exec/client.
     * If the operation is synchronous, the call completes and the lock is release.
     * If the operation is async, then we need to increment the counter and decrement it
     * on the callback.
     */
    MigrationTrackIOReqs trackIOReqs;

};  // DmMigrationMgr

}  // namespace fds


#endif  // SOURCE_DATA_MGR_INCLUDE_DMMIGRATIONMGR_H_
