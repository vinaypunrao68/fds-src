/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

#ifndef SOURCE_DATA_MGR_INCLUDE_DMMIGRATIONEXECUTOR_H_
#define SOURCE_DATA_MGR_INCLUDE_DMMIGRATIONEXECUTOR_H_

#include <FdsRandom.h>

namespace fds {

// Forward declaration.
class DmIoReqHandler;
class DataMgr;

/**
 * 	Simple callback to ensure that firing off the migration msg was ok
 */
typedef std::function<void (fds_volid_t volumeId,
                            const Error& error)> DmMigrationExecutorDoneCb;

class DmMigrationExecutor {
  public:
    explicit DmMigrationExecutor(DataMgr& _dataMgr,
    							 const NodeUuid& _srcDmUuid,
								 fpi::FDSP_VolumeDescType& _volDesc,
								 const fds_bool_t& _autoIncrement,
								 DmMigrationExecutorDoneCb _callback);
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
    Error processInitialBlobFilterSet();

    /**
     * Step 2:
     * Process the incoming deltaObject set coming from the source DM.
     * This method will apply everything in the msg into the levelDB once it's
     * finished. It will also update the client's internal mapping to know
     * which blob has applied which sequence numbers, and whether or not the whole
     * set has been applied.
     */
    Error processIncomingDeltaSet(fpi::CtrlNotifyDeltaBlobsMsgPtr &msg);

    inline fds_bool_t shouldAutoExecuteNext()
    {
    	return autoIncrement;
    }

  private:
    /** Reference to the DataManager
     */
    DataMgr& dataMgr;

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
     * A helper struct to put everything related to handling Delta Blobs here.
     */
    struct deltaSetHelper {
    	typedef std::function<void (const Error &,
                                blob_version_t,
                                const BlobObjList::const_ptr&,
                                const MetaDataList::const_ptr&,
                                const fds_uint64_t)> CommitCb;
    	/**
     	 * Bitmap of the msgs that we have received for CtrlNotifyDeltaBlobsMsg
    	 */
    	std::unordered_map<fds_uint64_t, fds_bool_t> msgMap;

    	/**
    	* The msg number that is counted as the last message in the CtrlNotifyDeltaBlobsMsg
    	*/
    	fds_uint64_t lastMsg;

    	/**
    	* Mark this msg sequence ID as the one that has been received.
    	*/
    	void recordMsgSeqId(fpi::CtrlNotifyDeltaBlobsMsgPtr &msg);

    	/**
    	* Recall the number of messages that have been received.
    	*/
    	fds_uint64_t recallNumOfMsgsReceived();

    	/**
    	 * Returns true if the complete Blob set has been received
    	 */
    	fds_bool_t blobSetIsComplete();

    	/**
    	 * Number generator for transactionID
    	 */
    	std::unique_ptr<RandNumGenerator> randNumGen;
    };
    deltaSetHelper dsHelper;

};  // DmMigrationExecutor

}  // namespace fds


#endif  // SOURCE_DATA_MGR_INCLUDE_DMMIGRATIONEXECUTOR_H_
