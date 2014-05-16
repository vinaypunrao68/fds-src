/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_DATA_MGR_INCLUDE_COMMITLOG_H_
#define SOURCE_DATA_MGR_INCLUDE_COMMITLOG_H_

#include <fds_error.h>
#include <fds_module.h>
#include <blob/BlobTypes.h>
#include <lib/Catalog.h>

namespace fds {

/**
 * Manages pending DM catalog updates and commits. Used
 * for 2-phase commit operations. This management
 * includes the on-disk log and the in-memory state.
 */
class DmCommitLog : public Module {
  private:
    fds_uint32_t temp;
    // TODO(Andrew): Make into log specific catalog
    Catalog::ptr ondiskLog;

  public:
    explicit DmCommitLog(const std::string &modName);
    ~DmCommitLog();

    typedef boost::shared_ptr<DmCommitLog> ptr;
    typedef boost::shared_ptr<const DmCommitLog> const_ptr;

    Error openTrans(BlobTxId::const_ptr txDesc);
    Error commitTrans(BlobTxId::const_ptr txDesc);

    int  mod_init(SysParams const *const param);
    void mod_startup();
    void mod_shutdown();
};

}  // namespace fds

#endif  // DATA_MGR_INCLUDE_COMMITLOG_H_
