/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <string>
#include <fds_process.h>
#include <net/net-service-tmpl.hpp>
#include <fdsp/PlatNetSvc.h>
#include <ep-map.h>
#include <fdsp/SMSvc.h>
#include <fdsp/DMSvc.h>
#include <fdsp/AMSvc.h>
#include <fdsp/FDSP_ControlPathReq.h>
#include <fdsp/Streaming.h>
#include <net/BaseAsyncSvcHandler.h>


// New includes
#include "platform/net_plat_svc.h"

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

// ep_get_task_executor
// --------------
//
SynchronizedTaskExecutor<uint64_t>*
NetMgr::ep_get_task_executor() {
    return ep_task_executor;
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
    ep_task_executor = new SynchronizedTaskExecutor<uint64_t>(*NetMgr::ep_mgr_thrpool());
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
    UuidIntKey          key;
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

        key.h_key  = mine;
        key.h_int1 = myep->ep_version(&key.h_int2);

        ep_mtx.lock();
        ep_uuid_map[key] = idx;
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
    if (ep->ep_is_connection() == true) {
        // TODO(Andrew): We're not joining on this thread right now...
        ep_register_thread = boost::shared_ptr<boost::thread>(
            new boost::thread(boost::bind(
                &NetMgr::ep_register_thr, this, ep, update_domain)));
    } else {
        /* This is the logical service, record it in svc map. */
        ep_mtx.lock();
        ep_svc_map[ep->ep_my_uuid()] = ep;
        ep_mtx.unlock();
    }
}

