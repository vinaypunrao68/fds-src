/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#ifndef SOURCE_UNIT_TEST_FDS_PROBE_MOCK_SERVER_MOCK_SERVER_H_
#define SOURCE_UNIT_TEST_FDS_PROBE_MOCK_SERVER_MOCK_SERVER_H_

/*
 * Header file template for probe adapter.  Replace Mock with your probe name
 */
#include <string>
#include <fds-probe/fds_probe.h>

namespace fds {

class Mock_ProbeMod : public ProbeMod
{
  public:
    Mock_ProbeMod(char const *const name, probe_mod_param_t *param, Module *owner)
        : ProbeMod(name, param, owner) {}
    virtual ~Mock_ProbeMod() {}

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

// Mock Probe Adapter.
//
extern Mock_ProbeMod           gl_Mock_ProbeMod;

// ----------------------------------------------------------------------------------
// Mock Server Setup
// ----------------------------------------------------------------------------------
class UT_ServSM : public JsObject
{
  public:
    virtual JsObject *
    js_exec_obj(JsObject *parent, JsObjTemplate *templ, JsObjOutput *out);
};

class UT_ServSMTempl : public JsObjTemplate
{
  public:
    explicit UT_ServSMTempl(JsObjManager *mgr) : JsObjTemplate("sm-setup", mgr) {}

    virtual JsObject *js_new(json_t *in) {
        return js_parse(new UT_ServSM(), in, new int(10));
    }
};

class UT_ServSetupTempl : public JsObjTemplate
{
  public:
    explicit UT_ServSetupTempl(JsObjManager *mgr) : JsObjTemplate("server-setup", mgr)
    {
        js_decode["sm-setup"] = new UT_ServSMTempl(mgr);
    }
    virtual JsObject *js_new(json_t *in) {
        return js_parse(new JsObject(), in, NULL);
    }
};

// ----------------------------------------------------------------------------------
// Mock Server Runtime
// ----------------------------------------------------------------------------------
class UT_RunSM : public JsObject
{
  public:
    virtual JsObject *
    js_exec_obj(JsObject *parent, JsObjTemplate *templ, JsObjOutput *out);
};

class UT_RunSMTempl : public JsObjTemplate
{
  public:
    explicit UT_RunSMTempl(JsObjManager *mgr) : JsObjTemplate("sm-run", mgr) {}

    virtual JsObject *js_new(json_t *in) {
        return js_parse(new UT_RunSM(), in, new int(10));
    }
};

class UT_RunServTempl : public JsObjTemplate
{
  public:
    explicit UT_RunServTempl(JsObjManager *mgr) : JsObjTemplate("server-run", mgr)
    {
        js_decode["sm-run"] = new UT_RunSMTempl(mgr);
    }
    virtual JsObject *js_new(json_t *in) {
        return js_parse(new JsObject(), in, NULL);
    }
};

}  // namespace fds

#endif  // SOURCE_UNIT_TEST_FDS_PROBE_MOCK_SERVER_MOCK_SERVER_H_
