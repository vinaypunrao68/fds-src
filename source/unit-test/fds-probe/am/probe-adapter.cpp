/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#include <string>
#include <probe-adapter.h>
#include <in.h>

namespace fds {

probe_mod_param_t am_probe_param =
{
    .pr_stat_cnt     = 0,
    .pr_inj_pts_cnt  = 0,
    .pr_inj_act_cnt  = 0,
    .pr_max_sec_tout = 0
};

AmProbeMod gl_AmProbeMod("XX Probe Adapter", &am_probe_param, nullptr);

// pr_new_instance
// ---------------
//
ProbeMod *
AmProbeMod::pr_new_instance()
{
    AmProbeMod *adapter = new AmProbeMod("XX Inst", &am_probe_param, NULL);

    adapter->mod_init(mod_params);
    adapter->mod_startup();
    return adapter;
}

// pr_intercept_request
// --------------------
//
void
AmProbeMod::pr_intercept_request(ProbeRequest *req)
{
}

// pr_put
// ------
//
void
AmProbeMod::pr_put(ProbeRequest *probe)
{
}

// pr_get
// ------
//
void
AmProbeMod::pr_get(ProbeRequest *req)
{
}

// pr_delete
// ---------
//
void
AmProbeMod::pr_delete(ProbeRequest *req)
{
}

// pr_verify_request
// -----------------
//
void
AmProbeMod::pr_verify_request(ProbeRequest *req)
{
}

// pr_gen_report
// -------------
//
void
AmProbeMod::pr_gen_report(std::string *out)
{
}

// mod_init
// --------
//
int
AmProbeMod::mod_init(SysParams const *const param)
{
    Module::mod_init(param);
    return 0;
}

// mod_startup
// -----------
//
void
AmProbeMod::mod_startup()
{
    JsObjManager  *mgr;
    JsObjTemplate *svc;

    if (pr_parent != NULL) {
        /* Hookup the class matching "Run-Setup" in your .h file here. */
        mgr = pr_parent->pr_get_obj_mgr();
        mgr->js_register_template(new RunSetupTmpl(mgr));
    }
}

// mod_shutdown
// ------------
//
void
AmProbeMod::mod_shutdown()
{
}

}  // namespace fds
