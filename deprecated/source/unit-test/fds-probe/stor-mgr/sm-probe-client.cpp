/*
 * Copyright 2013 Formation Data Systems, Inc.
 *
 * Template to write probe adapter.  Replace Thrpool with your namespace.
 */
#include <sm-probe-client.h>
#include <string>
#include <list>
#include <utest-types.h>

namespace fds {

probe_mod_param_t Sm_probe_param =
{
    .pr_stat_cnt     = 0,
    .pr_inj_pts_cnt  = 0,
    .pr_inj_act_cnt  = 0,
    .pr_max_sec_tout = 0
};

Sm_ProbeMod gl_Sm_ProbeMod("Storage Manager Client Probe Adapter",
                           &Sm_probe_param, nullptr);

/**
 * Global map of written objects used for verification.
 * This is global so that all probemod objects can access
 * the same map. When a response is received, we don't know
 * which probemod sent it.
 */
std::unordered_map<ObjectID, std::string, ObjectHash> writtenObjs;
fds_mutex objMapLock("Written object map lock");

// pr_new_instance
// ---------------
//
ProbeMod *
Sm_ProbeMod::pr_new_instance()
{
    Sm_ProbeMod *spm = new Sm_ProbeMod("Sm Client Inst", &Sm_probe_param, NULL);
    spm->mod_init(mod_params);
    spm->mod_startup();

    return spm;
}

// pr_intercept_request
// --------------------
//
void
Sm_ProbeMod::pr_intercept_request(ProbeRequest *req)
{
}

// pr_put
// ------
//
void
Sm_ProbeMod::pr_put(ProbeRequest *probe)
{
}

void
Sm_ProbeMod::sendPut(const PutParams &putReq)
{
    boost::shared_ptr<FDSP_MsgHdrType> fdspMsg =
            boost::shared_ptr<FDSP_MsgHdrType>(new FDSP_MsgHdrType());
    fdspMsg->msg_code = FDS_ProtocolInterface::FDSP_MSG_PUT_OBJ_REQ;
    fdspMsg->src_id   = FDS_ProtocolInterface::FDSP_STOR_HVISOR;
    fdspMsg->dst_id   = FDS_ProtocolInterface::FDSP_STOR_MGR;
    fdspMsg->result   = FDS_ProtocolInterface::FDSP_ERR_OK;
    fdspMsg->err_code = FDS_ProtocolInterface::FDSP_ERR_SM_NO_SPACE;
    fdspMsg->src_node_name = myName;
    fdspMsg->session_uuid  = sessionId;
    fdspMsg->src_port       = 0;  // Don't know port at the moment
    fdspMsg->glob_volume_id = putReq.volId;
    fdspMsg->num_objects    = 1;

    boost::shared_ptr<FDSP_PutObjType> putObjReq =
            boost::shared_ptr<FDSP_PutObjType>(new FDSP_PutObjType());
    putObjReq->volume_offset         = 0;  // Put() doesn't need this...
    putObjReq->data_obj_id.digest =
      std::string((const char *)putReq.objId.GetId(), (size_t)putReq.objId.GetLen());
//    putObjReq->data_obj_id.hash_high = putReq.objId.GetHigh();
//    putObjReq->data_obj_id.hash_low  = putReq.objId.GetLow();
    putObjReq->data_obj_len          = putReq.dataLen;
    putObjReq->data_obj              = putReq.objData;

    updatePutObj(putReq.objId, putReq.objData);
    dpClient->PutObject(fdspMsg, putObjReq);
}

// pr_get
// ------
//
void
Sm_ProbeMod::pr_get(ProbeRequest *req)
{
}

void
Sm_ProbeMod::sendGet(const GetParams &getReq)
{
    boost::shared_ptr<FDSP_MsgHdrType> fdspMsg =
            boost::shared_ptr<FDSP_MsgHdrType>(new FDSP_MsgHdrType());
    fdspMsg->msg_code = FDS_ProtocolInterface::FDSP_MSG_GET_OBJ_REQ;
    fdspMsg->src_id   = FDS_ProtocolInterface::FDSP_STOR_HVISOR;
    fdspMsg->dst_id   = FDS_ProtocolInterface::FDSP_STOR_MGR;
    fdspMsg->result   = FDS_ProtocolInterface::FDSP_ERR_OK;
    fdspMsg->err_code = FDS_ProtocolInterface::FDSP_ERR_SM_NO_SPACE;
    fdspMsg->src_node_name = myName;
    fdspMsg->session_uuid  = sessionId;
    fdspMsg->src_port       = 0;  // Don't know port at the moment
    fdspMsg->glob_volume_id = getReq.volId;

    FDS_ProtocolInterface::FDSP_GetObjTypePtr getObjReq(
          new FDS_ProtocolInterface::FDSP_GetObjType);
    getObjReq->data_obj_id.digest =
       std::string((const char *)getReq.objId.GetId(), (size_t)getReq.objId.GetLen());
//    getObjReq->data_obj_id.hash_high = getReq.objId.GetHigh();
//    getObjReq->data_obj_id.hash_low  = getReq.objId.GetLow();
    getObjReq->data_obj_len          = 0;  // Get() doesn't need this...

    dpClient->GetObject(fdspMsg, getObjReq);
}

// pr_delete
// ---------
//
void
Sm_ProbeMod::pr_delete(ProbeRequest *req)
{
}

void
Sm_ProbeMod::sendDelete(const DeleteParams &delReq)
{
    FDS_ProtocolInterface::FDSP_MsgHdrTypePtr fdspMsg(
        new FDS_ProtocolInterface::FDSP_MsgHdrType);
    fdspMsg->msg_code = FDS_ProtocolInterface::FDSP_MSG_GET_OBJ_REQ;
    fdspMsg->src_id   = FDS_ProtocolInterface::FDSP_STOR_HVISOR;
    fdspMsg->dst_id   = FDS_ProtocolInterface::FDSP_STOR_MGR;
    fdspMsg->result   = FDS_ProtocolInterface::FDSP_ERR_OK;
    fdspMsg->err_code = FDS_ProtocolInterface::FDSP_ERR_SM_NO_SPACE;
    fdspMsg->src_node_name = myName;
    fdspMsg->session_uuid  = sessionId;
    fdspMsg->src_port       = 0;  // Don't know port at the moment
    fdspMsg->glob_volume_id = delReq.volId;

    FDS_ProtocolInterface::FDSP_DeleteObjTypePtr delObjReq(
          new FDS_ProtocolInterface::FDSP_DeleteObjType);
    delObjReq->data_obj_id.digest =
         std::string((const char *)delReq.objId.GetId(), (size_t)delReq.objId.GetLen());
//    delObjReq->data_obj_id.hash_high = delReq.objId.GetHigh();
//    delObjReq->data_obj_id.hash_low  = delReq.objId.GetLow();
    delObjReq->data_obj_len          = 0;  // Delete() doesn't need this...

    dpClient->DeleteObject(fdspMsg, delObjReq);
}

// pr_verify_request
// -----------------
//
void
Sm_ProbeMod::pr_verify_request(ProbeRequest *req)
{
}

// pr_gen_report
// -------------
//
void
Sm_ProbeMod::pr_gen_report(std::string *out)
{
}

// mod_init
// --------
//
int
Sm_ProbeMod::mod_init(SysParams const *const param)
{
    Module::mod_init(param);

    // Create session table
    nsTbl = boost::shared_ptr<netSessionTbl>(new netSessionTbl(myIp,
                                                               0,
                                                               7777,
                                                               10,
                                                               FDSP_STOR_HVISOR));
    // Create interface instance
    pdpri.reset(new probeDataPathRespIf());

    // Init session with SM server
    netDataPathClientSession *dpSession = nsTbl->\
                                          startSession<netDataPathClientSession>(
                                              smIp,
                                              6902,
                                              FDSP_STOR_MGR,
                                              numChannels,
                                              pdpri);
    // Get the interface for this session
    dpClient = dpSession->getClient();  // NOLINT

    // Get the ID for the session
    sessionId = dpSession->getSessionId();  // NOLINT

    return 0;
}

// mod_startup
// -----------
//
void
Sm_ProbeMod::mod_startup()
{
    JsObjManager  *mgr;
    JsObjTemplate *svc;

    if (pr_parent != NULL) {
        mgr = pr_parent->pr_get_obj_mgr();
        svc = mgr->js_get_template("Run-Input");
        svc = svc->js_get_template("Server-Load");
        svc->js_register_template(new UT_SmSetupTemplate(mgr));
        svc->js_register_template(new UT_SmWorkloadTemplate(mgr));
    }
}

// mod_shutdown
// ------------
//
void
Sm_ProbeMod::mod_shutdown()
{
    // End all sessions
    nsTbl->endAllSessions();
}

void
Sm_ProbeMod::setTestParams(const TestParams &p) {
    numVols  = p.numVols;
    numObjs  = p.numObjs;
    randData = p.randData;
    verify   = p.verify;
}

fds_bool_t
Sm_ProbeMod::checkGetObj(const ObjectID& oid,
                         const std::string& objData) {
    objMapLock.lock();
    if (objData != writtenObjs[oid]) {
        std::cout << "Failed get correct object! For object "
                  << oid << " Got ["
                  << objData << "] but expected ["
                  << writtenObjs[oid] << "]";
        objMapLock.unlock();
        return false;
    }
    std::cout << "Get check for object " << oid
              << ": SUCCESS" << std::endl;
    objMapLock.unlock();
    return true;
}

void
Sm_ProbeMod::updatePutObj(const ObjectID& oid,
                          const std::string& objData) {
    objMapLock.lock();
    /*
     * Note that this overwrites whatever was previously
     * cached at that oid.
     */
    writtenObjs[oid] = objData;
    std::cout << "Put object " << oid << " into map" << std::endl;
    objMapLock.unlock();
}

// ----------------------------------------------------------------------------
// Sm client control path
// ----------------------------------------------------------------------------
void
UT_ObjectOpcall::parseOp(JsObject *parent, Sm_ProbeMod *probeMod)
{
    JsObject *obj;

    obj = (*parent)[0];
    std::string op = static_cast<char *>(obj->js_pod_object());

    if (op == "put") {
        Sm_ProbeMod::PutParams p;
        obj = (*parent)[1];
        p.volId = strtoul((static_cast<char *>(obj->js_pod_object())), NULL, 10);

        std::string objIdHexStr;
        obj = (*parent)[2];
        objIdHexStr = static_cast<char *>(obj->js_pod_object());
        p.objId = objIdHexStr;

        obj = (*parent)[3];
        p.dataLen = strtoul(static_cast<char *>(obj->js_pod_object()), NULL, 10);

        obj = (*parent)[4];
        p.objData = static_cast<char *>(obj->js_pod_object());

        probeMod->sendPut(p);
    } else if (op == "get") {
        Sm_ProbeMod::GetParams g;
        obj = (*parent)[1];
        g.volId = strtoul((static_cast<char *>(obj->js_pod_object())), NULL, 10);

        std::string objIdHexStr;
        obj = (*parent)[2];
        objIdHexStr = static_cast<char *>(obj->js_pod_object());
        g.objId = objIdHexStr;

        probeMod->sendGet(g);
    } else if (op == "delete") {
        Sm_ProbeMod::DeleteParams d;
        obj = (*parent)[1];
        d.volId = strtoul((static_cast<char *>(obj->js_pod_object())), NULL, 10);

        std::string objIdHexStr;
        obj = (*parent)[2];
        objIdHexStr = static_cast<char *>(obj->js_pod_object());
        d.objId = objIdHexStr;

        probeMod->sendDelete(d);
    } else {
        fds_panic("Unknown object operation in json");
    }
}

JsObject *
UT_ObjectOpcall::js_exec_obj(JsObject *parent,
                             JsObjTemplate *templ,
                             JsObjOutput *out)
{
    fds_uint32_t numElems;

    parseOp(parent, static_cast<Sm_ProbeMod *>(out->js_get_context()));

    return JsObject::js_exec_obj(this, templ, out);
}

JsObject *
UT_ThpoolBoost::js_exec_obj(JsObject *parent,
                            JsObjTemplate *templ,
                            JsObjOutput *out)
{
    std::cout << "Thread boost args is called... " << std::endl;
    return JsObject::js_exec_obj(this, templ, out);
}

JsObject *
UT_ThpoolMath::js_exec_obj(JsObject *parent,
                           JsObjTemplate *templ,
                           JsObjOutput *out)
{
    std::cout << "Thread pool math is called... " << std::endl;
    return JsObject::js_exec_obj(this, templ, out);
}

JsObject *
UT_SetupVols::js_exec_obj(JsObject *parent,
                          JsObjTemplate *templ,
                          JsObjOutput *out)
{
    Sm_ProbeMod::TestParams p;
    fds_uint32_t result = json_unpack(js_data, "{s:i, s:i, s:b, s:b}",
                                      "num-vols",  &p.numVols,
                                      "num-objs",  &p.numObjs,
                                      "rand-data", &p.randData,
                                      "verify",    &p.verify);
    fds_verify(result == 0);

    std::cout << "Setting test paramters " << std::endl
              << "Number of volumes is " << p.numVols << std:: endl
              << "Number of object is " << p.numObjs << std::endl
              << "Data is " << (p.randData == true ? "random" : "fixed") << std::endl
              << "Verification is " << (p.verify == true ? "on" : "off") << std::endl;

    gl_Sm_ProbeMod.setTestParams(p);
    return JsObject::js_exec_obj(this, templ, out);
}

}  // namespace fds
