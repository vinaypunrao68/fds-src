/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <string>
#include <platform/platform-lib.h>
#include <NetSession.h>

namespace fds {

Platform *gl_PlatformSvc;

// -------------------------------------------------------------------------------------
// Common Platform Services
// -------------------------------------------------------------------------------------
Platform::Platform(char const *const         name,
                   FDSP_MgrIdType            node_type,
                   DomainNodeInv::pointer    node_inv,
                   DomainClusterMap::pointer cluster,
                   DomainResources::pointer  resources,
                   OmAgent::pointer          master)
    : Module(name), plf_node_type(node_type), plf_master(master),
      plf_node_inv(node_inv), plf_clus_map(cluster), plf_resources(resources)
{
    plf_node_evt         = NULL;
    plf_vol_evt          = NULL;
    plf_throttle_evt     = NULL;
    plf_migrate_evt      = NULL;
    plf_tier_evt         = NULL;
    plf_bucket_stats_evt = NULL;

    plf_rpc_thrd  = NULL;
    plf_net_sess  = NULL;
    plf_rpc_reqt  = NULL;
    plf_my_sess   = NULL;
    plf_om_resp   = NULL;
}

Platform::~Platform()
{
    if (plf_my_sess != NULL) {
        plf_net_sess->endSession(plf_my_sess->getSessionTblKey());
    }
    if (plf_rpc_thrd != NULL) {
        plf_rpc_thrd->join();
    }
}

// -----------------------------------------------------------------------------------
// Platform node/cluster methods.
// -----------------------------------------------------------------------------------

// plf_reg_node_info
// -----------------
//
void
Platform::plf_reg_node_info(const NodeUuid &uuid, const FdspNodeRegPtr msg)
{
    NodeAgent::pointer     newNode;
    DomainNodeInv::pointer local;

    local = plf_node_inventory();
    Error err = local->dc_register_node(uuid, msg, &newNode);
}

// plf_del_node_info
// -----------------
//
void
Platform::plf_del_node_info(const NodeUuid &uuid, const std::string &name)
{
    DomainNodeInv::pointer local;

    local = plf_node_inventory();
    Error err = local->dc_unregister_node(uuid, name);
}

// plf_update_cluster
// ------------------
//
void
Platform::plf_update_cluster()
{
}

// plf_persist_inventory
// ---------------------
//
void
Platform::plf_persist_inventory(const NodeUuid &uuid)
{
}

// -----------------------------------------------------------------------------------
// Module methods.
// -----------------------------------------------------------------------------------
int
Platform::mod_init(SysParams const *const param)
{
    Module::mod_init(param);

    plf_net_sess = boost::shared_ptr<netSessionTbl>(new netSessionTbl(FDSP_PLATFORM));
    plf_rpc_reqt = boost::shared_ptr<PlatRpcReqt>(plat_creat_reqt_disp());

    return 0;
}

void
Platform::mod_startup()
{
    plf_rpc_thrd = boost::shared_ptr<std::thread>(
            new std::thread(&Platform::plf_rpc_server_thread, this));

    plf_rpc_om_handshake();
}

void
Platform::mod_shutdown()
{
}

// -----------------------------------------------------------------------------------
// RPC endpoints
// -----------------------------------------------------------------------------------

// plf_rpc_server_thread
// ---------------------
//
void
Platform::plf_rpc_server_thread()
{
    // TODO(Rao): Ideally createServerSession should take a shared pointer for
    // plf_rpc_sess.  Make sure that happens; otherwise you end up with a pointer leak.
    //
    plf_my_sess = plf_net_sess->createServerSession<netControlPathServerSession>(
            netSession::ipString2Addr(netSession::getLocalIp()),
            plf_my_ctrl_port, plf_my_node_name,
            FDSP_ORCH_MGR, plf_rpc_reqt);

    plf_net_sess->listenServer(plf_my_sess);
}

// prf_rpc_om_handshake
// --------------------
// Perform the handshake connection with OM.
//
void
Platform::plf_rpc_om_handshake()
{
    if (plf_master == NULL) {
        fds_verify(plf_om_resp == NULL);

        plf_master  = new OmAgent(0);
        plf_om_resp = boost::shared_ptr<PlatRpcResp>(plat_creat_resp_disp());
        plf_master->om_handshake(plf_net_sess, plf_om_resp,
                                 plf_om_ip_str, plf_om_conf_port);
    }
    FDSP_RegisterNodeTypePtr reg(new FDSP_RegisterNodeType);
    plf_master->init_node_reg_pkt(reg);
    plf_master->om_register_node(reg);
}

// --------------------------------------------------------------------------------------
// RPC request handlers
// --------------------------------------------------------------------------------------
PlatRpcReqt::PlatRpcReqt(const Platform *mgr) : plf_mgr(mgr) {}
PlatRpcReqt::~PlatRpcReqt() {}

void
PlatRpcReqt::NotifyAddVol(const FDSP_MsgHdrType    &fdsp_msg,
                          const FDSP_NotifyVolType &not_add_vol_req) {}

void
PlatRpcReqt::NotifyAddVol(fpi::FDSP_MsgHdrTypePtr    &msg_hdr,
                          fpi::FDSP_NotifyVolTypePtr &msg)
{
    if (plf_mgr->plf_vol_evt != NULL) {
        plf_mgr->plf_vol_evt->plat_add_vol(msg_hdr, msg);
    }
}

void
PlatRpcReqt::NotifyRmVol(const FDSP_MsgHdrType    &fdsp_msg,
                         const FDSP_NotifyVolType &not_rm_vol_req) {}

void
PlatRpcReqt::NotifyRmVol(fpi::FDSP_MsgHdrTypePtr    &msg_hdr,
                         fpi::FDSP_NotifyVolTypePtr &msg)
{
    if (plf_mgr->plf_vol_evt != NULL) {
        plf_mgr->plf_vol_evt->plat_rm_vol(msg_hdr, msg);
    }
}

void
PlatRpcReqt::NotifyModVol(const FDSP_MsgHdrType    &fdsp_msg,
                          const FDSP_NotifyVolType &not_mod_vol_req) {}

void
PlatRpcReqt::NotifyModVol(fpi::FDSP_MsgHdrTypePtr    &msg_hdr,
                          fpi::FDSP_NotifyVolTypePtr &msg)
{
    fds_verify(0);
}

void
PlatRpcReqt::AttachVol(const FDSP_MsgHdrType    &fdsp_msg,
                       const FDSP_AttachVolType &atc_vol_req) {}

void
PlatRpcReqt::AttachVol(fpi::FDSP_MsgHdrTypePtr    &msg_hdr,
                       fpi::FDSP_AttachVolTypePtr &vol_msg)
{
    fds_verify(0);
}

void
PlatRpcReqt::DetachVol(const FDSP_MsgHdrType    &fdsp_msg,
                       const FDSP_AttachVolType &dtc_vol_req) {}

void
PlatRpcReqt::DetachVol(fpi::FDSP_MsgHdrTypePtr    &msg_hdr,
                      fpi::FDSP_AttachVolTypePtr &vol_msg)
{
    fds_verify(0);
}

void
PlatRpcReqt::NotifyNodeAdd(const FDSP_MsgHdrType     &fdsp_msg,
                           const FDSP_Node_Info_Type &node_info) {}

void
PlatRpcReqt::NotifyNodeAdd(fpi::FDSP_MsgHdrTypePtr     &msg_hdr,
                           fpi::FDSP_Node_Info_TypePtr &node_info)
{
    fds_verify(0);
}

void
PlatRpcReqt::NotifyNodeActive(const FDSP_MsgHdrType     &fdsp_msg,
                              const FDSP_Node_Info_Type &node_info) {}

void
PlatRpcReqt::NotifyNodeActive(fpi::FDSP_MsgHdrTypePtr     &msg_hdr,
                              fpi::FDSP_Node_Info_TypePtr &node_info)
{
    fds_verify(0);
}

void
PlatRpcReqt::NotifyNodeRmv(const FDSP_MsgHdrType     &fdsp_msg,
                           const FDSP_Node_Info_Type &node_info) {}

void
PlatRpcReqt::NotifyNodeRmv(fpi::FDSP_MsgHdrTypePtr     &msg_hdr,
                           fpi::FDSP_Node_Info_TypePtr &node_info)
{
    fds_verify(0);
}

void
PlatRpcReqt::NotifyDLTUpdate(const FDSP_MsgHdrType    &fdsp_msg,
                             const FDSP_DLT_Data_Type &dlt_info) {}

void
PlatRpcReqt::NotifyDLTUpdate(fpi::FDSP_MsgHdrTypePtr    &msg_hdr,
                             fpi::FDSP_DLT_Data_TypePtr &dlt_info)
{
    fds_verify(0);
}

void
PlatRpcReqt::NotifyDMTUpdate(const FDSP_MsgHdrType &msg_hdr,
                             const FDSP_DMT_Type   &dmt_info) {}

void
PlatRpcReqt::NotifyDMTUpdate(fpi::FDSP_MsgHdrTypePtr &msg_hdr,  // NOLINT
                             fpi::FDSP_DMT_TypePtr   &dmt)
{
    fds_verify(0);
}

void
PlatRpcReqt::SetThrottleLevel(const FDSP_MsgHdrType      &msg_hdr,
                              const FDSP_ThrottleMsgType &throttle_msg) {}

void
PlatRpcReqt::SetThrottleLevel(fpi::FDSP_MsgHdrTypePtr      &msg_hdr,
                              fpi::FDSP_ThrottleMsgTypePtr &throttle_msg)
{
    fds_verify(0);
}

void
PlatRpcReqt::TierPolicy(const FDSP_TierPolicy &tier) {}

void
PlatRpcReqt::TierPolicy(fpi::FDSP_TierPolicyPtr &tier)  // NOLINT
{
    fds_verify(0);
}

void
PlatRpcReqt::TierPolicyAudit(const FDSP_TierPolicyAudit &audit) {}

void
PlatRpcReqt::TierPolicyAudit(fpi::FDSP_TierPolicyAuditPtr &audit)  // NOLINT
{
    fds_verify(0);
}

void
PlatRpcReqt::NotifyBucketStats(const fpi::FDSP_MsgHdrType          &msg_hdr,
                               const fpi::FDSP_BucketStatsRespType &buck_stats_msg) {}

void
PlatRpcReqt::NotifyBucketStats(fpi::FDSP_MsgHdrTypePtr          &hdr,
                               fpi::FDSP_BucketStatsRespTypePtr &msg)
{
    fds_verify(0);
}

void
PlatRpcReqt::NotifyStartMigration(const fpi::FDSP_MsgHdrType    &msg_hdr,
                                  const fpi::FDSP_DLT_Data_Type &dlt_info) {}

void
PlatRpcReqt::NotifyStartMigration(fpi::FDSP_MsgHdrTypePtr    &hdr,
                                  fpi::FDSP_DLT_Data_TypePtr &dlt)
{
    fds_verify(0);
}

// --------------------------------------------------------------------------------------
// RPC response handlers
// --------------------------------------------------------------------------------------
PlatRpcResp::PlatRpcResp(const Platform *mgr) : plf_mgr(mgr) {}
PlatRpcResp::~PlatRpcResp() {}

void
PlatRpcResp::CreateBucketResp(const FDSP_MsgHdrType      &fdsp_msg,
                              const FDSP_CreateVolType   &crt_buck_rsp) {}

void
PlatRpcResp::CreateBucketResp(fpi::FDSP_MsgHdrTypePtr    &fdsp_msg,
                              fpi::FDSP_CreateVolTypePtr &crt_buck_rsp)
{
    fds_verify(0);
}

void
PlatRpcResp::DeleteBucketResp(const FDSP_MsgHdrType      &fdsp_msg,
                              const FDSP_DeleteVolType   &del_buck_rsp) {}

void
PlatRpcResp::DeleteBucketResp(fpi::FDSP_MsgHdrTypePtr    &fdsp_msg,
                              fpi::FDSP_DeleteVolTypePtr &del_buck_rsp)
{
    fds_verify(0);
}

void
PlatRpcResp::ModifyBucketResp(const FDSP_MsgHdrType      &fdsp_msg,
                              const FDSP_ModifyVolType   &mod_buck_rsp) {}

void
PlatRpcResp::ModifyBucketResp(fpi::FDSP_MsgHdrTypePtr    &fdsp_msg,
                              fpi::FDSP_ModifyVolTypePtr &mod_buck_rsp)
{
    fds_verify(0);
}

void
PlatRpcResp::AttachBucketResp(const FDSP_MsgHdrType         &fdsp_msg,
                              const FDSP_AttachVolCmdType   &atc_buck_req) {}

void
PlatRpcResp::AttachBucketResp(fpi::FDSP_MsgHdrTypePtr       &fdsp_msg,
                              fpi::FDSP_AttachVolCmdTypePtr &atc_buck_req)
{
    fds_verify(0);
}

void
PlatRpcResp::RegisterNodeResp(const FDSP_MsgHdrType         &fdsp_msg,
                              const FDSP_RegisterNodeType   &reg_node_rsp) {}

void
PlatRpcResp::RegisterNodeResp(fpi::FDSP_MsgHdrTypePtr       &fdsp_msg,
                              fpi::FDSP_RegisterNodeTypePtr &reg_node_rsp)
{
    fds_verify(0);
}

void
PlatRpcResp::NotifyQueueFullResp(const FDSP_MsgHdrType           &fdsp_msg,
                                 const FDSP_NotifyQueueStateType &queue_state_rsp) {}

void
PlatRpcResp::NotifyQueueFullResp(fpi::FDSP_MsgHdrTypePtr           &fdsp_msg,
                                 fpi::FDSP_NotifyQueueStateTypePtr &queue_state_rsp)
{
    fds_verify(0);
}

void
PlatRpcResp::NotifyPerfstatsResp(const FDSP_MsgHdrType    &fdsp_msg,
                                 const FDSP_PerfstatsType &perf_stats_rsp) {}

void
PlatRpcResp::NotifyPerfstatsResp(fpi::FDSP_MsgHdrTypePtr    &fdsp_msg,
                                 fpi::FDSP_PerfstatsTypePtr &perf_stats_rsp)
{
    fds_verify(0);
}

void
PlatRpcResp::TestBucketResp(const FDSP_MsgHdrType    &fdsp_msg,
                            const FDSP_TestBucket    &test_buck_rsp) {}

void
PlatRpcResp::TestBucketResp(fpi::FDSP_MsgHdrTypePtr  &fdsp_msg,
                            fpi::FDSP_TestBucketPtr  &test_buck_rsp)
{
    fds_verify(0);
}

void
PlatRpcResp::GetDomainStatsResp(const FDSP_MsgHdrType           &fdsp_msg,
                                const FDSP_GetDomainStatsType   &get_stats_rsp) {}

void
PlatRpcResp::GetDomainStatsResp(fpi::FDSP_MsgHdrTypePtr         &fdsp_msg,
                                fpi::FDSP_GetDomainStatsTypePtr &get_stats_rsp)
{
    fds_verify(0);
}

void
PlatRpcResp::MigrationDoneResp(const FDSP_MsgHdrType            &fdsp_msg,
                               const FDSP_MigrationStatusType   &status_resp) {}

void
PlatRpcResp::MigrationDoneResp(fpi::FDSP_MsgHdrTypePtr          &fdsp_msg,
                               fpi::FDSP_MigrationStatusTypePtr &status_resp)
{
    fds_verify(0);
}

}  // namespace fds
