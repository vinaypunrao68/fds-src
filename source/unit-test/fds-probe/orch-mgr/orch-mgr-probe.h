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
#include <dlt.h>
#include <OmClusterMap.h>

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
enum ut_rs_type {
    ut_rs_sm = 0,
    ut_rs_dm = 1,
    ut_rs_am = 2,
    ut_rs_pm = 3,
    ut_rs_vol = 4
};

typedef struct ut_node_info ut_node_info_t;
struct ut_node_info
{
    fds_bool_t               add;
    fds_uint64_t             nd_uuid;
    const char              *nd_node_name;
    fds_uint64_t             nd_weight;
};

typedef struct ut_resource_info ut_resource_info_t;
struct ut_resource_info
{
    fds_bool_t               add;
    fds_uint64_t             rs_uuid;
    const char              *rs_name;
    ut_rs_type               rs_type;
};

class UT_DLT_EvalHelper
{
  public:
    UT_DLT_EvalHelper()
            : old_dlt_ptr(NULL),
            old_depth(0),
            old_tokens(0),
            new_dlt_ptr(NULL),
            new_depth(0),
            new_tokens(0) {
    }

    ~UT_DLT_EvalHelper() {
        if (old_dlt_ptr) {
            delete[] old_dlt_ptr;
        }
        if (new_dlt_ptr) {
            delete[] new_dlt_ptr;
        }
    }

    // For simple test of changing dlt once, get DLT before
    // doing an update and call setOldDlt(). Then, update cluster
    // map/recompute DLT and call setNewDlt().
    // Then calling compareDlts() will compare old and new DLTs.
    void setOldDlt(const DLT* dlt) {
        if (dlt != NULL) {
            old_dlt_ptr = copy_dlt(dlt);
            old_depth = dlt->getDepth();
            old_tokens = dlt->getNumTokens();
        }
    }
    void setNewDlt(const DLT* dlt) {
        fds_verify(dlt != NULL);
        new_dlt_ptr = copy_dlt(dlt);
        new_depth = dlt->getDepth();
        new_tokens = dlt->getNumTokens();
    }
    void printAndCompareDlts(const ClusterMap *cm) {
        std::cout << "Old DLT: " << std::endl;
        print_dlt(old_dlt_ptr, old_depth, old_tokens, NULL);
        std::cout << "New DLT: " << std::endl;
        print_dlt(new_dlt_ptr, new_depth, new_tokens, cm);

        compare_dlts(old_dlt_ptr, old_depth, old_tokens,
                     new_dlt_ptr, new_depth, new_tokens);
    }

    // static funcs if need to print/compare many different DLTs
    static void print_dlt(const fds_uint64_t* tbl,
                          fds_uint32_t depth,
                          fds_uint32_t toks,
                          const ClusterMap* cm);
    static void compare_dlts(const fds_uint64_t* old_tbl,
                             fds_uint32_t old_depth,
                             fds_uint32_t old_toks,
                             const fds_uint64_t* new_tbt,
                             fds_uint32_t new_depth,
                             fds_uint32_t new_toks);
    static fds_uint64_t* copy_dlt(const DLT* dlt);

  private:
    fds_uint64_t* old_dlt_ptr;
    fds_uint32_t old_depth;
    fds_uint32_t old_tokens;

    fds_uint64_t* new_dlt_ptr;
    fds_uint32_t new_depth;
    fds_uint32_t new_tokens;
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

class UT_DP_NodeInfo : public JsObject
{
  public:
    virtual JsObject *
    js_exec_obj(JsObject *parent, JsObjTemplate *templ, JsObjOutput *out);

    inline ut_node_info_t *dp_node_info() {
        return static_cast<ut_node_info_t *>(js_pod_object());
    }
};

class UT_VP_NodeInfo : public JsObject
{
  public:
    virtual JsObject *
    js_exec_obj(JsObject *parent, JsObjTemplate *templ, JsObjOutput *out);

    inline ut_resource_info_t *vp_node_info() {
        return static_cast<ut_resource_info_t *>(js_pod_object());
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
            fds_panic("Unknown node action received");
        }
        p->nd_uuid = strtoull(uuid, NULL, 16);
        return js_parse(new UT_OM_NodeInfo(), in, p);
    }
};

//
// Template for Data Placement (DP) algorithms unit test
//
class UT_DP_NodeInfoTemplate : public JsObjTemplate
{
  public:
    explicit UT_DP_NodeInfoTemplate(JsObjManager *mgr)
        : JsObjTemplate("dp-node-info", mgr) {}

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
        return js_parse(new UT_DP_NodeInfo(), in, p);
    }
};


//
// Template for Volume Placement (VP) unit test
//
class UT_VP_NodeInfoTemplate : public JsObjTemplate
{
  public:
    explicit UT_VP_NodeInfoTemplate(JsObjManager *mgr)
        : JsObjTemplate("vp-dm-info", mgr) {}

    virtual JsObject *js_new(json_t *in)
    {
        char           *uuid;
        char           *action;
        char           *type;
        ut_resource_info_t *p = new ut_resource_info_t;

        std::cout << "in js_new vol place" << std::endl;

        if (json_unpack(in, "{s:s, s:s, s:s}",
                        "rs-action", &action,
                        "rs-uuid",   &uuid,
                        "rs-name",   &p->rs_name)) {
            delete p;
            return NULL;
        }
        json_unpack(in, "{s:s}",
                    "rs-type", &type);

        if (strcmp(type, "dm") == 0) {
            p->rs_type = ut_rs_dm;
        } else if (strcmp(type, "vol") == 0) {
            p->rs_type = ut_rs_vol;
        } else {
            fds_panic("Unknown resource type received");
        }

        if (strcmp(action, "add") == 0) {
            p->add = true;
        } else if (strcmp(action, "rm") == 0) {
            p->add = false;
        } else {
            fds_panic("Unknown node action recieved");
        }
        p->rs_uuid = strtoull(uuid, NULL, 16);
        return js_parse(new UT_VP_NodeInfo(), in, p);
    }
};


class UT_OMSetupTemplate : public JsObjTemplate
{
  public:
    explicit UT_OMSetupTemplate(JsObjManager *mgr) : JsObjTemplate("om-setup", mgr)
    {
        js_decode["node-info"] = new UT_OM_NodeInfoTemplate(mgr);
        js_decode["dp-node-info"] = new UT_DP_NodeInfoTemplate(mgr);
        js_decode["vp-dm-info"] = new UT_VP_NodeInfoTemplate(mgr);
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
