/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_UNIT_TEST_FDS_PROBE_STOR_MGR_FULL_SM_PROBE_H_
#define SOURCE_UNIT_TEST_FDS_PROBE_STOR_MGR_FULL_SM_PROBE_H_

/*
 * Header file template for probe adapter.  Replace Thrpool with your probe name
 */
#include <string>
#include <list>
#include <fds-probe/fds_probe.h>
#include <utest-types.h>
#include <fds_types.h>
#include <NetSession.h>

namespace fds {

class probeDataPathRespIf;

extern std::unordered_map<ObjectID, std::string, ObjectHash> writtenObjs;
extern fds_mutex objMapLock;

/**
 * SM probe that acts as an SM client.
 * Provides implementation of JSON interface to
 * SM requests.
 */
class Sm_ProbeMod : public ProbeMod
{
  public:
    Sm_ProbeMod(char const *const name, probe_mod_param_t *param, Module *owner)
            : ProbeMod(name, param, owner),
            smIp("127.0.0.1"),
            myIp("127.0.0.1"),
            myName("SM probe client"),
            numChannels(1),
            numVols(1),
            numObjs(10),
            randData(false),
            verify(true) {
    }
    virtual ~Sm_ProbeMod() {
    }

    ProbeMod *pr_new_instance();
    void pr_intercept_request(ProbeRequest *req);
    void pr_put(ProbeRequest *req);
    void pr_get(ProbeRequest *req);
    void pr_delete(ProbeRequest *req);
    void pr_verify_request(ProbeRequest *req);
    void pr_gen_report(std::string *out);

    class PutParams {
  public:
        fds_volid_t  volId;
        ObjectID     objId;
        fds_uint32_t dataLen;
        char         *objData;
    };
    void sendPut(const PutParams &objData);

    class GetParams {
  public:
        fds_volid_t  volId;
        ObjectID     objId;
    };
    void sendGet(const GetParams &objData);

    class DeleteParams {
  public:
        fds_volid_t  volId;
        ObjectID     objId;
    };
    void sendDelete(const DeleteParams &delReq);

    int  mod_init(SysParams const *const param);
    void mod_startup();
    void mod_shutdown();

    class TestParams {
  public:
        fds_int32_t numVols;
        fds_int32_t numObjs;
        fds_bool_t  randData;
        fds_bool_t  verify;
    };
    void setTestParams(const TestParams &p);

    void updatePutObj(const ObjectID& oid,
                      const std::string& objData);
    fds_bool_t checkGetObj(const ObjectID& oid,
                           const std::string& objData);


  private:
    std::string smIp; /**< Remote SM's IP */
    std::string myIp; /**< This client's IP */
    std::string myName;  /**< This client's name */
    fds_uint32_t numChannels;  /**< Number of channels per session*/
    std::string sessionId;  /**< UUID for client's session */

    fds_uint32_t numVols;   /**< Number of objects to use */
    fds_uint32_t numObjs;   /**< Number of objects to use */
    fds_bool_t   randData;  /**< Use random data or not */
    fds_bool_t   verify;    /**< Verify data or not */
};

// ----------------------------------------------------------------------------

/**
 * Unit test thread pool syscall service, type name "thpool-syscall",
 * "thpool-boost", "thpool-math"
 */
class UT_ObjectOpcall : public JsObject
{
  public:
    virtual JsObject *js_exec_obj(JsObject *array,
                                  JsObjTemplate *templ,
                                  JsObjOutput *out);
  private:
    void parseOp(JsObject *parent, Sm_ProbeMod *probeMod);
};

class UT_SetupVols : public JsObject
{
  public:
    virtual JsObject *js_exec_obj(JsObject *array,
                                  JsObjTemplate *templ,
                                  JsObjOutput *out);
};

class UT_SmWorkloadTemplate : public JsObjTemplate
{
  public:
    virtual ~UT_SmWorkloadTemplate() {}
    explicit UT_SmWorkloadTemplate(JsObjManager *mgr) : JsObjTemplate("dp-workload", mgr)
    {
        const char *s;

        s = "object-ops";
        js_decode[s] = new JsObjStrTemplate<UT_ObjectOpcall>(s, mgr);

        s = "";
    }

    virtual JsObject *js_new(json_t *in) {
        return js_parse(new JsObject(), in, NULL);
    }
};

class UT_SmSetupTemplate : public JsObjTemplate
{
  public:
    virtual ~UT_SmSetupTemplate() {}
    explicit UT_SmSetupTemplate(JsObjManager *mgr) : JsObjTemplate("dp-setup", mgr)
    {
        const char *s;

        s = "num-vols";
        js_decode[s] = new JsObjStrTemplate<UT_SetupVols>(s, mgr);

        s = "";
    }

    virtual JsObject *js_new(json_t *in) {
        return js_parse(new UT_SetupVols(), in, NULL);
    }
};

// Adapter class
extern Sm_ProbeMod           gl_Sm_ProbeMod;

}  // namespace fds

#endif  // SOURCE_UNIT_TEST_FDS_PROBE_STOR_MGR_FULL_SM_PROBE_H_
