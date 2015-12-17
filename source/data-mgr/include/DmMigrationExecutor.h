/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

#ifndef SOURCE_DATA_MGR_INCLUDE_DMMIGRATIONEXECUTOR_H_
#define SOURCE_DATA_MGR_INCLUDE_DMMIGRATIONEXECUTOR_H_

#include <MigrationUtility.h>
#include <fds_timer.h>
#include <DmMigrationBase.h>
#include <dmhandler.h>
namespace fds {

// Forward declaration.
class DmIoReqHandler;
class DataMgr;

/**
 * 	Simple callback to ensure that firing off the migration msg was ok
 */
typedef std::function<void (NodeUuid srcNodeUuid,
							fds_volid_t volumeId,
                            const Error& error)> DmMigrationExecutorDoneCb;

class DmMigrationExecutor : public DmMigrationBase {
  public:
    explicit DmMigrationExecutor(DataMgr& _dataMgr,
    							 const NodeUuid& _srcDmUuid,
								 fpi::FDSP_VolumeDescType& _volDesc,
                                 int64_t migrationId,
								 const fds_bool_t& _autoIncrement,
								 DmMigrationExecutorDoneCb _callback,
                                 uint32_t _timeout,
                                 bool _volumeGroupMode);
    ~DmMigrationExecutor();

    typedef std::unique_ptr<DmMigrationExecutor> unique_ptr;
    typedef std::shared_ptr<DmMigrationExecutor> shared_ptr;
    using blobObjListIter = std::vector<fpi::DMMigrationObjListDiff>::const_iterator;


    /**
     * Executes the specified executor and runs the callback once done.
     */
    Error startMigration();

    /**
     * Step 1:
     * process initial filter set of blobs to be sent to the source DM.
     */
    Error processInitialBlobFilterSet(bool volumeGroupMode = false);

    /**
     * Step 2.1:
     * Process the incoming delta blobs set coming from the source DM.
     */
    Error processDeltaBlobs(fpi::CtrlNotifyDeltaBlobsMsgPtr &msg);

    /**
     * Step 2.2:
     * Process the incoming delta blob descriptors set coming from the source DM.
     */
    Error processDeltaBlobDescs(fpi::CtrlNotifyDeltaBlobDescMsgPtr &msg, migrationCb cb);

    /**
     * Step 3:
     * As each list of blobs are applied to the levelDB, the commit is done asynchronously
     * and this is the callback from having the blobs committed. The callback will keep
     * track of whether or not the blobs have all been applied, and then synchronize with
     * the blobs descriptor application process and call the method to commit the blobs desc
     * if that's the case.
     */
    Error processIncomingDeltaSetCb();

    /**
    * @brief Processes forward commit messages.  If the static migration is in progress
    * these messages are buffered.  Otherwise they are sent to QOS controller immediatel
    *
    * @return
    */
    Error processForwardedCommits(DmIoFwdCat* req);

    /**
     * Apply queue blob descriptor.
     */
    Error applyQueuedBlobDescs();

    /**
     * Apply blob descriptor
     */
    Error applyBlobDesc(fpi::CtrlNotifyDeltaBlobDescMsgPtr& msg);

    fds_bool_t shouldAutoExecuteNext();

    /**
     * Destination DM:
     * Message with the in-flight transaction state (Commit Log) for a volume
     */
    Error processTxState(fpi::CtrlNotifyTxStateMsgPtr txStateMsg);

    /**
     * Finish the active migration - in case where we have NO forwards, this takes care
     * of the state machine change.
     */
    Error finishActiveMigration();

    /**
     * Overrides the base and routes to the mgr
     */
    void routeAbortMigration() override;

    /**
     * Called by MigrationMgr to clean up any mess that this executor has caused
     */
    void abortMigration();

    inline bool isMigrationComplete() {
    	return (migrationProgress == MIGRATION_COMPLETE);
    }
    /**
    * Returns true if migration has been idle
    */
    bool isMigrationIdle(const util::TimeStamp& curTsSec) const;

