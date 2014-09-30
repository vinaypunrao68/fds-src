/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#include <string>
#include <plat-probe.h>
#include <node-work.h>

namespace fds {

probe_mod_param_t plat_probe_param =
{
    .pr_stat_cnt     = 0,
    .pr_inj_pts_cnt  = 0,
    .pr_inj_act_cnt  = 0,
    .pr_max_sec_tout = 0
};

Plat_ProbeMod gl_PlatProbeMod("Plat Probe Adapter", &plat_probe_param, nullptr);

// pr_new_instance
// ---------------
//
ProbeMod *
Plat_ProbeMod::pr_new_instance()
{
    Plat_ProbeMod *adapter = new Plat_ProbeMod("Plat Inst", &plat_probe_param, NULL);

    adapter->mod_init(mod_params);
    adapter->mod_startup();
    return adapter;
}

// pr_intercept_request
// --------------------
//
void
Plat_ProbeMod::pr_intercept_request(ProbeRequest *req)
{
}

// pr_put
// ------
//
void
Plat_ProbeMod::pr_put(ProbeRequest *probe)
{
}

// pr_get
// ------
//
void
Plat_ProbeMod::pr_get(ProbeRequest *req)
{
}

// pr_delete
// ---------
//
void
Plat_ProbeMod::pr_delete(ProbeRequest *req)
{
}

// pr_verify_request
// -----------------
//
void
Plat_ProbeMod::pr_verify_request(ProbeRequest *req)
{
}

// pr_gen_report
// -------------
//
void
Plat_ProbeMod::pr_gen_report(std::string *out)
{
}

// mod_init
// --------
//
int
Plat_ProbeMod::mod_init(SysParams const *const param)
{
    Module::mod_init(param);
    return 0;
}

// mod_startup
// -----------
//
void
Plat_ProbeMod::mod_startup()
{
    JsObjManager  *mgr;
    JsObjTemplate *svc;

    if (pr_parent != NULL) {
        mgr = pr_parent->pr_get_obj_mgr();
        mgr->js_register_template(new UT_ServSetupTempl(mgr));
        mgr->js_register_template(new UT_RunServTempl(mgr));
        mgr->js_register_template(new RunInputTmpl(mgr));
    }
}

// mod_shutdown
// ------------
//
void
Plat_ProbeMod::mod_shutdown()
{
}

// -----------------------------------------------------------------------------------
// Mock Server Setup
// -----------------------------------------------------------------------------------
JsObject *
UT_ServPlat::js_exec_obj(JsObject *parent, JsObjTemplate *templ, JsObjOutput *out)
{
    return this;
}

// -----------------------------------------------------------------------------------
// Mock Server Runtime
// -----------------------------------------------------------------------------------
JsObject *
UT_RunPlat::js_exec_obj(JsObject *parent, JsObjTemplate *templ, JsObjOutput *out)
{
    std::cout << "Hello, run something" << std::endl;
    return this;
}

}  // namespace fds
