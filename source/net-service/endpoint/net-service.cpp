/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <string>
#include <net/net-service-tmpl.hpp>
#include <platform/platform-lib.h>
#include <ep-map.h>
#include <fdsp/PlatNetSvc.h>

namespace fds {

NetMgr  gl_netService("EndPoint Mgr");

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
    int               idx;
    fds_uint64_t      uuid;
    ep_map_rec_t      map;
    EpSvc::pointer    ep;

    uuid = svc_uuid.svc_uuid;
    ep_mtx.lock();
    try {
        idx = ep_uuid_map.at(uuid);
    } catch(const std::out_of_range &oor) {
        idx = -1;
    }
    try {
        ep = ep_svc_map.at(uuid);
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
                          boost::shared_ptr<fpi::PlatNetSvcClient> *rpc)
{
    if (ep_domain_clnt == NULL) {
        bo::shared_ptr<tt::TTransport> trans;
        int port = plat_lib->plf_get_om_svc_port();
        const std::string *ip = plat_lib->plf_get_om_ip();

        endpoint_connect_server<fpi::PlatNetSvcClient>(port, *ip, rpc, &trans);
        ep_domain_clnt =
            new EpSvcHandle(NULL, boost::static_pointer_cast<void>(*rpc), trans);
    }
    return ep_domain_clnt;
}

EpSvc::pointer
NetMgr::endpoint_lookup(const fpi::SvcUuid &uuid)
{
    return NULL;
}

EpSvc::pointer
NetMgr::endpoint_lookup(const char *name)
{
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

}  // namespace fds
