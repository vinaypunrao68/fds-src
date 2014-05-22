/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <vector>
#include <net/net-service-tmpl.hpp>
#include <net-platform.h>

namespace fds {

/*
 * -----------------------------------------------------------------------------------
 * RPC Handlers
 * -----------------------------------------------------------------------------------
 */
PlatformEpHandler::~PlatformEpHandler() {}
PlatformEpHandler::PlatformEpHandler(PlatformdNetSvc *svc)
    : fpi::PlatNetSvcIf(), net_plat(svc) {}

// allUuidBinding
// --------------
//
void
PlatformEpHandler::allUuidBinding(std::vector<fpi::UuidBindMsg>    &ret,
                                  bo::shared_ptr<fpi::UuidBindMsg> &msg)
{
    std::cout << "Platform domain master, all uuidBind is called" << std::endl;
}

// notifyNodeInfo
// --------------
//
void
PlatformEpHandler::notifyNodeInfo(std::vector<fpi::NodeInfoMsg> &ret,
                                  bo::shared_ptr<fpi::NodeInfoMsg> &info)
{
    std::cout << "Node notify "
        << "\nDisk iops max......... " << info->node_stor.disk_iops_max
        << "\nDisk iops min......... " << info->node_stor.disk_iops_min
        << "\nDisk capacity......... " << info->node_stor.disk_capacity << std::endl;
}

// notifyNodeUp
// ------------
//
void
PlatformEpHandler::notifyNodeUp(fpi::RespHdr &ret,
                                bo::shared_ptr<fpi::NodeInfoMsg> &info)
{
}

}  // namespace fds
