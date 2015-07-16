/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

#ifndef SOURCE_DATA_MGR_INCLUDE_DMMIGRATIONEXECUTOR_H_
#define SOURCE_DATA_MGR_INCLUDE_DMMIGRATIONEXECUTOR_H_

#include <FdsRandom.h>
#include <MigrationUtility.h>

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

    /**
     * Step 3:
     * As each list of blobs are applied to the levelDB, the commit is done asynchronously
     * and this is the callback from having the blobs committed. The callback will keep
     * track of whether or not the blobs have all been applied, and then synchronize with
     * the blobs descriptor application process and call the method to commit the blobs desc
     * if that's the case.
     */
    Error processIncomingDeltaSetCb();

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
     * Used for generating a random TransactionID.
     */
    RandNumGenerator randNumGen;

    /**
     * Used for Handling seq numbers for processing Blobs
     */
    MigrationSeqNum deltaBlobSetHelper;

    /**
     * Because we're taking advantage of commit's callback instead of executor's
     * own QoS callback, we can't use a MigrationDoubleSeqNum as easily.
     * Since we know that the Cb only gets called after a commitTx has been issued,
     * we can take adv and use this as a cheating way of just doing counting, and
     * to know when all the Cbs have finished before moving on to BlobsDescs.
     */
    struct seqNumHelper {
    	fds_mutex 		mtx;
    	fds_uint64_t 	expectedCount;
    	fds_bool_t		expectedCountFinalized;
    	fds_uint64_t	actualCbCounted;
    };
    seqNumHelper deltaBlobSetCbHelper;

};  // DmMigrationExecutor

}  // namespace fds


#endif  // SOURCE_DATA_MGR_INCLUDE_DMMIGRATIONEXECUTOR_H_
