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

// Adapters to export OM API.
//
typedef struct ut_node_info ut_node_info_t;
struct ut_node_info
{
    fds_bool_t               add;
    fds_uint64_t             nd_uuid;
    const char              *nd_node_name;
    fds_uint64_t             nd_weight;
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
        p->nd_weight = 0;
        json_unpack(in, "{s:i}",
                    "node-weight", &p->nd_weight);

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

}  // namespace fds

#endif  // SOURCE_UNIT_TEST_FDS_PROBE_ORCH_MGR_ORCH_MGR_PROBE_H_
