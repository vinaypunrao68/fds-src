/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#include <pm_probe.h>
#include <persistent_layer/dm_io.h>

namespace fds {

probe_mod_param_t pm_probe_param =
{
    .pr_stat_cnt     = 0,
    .pr_inj_pts_cnt  = 0,
    .pr_inj_act_cnt  = 0,
    .pr_max_sec_tout = 0
};

PM_ProbeMod gl_PM_ProbeMod("PM Probe Adapter",
                           pm_probe_param,
                           &diskio::gl_dataIOMod);

// pr_intercept_request
// --------------------
//
void
PM_ProbeMod::pr_intercept_request(ProbeRequest &req)
{
}

// pr_put
// ------
//
void
PM_ProbeMod::pr_put(ProbeRequest &req)
{
}

// pr_get
// ------
//
void
PM_ProbeMod::pr_get(ProbeRequest &req)
{
}

// pr_delete
// ---------
//
void
PM_ProbeMod::pr_delete(ProbeRequest &req)
{
}

// pr_verify_request
// -----------------
//
void
PM_ProbeMod::pr_verify_request(ProbeRequest &req)
{
}

// pr_gen_report
// -------------
//
void
PM_ProbeMod::pr_gen_report(std::string &out)
{
}

// mod_init
// --------
//
int
PM_ProbeMod::mod_init(SysParams const *const param)
{
    return 0;
}

// mod_startup
// -----------
//
void
PM_ProbeMod::mod_startup()
{
}

// mod_shutdown
// ------------
//
void
PM_ProbeMod::mod_shutdown()
{
}

} // namespace fds

extern "C" {

void
init_fds_mod(int argc, char **argv)
{
    fds::Module *pm_probe_vec[] =
    {
        &diskio::gl_dataIOMod,
        &fds::gl_PM_ProbeMod,
        nullptr
    };
    fds::ModuleVector pm_probe(argc, argv, pm_probe_vec);
    pm_probe.mod_execute();
}

} // extern "C"
