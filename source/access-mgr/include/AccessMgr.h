/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_ACCESS_MGR_INCLUDE_ACCESSMGR_H_
#define SOURCE_ACCESS_MGR_INCLUDE_ACCESSMGR_H_

#include <condition_variable>
#include <mutex>
#include <string>
#include <fds_types.h>
#include <fds_module_provider.h>
#include <fds_volume.h>
#include <fds_module.h>

namespace fds {

struct AmDataApi;
struct AmProcessor;
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
    void stop();

    /// Shared ptr to AM's data API. It's public so that
    /// other components (e.g., unit tests, perf tests) can
    /// directly call it. It may be shared by this and the
    /// fdsn server.
    boost::shared_ptr<AmDataApi> dataApi;

    std::shared_ptr<AmProcessor> getProcessor()
    { return amProcessor; }

  private:
    /// Raw pointer to an external dependency manager
    CommonModuleProviderIf *modProvider_;

    /// Block connector
    std::unique_ptr<NbdConnector> blkConnector;

    /// Unique ptr to the fdsn server that communicates with XDI
    std::unique_ptr<FdsnServer> fdsnServer;

    /// Unique ptr to the async server that communicates with XDI
    std::unique_ptr<AsyncDataServer> asyncServer;

    /// Processing Layer
    std::shared_ptr<AmProcessor> amProcessor;

    std::mutex stop_lock;
    std::condition_variable stop_signal;
    bool shutting_down;

    bool standalone_mode;
};

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_ACCESSMGR_H_
