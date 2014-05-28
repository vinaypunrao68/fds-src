/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <string>
#include <fds_process.h>
#include <platform/platform-lib.h>
#include <net/net-service-tmpl.hpp>
#include <fdsp/PlatNetSvc.h>
#include <ep-map.h>
#include <net-plat-shared.h>

namespace fds {

NetMgr              gl_NetService("EndPoint Mgr");
const fpi::SvcUuid  NullSvcUuid;

/*
 * -----------------------------------------------------------------------------------
 * EndPoint Manager
 * -----------------------------------------------------------------------------------
 */
NetMgr::~NetMgr() {}
NetMgr::NetMgr(const char *name)
    : Module(name), plat_lib(NULL), ep_shm(NULL),
      ep_domain_clnt(NULL), ep_mtx("Ep mtx") {}

// ep_mgr_thrpool
// --------------
//
fds_threadpool *
NetMgr::ep_mgr_thrpool()
{
    return g_fdsprocess->proc_thrpool();
}

// mod_init
// --------
//
int
NetMgr::mod_init(SysParams const *const p)
{
    static Module *ep_mgr_mods[] = {
        gl_EpShmPlatLib,
        gl_NetPlatSvc,
        NULL
    };
    ep_shm     = gl_EpShmPlatLib;
    plat_lib   = Platform::platf_singleton();
    plat_net   = static_cast<NetPlatSvc *>(gl_NetPlatSvc);
    mod_intern = ep_mgr_mods;
    return Module::mod_init(p);
}

// mod_startup
// -----------
//
void
NetMgr::mod_startup()
{
    Module::mod_startup();
}

// mod_enable_services
// -------------------
//
void
NetMgr::mod_enable_service()
{
    Module::mod_enable_service();
}

// mod_shutdown
// ------------
//
void
NetMgr::mod_shutdown()
{
    Module::mod_shutdown();
}

// ep_register_thr
// ---------------
// Register the endpoint to the local domain.
//
void
NetMgr::ep_register_thr(EpSvc::pointer ep, bool update_domain)
{
    int                 idx, port;
    fds_uint64_t        mine, peer;
    ep_map_rec_t        map;
    EpSvcImpl::pointer  myep;

    myep = ep_cast_ptr<EpSvcImpl>(ep);
    myep->ep_fillin_binding(&map);

    port = EpAttr::netaddr_get_port(&map.rmp_addr);
    fds_verify(port != 0);
    fds_verify(map.rmp_uuid != 0);
    fds_assert(map.rmp_uuid == myep->ep_my_uuid());

    // TODO(Vy): Check if we have any ep operated on the same port.
    //
    idx = ep_shm->ep_map_record(&map);

    // Add to the local mapping cache.
    if (idx != -1) {
        mine = myep->ep_my_uuid();
        peer = myep->ep_peer_uuid();

        ep_mtx.lock();
        ep_uuid_map[mine] = idx;
        if (myep->ep_is_connection() == true) {
            // This is the physical endpoint, record it in ep map.
            ep_map[mine].push_front(ep);
            if ((peer != 0) && (peer != mine)) {
                ep_map[peer].push_front(ep);
            }
        } else {
            // This is the logical service, record it in svc map.
            ep_svc_map[mine] = ep;
            if ((peer != 0) && (peer != mine)) {
                ep_svc_map[peer] = ep;
            }
        }
        ep_mtx.unlock();
    }
    if (update_domain == true) {
    }
    myep->ep_activate();
    // TODO(Vy): error handling.
}

// ep_register
// -----------
//
void
NetMgr::ep_register(EpSvc::pointer ep, bool update_domain)
{
    fds_threadpool *pool;

    pool = ep_mgr_thrpool();
    pool->schedule(&NetMgr::ep_register_thr, this, ep, update_domain);
}

// ep_unregister
// -------------
// Unregister the endpoint.  This is lazy unregister.  Stale endpoint will be notified
// when the sender sends to it.
//
void
NetMgr::ep_unregister(const fpi::SvcUuid &uuid)
{
    int            idx;
    EpSvc::pointer ep;

    ep_mtx.lock();
    try {
        idx = ep_uuid_map.at(uuid.svc_uuid);
        ep_uuid_map.erase(uuid.svc_uuid);
    } catch(...) {
        idx = -1;
    }

    try {
        ep = ep_svc_map.at(uuid.svc_uuid);
        ep_svc_map.erase(uuid.svc_uuid);
    } catch(...) {}

    try {
        EpSvcList &list = ep_map.at(uuid.svc_uuid);
        list.clear();
        ep_map.erase(uuid.svc_uuid);
    } catch(...) {}
    ep_mtx.unlock();

    if (idx != -1) {
        ep_shm->ep_unmap_record(uuid.svc_uuid, idx);
        // Need to ask domain controller to unmap
    }
}

// ep_handler_register
// -------------------
//
void
NetMgr::ep_handler_register(EpSvcHandle::pointer handle)
{
    fpi::SvcUuid peer;

    handle->ep_peer_uuid(peer);
    fds_verify(peer != NullSvcUuid);

    ep_mtx.lock();
    ep_handle_map[peer.svc_uuid] = handle;
    ep_mtx.unlock();
}

// ep_handler_unregister
// ---------------------
//
void
NetMgr::ep_handler_unregister(const fpi::SvcUuid &peer)
{
    ep_mtx.lock();
    ep_handle_map.erase(peer.svc_uuid);
    ep_mtx.unlock();
}

void
NetMgr::ep_handler_unregister(EpSvcHandle::pointer handle)
{
}

// svc_lookup
// ----------
// Lookup a service handle based on uuid and correct version.
//
EpSvc::pointer
NetMgr::svc_lookup(const fpi::SvcUuid &svc_uuid, fds_uint32_t maj, fds_uint32_t min)
{
    EpSvc::pointer    ep;

    ep_mtx.lock();
    try {
        ep = ep_svc_map.at(svc_uuid.svc_uuid);
    } catch(...) {
        ep = NULL;
    }
    ep_mtx.unlock();

    return ep;
}

// svc_lookup
// ----------
// Lookup a service handle based on its well-known name and correct version.
//
EpSvc::pointer
NetMgr::svc_lookup(const char *peer_name, fds_uint32_t maj, fds_uint32_t min)
{
    return NULL;
}

// svc_handler_lookup
// ------------------
//
EpSvcHandle::pointer
NetMgr::svc_handler_lookup(const fpi::SvcUuid &uuid)
{
    EpSvcHandle::pointer  client;

    ep_mtx.lock();
    try {
        client = ep_handle_map[uuid.svc_uuid];
    } catch(...) {
        client = NULL;
    }
    ep_mtx.unlock();

    return client;
}

// svc_domain_master
// -----------------
//
EpSvcHandle::pointer
NetMgr::svc_domain_master(const fpi::DomainID &id,
                          bo::shared_ptr<fpi::PlatNetSvcClient> &rpc)
{
    return ep_domain_clnt;
}

// endpoint_lookup
// ---------------
//
EpSvc::pointer
NetMgr::endpoint_lookup(const fpi::SvcUuid &peer)
{
    EpSvc::pointer ep;

    ep_mtx.lock();
    try {
        EpSvcList &list = ep_map.at(peer.svc_uuid);
        if (list.empty()) {
            ep_map.erase(peer.svc_uuid);
        } else {
            // Just return the front of the list and swap it to the back.
            ep = list.front();
            fds_verify(ep != NULL);
        }
    } catch(...) {
        ep = NULL;
    }
    ep_mtx.unlock();
    return ep;
}

EpSvc::pointer
NetMgr::endpoint_lookup(const fpi::SvcUuid &mine, const fpi::SvcUuid &peer)
{
    EpSvc::pointer ep;

    ep_mtx.lock();
    try {
        EpSvcList &list = ep_map.at(peer.svc_uuid);
        if (list.empty()) {
            ep_map.erase(peer.svc_uuid);
        } else {
            ep = NULL;
            for (auto it = list.cbegin(); it != list.cend(); it++) {
                if ((*it)->ep_my_uuid() == (fds_uint64_t)mine.svc_uuid) {
                    ep = *it;
                    break;
                }
            }
        }
    } catch(...) {
        ep = NULL;
    }
    ep_mtx.unlock();
    return ep;
}

EpSvc::pointer
NetMgr::endpoint_lookup(const char *name)
{
    EpSvc::pointer ep;

    ep = svc_lookup(name, 0, 0);
    if (ep != NULL) {
        if (ep->ep_is_connection()) {
            return ep;
        }
    }
    return NULL;
}

// ep_lookup_port
// --------------
//
EpSvc::pointer
NetMgr::ep_lookup_port(int port)
{
    return NULL;
}

// ep_uuid_bindings
// ----------------
//
int
NetMgr::ep_uuid_bindings(const struct ep_map_rec **map)
{
    return 0;
}

// ep_uuid_binding
// ---------------
// Return the <ip, port> binding to the given uuid.
//
int
NetMgr::ep_uuid_binding(const fpi::SvcUuid &uuid, std::string *ip)
{
    int          idx, ret;
    char         str[INET6_ADDRSTRLEN + 1];
    ep_map_rec_t map;

    ep_mtx.lock();
    try {
        idx = ep_uuid_map.at(uuid.svc_uuid);
    } catch(...) {
        idx = -1;
    }
    ep_mtx.unlock();

    if (idx < 0) {
        ip->clear();
        return -1;
    }
    ret = ep_shm->ep_lookup_rec(idx, uuid.svc_uuid, &map);
    if (ret == -1) {
        idx = ep_shm->node_info_lookup(idx, uuid.svc_uuid, &map);
    } else {
        idx = ret;
    }
    if (idx >= 0) {
        EpAttr::netaddr_to_str(&map.rmp_addr, str, INET6_ADDRSTRLEN);
        ip->assign(str);
        return EpAttr::netaddr_get_port(&map.rmp_addr);
    }
    return -1;
}

// ep_register_binding
// -------------------
//
void
NetMgr::ep_register_binding(const ep_map_rec_t *rec, int idx)
{
    int port;

    fds_assert(idx >= 0);
    port = EpAttr::netaddr_get_port(&rec->rmp_addr);
    if ((rec->rmp_uuid == 0) || (port == 0)) {
        // TODO(Vy): log the error.
        return;
    }
    ep_mtx.lock();
    ep_uuid_map[rec->rmp_uuid] = idx;
    ep_mtx.unlock();
}

// ep_my_platform_uuid
// -------------------
//
ResourceUUID const *const
NetMgr::ep_my_platform_uuid()
{
    return plat_lib->plf_get_my_node_uuid();
}

// ep_get_timer
// -------------------
//
FdsTimerPtr NetMgr::ep_get_timer() const
{
    return g_fdsprocess->getTimer();
}

// ep_actionable_error
// -------------------
//
bool
NetMgr::ep_actionable_error(/*const fpi::SvcUuid &uuid, */const Error &e) const
{
    return false;
}

// ep_handle_error
// ---------------
//
void
NetMgr::ep_handle_error(const fpi::SvcUuid &uuid, const Error &e)
{
}

fpi::AsyncHdr NetMgr::ep_swap_header(const fpi::AsyncHdr &req_hdr)
{
    auto resp_hdr = req_hdr;
    resp_hdr.msg_src_uuid = req_hdr.msg_dst_uuid;
    resp_hdr.msg_dst_uuid = req_hdr.msg_src_uuid;
    return resp_hdr;
}
}  // namespace fds
