/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <string>
#include <am-platform.h>

namespace fds {

AmPlatform gl_AmPlatform;

// -------------------------------------------------------------------------------------
// DM Platform Event Handler
// -------------------------------------------------------------------------------------
void
AmVolEvent::plat_evt_handler(const FDSP_MsgHdrTypePtr &hdr)
{
}

// -------------------------------------------------------------------------------------
// DM Specific Platform
// -------------------------------------------------------------------------------------
AmPlatform::AmPlatform()
    : Platform("DM-Platform",
               FDSP_DATA_MGR,
               new DomainNodeInv("AM-Platform-NodeInv",
                                 NULL,
                                 new SmContainer(FDSP_STOR_HVISOR),
                                 new DmContainer(FDSP_STOR_HVISOR),
                                 new AmContainer(FDSP_STOR_HVISOR),
                                 new PmContainer(FDSP_STOR_HVISOR),
                                 new OmContainer(FDSP_STOR_HVISOR)),
               new DomainClusterMap("AM-Platform-ClusMap",
                                 NULL,
                                 new SmContainer(FDSP_STOR_HVISOR),
                                 new DmContainer(FDSP_STOR_HVISOR),
                                 new AmContainer(FDSP_STOR_HVISOR),
                                 new PmContainer(FDSP_STOR_HVISOR),
                                 new OmContainer(FDSP_STOR_HVISOR)),
               new DomainResources("AM-Resources"),
               NULL)
{
    Platform::platf_assign_singleton(&gl_AmPlatform);
    plf_node_type = FDSP_STOR_HVISOR;
}

int
AmPlatform::mod_init(SysParams const *const param)
{
    fds_uint32_t base;

    FdsConfigAccessor conf(g_fdsprocess->get_conf_helper());
    Platform::mod_init(param);

    plf_om_ip_str    = conf.get_abs<std::string>("fds.plat.om_ip");
    plf_om_ctrl_port = conf.get_abs<int>("fds.plat.om_port");
    base             = conf.get_abs<int>("fds.plat.control_port");
    base             = PlatformProcess::plf_get_am_port(base);
    plf_my_ctrl_port = plf_ctrl_port(base);
    plf_my_data_port = plf_data_port(base);
    plf_my_conf_port = plf_conf_port(base);
    plf_my_migr_port = plf_migration_port(base);
    plf_my_ip        = util::get_local_ip();
    plf_my_node_name = plf_my_ip;

    plf_vol_evt  = new AmVolEvent(plf_resources, plf_clus_map, this);
    plf_node_evt = new NodePlatEvent(plf_resources, plf_clus_map, this);

    return 0;
}

void
AmPlatform::mod_startup()
{
}

void
AmPlatform::mod_shutdown()
{
}

PlatRpcReqt *
AmPlatform::plat_creat_reqt_disp()
{
    return new AmRpcReq(this);
}

PlatRpcResp *
AmPlatform::plat_creat_resp_disp()
{
    return new PlatRpcResp(this);
}

// --------------------------------------------------------------------------------------
// RPC handlers
// --------------------------------------------------------------------------------------
AmRpcReq::AmRpcReq(const Platform *plf) : PlatRpcReqt(plf) {}
AmRpcReq::~AmRpcReq() {}

void
AmRpcReq::NotifyAddVol(fpi::FDSP_MsgHdrTypePtr    &msg_hdr,
                       fpi::FDSP_NotifyVolTypePtr &msg)
{
}

void
AmRpcReq::NotifyRmVol(fpi::FDSP_MsgHdrTypePtr    &msg_hdr,
                      fpi::FDSP_NotifyVolTypePtr &msg)
{
}

void
AmRpcReq::NotifyModVol(fpi::FDSP_MsgHdrTypePtr    &msg_hdr,
                       fpi::FDSP_NotifyVolTypePtr &msg)
{
}

void
AmRpcReq::AttachVol(fpi::FDSP_MsgHdrTypePtr    &msg_hdr,
                    fpi::FDSP_AttachVolTypePtr &vol_msg)
{
}

void
AmRpcReq::DetachVol(fpi::FDSP_MsgHdrTypePtr    &msg_hdr,
                    fpi::FDSP_AttachVolTypePtr &vol_msg)
{
}

void
AmRpcReq::NotifyNodeAdd(fpi::FDSP_MsgHdrTypePtr     &msg_hdr,
                        fpi::FDSP_Node_Info_TypePtr &node_info)
{
}

void
AmRpcReq::NotifyNodeRmv(fpi::FDSP_MsgHdrTypePtr     &msg_hdr,
                        fpi::FDSP_Node_Info_TypePtr &node_info)
{
}

void
AmRpcReq::NotifyDLTUpdate(fpi::FDSP_MsgHdrTypePtr    &msg_hdr,
                          fpi::FDSP_DLT_Data_TypePtr &dlt_info)
{
}

void
AmRpcReq::NotifyDMTUpdate(fpi::FDSP_MsgHdrTypePtr &msg_hdr,  // NOLINT
                          fpi::FDSP_DMT_TypePtr   &dmt)
{
}

}  // namespace fds
