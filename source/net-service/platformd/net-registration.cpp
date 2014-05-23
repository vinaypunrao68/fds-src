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
}

// notifyNodeInfo
// --------------
//
void
PlatformEpHandler::notifyNodeInfo(std::vector<fpi::NodeInfoMsg> &ret,
                                  bo::shared_ptr<fpi::NodeInfoMsg> &info)
{
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
