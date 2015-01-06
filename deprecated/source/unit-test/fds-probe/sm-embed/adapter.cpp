/*
 * Copyright 2013 Formation Data Systems, Inc.
 *
 * Template to write probe adapter.  Replace XX with your namespace.
 */
#include <adapter.h>
#include <string>

namespace fds {

probe_mod_param_t SMprobe_param =
{
    .pr_stat_cnt     = 0,
    .pr_inj_pts_cnt  = 0,
    .pr_inj_act_cnt  = 0,
    .pr_max_sec_tout = 0
};

SMProbeMod gl_SMProbeMod("XX Probe Adapter",
                           &SMprobe_param, nullptr);

// pr_new_instance
// ---------------
//
ProbeMod *
SMProbeMod::pr_new_instance()
{
    SMProbeMod *adapter = new SMProbeMod("XX Inst", &SMprobe_param, NULL);

    adapter->mod_init(mod_params);
    adapter->mod_startup();
    return adapter;
}

// pr_intercept_request
// --------------------
//
void
SMProbeMod::pr_intercept_request(ProbeRequest *req)
{
}

// pr_put
// ------
//
void
SMProbeMod::pr_put(ProbeRequest *probe)
{
}

// pr_get
// ------
//
void
SMProbeMod::pr_get(ProbeRequest *req)
{
}

// pr_delete
// ---------
//
void
SMProbeMod::pr_delete(ProbeRequest *req)
{
}

// pr_verify_request
// -----------------
//
void
SMProbeMod::pr_verify_request(ProbeRequest *req)
{
}

// pr_gen_report
// -------------
//
void
SMProbeMod::pr_gen_report(std::string *out)
{
}

// mod_init
// --------
//
int
SMProbeMod::mod_init(SysParams const *const param)
{
    Module::mod_init(param);
    return 0;
}

// mod_startup
// -----------
//
void
SMProbeMod::mod_startup()
{
    JsObjManager  *mgr;
    JsObjTemplate *svc;

    if (pr_parent != NULL) {
        /* Hookup the class matching "Run-Setup" in your .h file here. */
        mgr = pr_parent->pr_get_obj_mgr();
        mgr->js_register_template(new RunInputTmpl(mgr));
    }
}

// mod_shutdown
// ------------
//
void
SMProbeMod::mod_shutdown()
{
}

// -----------------------------------------------------------------------------------
// Replace it with your handlers
// -----------------------------------------------------------------------------------
JsObject *
SmOpsObj::js_exec_obj(JsObject *parent, JsObjTemplate *tmpl, JsObjOutput *out)
{
    sm_ops_in_t *ops = sm_ops_in();

    std::cout << "Sm ops " << ops->op << ", vol id " << ops->volume_id;
    return this;
}

}  // namespace fds
