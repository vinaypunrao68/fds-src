/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

#ifndef SOURCE_DATA_MGR_INCLUDE_DMMIGRATIONEXECUTOR_H_
#define SOURCE_DATA_MGR_INCLUDE_DMMIGRATIONEXECUTOR_H_

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
     * process initial filter set of blobs to be sent to the source DM.
     */
    Error processInitialBlobFilterSet();

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

};  // DmMigrationExecutor

}  // namespace fds


#endif  // SOURCE_DATA_MGR_INCLUDE_DMMIGRATIONEXECUTOR_H_
