/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <sm-platform.h>
#include <fds_process.h>

namespace fds {

SmPlatform gl_SmPlatform;

// -------------------------------------------------------------------------------------
// SM Platform Event Handler
// -------------------------------------------------------------------------------------
void
SmVolEvent::plat_evt_handler(const FDSP_MsgHdrTypePtr &hdr)
{
}

// -------------------------------------------------------------------------------------
// DM Specific Platform
// -------------------------------------------------------------------------------------
SmPlatform::SmPlatform()
    : Platform("SM-Platform",
               FDSP_STOR_MGR,
               new DomainNodeInv("SM-Platform-NodeInv",
                                 NULL,
                                 new SmContainer(FDSP_STOR_MGR),
                                 new DmContainer(FDSP_STOR_MGR),
                                 new AmContainer(FDSP_STOR_MGR),
                                 new PmContainer(FDSP_STOR_MGR),
                                 new OmContainer(FDSP_STOR_MGR)),
               new DomainClusterMap("SM-Platform-ClusMap",
                                 NULL,
                                 new SmContainer(FDSP_STOR_MGR),
                                 new DmContainer(FDSP_STOR_MGR),
                                 new AmContainer(FDSP_STOR_MGR),
                                 new PmContainer(FDSP_STOR_MGR),
                                 new OmContainer(FDSP_STOR_MGR)),
               new DomainResources("DM-Resources"),
               NULL)
{
    Platform::platf_assign_singleton(&gl_SmPlatform);
    plf_node_type  = FDSP_STOR_MGR;
}

int
SmPlatform::mod_init(SysParams const *const param)
{
    FdsConfigAccessor conf(g_fdsprocess->get_conf_helper());

    Platform::mod_init(param);

    plf_om_ip_str    = conf.get_abs<std::string>("fds.sm.om_ip");
    plf_my_ip        = util::get_local_ip();
    plf_my_node_name = plf_my_ip;

    plf_vol_evt  = new SmVolEvent(plf_resources, plf_clus_map, this);
    plf_node_evt = new NodePlatEvent(plf_resources, plf_clus_map, this);

    return 0;
}

void
SmPlatform::mod_startup()
{
}

void
SmPlatform::mod_shutdown()
{
}

// plf_reg_node_info
// -----------------
//
void
SmPlatform::plf_reg_node_info(const NodeUuid &uuid, const FdspNodeRegPtr msg)
{
    NodeAgent::pointer     new_node;
    DomainNodeInv::pointer local;

    local = plf_node_inventory();
    Error err = local->dc_register_node(uuid, msg, &new_node);
    fds_verify(err == ERR_OK);

    AgentContainer::pointer svc = local->dc_container_frm_msg(msg->node_type);
    svc->agent_handshake(plf_net_sess, plf_dpath_resp, new_node);
}

PlatRpcReqt *
SmPlatform::plat_creat_reqt_disp()
{
    return new SmRpcReq(this);
}

PlatRpcResp *
SmPlatform::plat_creat_resp_disp()
{
    return new PlatRpcResp(this);
}

PlatDataPathResp *
SmPlatform::plat_creat_dpath_resp()
{
    return new PlatDataPathResp(this);
}

// --------------------------------------------------------------------------------------
// RPC handlers
// --------------------------------------------------------------------------------------
SmRpcReq::SmRpcReq(const Platform *plf) : PlatRpcReqt(plf) {}
SmRpcReq::~SmRpcReq() {}

void
SmRpcReq::NotifyAddVol(fpi::FDSP_MsgHdrTypePtr    &msg_hdr,
                       fpi::FDSP_NotifyVolTypePtr &msg)
{
}

void
SmRpcReq::NotifyRmVol(fpi::FDSP_MsgHdrTypePtr    &msg_hdr,
                      fpi::FDSP_NotifyVolTypePtr &msg)
{
}

void
SmRpcReq::NotifyModVol(fpi::FDSP_MsgHdrTypePtr    &msg_hdr,
                       fpi::FDSP_NotifyVolTypePtr &msg)
{
}

void
SmRpcReq::AttachVol(fpi::FDSP_MsgHdrTypePtr    &msg_hdr,
                    fpi::FDSP_AttachVolTypePtr &vol_msg)
{
}

void
SmRpcReq::DetachVol(fpi::FDSP_MsgHdrTypePtr    &msg_hdr,
                    fpi::FDSP_AttachVolTypePtr &vol_msg)
{
}

void
SmRpcReq::NotifyNodeAdd(fpi::FDSP_MsgHdrTypePtr     &msg_hdr,
                        fpi::FDSP_Node_Info_TypePtr &node_info)
{
}

void
SmRpcReq::NotifyNodeRmv(fpi::FDSP_MsgHdrTypePtr     &msg_hdr,
                        fpi::FDSP_Node_Info_TypePtr &node_info)
{
}

void
SmRpcReq::NotifyDLTUpdate(fpi::FDSP_MsgHdrTypePtr    &msg_hdr,
                          fpi::FDSP_DLT_Data_TypePtr &dlt_info)
{
}

void
SmRpcReq::NotifyDMTUpdate(fpi::FDSP_MsgHdrTypePtr &msg_hdr,  // NOLINT
                          fpi::FDSP_DMT_TypePtr   &dmt)
{
}

void
SmRpcReq::NotifyStartMigration(fpi::FDSP_MsgHdrTypePtr    &hdr,
                               fpi::FDSP_DLT_Data_TypePtr &dlt)
{
}

}  // namespace fds
