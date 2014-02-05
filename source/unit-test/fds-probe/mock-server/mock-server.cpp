/*
 * Copyright 2013 Formation Data Systems, Inc.
 *
 * Template to write probe adapter.  Replace Mock with your namespace.
 */
#include <mock-server.h>
#include <string>
#include <iostream>

namespace fds {

probe_mod_param_t Mock_probe_param =
{
    .pr_stat_cnt     = 0,
    .pr_inj_pts_cnt  = 0,
    .pr_inj_act_cnt  = 0,
    .pr_max_sec_tout = 0
};

Mock_ProbeMod gl_Mock_ProbeMod("Mock Probe Adapter",
                           &Mock_probe_param, nullptr);

// pr_new_instance
// ---------------
//
ProbeMod *
Mock_ProbeMod::pr_new_instance()
{
    Mock_ProbeMod *adapter = new Mock_ProbeMod("Mock Inst", &Mock_probe_param, NULL);

    adapter->mod_init(mod_params);
    adapter->mod_startup();
    return adapter;
}

// pr_intercept_request
// --------------------
//
void
Mock_ProbeMod::pr_intercept_request(ProbeRequest *req)
{
}

// pr_put
// ------
//
void
Mock_ProbeMod::pr_put(ProbeRequest *probe)
{
}

// pr_get
// ------
//
void
Mock_ProbeMod::pr_get(ProbeRequest *req)
{
}

// pr_delete
// ---------
//
void
Mock_ProbeMod::pr_delete(ProbeRequest *req)
{
}

// pr_verify_request
// -----------------
//
void
Mock_ProbeMod::pr_verify_request(ProbeRequest *req)
{
}

// pr_gen_report
// -------------
//
void
Mock_ProbeMod::pr_gen_report(std::string *out)
{
}

// mod_init
// --------
//
int
Mock_ProbeMod::mod_init(SysParams const *const param)
{
    Module::mod_init(param);
    return 0;
}

// mod_startup
// -----------
//
void
Mock_ProbeMod::mod_startup()
{
    JsObjManager  *mgr;
    JsObjTemplate *svc;

    if (pr_parent != NULL) {
        mgr = pr_parent->pr_get_obj_mgr();
        svc = mgr->js_get_template("Run-Setup");
        svc->js_register_template(new UT_ServSetupTempl(mgr));

        svc = mgr->js_get_template("Run-Input");
        svc->js_register_template(new UT_RunServTempl(mgr));
    }
}

// mod_shutdown
// ------------
//
void
Mock_ProbeMod::mod_shutdown()
{
}

// -----------------------------------------------------------------------------------
// Mock Server Setup
// -----------------------------------------------------------------------------------
JsObject *
UT_ServSM::js_exec_obj(JsObject *parent, JsObjTemplate *templ, JsObjOutput *out)
{
    std::cout << "In SM setup" << std::endl;
    return this;
}

// -----------------------------------------------------------------------------------
// Mock Server Runtime
// -----------------------------------------------------------------------------------
JsObject *
UT_RunSM::js_exec_obj(JsObject *parent, JsObjTemplate *templ, JsObjOutput *out)
{
    std::cout << "In SM Run" << std::endl;
    return this;
}

}  // namespace fds
