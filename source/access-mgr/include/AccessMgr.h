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
    void mod_enable_service() override;
    void mod_disable_service() override;
    void mod_shutdown() override;

    void run();

    void initilizeConnectors();

    std::shared_ptr<AmProcessor> getProcessor()
    { return amProcessor; }

  private:
    /// Raw pointer to an external dependency manager
    CommonModuleProviderIf *modProvider_;

    /// Unique ptr to the async server that communicates with XDI
    std::unique_ptr<AsyncDataServer> asyncServer;

    /// Processing Layer
    std::shared_ptr<AmProcessor> amProcessor;

    std::mutex stop_lock;
    std::condition_variable stop_signal;
    bool shutting_down;

    bool standalone_mode;

    /**
     * FEATURE TOGGLE: Scst connector
     * Fri 11 Sep 2015 09:33:33 AM MDT
     */
    bool nbd_enabled {true};
    bool scst_enabled {false};
};

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_ACCESSMGR_H_
