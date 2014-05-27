/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <string>
#include <vector>
#include <disk.h>
#include <ep-map.h>
#include <platform.h>
#include <net/net-service-tmpl.hpp>
#include <platform/platform-lib.h>
#include <platform/node-inv-shmem.h>
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

// ep_shm_singleton
// ----------------
//
/*  static */ EpPlatformdMod *
EpPlatformdMod::ep_shm_singleton()
{
    return &gl_PlatformdShmLib;
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

    plat_agent  = new PlatAgent(*plat_lib->plf_get_my_node_uuid());
    plat_recv   = bo::shared_ptr<PlatformEpHandler>(new PlatformEpHandler(this));
    plat_plugin = new PlatformdPlugin(this);
    plat_ep     = new PlatNetEp(
            plat_lib->plf_get_my_node_port(),
            *plat_lib->plf_get_my_node_uuid(),
            NodeUuid(0ULL),
            bo::shared_ptr<fpi::PlatNetSvcProcessor>(
                new fpi::PlatNetSvcProcessor(plat_recv)), plat_plugin);

    LOGNORMAL << "Startup platform specific net svc, port "
              << plat_lib->plf_get_my_node_port();
}

// mod_enable_service
// ------------------
//
void
PlatformdNetSvc::mod_enable_service()
{
    plat_agent->pda_register(Platform::plf_pm_nodes());
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

// nplat_register_node
// -------------------
// Platform daemon registers this node info from the network packet.
//
void
PlatformdNetSvc::nplat_register_node(const fpi::NodeInfoMsg *msg)
{
    node_data_t  rec;

    // Notify all services about this node through shared memory queue.
    NodeInventory::node_info_msg_to_shm(msg, &rec);
    EpPlatformdMod::ep_shm_singleton()->node_reg_notify(&rec);
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
    node_svc_type  = fpi::FDSP_PLATFORM;
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

// pda_register
// ------------
//
void
PlatAgent::pda_register(PmContainer::pointer container)
{
    int                idx;
    node_data_t        rec;
    fpi::NodeInfoMsg   msg;
    ShmObjRWKeyUint64 *shm;

    this->init_plat_info_msg(&msg);
    this->node_info_msg_to_shm(&msg, &rec);

    shm = NodeShmRWCtrl::shm_node_rw_inv();
    idx = shm->shm_insert_rec(static_cast<void *>(&rec.nd_node_uuid),
                              static_cast<void *>(&rec), sizeof(rec));
    fds_verify(idx != -1);

    this->node_fill_shm_inv(shm, idx, idx);
    container->agent_activate(this);
}

// ep_connected
// ------------
//
void
PlatAgentPlugin::ep_connected()
{
    NetPlatform                   *net_plat;
    fpi::NodeInfoMsg              *msg;
    std::vector<fpi::NodeInfoMsg>  ret;

    std::cout << "Platform agent connected to domain controller" << std::endl;

    msg = new fpi::NodeInfoMsg();
    pda_agent->init_plat_info_msg(msg);

    auto rpc = pda_agent->pda_rpc();
    rpc->notifyNodeInfo(ret, *msg, true);

    std::cout << "Got " << ret.size() << " elements back" << std::endl;

    net_plat = NetPlatform::nplat_singleton();
    for (auto it = ret.cbegin(); it != ret.cend(); it++) {
        net_plat->nplat_register_node(&*it);
    }
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
