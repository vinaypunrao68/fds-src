/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_ACCESS_MGR_INCLUDE_ACCESSMGR_H_
#define SOURCE_ACCESS_MGR_INCLUDE_ACCESSMGR_H_

#include <string>
#include <fds_types.h>
#include <fds_module_provider.h>
#include <fdsn-server.h>
#include <AmDataApi.h>
#include <StorHvisorNet.h>

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
    int mod_init(SysParams const *const param);
    void mod_startup();
    void mod_shutdown();

    void run();

    /// Shared ptr to AM's data API. It's public so that
    /// other components (e.g., unit tests, perf tests) can
    /// directly call it. It may be shared by this and the
    /// fdsn server.
    AmDataApi::shared_ptr dataApi;

  private:
    /// Raw pointer to an external dependancy manager
    CommonModuleProviderIf *modProvider_;

    /// Unique ptr to the fdsn server that communicates with XDI
    FdsnServer::unique_ptr fdsnServer;
};

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_ACCESSMGR_H_
