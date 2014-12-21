/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <string>
#include <vector>
#include <map>
#include <ep-map.h>
#include <fds_process.h>
#include <net/net-service.h>
#include <platform/platform-lib.h>
#include <platform/node-inv-shmem.h>
#include <platform/service-ep-lib.h>

// New includes
#include "platform/node_info_shm_iter.h"
#include "platform/net_plat_svc.h"

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
    netmgr     = NULL;
    plat_lib   = NULL;
}

NetPlatSvc::~NetPlatSvc() {}
NetPlatSvc::NetPlatSvc(const char *name) : NetPlatform(name)
{
    plat_ep        = NULL;
    plat_ep_plugin = NULL;
    plat_ep_hdler  = NULL;
    plat_self      = NULL;
    plat_master    = NULL;
}

int
NetPlatSvc::mod_init(SysParams const *const p)
{
    static Module *net_plat_deps[] = {
        NULL
    };
    mod_intern = net_plat_deps;
    netmgr     = NetMgr::ep_mgr_singleton();
    plat_lib   = Platform::platf_singleton();
    return Module::mod_init(p);
}

void
NetPlatSvc::mod_startup()
{
    Module::mod_startup();

    if (plat_self == NULL) {
        plat_self = new DomainAgent(*plat_lib->plf_get_my_node_uuid());
    }
}

void
NetPlatSvc::mod_enable_service()
{
    Module::mod_enable_service();

    plat_self->pda_register();
    nplat_refresh_shm();

    if (!plat_lib->plf_is_om_node() && (plat_master != NULL)) {
        plat_master->pda_connect_domain(fpi::DomainID());
    }
}

void
NetPlatSvc::mod_shutdown()
{
    Module::mod_shutdown();
}

NodeInfoShmIter::NodeInfoShmIter(bool rw)
    : it_no_rec(0), it_no_rec_quit(10), it_shm_rw(rw)
{
    it_shm   = NodeShmCtrl::shm_node_inventory();
    it_local = Platform::platf_singleton()->plf_node_inventory();
}

// shm_obj_iter_fn
// ---------------
// Iterate through node info records in shared memory and create agent objs in the
// node inventory.
//
bool
NodeInfoShmIter::shm_obj_iter_fn(int         idx,
                                 const void *key,
                                 const void *rec,
                                 size_t      key_sz,
                                 size_t      rec_sz)
{
    int                 rw_idx;
    ep_map_rec_t        epmap;
    NodeAgent::pointer  agent;
    const fds_uint64_t *uuid;

    fds_assert(sizeof(*uuid) == key_sz);
    fds_assert(sizeof(node_data_t) == rec_sz);

    uuid = reinterpret_cast<const fds_uint64_t *>(key);
    if (*uuid == 0) {
        return ++it_no_rec < it_no_rec_quit ? true : false;
    }
    // My node is taken care by the domain master.
    if (*uuid == Platform::plf_get_my_node_uuid()->uuid_get_val()) {
        return true;
    }
    agent  = NULL;
    rw_idx = it_shm_rw == false ? -1 : idx;
    it_local->dc_register_node(it_shm, &agent, idx, rw_idx, NODE_DO_PROXY_ALL_SVCS);

    return true;
}

// nplat_refresh_shm
// -----------------
//
void
NetPlatSvc::nplat_refresh_shm()
{
    const ShmObjRO  *shm;
    NodeInfoShmIter  iter(false);

    shm = NodeShmCtrl::shm_node_inventory();
    shm->shm_iter_objs(&iter);
}

// nplat_register_node
// -------------------
//
void
NetPlatSvc::nplat_register_node(fpi::NodeInfoMsg *msg, NodeAgent::pointer node)
{
}

// nplat_register_node
// -------------------
// Platform lib which linked to FDS daemon registers node inventory based on RO data
// from the shared memory segment.
//
void
NetPlatSvc::nplat_register_node(const fpi::NodeInfoMsg *msg)
{
    int                     idx;
    node_data_t             rec;
    ep_map_rec_t            map;
    NodeAgent::pointer      agent;
    DomainContainer::pointer  local;

    NodeInventory::node_info_msg_to_shm(msg, &rec);
    EpPlatLibMod::ep_node_info_to_mapping(&rec, &map);

    /* Search through shm for matching key, return back the index */
    idx = 0;
    NetMgr::ep_mgr_singleton()->ep_register_binding(&map, idx);

    local = Platform::platf_singleton()->plf_node_inventory();
    // local->dc_register_node(shm, &agent, idx, -1);
}

// nplat_domain_rpc
// ----------------
// Get the RPC handles needed to contact the master platform services.
//
EpSvcHandle::pointer
NetPlatSvc::nplat_domain_rpc(const fpi::DomainID &id)
{
    return plat_self->pda_rpc_handle();
}

// nplat_my_ep
// -----------
//
EpSvcImpl::pointer
NetPlatSvc::nplat_my_ep()
{
    return plat_ep;
}

// nplat_set_my_ep
// ---------------
//
void
NetPlatSvc::nplat_set_my_ep(EpSvcImpl::pointer ep)
{
    fds_verify(plat_ep == NULL);
    plat_ep = static_cast<PlatNetEp *>(ep.get());
}

/*
 * -----------------------------------------------------------------------------------
 * Domain Agent
 *
 * This obj is also platform agent (DmAgent) but it has special methods to communicate
 * with "the" domain master.  In the future, domain master can move to other node but
 * the interface from this obj remains unchanged.  It always knows how to talk to "the"
 * domain master.
 * -----------------------------------------------------------------------------------
 */
DomainAgent::~DomainAgent() {}
DomainAgent::DomainAgent(const NodeUuid &uuid, bool alloc_plugin)
    : PmAgent(uuid), agt_domain_ep(NULL) {}

/**
 * pda_connect_domain
 * ------------------
 * Establish connection to the domain master.  Null domain implies local one.
 */
void
DomainAgent::pda_connect_domain(const fpi::DomainID &id)
{
    int                port;
    NetPlatform       *net;
    PlatNetEpPtr       eptr;

    net  = NetPlatform::nplat_singleton();
    eptr = ep_cast_ptr<PlatNetEp>(net->nplat_my_ep());
    fds_verify(eptr != NULL);

    std::string const *const om_ip = net->nplat_domain_master(&port);
    eptr->ep_new_handle(eptr, port, *om_ip, &agt_domain_ep, pm_ep_svc->ep_evt_plugin());
    LOGNORMAL << "Domain master ip " << *om_ip << ", port " << port;
}

/**
 * pda_register
 * ------------
 */
void
DomainAgent::pda_register()
{
    int                     idx;
    node_data_t             rec;
    fds_uint64_t            uid;
    ShmObjROKeyUint64      *shm;
    NodeAgent::pointer        agent;
    DomainContainer::pointer  local;

    uid = rs_uuid.uuid_get_val();
    shm = NodeShmCtrl::shm_node_inventory();
    idx = shm->shm_lookup_rec(static_cast<const void *>(&uid),
                              static_cast<void *>(&rec), sizeof(rec));
    fds_verify(idx != -1);

    agent = this;
    local = Platform::platf_singleton()->plf_node_inventory();
    local->dc_register_node(shm, &agent, idx, -1, NODE_DO_PROXY_ALL_SVCS);
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

}  // namespace fds
