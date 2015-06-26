/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

#ifndef SOURCE_DATA_MGR_INCLUDE_DMMIGRATIONEXECUTOR_H_
#define SOURCE_DATA_MGR_INCLUDE_DMMIGRATIONEXECUTOR_H_

namespace fds {

// Forward declaration.
class DmIoReqHandler;

/**
 * 	Simple callback to ensure that firing off the migration msg was ok
 */
typedef std::function<void (fds_volid_t volumeId,
                            const Error& error)> DmMigrationExecutorDoneCb;


class DmMigrationExecutor {
  public:
    explicit DmMigrationExecutor(DmIoReqHandler* DmReqHandle,
    							 const NodeUuid& srcDmUuid,
								 fpi::FDSP_VolumeDescType& volDesc,
								 DmMigrationExecutorDoneCb callback);
    ~DmMigrationExecutor();

    /**
     * Executes the specified executor and runs the callback once done.
     */
    Error startMigration();

    typedef std::unique_ptr<DmMigrationExecutor> unique_ptr;
    typedef std::shared_ptr<DmMigrationExecutor> shared_ptr;

  private:
    /** DataManager IO request handler */
    DmIoReqHandler* DmReqHandler;

    /** Uuid of source DM
     */
    NodeUuid srcDmSvcUuid;

    /**
     * Volume descriptor owned by this executor.
     */
    VolumeDesc volDesc;

    /**
     * Callback to talk to DM Migration Manager
     */
    DmMigrationExecutorDoneCb migrDoneCb;

};  // DmMigrationExecutor

}  // namespace fds


#endif  // SOURCE_DATA_MGR_INCLUDE_DMMIGRATIONEXECUTOR_H_
