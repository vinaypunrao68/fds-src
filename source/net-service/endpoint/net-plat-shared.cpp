/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <string>
#include <vector>
#include <ep-map.h>
#include <net/net-service-tmpl.hpp>
#include <platform/platform-lib.h>
#include <net-plat-shared.h>

namespace fds {

NetPlatSvc                   gl_NetPlatform("NetPlatform");
NetPlatform                 *gl_NetPlatSvc = &gl_NetPlatform;

/*
 * -----------------------------------------------------------------------------------
 * Exported module
 * -----------------------------------------------------------------------------------
 */
NetPlatform::NetPlatform(const char *name) : Module(name)
{
    static Module *net_plat_deps[] = {
        gl_PlatformSvc,
        NULL
    };
    netmgr     = NULL;
    plat_lib   = NULL;
    mod_intern = net_plat_deps;
}

NetPlatSvc::~NetPlatSvc() {}
NetPlatSvc::NetPlatSvc(const char *name) : NetPlatform(name)
{
    plat_ep        = NULL;
    plat_ep_plugin = NULL;
    plat_ep_hdler  = NULL;
    plat_agent     = NULL;
}

int
NetPlatSvc::mod_init(SysParams const *const p)
{
    netmgr   = NetMgr::ep_mgr_singleton();
    plat_lib = Platform::platf_singleton();
    return Module::mod_init(p);
}

void
NetPlatSvc::mod_startup()
{
    Module::mod_startup();

    // Create the agent and register the ep
    plat_agent     = new DomainAgent(plat_lib->plf_my_node_uuid());
    plat_ep_hdler  = bo::shared_ptr<NetPlatHandler>(new NetPlatHandler(this));
    plat_ep_plugin = new PlatNetPlugin(this);
    plat_ep        = new PlatNetEp(
            plat_lib->plf_get_my_data_port(), /* hack, need to consolidate ports */
            plat_lib->plf_my_node_uuid(),     /* bind to my uuid  */
            NodeUuid(0ULL),                   /* pure server mode */
            bo::shared_ptr<fpi::PlatNetSvcProcessor>(
                new fpi::PlatNetSvcProcessor(plat_ep_hdler)),
            plat_ep_plugin);
}

void
NetPlatSvc::mod_enable_service()
{
    Module::mod_enable_service();

    // Regiser my node endpoint.
    netmgr->ep_register(plat_ep, false);
    plat_agent->pda_connect_domain(fpi::DomainID());
}

void
NetPlatSvc::mod_shutdown()
{
    Module::mod_shutdown();
}

// nplat_domain_rpc
// ----------------
// Get the RPC handles needed to contact the master platform services.
//
EpSvcHandle::pointer
NetPlatSvc::nplat_domain_rpc(const fpi::DomainID &id)
{
    return plat_agent->pda_rpc();
}

/*
 * -----------------------------------------------------------------------------------
 * Domain Agent
 * -----------------------------------------------------------------------------------
 */
void
DomainAgent::pda_connect_domain(const fpi::DomainID &id)
{
}

void
DomainAgent::pda_update_binding(const struct ep_map_rec *rec, int cnt)
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
