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
#include <plat-node-workflow.h>
#include <net-platform.h>
#include <net/PlatNetSvcHandler.h>

namespace fds {

/*
 * -----------------------------------------------------------------------------------
 * Internal module
 * -----------------------------------------------------------------------------------
 */
PlatformdNetSvc              gl_PlatformdNetSvc("platformd net");
static EpPlatformdMod        gl_PlatformdShmLib("Platformd Shm Lib");

PlatformdNetSvc::~PlatformdNetSvc() {}
PlatformdNetSvc::PlatformdNetSvc(const char *name) : NetPlatSvc(name) {}

// ep_shm_singleton
// ----------------
//
/* static */ EpPlatformdMod *
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
    static Module *platd_net_deps[] = {
        &gl_PlatformdShmLib,
        &gl_PlatWorkFlow,
        NULL
    };
    gl_NetPlatSvc   = this;
    gl_EpShmPlatLib = &gl_PlatformdShmLib;

    /* Init the same code as NetPlatSvc::mod_init()  */
    netmgr          = NetMgr::ep_mgr_singleton();
    plat_lib        = Platform::platf_singleton();
    mod_intern      = platd_net_deps;
    return Module::mod_init(p);
}

// mod_startup
// -----------
//
void
PlatformdNetSvc::mod_startup()
{
    Platform          *plat;
    NodeWorkFlow      *work;
    fpi::SvcUuid       om;
    fpi::NodeInfoMsg   msg;

    Module::mod_startup();
    plat_recv   = bo::shared_ptr<PlatformEpHandler>(new PlatformEpHandler(this));
    plat_plugin = new PlatformdPlugin(this);
    plat_ep     = new PlatNetEp(
            plat_lib->plf_get_my_node_port(),
            *plat_lib->plf_get_my_node_uuid(),
            NodeUuid(0ULL),
            bo::shared_ptr<fpi::PlatNetSvcProcessor>(
                new fpi::PlatNetSvcProcessor(plat_recv)), plat_plugin);

    plat_self   = new PlatAgent(*plat_lib->plf_get_my_node_uuid());
    plat_master = new PlatAgent(gl_OmPmUuid);

    plat_self->init_plat_info_msg(&msg);
    nplat_register_node(&msg, plat_self);

    gl_OmUuid.uuid_assign(&om);
    plat = Platform::platf_singleton();
    work = NodeWorkFlow::nd_workflow_sgt();
    work->wrk_item_create(om, plat_self, plat->plf_node_inventory());

    plat_master->init_om_pm_info_msg(&msg);
    nplat_register_node(&msg, plat_master);

    netmgr->ep_register(plat_ep, false);
    LOGNORMAL << "Startup platform specific net svc, port "
              << plat_lib->plf_get_my_node_port();
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

// nplat_register_node
// -------------------
//
void
PlatformdNetSvc::nplat_register_node(fpi::NodeInfoMsg *msg, NodeAgent::pointer node)
{
    int                 idx;
    node_data_t         rec;
    ShmObjRWKeyUint64  *shm;
    NodeAgent::pointer  agent;

    node->node_info_msg_to_shm(msg, &rec);
    shm = NodeShmRWCtrl::shm_node_rw_inv();
    idx = shm->shm_insert_rec(static_cast<void *>(&rec.nd_node_uuid),
                              static_cast<void *>(&rec), sizeof(rec));
    fds_verify(idx != -1);

    agent = node;
    Platform::platf_singleton()->
        plf_node_inventory()->dc_register_node(shm, &agent, idx, idx, 0);
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

// nplat_refresh_shm
// -----------------
//
void
PlatformdNetSvc::nplat_refresh_shm()
{
    std::cout << "Platform daemon refresh shm" << std::endl;
}

bo::shared_ptr<PlatNetSvcHandler> PlatformdNetSvc::getPlatNetSvcHandler()
{
    return plat_recv;
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
    // if we are in perf test mode, over-write some node capabilities
    // from config
    FdsConfigAccessor conf(g_fdsprocess->get_fds_config(), "fds.plat.testing.");
    NodeAgent::init_stor_cap_msg(msg);
    if (conf.get<bool>("manual_nodecap")) {
        msg->disk_iops_min = conf.get<int>("disk_iops_min");
        msg->disk_iops_max = conf.get<int>("disk_iops_max");
        LOGNORMAL << "Over-writing node perf capability from config ";
    }
}

// pda_register
// ------------
//
void
PlatAgent::pda_register()
{
    int                     idx;
    node_data_t             rec;
    fpi::NodeInfoMsg        msg;
    ShmObjRWKeyUint64      *shm;
    NodeAgent::pointer        agent;
    DomainContainer::pointer  local;

    this->init_plat_info_msg(&msg);
    this->node_info_msg_to_shm(&msg, &rec);

    shm = NodeShmRWCtrl::shm_node_rw_inv();
    idx = shm->shm_insert_rec(static_cast<void *>(&rec.nd_node_uuid),
                              static_cast<void *>(&rec), sizeof(rec));
    fds_verify(idx != -1);

    agent = this;
    local = Platform::platf_singleton()->plf_node_inventory();
    local->dc_register_node(shm, &agent, idx, idx, NODE_DO_PROXY_ALL_SVCS);
}

// agent_publish_ep
// ----------------
// Called by the platform daemon side to publish uuid binding info to shared memory.
//
void
PlatAgent::agent_publish_ep()
{
    int                idx, base_port;
    node_data_t        ninfo;
    ep_map_rec_t       rec;
    Platform          *plat;
    EpPlatformdMod    *ep_map;

    plat   = Platform::platf_singleton();
    ep_map = EpPlatformdMod::ep_shm_singleton();
    node_info_frm_shm(&ninfo);
    LOGDEBUG << "Platform agent uuid " << std::hex << ninfo.nd_node_uuid;

    if (rs_uuid == gl_OmPmUuid) {
        /* Record both OM binding and local binding. */
        base_port          = ninfo.nd_base_port;
        ninfo.nd_node_uuid = gl_OmPmUuid.uuid_get_val();
        EpPlatLibMod::ep_node_info_to_mapping(&ninfo, &rec);
        idx = ep_map->ep_map_record(&rec);

        ninfo.nd_node_uuid = gl_OmUuid.uuid_get_val();
        ninfo.nd_base_port = plat->plf_get_om_ctrl_port();
        EpPlatLibMod::ep_node_info_to_mapping(&ninfo, &rec);
        idx = ep_map->ep_map_record(&rec);

        /* TODO(Vy): hack, record OM binding for the control path. */
        ninfo.nd_base_port = plat->plf_get_my_data_port(base_port);
        EpPlatLibMod::ep_node_info_to_mapping(&ninfo, &rec);
        rec.rmp_major = 0;
        rec.rmp_minor = NET_SVC_CTRL;
        idx = ep_map->ep_map_record(&rec);

        LOGDEBUG << "Platform daemon binds OM " << std::hex
            << ninfo.nd_node_uuid << "@" << ninfo.nd_ip_addr << ":" << std::dec
            << ninfo.nd_base_port << ", idx " << idx;
    } else {
        agent_bind_svc(ep_map, &ninfo, fpi::FDSP_STOR_MGR);
        agent_bind_svc(ep_map, &ninfo, fpi::FDSP_DATA_MGR);
        agent_bind_svc(ep_map, &ninfo, fpi::FDSP_STOR_HVISOR);
        agent_bind_svc(ep_map, &ninfo, fpi::FDSP_ORCH_MGR);
    }
}

// agent_bind_svc
// --------------
//
void
PlatAgent::agent_bind_svc(EpPlatformdMod *map, node_data_t *ninfo, fpi::FDSP_MgrIdType t)
{
    int           i, idx, base_port, max_port, saved_port;
    NodeUuid      node, svc;
    fds_uint64_t  saved_uuid;
    ep_map_rec_t  rec;

    saved_port = ninfo->nd_base_port;
    saved_uuid = ninfo->nd_node_uuid;

    node.uuid_set_val(saved_uuid);
    Platform::plf_svc_uuid_from_node(node, &svc, t);

    ninfo->nd_node_uuid = svc.uuid_get_val();
    ninfo->nd_base_port = Platform::plf_svc_port_from_node(saved_port, t);
    EpPlatLibMod::ep_node_info_to_mapping(ninfo, &rec);

    /* Register services sharing the same uuid but using different port, encode in maj. */
    max_port  = Platform::plf_get_my_max_svc_ports();
    base_port = ninfo->nd_base_port;

    for (i = 0; i < max_port; i++) {
        idx = map->ep_map_record(&rec);
        LOGDEBUG << "Platform daemon binds " << t << ":" << std::hex << svc.uuid_get_val()
            << "@" << ninfo->nd_ip_addr << ":" << std::dec
            << rec.rmp_major << ":" << rec.rmp_minor << " idx " << idx;

        rec.rmp_minor = Platform::plat_svc_types[i];
        if ((t == fpi::FDSP_STOR_HVISOR) && (rec.rmp_minor == NET_SVC_CONFIG)) {
            /* Hard-coded to bind to Java endpoint in AM. */
            ep_map_set_port_info(&rec, 8999);
        } else {
            ep_map_set_port_info(&rec, base_port + rec.rmp_minor);
        }
    }
    /* Restore everything back to ninfo for the next loop */
    ninfo->nd_base_port = saved_port;
    ninfo->nd_node_uuid = saved_uuid;
}

// ep_connected
// ------------
//
void
PlatAgentPlugin::ep_connected()
{
    NodeUuid                       uuid;
    Platform                      *plat;
    NetPlatform                   *net_plat;
    fpi::NodeInfoMsg              *msg;
    NodeAgent::pointer             pm;
    EpSvcHandle::pointer           eph;
    std::vector<fpi::NodeInfoMsg>  ret, ignore;
    bo::shared_ptr<fpi::PlatNetSvcClient> rpc;

    plat = Platform::platf_singleton();
    msg  = new fpi::NodeInfoMsg();
    na_owner->init_plat_info_msg(msg);

    rpc = na_owner->node_svc_rpc(&eph);
    rpc->notifyNodeInfo(ret, *msg, true);

    std::cout << "Got " << ret.size() << " elements back" << std::endl;

    net_plat = NetPlatform::nplat_singleton();
    for (auto it = ret.cbegin(); it != ret.cend(); it++) {
        net_plat->nplat_register_node(&*it);

        /* Send to the peer node info about myself. */
        uuid.uuid_set_val((*it).node_loc.svc_node.svc_uuid.svc_uuid);
        pm = plat->plf_find_node_agent(uuid);
        fds_assert(pm != NULL);

        rpc = pm->node_svc_rpc(&eph);
        ignore.clear();
        rpc->notifyNodeInfo(ignore, *msg, false);
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
