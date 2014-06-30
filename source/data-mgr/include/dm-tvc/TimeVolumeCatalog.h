/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_DATA_MGR_INCLUDE_DM_TVC_TIMEVOLUMECATALOG_H_
#define SOURCE_DATA_MGR_INCLUDE_DM_TVC_TIMEVOLUMECATALOG_H_

#include <string>
#include <fds_error.h>
#include <fds_module.h>
#include <blob/BlobTypes.h>
#include <lib/Catalog.h>
#include <util/Log.h>

namespace fds {

/**
 * A persistent history log of volume events. The log
 * allows for events to be updated and queried.
 */
class DmTimeVolCatalog : public Module, boost::noncopyable {
  private:
    // TODO(Andrew): Make into log specific catalog
    /// Persistent log of events.
    /// Updates to the log are atomic.
    Catalog::ptr ondiskLog;

  public:
    explicit DmTimeVolCatalog(const std::string &modName);
    ~DmTimeVolCatalog();

    typedef boost::shared_ptr<DmTimeVolCatalog> ptr;
    typedef boost::shared_ptr<const DmTimeVolCatalog> const_ptr;

    /**
     * Starts a new transaction for blob.
     * On return, if successful, the transaction is ensured
     * to recover following a crash.
     */
    Error startBlobTx(const std::string &blobName,
                      BlobTxId::const_ptr txDesc);

    int  mod_init(SysParams const *const param);
    void mod_startup();
    void mod_shutdown();
};

}  // namespace fds

#endif  // SOURCE_DATA_MGR_INCLUDE_DM_TVC_TIMEVOLUMECATALOG_H_
