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
typedef std::function<void (fds_uint64_t executorId,
                            const Error& error)> DmMigrationExecutorDoneHandler;


class DmMigrationExecutor {
  public:
    explicit DmMigrationExecutor(DmIoReqHandler* DmReqHandle,
    							 NodeUuid& srcDmUuid,
								 const NodeUuid& mySvcUuid,
								 fpi::FDSP_VolumeDescType& vol,
								 DmMigrationExecutorDoneHandler handle);
    ~DmMigrationExecutor();

    /**
     * Executes the specified executor and runs the callback once done.
     */
    void execute();

    fpi::FDSP_VolumeDescType getVolDesc();

    typedef std::unique_ptr<DmMigrationExecutor> unique_ptr;
    typedef std::shared_ptr<DmMigrationExecutor> shared_ptr;

  private:
    DmIoReqHandler* DmReqHandler;
    /**
     * Local copies of needed information to do migrations.
     */
    NodeUuid _srcDmUuid;
    NodeUuid _mySvcUuid;
    fpi::FDSP_VolumeDescType _vol;

    /**
     * Callback to talk to DM Migration Manager
     */
    DmMigrationExecutorDoneHandler migrDoneHandler;

};  // DmMigrationExecutor

}  // namespace fds


#endif  // SOURCE_DATA_MGR_INCLUDE_DMMIGRATIONEXECUTOR_H_
