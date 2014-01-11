/*
 * Copyright 2013 Formation Data Systems, Inc.
 *
 * Template to write probe adapter.  Replace XX with your namespace.
 */
#include <template_probe.h>
#include <string>

namespace fds {

probe_mod_param_t XX_probe_param =
{
    .pr_stat_cnt     = 0,
    .pr_inj_pts_cnt  = 0,
    .pr_inj_act_cnt  = 0,
    .pr_max_sec_tout = 0
};

XX_ProbeMod gl_XX_ProbeMod("XX Probe Adapter",
                           &XX_probe_param, nullptr);

// pr_new_instance
// ---------------
//
ProbeMod *
XX_ProbeMod::pr_new_instance()
{
    return new XX_ProbeMod("XX Inst", &XX_probe_param, NULL);
}

// pr_intercept_request
// --------------------
//
void
XX_ProbeMod::pr_intercept_request(ProbeRequest *req)
{
}

// pr_put
// ------
//
void
XX_ProbeMod::pr_put(ProbeRequest *probe)
{
}

// pr_get
// ------
//
void
XX_ProbeMod::pr_get(ProbeRequest *req)
{
}

// pr_delete
// ---------
//
void
XX_ProbeMod::pr_delete(ProbeRequest *req)
{
}

// pr_verify_request
// -----------------
//
void
XX_ProbeMod::pr_verify_request(ProbeRequest *req)
{
}

// pr_gen_report
// -------------
//
void
XX_ProbeMod::pr_gen_report(std::string *out)
{
}

// mod_init
// --------
//
int
XX_ProbeMod::mod_init(SysParams const *const param)
{
    Module::mod_init(param);
    return 0;
}

// mod_startup
// -----------
//
void
XX_ProbeMod::mod_startup()
{
}

// mod_shutdown
// ------------
//
void
XX_ProbeMod::mod_shutdown()
{
}

}  // namespace fds
