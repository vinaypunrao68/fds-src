/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <string>
#include <net/net-service-tmpl.hpp>
#include <platform/platform-lib.h>
#include <fdsp/PlatNetSvc.h>
#include <ep-map.h>

namespace fds {

NetMgr              gl_netService("EndPoint Mgr");
const fpi::SvcUuid  NullSvcUuid;

/*
 * -----------------------------------------------------------------------------------
 * EndPoint Manager
 * -----------------------------------------------------------------------------------
 */
NetMgr::~NetMgr() {}
NetMgr::NetMgr(const char *name)
    : Module(name), plat_lib(NULL), ep_shm(NULL), ep_domain_clnt(NULL), ep_mtx("Ep mtx")
{
    static Module *ep_mgr_mods[] = {
        &gl_EpPlatLib,
        NULL
    };
    ep_shm     = &gl_EpPlatLib;
    mod_intern = ep_mgr_mods;
}

// mod_init
// --------
//
int
NetMgr::mod_init(SysParams const *const p)
{
    Module::mod_init(p);
    plat_lib = Platform::platf_singleton();
    return 0;
}

// mod_startup
// -----------
//
void
NetMgr::mod_startup()
{
}

// mod_shutdown
// ------------
//
void
NetMgr::mod_shutdown()
{
}

// ep_register
// -----------
// Register the endpoint to the local domain.
//
void
NetMgr::ep_register(EpSvc::pointer ep, bool update_domain)
{
    int                 idx, port;
    fds_uint64_t        uuid, peer;
    ep_map_rec_t        map;
    EpSvcImpl::pointer  myep, exist;

    myep = EP_CAST_PTR(EpSvcImpl, ep);
    myep->ep_fillin_binding(&map);

    // Check if we have any ep operated on the same port.
    port  = EpAttr::netaddr_get_port(&map.rmp_addr);
    exist = EP_CAST_PTR(EpSvcImpl, ep_lookup_port(port));
    if (exist != NULL) {
        // fds_verify(exist->ep_get_rcv_handler() == myep->ep_get_rcv_handler());
    }
    idx = ep_shm->ep_map_record(&map);

    // Add to the local mapping.
    if (idx != -1) {
        ep_mtx.lock();
        if (port != 0) {
            ep_port_map[port].push_back(ep);
        }
        uuid = myep->ep_my_uuid();
        ep_uuid_map[uuid] = idx;
        ep_svc_map[uuid]  = ep;

        peer = myep->ep_peer_uuid();
        if ((peer != 0) && (peer != uuid)) {
            ep_svc_map[peer] = ep;
        }
        ep_mtx.unlock();
    }
}

// ep_unregister
// -------------
// Unregister the endpoint.  This is lazy unregister.  Stale endpoint will be notified
// when the sender sends to the down endpoint.
//
void
NetMgr::ep_unregister(const fpi::SvcUuid &uuid)
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
    } catch(const std::out_of_range &oor) {
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

// svc_domain_master
// -----------------
//
EpSvcHandle::pointer
NetMgr::svc_domain_master(const fpi::DomainID &id,
                          bo::shared_ptr<fpi::PlatNetSvcClient> &rpc)
{
    int                            port;
    bo::shared_ptr<void>           ep_rpc;
    bo::shared_ptr<tt::TTransport> trans;

    if (ep_domain_clnt == NULL) {
        const std::string *ip = plat_lib->plf_get_om_ip();

        port = plat_lib->plf_get_om_svc_port();
        endpoint_connect_server<fpi::PlatNetSvcClient>(port, *ip, rpc, trans);

        ep_rpc = rpc;
        ep_domain_clnt = new EpSvcHandle(NULL, ep_rpc, trans);
    }
    return ep_domain_clnt;
}

// endpoint_lookup
// ---------------
//
EpSvc::pointer
NetMgr::endpoint_lookup(const fpi::SvcUuid &uuid)
{
    EpSvc::pointer ep;

    ep = svc_lookup(uuid, 0, 0);
    if (ep != NULL) {
        if (ep->ep_is_connection()) {
            return ep;
        }
    }
    return NULL;
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
//
int
NetMgr::ep_uuid_binding(const fpi::SvcUuid &uuid, std::string *ip)
{
    int          idx;
    ep_map_rec_t map;

    ep_mtx.lock();
    try {
        idx = ep_uuid_map.at(uuid.svc_uuid);
    } catch(const std::out_of_range &oor) {
        idx = -1;
    }
    ep_mtx.unlock();

    if (idx < 0) {
        ip->clear();
        return 0;
    }
    if (ep_shm->ep_lookup_rec(idx, uuid.svc_uuid, &map) >= 0) {
        ip->reserve(INET6_ADDRSTRLEN + 1);
        EpAttr::netaddr_to_str(&map.rmp_addr,
                               const_cast<char *>(ip->c_str()), INET6_ADDRSTRLEN);
        return EpAttr::netaddr_get_port(&map.rmp_addr);
    }
    return 0;
}

// ep_register_binding
// -------------------
//
void
NetMgr::ep_register_binding(const ep_map_rec_t *rec)
{
    int                idx;
    EpSvc::pointer     ep;
    EpSvcImpl::pointer ept;

    if (rec->rmp_uuid == 0) {
        return;
    }
    idx = ep_shm->ep_map_record(rec);
    if (idx != -1) {
        ep_mtx.lock();
        ep_uuid_map[rec->rmp_uuid] = idx;
        try {
            ep  = ep_svc_map.at(rec->rmp_uuid);
            ept = EP_CAST_PTR(EpSvcImpl, ep);
        } catch(const std::out_of_range &oor) {
            ept = NULL;
        }
        ep_mtx.unlock();
        if (ept != NULL) {
        }
    }
}

// ep_my_platform_uuid
// -------------------
//
ResourceUUID const *const
NetMgr::ep_my_platform_uuid()
{
    return plat_lib->plf_get_my_uuid();
}

// ep_actionable_error
// -------------------
//
bool
NetMgr::ep_actionable_error(const fpi::SvcUuid &uuid, const Error &e) const
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

}  // namespace fds
