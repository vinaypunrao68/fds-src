/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_ACCESS_MGR_INCLUDE_ACCESSMGR_H_
#define SOURCE_ACCESS_MGR_INCLUDE_ACCESSMGR_H_

#include <string>
#include <fds_types.h>
#include <fds_module_provider.h>
#include <fds_volume.h>
#include <fdsn-server.h>
#include <AmAsyncService.h>
#include <AmDataApi.h>
#include <OmConfigService.h>
#include <NbdConnector.h>

namespace fds {

/**
 * AM module class.
 */
class AccessMgr : public Module, public boost::noncopyable {
  public:
    AccessMgr(const std::string &modName,
              CommonModuleProviderIf *modProvider);
    ~AccessMgr();
    typedef std::unique_ptr<AccessMgr> unique_ptr;

    /**
     * Module methods
     */
    int mod_init(SysParams const *const param) override;
    void mod_startup() override;
    void mod_lockstep_start_service() override;
    void mod_shutdown() override;

    void run();
    /// Interface to directly register a volume. Only
    /// used for testing today.
    Error registerVolume(const VolumeDesc& volDesc);

    // Set AM in shutdown mode.
    void setShutDown();

    // Check whether AM is in shutdown mode.
    bool isShuttingDown();

    /// Allow queues to drain and outstanding requests to complete.
    void prepareToShutDown();

    /// Shared ptr to AM's data API. It's public so that
    /// other components (e.g., unit tests, perf tests) can
    /// directly call it. It may be shared by this and the
    /// fdsn server.
    AmDataApi::shared_ptr dataApi;

  private:
    /// Raw pointer to an external dependency manager
    CommonModuleProviderIf *modProvider_;

    /// Unique ptr to the fdsn server that communicates with XDI
    FdsnServer::unique_ptr fdsnServer;

    /// Unique ptr to the async server that communicates with XDI
    AsyncDataServer::unique_ptr asyncServer;

    /// Unique instance ID of this AM
    fds_uint32_t instanceId;

    /// OM config service API
    OmConfigApi::shared_ptr omConfigApi;

    /// Block connector
    NbdConnector::shared_ptr blkConnector;

    std::atomic_bool  shuttingDown;
};

extern AccessMgr::unique_ptr am;

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_ACCESSMGR_H_
