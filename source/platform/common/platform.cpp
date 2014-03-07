/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <platform.h>
#include <fds_process.h>

namespace fds {

NodePlatform gl_NodePlatform;

namespace util {
/**
 * @return local ip
 */
static std::string get_local_ip()
{
    struct ifaddrs *ifAddrStruct = NULL;
    struct ifaddrs *ifa          = NULL;
    void   *tmpAddrPtr           = NULL;
    std::string myIp;

    /*
     * Get the local IP of the host.
     * This is needed by the OM.
     */
    getifaddrs(&ifAddrStruct);
    for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr->sa_family == AF_INET) {  // IPv4
            if (strncmp(ifa->ifa_name, "lo", 2) != 0) {
                tmpAddrPtr = &((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;  // NOLINT
                char addrBuf[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, tmpAddrPtr, addrBuf, INET_ADDRSTRLEN);
                myIp = std::string(addrBuf);
                if (myIp.find("10.1") != std::string::npos)
                    break; /* TODO: more dynamic */
            }
        }
    }
    if (ifAddrStruct != NULL) {
        freeifaddrs(ifAddrStruct);
    }
    return myIp;
}
}  // namespace util

// -------------------------------------------------------------------------------------
// Node Specific Platform
// -------------------------------------------------------------------------------------
NodePlatform::NodePlatform()
    : Platform("Node-Platform",
               FDSP_PLATFORM,
               new DomainNodeInv("Node-Platform-NodeInv",
                                 NULL,
                                 new SmContainer(FDSP_DATA_MGR),
                                 new DmContainer(FDSP_DATA_MGR),
                                 new AmContainer(FDSP_DATA_MGR),
                                 new OmContainer(FDSP_DATA_MGR)),
               new DomainClusterMap("Node-Platform-ClusMap",
                                 NULL,
                                 new SmContainer(FDSP_DATA_MGR),
                                 new DmContainer(FDSP_DATA_MGR),
                                 new AmContainer(FDSP_DATA_MGR),
                                 new OmContainer(FDSP_DATA_MGR)),
               new DomainResources("Node-Resources"),
               NULL)
{
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
    plf_my_data_port = 0;
    plf_my_ip        = util::get_local_ip();
    plf_my_node_name = plf_my_ip;

    plf_my_migration_port = 0;
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

PlatRpcReqt *
NodePlatform::plat_creat_reqt_disp()
{
    return new PlatformRpcReqt();
}

PlatRpcResp *
NodePlatform::plat_creat_resp_disp()
{
    return new PlatformRpcResp();
}

// --------------------------------------------------------------------------------------
// RPC handlers
// --------------------------------------------------------------------------------------
PlatformRpcReqt::PlatformRpcReqt() {}
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

PlatformRpcResp::PlatformRpcResp() {}
PlatformRpcResp::~PlatformRpcResp() {}

void
PlatformRpcResp::RegisterNodeResp(fpi::FDSP_MsgHdrTypePtr       &hdr,
                                  fpi::FDSP_RegisterNodeTypePtr &node)
{
    std::cout << "Got it resp!" << std::endl;
}

}  // namespace fds
