/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <string>
#include <vector>
#include <net-platform.h>
#include <platform/platform-lib.h>

namespace fds {

NetPlatform                  gl_netPlatform("NetPlatform");
static NetPlatSvc            lo_netSvc("NetSvc");

/*
 * -----------------------------------------------------------------------------------
 * Exported module
 * -----------------------------------------------------------------------------------
 */
NetPlatform::NetPlatform(const char *name) : Module(name)
{
    static Module *net_plat_deps[] = {
        gl_PlatformSvc,
        &lo_netSvc,
        NULL
    };
    mod_intern = net_plat_deps;
}

int
NetPlatform::mod_init(SysParams const *const p)
{
    Module::mod_init(p);
    return 0;
}

void
NetPlatform::mod_startup()
{
}

void
NetPlatform::mod_shutdown()
{
}

/*
 * -----------------------------------------------------------------------------------
 * Internal module
 * -----------------------------------------------------------------------------------
 */
NetPlatSvc::NetPlatSvc(const char *name) : Module(name) {}

int
NetPlatSvc::mod_init(SysParams const *const p)
{
    Module::mod_init(p);

    plat_lib = Platform::platf_singleton();
    return 0;
}

void
NetPlatSvc::mod_startup()
{
}

void
NetPlatSvc::mod_enable_service()
{
}

void
NetPlatSvc::mod_shutdown()
{
}

/*
 * -----------------------------------------------------------------------------------
 * RPC Handlers
 * -----------------------------------------------------------------------------------
 */
void
NetPlatHandler::asyncReqt(bo::shared_ptr<fpi::AsyncHdr> &hdr)
{
}

void
NetPlatHandler::asyncResp(bo::shared_ptr<fpi::AsyncHdr>  &hdr,
                          bo::shared_ptr<std::string>    &payload)
{
}

void
NetPlatHandler::uuidBind(fpi::RespHdr &ret, bo::shared_ptr<fpi::UuidBindMsg> &msg)
{
}

void
NetPlatHandler::allUuidBinding(std::vector<fpi::UuidBindMsg>   &ret,
                               bo::shared_ptr<fpi::UuidBindMsg> &msg)
{
}

void
NetPlatHandler::notifyNodeInfo(std::vector<fpi::NodeInfoMsg>    &ret,
                               bo::shared_ptr<fpi::NodeInfoMsg> &inf)
{
}

void
NetPlatHandler::notifyNodeUp(fpi::RespHdr &ret, bo::shared_ptr<fpi::NodeInfoMsg> &info)
{
}

}  // namespace fds
