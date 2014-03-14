/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
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

    plf_om_ctrl_port     = 0;
    plf_my_ctrl_port     = 0;
    plf_my_conf_port     = 0;
    plf_my_data_port     = 0;
    plf_my_migr_port     = 0;

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

// prf_rpc_om_handshake
// --------------------
// Perform the handshake connection with OM.
//
void
Platform::plf_rpc_om_handshake(fpi::FDSP_RegisterNodeTypePtr reg)
{
    if (plf_master == NULL) {
        fds_verify(plf_om_resp == NULL);

        plf_master  = new OmAgent(0);
        plf_om_resp = boost::shared_ptr<PlatRpcResp>(plat_creat_resp_disp());
        plf_master->om_handshake(plf_net_sess, plf_om_resp,
                                 plf_om_ip_str, plf_om_ctrl_port);
    }
    plf_master->init_node_reg_pkt(reg);
    plf_master->om_register_node(reg);
}

// plf_run_server
// --------------
//
void
Platform::plf_run_server(bool spawn_thr)
{
    if (spawn_thr == true) {
        plf_rpc_thrd = boost::shared_ptr<std::thread>(
               new std::thread(&Platform::plf_rpc_server_thread, this));
    } else {
        plf_rpc_thrd = NULL;
        plf_rpc_server_thread();
    }
}

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

// plf_change_info
// ---------------
//
void
Platform::plf_change_info(const plat_node_data_t *ndata)
{
    char         name[64];
    NodeUuid     uuid(ndata->nd_node_uuid);
    fds_uint32_t base;

    if (ndata->nd_node_uuid == 0) {
        // TODO(Vy): we need to think about if AM needs to persist its own node/service
        // uuid or can it generate only the fly during its bootstrap process.
        // When we're here and we don't have uuid, this must be AM.
        ///
        fds_verify(plf_node_type == FDSP_STOR_HVISOR);
        plf_my_uuid = NodeUuid(fds_get_uuid64(get_uuid()));
    } else {
        plf_my_uuid = uuid;
    }
    Platform::plf_svc_uuid_from_node(plf_my_uuid, &plf_my_svc_uuid, plf_node_type);
    // snprintf(name, sizeof(name), "node-%u", ndata->nd_node_number);
    // plf_my_node_name.assign(name);

    base = PlatformProcess::
        plf_get_platform_port(ndata->nd_plat_port, ndata->nd_node_number);

    if (base == 0) {
        return;
    }
    switch (plf_node_type) {
        case FDSP_STOR_MGR:
            base = PlatformProcess::plf_get_sm_port(base);
            break;

        case FDSP_DATA_MGR:
            base = PlatformProcess::plf_get_dm_port(base);
            break;

        case FDSP_STOR_HVISOR:
            base = PlatformProcess::plf_get_am_port(base);
            break;

        case FDSP_ORCH_MGR:
            return;

        default:
            break;
    }
    plf_om_ctrl_port = ndata->nd_om_port;
    plf_my_ctrl_port = plf_ctrl_port(base);
    plf_my_conf_port = plf_conf_port(base);
    plf_my_data_port = plf_data_port(base);
    plf_my_migr_port = plf_migration_port(base);

    LOGNOTIFY << "My ctrl port " << std::dec << plf_my_ctrl_port << std::endl
              << "My conf port " << plf_my_conf_port << std::endl
              << "My data port " << plf_my_data_port << std::endl
              << "My migr port " << plf_my_migr_port << std::endl
              << "My node uuid " << std::hex << plf_my_uuid.uuid_get_val() << std::endl
              << "My OM port   " << std::dec << plf_om_ctrl_port << std::endl
              << "My OM IP     " << plf_om_ip_str << std::endl;
}

// plf_svc_uuid_from_node
// ----------------------
// Simple formula to derrive service uuid from node uuid.
//
void
Platform::plf_svc_uuid_from_node(const NodeUuid &node,
                                 NodeUuid       *svc,
                                 FDSP_MgrIdType  type)
{
    if (type == FDSP_PLATFORM) {
        svc->uuid_set_val(node.uuid_get_val());
    } else {
        svc->uuid_set_val(node.uuid_get_val() + 1 + type);
    }
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
}

void
Platform::mod_shutdown()
{
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
                              const FDSP_ActivateNodeType &act_node_req) {}

void
PlatRpcReqt::NotifyNodeActive(fpi::FDSP_MsgHdrTypePtr     &msg_hdr,
                              fpi::FDSP_ActivateNodeTypePtr &act_node_req)
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

namespace util {
/**
 * @return local ip
 */
std::string get_local_ip()
{
    struct ifaddrs *ifAddrStruct = NULL;
    struct ifaddrs *ifa          = NULL;
    void   *tmpAddrPtr           = NULL;
    std::string myIp;

    /*
     * Get the local IP of the host.  This is needed by the OM.
     */
    getifaddrs(&ifAddrStruct);
    for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr->sa_family == AF_INET) {  // IPv4
            if (strncmp(ifa->ifa_name, "lo", 2) != 0) {
                tmpAddrPtr = &((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;  // NOLINT
                char addrBuf[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, tmpAddrPtr, addrBuf, INET_ADDRSTRLEN);
                myIp = std::string(addrBuf);
                if (myIp.find("10.1") != std::string::npos) {
                    break; /*  TODO: more dynamic */
                }
            }
        }
    }
    if (ifAddrStruct != NULL) {
        freeifaddrs(ifAddrStruct);
    }
    return myIp;
}

}  // namespace util
}  // namespace fds
