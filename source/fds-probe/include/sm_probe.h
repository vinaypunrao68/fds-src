/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#ifndef FDS_PROBE_INCLUDE_SM_PROBE_H_
#define FDS_PROBE_INCLUDE_SM_PROBE_H_

#include <fds-probe/fds_probe.h>

namespace fds {

class SM_ProbeMod : public ProbeMod
{
  public:
    SM_ProbeMod(char const *const name, probe_mod_param_t &param, Module *owner)
        : ProbeMod(name, param, owner) {}
    virtual ~SM_ProbeMod() {}

    void pr_intercept_request(ProbeRequest &req);
    void pr_put(ProbeRequest &req);
    void pr_get(ProbeRequest &req);
    void pr_delete(ProbeRequest &req);
    void pr_verify_request(ProbeRequest &req);
    void pr_gen_report(std::string &out);

    int  mod_init(SysParams const *const param);
    void mod_startup();
    void mod_shutdown();

  private:
};

// SM Probe Adapter.
//
extern SM_ProbeMod           gl_SM_ProbeMod;

} // namespace fds

#endif /* FDS_PROBE_INCLUDE_SM_PROBE_H_ */
