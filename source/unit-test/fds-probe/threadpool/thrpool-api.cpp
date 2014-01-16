/*
 * Copyright 2013 Formation Data Systems, Inc.
 *
 * Template to write probe adapter.  Replace Thrpool with your namespace.
 */
#include <thrpool-api.h>
#include <string>
#include <utest-types.h>

namespace fds {

probe_mod_param_t Thrpool_probe_param =
{
    .pr_stat_cnt     = 0,
    .pr_inj_pts_cnt  = 0,
    .pr_inj_act_cnt  = 0,
    .pr_max_sec_tout = 0
};

Thrpool_ProbeMod gl_Thrpool_ProbeMod("Thrpool Probe Adapter",
                           &Thrpool_probe_param, nullptr);

// pr_new_instance
// ---------------
//
ProbeMod *
Thrpool_ProbeMod::pr_new_instance()
{
    return new Thrpool_ProbeMod("Thrpool Inst", &Thrpool_probe_param, NULL);
}

// pr_intercept_request
// --------------------
//
void
Thrpool_ProbeMod::pr_intercept_request(ProbeRequest *req)
{
}

// pr_put
// ------
//
void
Thrpool_ProbeMod::pr_put(ProbeRequest *probe)
{
}

// pr_get
// ------
//
void
Thrpool_ProbeMod::pr_get(ProbeRequest *req)
{
}

// pr_delete
// ---------
//
void
Thrpool_ProbeMod::pr_delete(ProbeRequest *req)
{
}

// pr_verify_request
// -----------------
//
void
Thrpool_ProbeMod::pr_verify_request(ProbeRequest *req)
{
}

// pr_gen_report
// -------------
//
void
Thrpool_ProbeMod::pr_gen_report(std::string *out)
{
}

// mod_init
// --------
//
int
Thrpool_ProbeMod::mod_init(SysParams const *const param)
{
    Module::mod_init(param);

    return 0;
}

// mod_startup
// -----------
//
void
Thrpool_ProbeMod::mod_startup()
{
    JsObjManager  *mgr;
    JsObjTemplate *svc;

    if (pr_parent != NULL) {
        mgr = pr_parent->pr_get_obj_mgr();
        svc = mgr->js_get_template("Run-Input");
        svc = svc->js_get_template("Server-Load");
        svc->js_register_template(new UT_ThpoolSvcTemplate(mgr));
    }
}

// mod_shutdown
// ------------
//
void
Thrpool_ProbeMod::mod_shutdown()
{
}

// ----------------------------------------------------------------------------
// Threadpool control path
// ----------------------------------------------------------------------------
JsObject *
UT_ThpoolSyscall::js_exec_obj(JsObject *array, JsObjTemplate *templ)
{
    int       i, num;
    JsObject *obj;

    num = array->js_array_size();
    std::cout << "Thread pool syscall " << array
        << ", cnt " << num << std::endl;

    for (i = 0; i < num; i++) {
        obj = array->js_array_elm(i);
        std::cout << static_cast<char *>(obj->js_pod_object()) << ' ';
    }
    std::cout << std::endl;
    return JsObject::js_exec_obj(array, templ);
}

JsObject *
UT_ThpoolBoost::js_exec_obj(JsObject *array, JsObjTemplate *templ)
{
    std::cout << "Thread boost args is called... " << std::endl;
    return JsObject::js_exec_obj(array, templ);
}

JsObject *
UT_ThpoolMath::js_exec_obj(JsObject *array, JsObjTemplate *templ)
{
    std::cout << "Thread pool math is called... " << std::endl;
    return JsObject::js_exec_obj(array, templ);
}

}  // namespace fds
