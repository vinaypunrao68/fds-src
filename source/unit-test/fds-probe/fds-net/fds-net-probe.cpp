/*
 * Copyright 2013 Formation Data Systems, Inc.
 *
 */
#include <fds_process.h>
#include <NetSession.h>

#include <fds-net-probe.h>
#include <string>
#include <iostream>
#include <list>
#include <fds_assert.h>

namespace fds {

class exampleDataPathRespIf : public FDS_ProtocolInterface::FDSP_DataPathRespIf {
  public:
    exampleDataPathRespIf() {
    }
    ~exampleDataPathRespIf() {
    }

    void GetObjectResp(
        const FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
        const FDS_ProtocolInterface::FDSP_GetObjType& get_obj_req) {
        std::cout << "Got a non-shared-ptr get object message response" << std::endl;
    }
    void GetObjectResp(
        boost::shared_ptr<FDS_ProtocolInterface::FDSP_MsgHdrType>& fdsp_msg,
        boost::shared_ptr<FDS_ProtocolInterface::FDSP_GetObjType>& get_obj_req) {
        std::cout << "Got a get object message response" << std::endl;
    }
    void PutObjectResp(
        const FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
        const FDS_ProtocolInterface::FDSP_PutObjType& put_obj_req) {
        std::cout << "Got a non-shared-ptr put object message response" << std::endl;
    }
    void PutObjectResp(
        boost::shared_ptr<FDS_ProtocolInterface::FDSP_MsgHdrType>& fdsp_msg,
        boost::shared_ptr<FDS_ProtocolInterface::FDSP_PutObjType>& put_obj_req) {
        std::cout << "Got a put object message response" << std::endl;
    }
    void DeleteObjectResp(
        const FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
        const FDS_ProtocolInterface::FDSP_DeleteObjType& del_obj_req) {
    }
    void DeleteObjectResp(
        boost::shared_ptr<FDS_ProtocolInterface::FDSP_MsgHdrType>& fdsp_msg,
        boost::shared_ptr<FDS_ProtocolInterface::FDSP_DeleteObjType>& del_obj_req) {
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

static probe_mod_param_t fds_net_probe_param =
{
    .pr_stat_cnt     = 0,
    .pr_inj_pts_cnt  = 0,
    .pr_inj_act_cnt  = 0,
    .pr_max_sec_tout = 0
};

fdsNetProbeMod gl_fdsNetProbeMod("FDS Net Probe Adapter",
                                 &fds_net_probe_param, nullptr);

fdsNetProbeMod::fdsNetProbeMod(char const *const name,
                               probe_mod_param_t *param,
                               Module *owner)
        : ProbeMod(name, param, owner),
          remoteIp("127.0.0.1"),
          localIp("127.0.0.1"),
          numChannels(1) {
}

// pr_new_instance
// ---------------
//
fdsNetProbeMod *
fdsNetProbeMod::pr_new_instance()
{
    fdsNetProbeMod *adapter =
            new fdsNetProbeMod("Probe Adapter Inst",
                               &fds_net_probe_param,
                               nullptr);

    adapter->mod_init(mod_params);
    adapter->mod_startup();
    return adapter;
}

// pr_intercept_request
// --------------------
//
void
fdsNetProbeMod::pr_intercept_request(ProbeRequest *req)
{
}

// pr_put
// ------
//
void
fdsNetProbeMod::pr_put(ProbeRequest *req)
{
    ProbeIORequest *io;

    io = static_cast<ProbeIORequest *>(req);

    std::cout << "Sending a put()" << std::endl;
    boost::shared_ptr<FDSP_MsgHdrType> fdspMsg =
            boost::shared_ptr<FDSP_MsgHdrType>(new FDSP_MsgHdrType());
    fdspMsg->src_node_name = sid;
    fdspMsg->src_port      = 0;
    boost::shared_ptr<FDSP_PutObjType> putObjReq =
            boost::shared_ptr<FDSP_PutObjType>(new FDSP_PutObjType());
    dpClient->PutObject(fdspMsg, putObjReq);
}

// pr_get
// ------
//
void
fdsNetProbeMod::pr_get(ProbeRequest *req)
{
    ProbeIORequest  *io;

    io = static_cast<ProbeIORequest *>(req);

    std::cout << "Sending a get()" << std::endl;
    boost::shared_ptr<FDSP_MsgHdrType> fdspMsg =
            boost::shared_ptr<FDSP_MsgHdrType>(new FDSP_MsgHdrType());
    fdspMsg->src_node_name = sid;
    fdspMsg->src_port      = 0;
    boost::shared_ptr<FDSP_GetObjType> getObjReq =
            boost::shared_ptr<FDSP_GetObjType>(new FDSP_GetObjType());
    dpClient->GetObject(fdspMsg, getObjReq);
}

// pr_delete
// ---------
//
void
fdsNetProbeMod::pr_delete(ProbeRequest *req)
{
}

// pr_verify_request
// -----------------
//
void
fdsNetProbeMod::pr_verify_request(ProbeRequest *req)
{
}

// pr_gen_report
// -------------
//
void
fdsNetProbeMod::pr_gen_report(std::string *out)
{
}

// mod_init
// --------
//
int
fdsNetProbeMod::mod_init(SysParams const *const param)
{
    Module::mod_init(param);

    amTbl = boost::shared_ptr<netSessionTbl>(new netSessionTbl(localIp,
                                                               0,
                                                               7777,
                                                               10,
                                                               FDSP_STOR_HVISOR));

    edpri.reset(new exampleDataPathRespIf());
    return 0;
}

// mod_startup
// -----------
//
void
fdsNetProbeMod::mod_startup()
{
    std::cout << "Doing startup" << std::endl;

    /*
     * Connect to a storage manager. This pointer
     * is manager internall by the
     */
    netDataPathClientSession* dpSession = amTbl->\
                                          startSession<netDataPathClientSession>(
                                              remoteIp,
                                              8888,
                                              FDSP_STOR_MGR,
                                              numChannels,
                                              edpri);
    dpClient = dpSession->getClient();  // NOLINT
    sid = dpSession->getSessionId();  // NOLINT
}

// mod_shutdown
// ------------
//
void
fdsNetProbeMod::mod_shutdown()
{
    // if (rpc_client != NULL) {
    // rpc_trans->close();
    //  delete rpc_client;
    // }

    // End any sessions here
    amTbl->endAllSessions();
}

}  // namespace fds
