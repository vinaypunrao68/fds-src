/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#ifndef SOURCE_UNIT_TEST_FDS_PROBE_THREADPOOL_THRPOOL_API_H_
#define SOURCE_UNIT_TEST_FDS_PROBE_THREADPOOL_THRPOOL_API_H_

/*
 * Header file template for probe adapter.  Replace Thrpool with your probe name
 */
#include <string>
#include <fds-probe/fds_probe.h>
#include <utest-types.h>
#include <boost/multi_array.hpp>

namespace fds {

class Thrpool_ProbeMod : public ProbeMod
{
  public:
    Thrpool_ProbeMod(char const *const name, probe_mod_param_t *param, Module *owner)
        : ProbeMod(name, param, owner) {}
    virtual ~Thrpool_ProbeMod() {}

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

// ----------------------------------------------------------------------------

/**
 * Unit test thread pool syscall service, type name "thpool-syscall",
 * "thpool-boost", "thpool-math"
 */
class UT_ThpoolSyscall : public JsObject
{
  public:
    virtual JsObject *js_exec_obj(JsObject *array,
                                  JsObjTemplate *templ, JsObjOutput *out);
};

typedef boost::multi_array<int, 2> UT_ThpoolBoost_Array2D;
typedef UT_ThpoolBoost_Array2D::index UT_ThpoolBoost_Array2DIndex;

class UT_ThpoolBoost : public JsObject
{
  public:
    virtual JsObject *js_exec_obj(JsObject *array,
                                  JsObjTemplate *templ, JsObjOutput *out);
  protected:
    static UT_ThpoolBoost_Array2D ut_thpboost_array;
    void ut_boost_array_2d_print(void);
};

class UT_ThpoolMath : public JsObject
{
  public:
    virtual JsObject *js_exec_obj(JsObject *array,
                                  JsObjTemplate *templ, JsObjOutput *out);
};

class UT_ThpoolSvcTemplate : public JsObjTemplate
{
  public:
    virtual ~UT_ThpoolSvcTemplate() {}
    explicit UT_ThpoolSvcTemplate(JsObjManager *mgr) : JsObjTemplate("threadpool", mgr)
    {
        const char *s;

        s = "thpool-syscall";
        js_decode[s] = new JsObjStrTemplate<UT_ThpoolSyscall>(s, mgr);

        s = "thpool-boost";
        js_decode[s] = new JsObjStrTemplate<UT_ThpoolBoost>(s, mgr);

        s = "thpool-math";
        js_decode[s] = new JsObjStrTemplate<UT_ThpoolMath>(s, mgr);
    }

    virtual JsObject *js_new(json_t *in) {
        return js_parse(new JsObject(), in, NULL);
    }
};

// Thrpool Probe Adapter.
//
extern Thrpool_ProbeMod           gl_Thrpool_ProbeMod;

}  // namespace fds

#endif  // SOURCE_UNIT_TEST_FDS_PROBE_THREADPOOL_THRPOOL_API_H_
