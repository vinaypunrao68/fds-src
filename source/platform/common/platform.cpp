/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <string>
#include <platform.h>
#include <fds_process.h>

namespace fds {

NodePlatform gl_NodePlatform;

// -------------------------------------------------------------------------------------
// Node Specific Platform
// -------------------------------------------------------------------------------------
NodePlatform::NodePlatform()
    : Platform("Node-Platform",
               FDSP_PLATFORM,
               new DomainNodeInv("Node-Platform-NodeInv",
                                 NULL,
                                 new SmContainer(FDSP_PLATFORM),
                                 new DmContainer(FDSP_PLATFORM),
                                 new AmContainer(FDSP_PLATFORM),
                                 new PmContainer(FDSP_PLATFORM),
                                 new OmContainer(FDSP_PLATFORM)),
               new DomainClusterMap("Node-Platform-ClusMap",
                                 NULL,
                                 new SmContainer(FDSP_PLATFORM),
                                 new DmContainer(FDSP_PLATFORM),
                                 new AmContainer(FDSP_PLATFORM),
                                 new PmContainer(FDSP_PLATFORM),
                                 new OmContainer(FDSP_PLATFORM)),
               new DomainResources("Node-Resources"),
               NULL)
{
    plf_node_evt = new NodePlatEvent(plf_resources, plf_clus_map, this);
    plf_vol_evt  = new VolPlatEvent(plf_resources, plf_clus_map, this);

    Platform::platf_assign_singleton(&gl_NodePlatform);
}

int
NodePlatform::mod_init(SysParams const *const param)
{
    FdsConfigAccessor conf(g_fdsprocess->get_conf_helper());

    Platform::mod_init(param);

    plf_node_type    = FDSP_DATA_MGR;
    plf_om_ip_str    = conf.get_abs<std::string>("fds.plat.om_ip");
    plf_om_conf_port = conf.get_abs<int>("fds.plat.om_port");
    plf_my_ctrl_port = conf.get_abs<int>("fds.plat.control_port");
    plf_my_ip        = util::get_local_ip();
    plf_my_node_name = plf_my_ip;
    return 0;
}

void
NodePlatform::mod_startup()
{
    Platform::mod_startup();
}

void
NodePlatform::mod_shutdown()
{
    Platform::mod_shutdown();
}

/**
 * Required factory methods.
 */
PlatRpcReqt *
NodePlatform::plat_creat_reqt_disp()
{
    return new PlatformRpcReqt(this);
}

PlatRpcResp *
NodePlatform::plat_creat_resp_disp()
{
    return new PlatformRpcResp(this);
}

// --------------------------------------------------------------------------------------
// RPC handlers
// --------------------------------------------------------------------------------------
PlatformRpcReqt::PlatformRpcReqt(const Platform *plf) : PlatRpcReqt(plf) {}
PlatformRpcReqt::~PlatformRpcReqt() {}

void
PlatformRpcReqt::NotifyNodeAdd(fpi::FDSP_MsgHdrTypePtr     &msg_hdr,
                               fpi::FDSP_Node_Info_TypePtr &node_info)
{
    std::cout << "Got it!" << std::endl;
}

void
PlatformRpcReqt::NotifyNodeRmv(fpi::FDSP_MsgHdrTypePtr     &msg_hdr,
                               fpi::FDSP_Node_Info_TypePtr &node_info)
{
}

PlatformRpcResp::PlatformRpcResp(const Platform *plf) : PlatRpcResp(plf) {}
PlatformRpcResp::~PlatformRpcResp() {}

void
PlatformRpcResp::RegisterNodeResp(fpi::FDSP_MsgHdrTypePtr       &hdr,
                                  fpi::FDSP_RegisterNodeTypePtr &node)
{
    std::cout << "Got it resp!" << std::endl;
}

}  // namespace fds
