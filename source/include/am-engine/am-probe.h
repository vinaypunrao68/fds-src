/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_AM_ENGINE_AM_PROBE_H_
#define SOURCE_INCLUDE_AM_ENGINE_AM_PROBE_H_

#include <string>
#include <fds_module.h>
#include <native_api.h>
#include <fds-probe/fds_probe.h>

namespace fds {

/**
 * Class that provides interface mapping between
 * UBD, the user space block device, and AM.
 */
class AmProbe : public ProbeMod {
  private:
    FDS_NativeAPI::ptr               am_api;
    boost::shared_ptr<boost::thread> listen_thread;

  public:
    explicit AmProbe(const std::string &name,
                     probe_mod_param_t *param,
                     Module *owner);
    virtual ~AmProbe();

    typedef boost::shared_ptr<AmProbe> ptr;

    // Module related methods
    int  mod_init(SysParams const *const param);
    void mod_startup();
    void mod_shutdown();
    void init_server(FDS_NativeAPI::ptr api);

    // Probe related methods
    ProbeMod *pr_new_instance();
    void pr_intercept_request(ProbeRequest *req);
    void pr_put(ProbeRequest *req);
    void pr_get(ProbeRequest *req);
    void pr_delete(ProbeRequest *req);
    void pr_verify_request(ProbeRequest *req);
    void pr_gen_report(std::string *out);
};

extern AmProbe gl_AmProbe;

}  // namespace fds

#endif  // SOURCE_INCLUDE_AM_ENGINE_AM_PROBE_H_
