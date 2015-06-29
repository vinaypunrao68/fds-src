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
								 const fds_bool_t& _autoIncrement,
								 DmMigrationExecutorDoneHandler handle);
    ~DmMigrationExecutor();

    /**
     * Executes the specified executor and runs the callback once done.
     */
    void execute();

    // fpi::FDSP_VolumeDescType& getVolDesc();
    boost::shared_ptr<fpi::FDSP_VolumeDescType> getVolDesc();

    typedef std::unique_ptr<DmMigrationExecutor> unique_ptr;
    typedef std::shared_ptr<DmMigrationExecutor> shared_ptr;

    inline fds_bool_t shouldAutoExecuteNext()
    {
    	return autoIncrement;
    }

  private:
    DmIoReqHandler* DmReqHandler;
    /**
     * Local copies of needed information to do migrations.
     */
    boost::shared_ptr <NodeUuid> srcDmUuid;
    boost::shared_ptr <NodeUuid> mySvcUuid;
    boost::shared_ptr <fpi::FDSP_VolumeDescType> vol;

    /**
     * Callback to talk to DM Migration Manager
     */
    DmMigrationExecutorDoneHandler migrDoneHandler;

    /**
     * Means that the callback above should now call the next executor
     */
    fds_bool_t autoIncrement;

};  // DmMigrationExecutor

}  // namespace fds


#endif  // SOURCE_DATA_MGR_INCLUDE_DMMIGRATIONEXECUTOR_H_
