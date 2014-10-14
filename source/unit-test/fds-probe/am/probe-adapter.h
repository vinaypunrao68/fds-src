/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#ifndef SOURCE_UNIT_TEST_FDS_PROBE_AM_PROBE_ADAPTER_H_
#define SOURCE_UNIT_TEST_FDS_PROBE_AM_PROBE_ADAPTER_H_

/*
 * Header file template for probe adapter.  Replace XX with your probe name
 */
#include <string>
#include <fds-probe/fds_probe.h>

namespace fds {

class AmProbeMod : public ProbeMod
{
  public:
    AmProbeMod(char const *const name, probe_mod_param_t *param, Module *owner)
        : ProbeMod(name, param, owner) {}
    virtual ~AmProbeMod() {}

    ProbeMod *pr_new_instance();
    void pr_intercept_request(ProbeRequest *req);
    void pr_put(ProbeRequest *req);
    void pr_get(ProbeRequest *req);
    void pr_delete(ProbeRequest *req);
    void pr_verify_request(ProbeRequest *req);
    void pr_gen_report(std::string *out);

    int  mod_init(SysParams const *const param) override;
    void mod_startup() override;
    void mod_shutdown() override;

  private:
};

// XX Probe Adapter.
//
extern AmProbeMod           gl_AmProbeMod;

}  // namespace fds

#endif  // SOURCE_UNIT_TEST_FDS_PROBE_AM_PROBE_ADAPTER_H_
