/*
 * Copyright 2013 Formation Data Systems, Inc.
 *
 * Template to write probe adapter.  Replace Thrpool with your namespace.
 */
#include <string>
#include <thrpool-api.h>
#include <utest-types.h>
#include <boost/multi_array.hpp>

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
    Thrpool_ProbeMod *adapter;

    adapter = new Thrpool_ProbeMod("Thrpool Inst", &Thrpool_probe_param, NULL);

    adapter->mod_init(mod_params);
    adapter->mod_startup();
    return adapter;
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
    std::cout << "Thrpool_ProbeMode: Put data is called " << std::endl;
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
UT_ThpoolSyscall::js_exec_obj(JsObject *parent,
                              JsObjTemplate *templ, JsObjOutput *out)
{
    int       i, num;
    JsObject *obj;

    num = parent->js_array_size();
    std::cout << "UT_ThpoolSyscall::js_exec_obj: Thread pool syscall " << parent
        << ", cnt " << num << std::endl;

    for (i = 0; i < num; i++) {
        obj = (*parent)[i];
        std::cout << static_cast<char *>(obj->js_pod_object()) << ' ';
    }
    std::cout << std::endl;
    return JsObject::js_exec_obj(this, templ, out);
}

JsObject *
UT_ThpoolBoost::js_exec_obj(JsObject *parent,
                            JsObjTemplate *templ, JsObjOutput *out)
{
    int         i, num;
    JsObject   *obj;
    char       *op, *index0_str, *index1_str, *value_str;
    int         index0, index1, value;

    num = parent->js_array_size();
    std::cout << "UT_ThpoolBoost::js_exec_obj: Thread boost args is called, " << parent
              << ", cnt " << num << std::endl;

    for (i = 0; i < num; i++) {
        obj = (*parent)[i];
        std::cout << static_cast<char *>(obj->js_pod_object()) << ' ';
    }
    std::cout << std::endl;

    if (num != 4) {
        std::cout << "Invalid number of arguments for boost multi_array API: " << num
                  << ".  Expected <op> <index0> <index1> <value>" << std::endl;
    }

    obj = (*parent)[0];
    op = static_cast<char *>(obj->js_pod_object());

    obj = (*parent)[1];
    index0_str = static_cast<char *>(obj->js_pod_object());
    index0 = atoi(index0_str);

    obj = (*parent)[2];
    index1_str = static_cast<char *>(obj->js_pod_object());
    index1 = atoi(index1_str);

    obj = (*parent)[3];
    value_str = static_cast<char *>(obj->js_pod_object());
    value = atoi(value_str);

    if (strcmp(op, "add") == 0) {
        ut_thpboost_array[index0][index1] = ut_thpboost_array[index0][index1] + value;
        std::cout << "array: " << ut_thpboost_array[index0][index1] << std::endl;
    } else if (strcmp(op, "sub") == 0) {
        ut_thpboost_array[index0][index1] = ut_thpboost_array[index0][index1] - value;
    } else if (strcmp(op, "print") == 0) {
        UT_ThpoolBoost::ut_boost_array_2d_print();
    } else {
        std::cout << "Invalid op: " << op << std::endl;
    }

    return JsObject::js_exec_obj(this, templ, out);
}

UT_ThpoolBoost_Array2D UT_ThpoolBoost::ut_thpboost_array(boost::extents[10][10]);

void
UT_ThpoolBoost::ut_boost_array_2d_print(void)
{
    for (UT_ThpoolBoost_Array2DIndex i = 0; i < 10; i++) {
        for (UT_ThpoolBoost_Array2DIndex j = 0; j < 10; j++) {
            std::cout << "array[" << i << "][" << j << "] = "
                      << ut_thpboost_array[i][j] << std::endl;
        }
    }
}

JsObject *
UT_ThpoolMath::js_exec_obj(JsObject *parent,
                           JsObjTemplate *templ, JsObjOutput *out)
{
    std::cout << "Thread pool math is called... " << std::endl;
    return JsObject::js_exec_obj(this, templ, out);
}

}  // namespace fds