    inline util::TimeStamp getLastUpdateFromClientTsSec() const {
        return lastUpdateFromClientTsSec_;
    }

  protected:
    /** Uuid of source DM
     */
    NodeUuid srcDmSvcUuid;

    /** Cache the volume id for easy lookup.
     */
    fds_volid_t volumeUuid;

    /**
     * Volume descriptor owned by this executor.
     */
    VolumeDesc volDesc;

    /**
     * Callback to talk to DM Migration Manager
     */
    DmMigrationExecutorDoneCb migrDoneCb;

    /**
     * Means that the callback above should now call the next executor
     */
    fds_bool_t autoIncrement;

    /**
     * Timeout between each Blobs/BlobsDesc messages.
     */
    uint32_t 	timerInterval;
    FdsTimerPtr seqTimer;

    /**
     * Used for Handling seq numbers for processing Blobs.
     * Because we're taking advantage of commit's callback instead of executor's
     * own QoS callback, we can't use a MigrationDoubleSeqNum as easily.
     * Since we know that the Cb only gets called after a commitTx has been issued,
     * we can take adv and use this as a cheating way of just doing counting, and
     * to know when all the Cbs have finished before moving on to BlobsDescs.
     *
     * NOTE: Should be called holding the blobDescListMutex.
     */
    MigrationSeqNum deltaBlobsSeqNum;

    /*
     * Maintain messages from the source DM, so we don't lost it.
     * Each async message from the DM is unique.  We use double
     * sequence number to ensure that all delta messages are handled
     * and local IO through qos is complete.
     */
    MigrationSeqNum deltaBlobDescsSeqNum;

    void sequenceTimeoutHandler();

    /**
     * Called by each function whose completion could signal the end of static migration.
     * Tests that all the component operations have been applied, and if so triggers the actions for
     * the next phase exactly once. Safe against races between the terminations conditions being met
     * and the actual call to this function, i.e. safe to call even after the state machine has moved
     * beyond static migration.
     */
    void testStaticMigrationComplete();

    /**
     * Mutex for blob offset list and blob descriptor list coordination
     */
    fds_mutex blobDescListMutex;

    /**
     * List of blob descriptors that have been queued waiting for
     * blob offsets to be written out to disk.
     */
    std::vector<std::pair<fpi::CtrlNotifyDeltaBlobDescMsgPtr, migrationCb>> blobDescList;

    /* enum to track migration progress */
    enum {
        /* In this state forwared io is buffered util static migratio ops are applied
         * to leveldb
         */
        INIT,
        /* In this state forwared io is applied as it arrives from the wire.  Any active IO
         * assumed to be quiesced
         */
        STATICMIGRATION_IN_PROGRESS,
        /* In this state we shouldn't receive any migration related ops.  Client IO quiesce is
         * lifted
         */
        APPLYING_FORWARDS_IN_PROGRESS,
		/**
		 * This state means migration is completed and waiting to be cleaned up.
		 */
        MIGRATION_COMPLETE,
		/**
		 * This state is in case migration aborted
		 */
		MIGRATION_ABORTED
    } migrationProgress;
    dm::Handler                                     msgHandler;
    /* Queue to buffer forwarded messages */
    std::list<DmIoFwdCat*>                          forwardedMsgs;
    /* Lock to synchronize access to forwardedMsgs and migrationProgress */
    fds_mutex                                       progressLock;

    /* boolean set to true once the commit log state migration message has been received AND applied.
       access to this variable should be protected with the progressLock */
    fds_bool_t txStateIsMigrated;

    // DMT at the time migration began
    fds_uint64_t dmtVersion;
    
    /* Last timestamp of when we heard from the client */
    util::TimeStamp lastUpdateFromClientTsSec_;

    bool volumeGroupMode;
    friend class DmMigrationMgr;

};  // DmMigrationExecutor

}  // namespace fds


#endif  // SOURCE_DATA_MGR_INCLUDE_DMMIGRATIONEXECUTOR_H_
