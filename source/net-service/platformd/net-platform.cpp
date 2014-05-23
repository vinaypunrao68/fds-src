/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <string>
#include <vector>
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
static EpPlatLibMod          gl_PlatformdShmLib("Platformd Shm Lib");

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

    plat_recv      = bo::shared_ptr<PlatformEpHandler>(new PlatformEpHandler(this));
    plat_plugin    = new PlatformdPlugin(this);
    plat_ep        = new PlatNetEp(
            plat_lib->plf_get_my_ctrl_port(),
            plat_lib->plf_my_node_uuid(),
            NodeUuid(0ULL),
            bo::shared_ptr<fpi::PlatNetSvcProcessor>(
                new fpi::PlatNetSvcProcessor(plat_recv)), plat_plugin);
    std::cout << "Startup platform specific net svc" << std::endl;
}

// mod_enable_service
// ------------------
//
void
PlatformdNetSvc::mod_enable_service()
{
    Module::mod_enable_service();

    netmgr->ep_register(plat_ep, false);
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

}  // namespace fds
