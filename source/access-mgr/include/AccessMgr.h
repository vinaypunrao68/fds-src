/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_ACCESS_MGR_INCLUDE_ACCESSMGR_H_
#define SOURCE_ACCESS_MGR_INCLUDE_ACCESSMGR_H_

#include <string>
#include <fds_types.h>
#include <fds_module_provider.h>
#include <fds_volume.h>
#include <AmDataApi.h>
#include <OmConfigService.h>

namespace fds {

struct AsyncDataServer;
struct FdsnServer;
struct NbdConnector;

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
    virtual void mod_enable_service() override;
    void mod_shutdown() override;

    void run();
    /// Interface to directly register a volume. Only
    /// used for testing today.
    Error registerVolume(const VolumeDesc& volDesc);

    // Set AM in shutdown mode.
    void setShutDown();
    void stop();

    // Check whether AM is in shutdown mode.
    bool isShuttingDown();

    /// Unique ptr to the fdsn server that communicates with XDI
    std::unique_ptr<FdsnServer> fdsnServer;

    /// Unique ptr to the async server that communicates with XDI
    std::unique_ptr<AsyncDataServer> asyncServer;

    /// Shared ptr to AM's data API. It's public so that
    /// other components (e.g., unit tests, perf tests) can
    /// directly call it. It may be shared by this and the
    /// fdsn server.
    AmDataApi::shared_ptr dataApi;

  private:
    /// Raw pointer to an external dependency manager
    CommonModuleProviderIf *modProvider_;

    /// Unique instance ID of this AM
    fds_uint32_t instanceId;

    /// OM config service API
    OmConfigApi::shared_ptr omConfigApi;

    /// Block connector
    std::unique_ptr<NbdConnector> blkConnector;

    std::atomic_bool  shuttingDown;
};

extern AccessMgr::unique_ptr am;

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_ACCESSMGR_H_
