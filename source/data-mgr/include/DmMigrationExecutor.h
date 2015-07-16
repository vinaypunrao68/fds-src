/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

#ifndef SOURCE_DATA_MGR_INCLUDE_DMMIGRATIONEXECUTOR_H_
#define SOURCE_DATA_MGR_INCLUDE_DMMIGRATIONEXECUTOR_H_


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
     * Step 2.1:
     * Process the incoming delta blobs set coming from the source DM.
     */
    Error processDeltaBlobDescs(fpi::CtrlNotifyDeltaBlobDescMsgPtr &msg);

    /**
     * Step 2.2:
     * Process the incoming delta blob descriptors set coming from the source DM.
     */
    Error processDeltaBlobs(fpi::CtrlNotifyDeltaBlobsMsgPtr &msg);

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
     * Maintain messages from the source DM, so we don't lost it.
     * Each async message from the DM is unique.  We use double
     * sequence number to ensure that all delta messages are handled
     * and local IO through qos is complete.
     */
    MigrationDoubleSeqNum seqNumDeltaBlobDescs;
    MigrationDoubleSeqNum seqNumDeltaBlobs;

};  // DmMigrationExecutor

}  // namespace fds


#endif  // SOURCE_DATA_MGR_INCLUDE_DMMIGRATIONEXECUTOR_H_
