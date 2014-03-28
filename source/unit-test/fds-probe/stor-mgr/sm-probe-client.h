/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_UNIT_TEST_FDS_PROBE_STOR_MGR_SM_PROBE_CLIENT_H_
#define SOURCE_UNIT_TEST_FDS_PROBE_STOR_MGR_SM_PROBE_CLIENT_H_

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
    boost::shared_ptr<netSessionTbl> nsTbl;  /**< Session table */
    fds_uint32_t numChannels;  /**< Number of channels per session*/
    boost::shared_ptr<FDS_ProtocolInterface::
            FDSP_DataPathReqClient> dpClient;  /**< Data path instance */
    std::string sessionId;  /**< UUID for client's session */
    boost::shared_ptr<probeDataPathRespIf> pdpri;  /**< Data path functions */

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

class UT_ThpoolBoost : public JsObject
{
  public:
    virtual JsObject *js_exec_obj(JsObject *array,
                                  JsObjTemplate *templ,
                                  JsObjOutput *out);
};

class UT_ThpoolMath : public JsObject
{
  public:
    virtual JsObject *js_exec_obj(JsObject *array,
                                  JsObjTemplate *templ,
                                  JsObjOutput *out);
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

        s = "thpool-boost";
        js_decode[s] = new JsObjStrTemplate<UT_ThpoolBoost>(s, mgr);

        s = "thpool-math";
        js_decode[s] = new JsObjStrTemplate<UT_ThpoolMath>(s, mgr);

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

/**
 * Probe's derived class for how to handle data
 * path responses.
 */
class probeDataPathRespIf : public FDS_ProtocolInterface::FDSP_DataPathRespIf {
  public:
    probeDataPathRespIf() {
    }
    ~probeDataPathRespIf() {
    }

    void GetObjectResp(
        const FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
        const FDS_ProtocolInterface::FDSP_GetObjType& get_obj_req) {
        fds_panic("Got unhandled non-shared-ptr get object message response");
    }
    void GetObjectResp(
        boost::shared_ptr<FDS_ProtocolInterface::FDSP_MsgHdrType>& msgHdr,
        boost::shared_ptr<FDS_ProtocolInterface::FDSP_GetObjType>& getReq) {
        /*
         * TODO: May want to sanity check the other response fields.
         */
        fds_verify(msgHdr->result == FDS_ProtocolInterface::FDSP_ERR_OK);

        ObjectID oid(getReq->data_obj_id.digest);
        std::string objData = getReq->data_obj;
        fds_verify(gl_Sm_ProbeMod.checkGetObj(oid, objData) == true);
    }
    void PutObjectResp(
        const FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
        const FDS_ProtocolInterface::FDSP_PutObjType& put_obj_req) {
        fds_panic("Got unhandled non-shared-ptr put object message response");
    }
    void PutObjectResp(
        boost::shared_ptr<FDS_ProtocolInterface::FDSP_MsgHdrType>& msgHdr,
        boost::shared_ptr<FDS_ProtocolInterface::FDSP_PutObjType>& putObjReq) {
        /*
         * TODO: May want to sanity check the other response fields.
         */
        fds_verify(msgHdr->result == FDS_ProtocolInterface::FDSP_ERR_OK);
    }
    void DeleteObjectResp(
        const FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
        const FDS_ProtocolInterface::FDSP_DeleteObjType& del_obj_req) {
        fds_panic("Got unhandled non-shared-ptr delete object message response");
    }
    void DeleteObjectResp(
        boost::shared_ptr<FDS_ProtocolInterface::FDSP_MsgHdrType>& msgHdr,
        boost::shared_ptr<FDS_ProtocolInterface::FDSP_DeleteObjType>& delObjReq) {
        std::cout << "Received a delete object response" << std::endl;
        /*
         * TODO: May want to sanity check the other response fields.
         */
        fds_verify(msgHdr->result == FDS_ProtocolInterface::FDSP_ERR_OK);
    }
    void OffsetWriteObjectResp(
        const FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
        const FDS_ProtocolInterface::FDSP_OffsetWriteObjType& offset_write_obj_req) {
    }
    void OffsetWriteObjectResp(
        boost::shared_ptr<FDS_ProtocolInterface::FDSP_MsgHdrType>& fdsp_msg,
        boost::shared_ptr<FDS_ProtocolInterface::FDSP_OffsetWriteObjType>&
        offset_write_obj_req) {
    }
    void RedirReadObjectResp(
        const FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
        const FDS_ProtocolInterface::FDSP_RedirReadObjType& redir_write_obj_req) {
    }
    void RedirReadObjectResp(
        boost::shared_ptr<FDS_ProtocolInterface::FDSP_MsgHdrType>& fdsp_msg,
        boost::shared_ptr<FDS_ProtocolInterface::FDSP_RedirReadObjType>&
        redir_write_obj_req) {
    }
    void GetObjectMetadataResp(
            boost::shared_ptr<FDSP_GetObjMetadataResp>& metadata_resp) {
    }
    void GetObjectMetadataResp(const FDSP_GetObjMetadataResp& metadata_resp) {
    }
};

}  // namespace fds

#endif  // SOURCE_UNIT_TEST_FDS_PROBE_STOR_MGR_SM_PROBE_CLIENT_H_
