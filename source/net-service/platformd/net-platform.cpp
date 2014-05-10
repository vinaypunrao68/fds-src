/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <string>
#include <vector>
#include <ep-map.h>
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
        &gl_EpPlatform,
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
    Module::mod_startup();
}

void
NetPlatform::mod_shutdown()
{
    Module::mod_shutdown();
}

/*
 * -----------------------------------------------------------------------------------
 * Internal module
 * -----------------------------------------------------------------------------------
 */
static void
net_platform_server(PlatEpPtr ep)
{
    ep->ep_run_server();
}

NetPlatSvc::NetPlatSvc(const char *name) : Module(name) {}

int
NetPlatSvc::mod_init(SysParams const *const p)
{
    FdsConfigAccessor conf(g_fdsprocess->get_conf_helper());

    Module::mod_init(p);
    plat_lib       = Platform::platf_singleton();
    plat_ep_hdler  = bo::shared_ptr<NetPlatHandler>(new NetPlatHandler(this));
    plat_ep_plugin = new PlatNetPlugin(this);
    plat_ep        = new EndPoint<fpi::PlatNetSvcClient, fpi::PlatNetSvcProcessor>(
            plat_lib->plf_get_my_data_port(), /* hack, need to consolidate ports */
            plat_lib->plf_my_node_uuid(),     /* bind to my uuid  */
            NodeUuid(0ULL),                   /* pure server mode */
            bo::shared_ptr<fpi::PlatNetSvcProcessor>(
                new fpi::PlatNetSvcProcessor(plat_ep_hdler)),
            plat_ep_plugin);

    plat_ep->ep_setup_server();
    return 0;
}

void
NetPlatSvc::mod_startup()
{
    fds_threadpool *pool = g_fdsprocess->proc_thrpool();

    pool->schedule(net_platform_server, plat_ep);
}

void
NetPlatSvc::mod_enable_service()
{
    int port;

    if (plat_lib->plf_is_om_node()) {
        std::cout << "This is OM node" << std::endl;
        return;
    }
    port = plat_lib->plf_get_om_svc_port();
    std::cout << "Trying to contact platform on OM "
        << *plat_lib->plf_get_om_ip() << ", port " << port << std::endl;
    std::cout << "My ip " << *plat_lib->plf_get_my_ip() << std::endl;

    plat_ep->ep_connect_server(port, *plat_lib->plf_get_om_ip());
    std::cout << "connected..." << std::endl;
}

void
NetPlatSvc::mod_shutdown()
{
}

/*
 * -----------------------------------------------------------------------------------
 * Endpoint Plugin
 * -----------------------------------------------------------------------------------
 */
PlatNetPlugin::PlatNetPlugin(NetPlatSvc *svc) : plat_svc(svc) {}

void
PlatNetPlugin::ep_connected()
{
}

void
PlatNetPlugin::ep_down()
{
}

void
PlatNetPlugin::svc_up(EpSvcHandle::pointer handle)
{
}

void
PlatNetPlugin::svc_down(EpSvc::pointer svc, EpSvcHandle::pointer handle)
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
