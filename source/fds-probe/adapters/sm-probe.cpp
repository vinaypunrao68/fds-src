/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#include <iostream>
#include <string>
#include <sm_probe.h>

namespace fds {

probe_mod_param_t sm_probe_param =
{
    .pr_stat_cnt     = 0,
    .pr_inj_pts_cnt  = 0,
    .pr_inj_act_cnt  = 0,
    .pr_max_sec_tout = 0
};

SM_ProbeMod gl_SM_ProbeMod("SM Probe Adapter",
                           &sm_probe_param, nullptr);

// bao2: merge test

// pr_new_instance
// ---------------
//
ProbeMod *
SM_ProbeMod::pr_new_instance()
{
    return new SM_ProbeMod("SM Inst", &sm_probe_param, NULL);
}

// pr_intercept_request
// --------------------
//
void
SM_ProbeMod::pr_intercept_request(ProbeRequest *req)
{
}

// pr_put
// ------
//
void
SM_ProbeMod::pr_put(ProbeRequest *probe)
{
}

// pr_get
// ------
//
void
SM_ProbeMod::pr_get(ProbeRequest *req)
{
}

// pr_delete
// ---------
//
void
SM_ProbeMod::pr_delete(ProbeRequest *req)
{
}

// pr_verify_request
// -----------------
//
void
SM_ProbeMod::pr_verify_request(ProbeRequest *req)
{
}

// pr_gen_report
// -------------
//
void
SM_ProbeMod::pr_gen_report(std::string *out)
{
}

// mod_init
// --------
//
int
SM_ProbeMod::mod_init(SysParams const *const param)
{
    Module::mod_init(param);
    return 0;
}

// mod_startup
// -----------
//
void
SM_ProbeMod::mod_startup()
{
    Module::mod_startup();
}

// mod_shutdown
// ------------
//
void
SM_ProbeMod::mod_shutdown()
{
}

}  // namespace fds
