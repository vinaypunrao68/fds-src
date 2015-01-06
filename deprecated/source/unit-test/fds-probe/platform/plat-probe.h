/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#ifndef SOURCE_UNIT_TEST_FDS_PROBE_PLATFORM_PLAT_PROBE_H_
#define SOURCE_UNIT_TEST_FDS_PROBE_PLATFORM_PLAT_PROBE_H_

/*
 * Header file platform probe unit test.
 */
#include <string>
#include <fds-probe/fds_probe.h>

namespace fds {

class Plat_ProbeMod : public ProbeMod
{
  public:
    Plat_ProbeMod(char const *const name, probe_mod_param_t *param, Module *owner)
        : ProbeMod(name, param, owner) {}
    virtual ~Plat_ProbeMod() {}

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

// Platform Probe Adapter.
//
extern Plat_ProbeMod           gl_PlatProbeMod;

// ----------------------------------------------------------------------------------
// Mock Server Setup
// ----------------------------------------------------------------------------------
class UT_ServPlat : public JsObject
{
  public:
    virtual JsObject *
    js_exec_obj(JsObject *parent, JsObjTemplate *templ, JsObjOutput *out);
};

class UT_ServPlatTempl : public JsObjTemplate
{
  public:
    explicit UT_ServPlatTempl(JsObjManager *mgr) : JsObjTemplate("plat-setup", mgr) {}

    virtual JsObject *js_new(json_t *in) {
        return js_parse(new UT_ServPlat(), in, NULL);
    }
};

class UT_ServSetupTempl : public JsObjTemplate
{
  public:
    explicit UT_ServSetupTempl(JsObjManager *mgr) : JsObjTemplate("server-setup", mgr)
    {
        js_decode["plat-setup"] = new UT_ServPlatTempl(mgr);
    }
    virtual JsObject *js_new(json_t *in) {
        return js_parse(new JsObject(), in, NULL);
    }
};

// ----------------------------------------------------------------------------------
// Mock Server Runtime
// ----------------------------------------------------------------------------------
class UT_RunPlat : public JsObject
{
  public:
    virtual JsObject *
    js_exec_obj(JsObject *parent, JsObjTemplate *templ, JsObjOutput *out);
};

class UT_RunPlatTempl : public JsObjTemplate
{
  public:
    explicit UT_RunPlatTempl(JsObjManager *mgr) : JsObjTemplate("plat-run", mgr) {}

    virtual JsObject *js_new(json_t *in) {
        return js_parse(new UT_RunPlat(), in, NULL);
    }
};

class UT_RunServTempl : public JsObjTemplate
{
  public:
    explicit UT_RunServTempl(JsObjManager *mgr) : JsObjTemplate("server-run", mgr)
    {
        js_decode["plat-run"] = new UT_RunPlatTempl(mgr);
    }
    virtual JsObject *js_new(json_t *in) {
        return js_parse(new JsObject(), in, NULL);
    }
};

}  // namespace fds

#endif  // SOURCE_UNIT_TEST_FDS_PROBE_PLATFORM_PLAT_PROBE_H_
