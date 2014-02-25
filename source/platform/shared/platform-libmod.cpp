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
}

Platform::~Platform() {}

/**
 * Platform node/cluster methods.
 */

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

/**
 * Module methods.
 */
int
Platform::mod_init(SysParams const *const param)
{
    Module::mod_init(param);

    plf_net_sess = boost::shared_ptr<netSessionTbl>(new netSessionTbl(FDSP_DATA_MGR));
    plf_rpc_reqt = boost::shared_ptr<PlatRpcReq>(plat_creat_rpc_handler());

    return 0;
}

void
Platform::mod_startup()
{
    plf_rpc_thrd = boost::shared_ptr<std::thread>(
            new std::thread(&Platform::plf_rpc_server_thread, this));
}

void
Platform::mod_shutdown()
{
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
    plf_rpc_sess = plf_net_sess->createServerSession<netControlPathServerSession>(
            netSession::ipString2Addr(netSession::getLocalIp()),
            plf_my_ctrl_port, plf_my_node_name,
            FDSP_ORCH_MGR, plf_rpc_reqt);

    plf_net_sess->listenServer(plf_rpc_sess);
}

// --------------------------------------------------------------------------------------
// RPC handlers
// --------------------------------------------------------------------------------------
PlatRpcReq::PlatRpcReq() {}
PlatRpcReq::~PlatRpcReq() {}

void
PlatRpcReq::NotifyAddVol(const FDSP_MsgHdrType    &fdsp_msg,
                         const FDSP_NotifyVolType &not_add_vol_req) {}

void
PlatRpcReq::NotifyAddVol(fpi::FDSP_MsgHdrTypePtr    &msg_hdr,
                         fpi::FDSP_NotifyVolTypePtr &msg)
{
}

void
PlatRpcReq::NotifyRmVol(const FDSP_MsgHdrType    &fdsp_msg,
                        const FDSP_NotifyVolType &not_rm_vol_req) {}

void
PlatRpcReq::NotifyRmVol(fpi::FDSP_MsgHdrTypePtr    &msg_hdr,
                        fpi::FDSP_NotifyVolTypePtr &msg)
{
}

void
PlatRpcReq::NotifyModVol(const FDSP_MsgHdrType    &fdsp_msg,
                         const FDSP_NotifyVolType &not_mod_vol_req) {}

void
PlatRpcReq::NotifyModVol(fpi::FDSP_MsgHdrTypePtr    &msg_hdr,
                         fpi::FDSP_NotifyVolTypePtr &msg)
{
}

void
PlatRpcReq::AttachVol(const FDSP_MsgHdrType    &fdsp_msg,
                      const FDSP_AttachVolType &atc_vol_req) {}

void
PlatRpcReq::AttachVol(fpi::FDSP_MsgHdrTypePtr    &msg_hdr,
                      fpi::FDSP_AttachVolTypePtr &vol_msg)
{
}

void
PlatRpcReq::DetachVol(const FDSP_MsgHdrType    &fdsp_msg,
                      const FDSP_AttachVolType &dtc_vol_req) {}

void
PlatRpcReq::DetachVol(fpi::FDSP_MsgHdrTypePtr    &msg_hdr,
                      fpi::FDSP_AttachVolTypePtr &vol_msg)
{
}

void
PlatRpcReq::NotifyNodeAdd(const FDSP_MsgHdrType     &fdsp_msg,
                          const FDSP_Node_Info_Type &node_info) {}

void
PlatRpcReq::NotifyNodeAdd(fpi::FDSP_MsgHdrTypePtr     &msg_hdr,
                          fpi::FDSP_Node_Info_TypePtr &node_info)
{
}

void
PlatRpcReq::NotifyNodeRmv(const FDSP_MsgHdrType     &fdsp_msg,
                          const FDSP_Node_Info_Type &node_info) {}

void
PlatRpcReq::NotifyNodeRmv(fpi::FDSP_MsgHdrTypePtr     &msg_hdr,
                          fpi::FDSP_Node_Info_TypePtr &node_info)
{
}

void
PlatRpcReq::NotifyDLTUpdate(const FDSP_MsgHdrType    &fdsp_msg,
                            const FDSP_DLT_Data_Type &dlt_info) {}

void
PlatRpcReq::NotifyDLTUpdate(fpi::FDSP_MsgHdrTypePtr    &msg_hdr,
                            fpi::FDSP_DLT_Data_TypePtr &dlt_info)
{
}

void
PlatRpcReq::NotifyDMTUpdate(const FDSP_MsgHdrType &msg_hdr,
                            const FDSP_DMT_Type   &dmt_info) {}

void
PlatRpcReq::NotifyDMTUpdate(fpi::FDSP_MsgHdrTypePtr &msg_hdr,  // NOLINT
                            fpi::FDSP_DMT_TypePtr   &dmt)
{
}

void
PlatRpcReq::SetThrottleLevel(const FDSP_MsgHdrType      &msg_hdr,
                             const FDSP_ThrottleMsgType &throttle_msg) {}

void
PlatRpcReq::SetThrottleLevel(fpi::FDSP_MsgHdrTypePtr      &msg_hdr,
                             fpi::FDSP_ThrottleMsgTypePtr &throttle_msg)
{
}

void
PlatRpcReq::TierPolicy(const FDSP_TierPolicy &tier) {}

void
PlatRpcReq::TierPolicy(fpi::FDSP_TierPolicyPtr &tier)  // NOLINT
{
}

void
PlatRpcReq::TierPolicyAudit(const FDSP_TierPolicyAudit &audit) {}

void
PlatRpcReq::TierPolicyAudit(fpi::FDSP_TierPolicyAuditPtr &audit)  // NOLINT
{
}

void
PlatRpcReq::NotifyBucketStats(const fpi::FDSP_MsgHdrType          &msg_hdr,
                              const fpi::FDSP_BucketStatsRespType &buck_stats_msg) {}

void
PlatRpcReq::NotifyBucketStats(fpi::FDSP_MsgHdrTypePtr          &hdr,
                              fpi::FDSP_BucketStatsRespTypePtr &msg)
{
}

void
PlatRpcReq::NotifyStartMigration(const fpi::FDSP_MsgHdrType    &msg_hdr,
                                 const fpi::FDSP_DLT_Data_Type &dlt_info) {}

void
PlatRpcReq::NotifyStartMigration(fpi::FDSP_MsgHdrTypePtr    &hdr,
                                 fpi::FDSP_DLT_Data_TypePtr &dlt)
{
}

}  // namespace fds
