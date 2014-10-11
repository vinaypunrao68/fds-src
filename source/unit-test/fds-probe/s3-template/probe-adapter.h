/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#ifndef SOURCE_UNIT_TEST_FDS_PROBE_S3_TEMPLATE_PROBE_ADAPTER_H_
#define SOURCE_UNIT_TEST_FDS_PROBE_S3_TEMPLATE_PROBE_ADAPTER_H_

/*
 * Header file template for probe adapter.  Replace XX with your probe name
 */
#include <string>
#include <fds-probe/fds_probe.h>

namespace fds {

class XX_ProbeMod : public ProbeMod
{
  public:
    XX_ProbeMod(char const *const name, probe_mod_param_t *param, Module *owner)
        : ProbeMod(name, param, owner) {}
    virtual ~XX_ProbeMod() {}

    ProbeMod *pr_new_instance();
    void pr_intercept_request(ProbeRequest *req);
    void pr_put(ProbeRequest *req);
    void pr_get(ProbeRequest *req);
    void pr_delete(ProbeRequest *req);
    void pr_verify_request(ProbeRequest *req);
    void pr_gen_report(std::string *out);

    int  mod_init(SysParams const *const param);
    void mod_startup();
    void mod_shutdown();

  private:
};

// XX Probe Adapter.
//
extern XX_ProbeMod           gl_XX_ProbeMod;

}  // namespace fds

#endif  // SOURCE_UNIT_TEST_FDS_PROBE_S3_TEMPLATE_PROBE_ADAPTER_H_