// ep_unregister
// -------------
// Unregister the endpoint.  This is lazy unregister.  Stale endpoint will be notified
// when the sender sends to it.
//
void
NetMgr::ep_unregister(const fpi::SvcUuid &uuid, fds_uint32_t maj, fds_uint32_t min)
{
    int            idx;
    UuidIntKey     key;
    EpSvc::pointer ep;

    key.h_key  = uuid.svc_uuid;
    key.h_int1 = maj;
    key.h_int2 = min;

    ep_mtx.lock();
    try {
        idx = ep_uuid_map.at(key);
        ep_uuid_map.erase(key);
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
    bool         notif;
    UuidIntKey   key;
    fpi::SvcUuid peer;

    notif = false;
    handle->ep_peer_uuid(peer);
    if (peer != NullSvcUuid) {
        key.h_key  = peer.svc_uuid;
        key.h_int1 = handle->ep_version(&key.h_int2);

        ep_mtx.lock();
        auto it = ep_handle_map.find(key);
        if (it == ep_handle_map.end()) {
            notif = true;
            ep_handle_map.insert(std::make_pair(key, handle));
        } else {
            handle = NULL;
        }
        ep_mtx.unlock();
    }
    if (notif == true) {
        handle->ep_notify_plugin(true);
    }
}

// ep_handler_unregister
// ---------------------
//
void
NetMgr::ep_handler_unregister(const fpi::SvcUuid &peer)
{
}

void
NetMgr::ep_handler_unregister(EpSvcHandle::pointer handle)
{
    UuidIntKey   key;
    fpi::SvcUuid peer;

    handle->ep_my_uuid(peer);
    key.h_key  = peer.svc_uuid;
    key.h_int1 = handle->ep_version(&key.h_int2);

    ep_mtx.lock();
    ep_handle_map.erase(key);
    ep_mtx.unlock();
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
NetMgr::svc_handler_lookup(const fpi::SvcUuid &uuid, fds_uint32_t maj, fds_uint32_t min)
{
    UuidIntKey            key;
    EpSvcHandle::pointer  client;

    key.h_key  = uuid.svc_uuid;
    key.h_int1 = maj;
    key.h_int2 = min;

    ep_mtx.lock();
    try {
        client = ep_handle_map.at(key);
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
            for (auto it = list.cbegin(); it != list.cend(); ++it) {
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
NetMgr::ep_uuid_binding(const fpi::SvcUuid &uuid,
                        fds_uint32_t        maj,
                        fds_uint32_t        min,
                        std::string        *ip)
{
    int          idx, ret;
    char         str[INET6_ADDRSTRLEN + 1];
    UuidIntKey   key;
    ep_map_rec_t map;

    key.h_int1 = maj;
    key.h_int2 = min;
    key.h_key  = uuid.svc_uuid;

    ep_mtx.lock();
    try {
        idx = ep_uuid_map.at(key);
    } catch(...) {
        idx = -1;
    }
    ep_mtx.unlock();

    if (idx < 0) {
        // Don't have it in the cache, lookup in shared memory.
        idx = ep_shm->ep_lookup_rec(uuid.svc_uuid, maj, min, &map);
        if (idx == -1) {
            idx = ep_shm->node_info_lookup(uuid.svc_uuid, &map);
            if (idx == -1) {
                ip->clear();
                return -1;
            }
        }
        // Cache the binding to the shared memory location.
        ep_register_binding(&map, idx);
    } else {
        ret = ep_shm->ep_lookup_rec(idx, uuid.svc_uuid, maj, min, &map);
        if (ret == -1) {
            idx = ep_shm->node_info_lookup(idx, uuid.svc_uuid, &map);
        } else {
            idx = ret;
        }
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
    int         port;
    UuidIntKey  key;

    fds_assert(idx >= 0);
    key.h_key  = rec->rmp_uuid;
    key.h_int1 = rec->rmp_major;
    key.h_int2 = rec->rmp_minor;

    port = EpAttr::netaddr_get_port(&rec->rmp_addr);
    if ((rec->rmp_uuid == 0) || (port == 0)) {
        // TODO(Vy): log the error.
        return;
    }
    ep_mtx.lock();
    ep_uuid_map[key] = idx;
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
// ------------
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
NetMgr::ep_handle_error(const fpi::SvcUuid &uuid,
                        const Error        &e,
                        fds_uint32_t        maj,
                        fds_uint32_t        min)
{
    fds_threadpool *pool;

    pool = ep_mgr_thrpool();
    pool->schedule(&NetMgr::ep_handle_error_thr, this,
                   new fpi::SvcUuid(uuid), new Error(e), maj, min);
}

// ep_handle_error_thr
// -------------------
// Wire up errors in data path to the correct handling object and handle it in the
// common threadpool.
//
void
NetMgr::ep_handle_error_thr(fpi::SvcUuid *uuid, Error *e,
                            fds_uint32_t  maj,
                            fds_uint32_t  min)
{
    EpSvc::pointer       svc;
    EpSvcHandle::pointer eph;

    eph = svc_handler_lookup(*uuid, maj, min);
    if (eph != NULL) {
        eph->ep_handle_error(*e);
    } else {
        svc = svc_lookup(*uuid, maj, min);
        if (svc != NULL) {
            svc->ep_handle_error(*e);
        } else {
            LOGDEBUG << "Can not lookup uuid " << std::hex << uuid->svc_uuid
                << ", err: " << e->GetErrstr();
        }
    }
    delete uuid;
    delete e;
}

fpi::AsyncHdr
NetMgr::ep_swap_header(const fpi::AsyncHdr &req_hdr)
{
    auto resp_hdr = req_hdr;
    resp_hdr.msg_src_uuid = req_hdr.msg_dst_uuid;
    resp_hdr.msg_dst_uuid = req_hdr.msg_src_uuid;
    return resp_hdr;
}

boost::shared_ptr<void>
NetMgr::alloc_rpc_client(const fpi::SvcUuid   &peer,
                         fds_uint32_t          maj,
                         fds_uint32_t          min,
                         bo::shared_ptr<tp::TBinaryProtocol>& protocol)
{
    ResourceUUID resId(peer.svc_uuid);
    switch (resId.uuid_get_type()) {
        case fpi::FDSP_STOR_MGR:
            fds_verify(maj == 0 && min == 0);
            return bo::make_shared<fpi::SMSvcClient>(protocol);
        case fpi::FDSP_DATA_MGR:
            fds_verify(maj == 0 && min == 0);
            return bo::make_shared<fpi::DMSvcClient>(protocol);
        case fpi::FDSP_STOR_HVISOR:
            fds_verify(maj == 0 && (min == 0 || min ==2));
            if (min == 2) {
                return bo::make_shared<fpi::StreamingClient>(protocol);
            }
            return bo::make_shared<fpi::AMSvcClient>(protocol);
        case fpi::FDSP_ORCH_MGR:
            return bo::make_shared<fpi::PlatNetSvcClient>(protocol);
        case fpi::FDSP_PLATFORM:
            if (min == 1) {
                return bo::make_shared<fpi::FDSP_ControlPathReqClient>(protocol);
            }
            return bo::make_shared<fpi::PlatNetSvcClient>(protocol);
        default:
            fds_verify(!"Invalid type");
    }
    return nullptr;
}

}  // namespace fds
