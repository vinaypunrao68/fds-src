/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#ifndef SOURCE_UNIT_TEST_FDS_PROBE_ORCH_MGR_ORCH_MGR_PROBE_H_
#define SOURCE_UNIT_TEST_FDS_PROBE_ORCH_MGR_ORCH_MGR_PROBE_H_

/*
 * Header file template for probe adapter.  Replace OM with your probe name
 */
#include <string>
#include <fds-probe/js-object.h>
#include <fds-probe/fds_probe.h>

namespace fds {

class OM_ProbeMod : public ProbeMod
{
  public:
    OM_ProbeMod(char const *const name, probe_mod_param_t *param, Module *owner)
        : ProbeMod(name, param, owner) {}
    virtual ~OM_ProbeMod() {}

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

    friend class UT_OM_NodeInfo;

  private:
};

// OM Probe Adapter.
//
extern OM_ProbeMod           gl_OM_ProbeMod;

// ------------------------------------------------------------------------------------
// Handle OM Unit Test Setup
// ------------------------------------------------------------------------------------
typedef struct ut_node_info ut_node_info_t;
struct ut_node_info
{
    fds_bool_t               add;
    fds_uint64_t             nd_uuid;
    const char              *nd_node_name;
};
class UT_OM_NodeInfo : public JsObject
{
  public:
    virtual JsObject *
    js_exec_obj(JsObject *parent, JsObjTemplate *templ, JsObjOutput *out);

    inline ut_node_info_t *om_node_info() {
        return static_cast<ut_node_info_t *>(js_pod_object());
    }
};

class UT_OM_NodeInfoTemplate : public JsObjTemplate
{
  public:
    explicit UT_OM_NodeInfoTemplate(JsObjManager *mgr)
        : JsObjTemplate("node-info", mgr) {}

    virtual JsObject *js_new(json_t *in)
    {
        char           *uuid;
        char           *action;
        ut_node_info_t *p = new ut_node_info_t;

        if (json_unpack(in, "{s:s, s:s, s:s}",
                        "node-action", &action,
                        "node-uuid",   &uuid,
                        "node-name",   &p->nd_node_name)) {
            delete p;
            return NULL;
        }
        if (strcmp(action, "add") == 0) {
            p->add = true;
        } else if (strcmp(action, "rm") == 0) {
            p->add = false;
        } else {
            fds_panic("Unknown node action recieved");
        }
        p->nd_uuid = strtoull(uuid, NULL, 16);
        return js_parse(new UT_OM_NodeInfo(), in, p);
    }
};

class UT_OMSetupTemplate : public JsObjTemplate
{
  public:
    explicit UT_OMSetupTemplate(JsObjManager *mgr) : JsObjTemplate("om-setup", mgr)
    {
        js_decode["node-info"] = new UT_OM_NodeInfoTemplate(mgr);
    }
    virtual JsObject *js_new(json_t *in) {
        return js_parse(new JsObject(), in, NULL);
    }
};

// ------------------------------------------------------------------------------------
// Handle OM Unit Test Runtime
// ------------------------------------------------------------------------------------
typedef enum
{
    DLT_EVT_COMPUTE        = 0,
    DLT_EVT_UPDATE         = 1,
    DLT_EVT_UPDATE_DONE    = 2,
    DLT_EVT_COMMIT         = 3,
    DLT_EVT_COMMIT_DONE    = 4,
    DLT_EVT_MAX
} dlt_fsm_evt_e;

typedef struct ut_dlt_fsm_in ut_dlt_fsm_in_t;
struct ut_dlt_fsm_in
{
    dlt_fsm_evt_e            dlt_evt;
};

class UT_OM_DltFsm : public JsObject
{
  public:
    virtual JsObject *
    js_exec_obj(JsObject *parent, JsObjTemplate *templ, JsObjOutput *out);

    inline ut_dlt_fsm_in_t *om_dlt_evt() {
        return static_cast<ut_dlt_fsm_in_t *>(js_pod_object());
    }
};

class UT_OM_DltFsmTempl : public JsObjTemplate
{
  public:
    explicit UT_OM_DltFsmTempl(JsObjManager *mgr)
        : JsObjTemplate("dlt-fsm", mgr) {}

    virtual JsObject *js_new(json_t *in)
    {
        char            *evt;
        ut_dlt_fsm_in_t *p = new ut_dlt_fsm_in_t;

        if (json_unpack(in, "{s:s}", "input-event", &evt)) {
            delete p;
            return NULL;
        }
        if (strcmp(evt, "compute") == 0) {
            p->dlt_evt = DLT_EVT_COMPUTE;
        } else if (strcmp(evt, "update") == 0) {
            p->dlt_evt = DLT_EVT_UPDATE;
        } else if (strcmp(evt, "update-done") == 0) {
            p->dlt_evt = DLT_EVT_UPDATE_DONE;
        } else if (strcmp(evt, "commit") == 0) {
            p->dlt_evt = DLT_EVT_COMMIT;
        } else {
            p->dlt_evt = DLT_EVT_COMMIT_DONE;
        }
        return js_parse(new UT_OM_DltFsm(), in, p);
    }
};

class UT_OMRuntimeTempl : public JsObjTemplate
{
  public:
    explicit UT_OMRuntimeTempl(JsObjManager *mgr) : JsObjTemplate("om-runtime", mgr)
    {
        js_decode["dlt-fsm"] = new UT_OM_DltFsmTempl(mgr);
    }
    virtual JsObject *js_new(json_t *in) {
        return js_parse(new JsObject(), in, NULL);
    }
};

}  // namespace fds

#endif  // SOURCE_UNIT_TEST_FDS_PROBE_ORCH_MGR_ORCH_MGR_PROBE_H_
