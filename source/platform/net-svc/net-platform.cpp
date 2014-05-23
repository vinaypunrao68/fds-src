/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <string>
#include <vector>
#include <disk.h>
#include <ep-map.h>
#include <net/net-service-tmpl.hpp>
#include <platform/platform-lib.h>
#include <net-platform.h>

namespace fds {

/*
 * -----------------------------------------------------------------------------------
 * Internal module
 * -----------------------------------------------------------------------------------
 */
PlatformdNetSvc              gl_PlatformdNetSvc("platformd net");
static EpPlatformdMod        gl_PlatformdShmLib("Platformd Shm Lib");

PlatformdNetSvc::~PlatformdNetSvc() {}
PlatformdNetSvc::PlatformdNetSvc(const char *name) : NetPlatSvc(name)
{
    static Module *platd_net_deps[] = {
        &gl_PlatformdShmLib,
        NULL
    };
    gl_NetPlatSvc   = this;
    gl_EpShmPlatLib = &gl_PlatformdShmLib;
    mod_intern = platd_net_deps;
}

// mod_init
// --------
//
int
PlatformdNetSvc::mod_init(SysParams const *const p)
{
    return NetPlatSvc::mod_init(p);
}

// mod_startup
// -----------
//
void
PlatformdNetSvc::mod_startup()
{
    Module::mod_startup();

    plat_agent  = new PlatAgent(plat_lib->plf_my_node_uuid());
    plat_recv   = bo::shared_ptr<PlatformEpHandler>(new PlatformEpHandler(this));
    plat_plugin = new PlatformdPlugin(this);
    plat_ep     = new PlatNetEp(
            plat_lib->plf_get_my_nsvc_port(),
            plat_lib->plf_my_node_uuid(),
            NodeUuid(0ULL),
            bo::shared_ptr<fpi::PlatNetSvcProcessor>(
                new fpi::PlatNetSvcProcessor(plat_recv)), plat_plugin);

    LOGNORMAL << "Startup platform specific net svc, port "
              << plat_lib->plf_get_my_nsvc_port();
}

// mod_enable_service
// ------------------
//
void
PlatformdNetSvc::mod_enable_service()
{
    NetPlatSvc::mod_enable_service();
}

// mod_shutdown
// ------------
//
void
PlatformdNetSvc::mod_shutdown()
{
    Module::mod_shutdown();
}

// plat_update_local_binding
// -------------------------
//
void
PlatformdNetSvc::plat_update_local_binding(const ep_map_rec_t *rec)
{
}

// plat_update_domain_binding
// --------------------------
//
void
PlatformdNetSvc::plat_update_domain_binding(const ep_map_rec_t *rec)
{
}

// nplat_peer
// ----------
//
EpSvcHandle::pointer
PlatformdNetSvc::nplat_peer(const fpi::SvcUuid &uuid)
{
    return NULL;
}

// nplat_peer
// ----------
//
EpSvcHandle::pointer
PlatformdNetSvc::nplat_peer(const fpi::DomainID &id, const fpi::SvcUuid &uuid)
{
    return NULL;
}

/*
 * -----------------------------------------------------------------------------------
 * Endpoint Plugin
 * -----------------------------------------------------------------------------------
 */
PlatformdPlugin::~PlatformdPlugin() {}
PlatformdPlugin::PlatformdPlugin(PlatformdNetSvc *svc) : EpEvtPlugin(), plat_svc(svc) {}

// ep_connected
// ------------
//
void
PlatformdPlugin::ep_connected()
{
}

// ep_done
// -------
//
void
PlatformdPlugin::ep_down()
{
}

// svc_up
// ------
//
void
PlatformdPlugin::svc_up(EpSvcHandle::pointer handle)
{
}

// svc_down
// --------
//
void
PlatformdPlugin::svc_down(EpSvc::pointer svc, EpSvcHandle::pointer handle)
{
}

/*
 * -------------------------------------------------------------------------------------
 * Platform Node Agent
 * -------------------------------------------------------------------------------------
 */
PlatAgent::PlatAgent(const NodeUuid &uuid) : DomainAgent(uuid, false)
{
    fds_verify(agt_domain_evt == NULL);
    agt_domain_evt = new PlatAgentPlugin(this);
}

// init_stor_cap_msg
// -----------------
//
void
PlatAgent::init_stor_cap_msg(fpi::StorCapMsg *msg) const
{
    DiskPlatModule *disk;

    std::cout << "Platform agent fill in storage cap msg" << std::endl;
    disk = DiskPlatModule::dsk_plat_singleton();
    NodeAgent::init_stor_cap_msg(msg);
}

// ep_connected
// ------------
//
void
PlatAgentPlugin::ep_connected()
{
    fpi::NodeInfoMsg              *msg;
    std::vector<fpi::NodeInfoMsg>  ret;

    std::cout << "Platform agent connected to domain controller" << std::endl;

    msg = new fpi::NodeInfoMsg();
    pda_agent->init_plat_info_msg(msg);

    auto rpc = pda_agent->pda_rpc();
    auto pkt = bo::shared_ptr<fpi::NodeInfoMsg>(msg);
    rpc->notifyNodeInfo(ret, pkt);
}

// ep_down
// -------
//
void
PlatAgentPlugin::ep_down()
{
}

// svc_up
// ------
//
void
PlatAgentPlugin::svc_up(EpSvcHandle::pointer handle)
{
}

// svc_down
// --------
//
void
PlatAgentPlugin::svc_down(EpSvc::pointer svc, EpSvcHandle::pointer handle)
{
}

}  // namespace fds
