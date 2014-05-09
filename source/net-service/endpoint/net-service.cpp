/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <net/net-service-tmpl.hpp>
#include <ep-map.h>

namespace fds {

NetMgr  gl_netService("EndPoint Mgr");

/*
 * -----------------------------------------------------------------------------------
 * EndPoint Manager
 * -----------------------------------------------------------------------------------
 */
NetMgr::NetMgr(const char *name)
    : Module(name), ep_shm(NULL), ep_mtx("Ep mtx")
{
    static Module *ep_mgr_mods[] = {
        &gl_EpPlatLibMod,
        NULL
    };
    ep_shm     = &gl_EpPlatLibMod;
    mod_intern = ep_mgr_mods;
}

// mod_init
// --------
//
int
NetMgr::mod_init(SysParams const *const p)
{
    Module::mod_init(p);
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
NetMgr::ep_register(EpSvc::pointer ep)
{
    int                 idx, port;
    ep_map_rec_t        map;
    EpSvcImpl::pointer  myep, exist;

    myep = EP_CAST_PTR(EpSvcImpl, ep);
    myep->ep_fillin_binding(&map);

    // Check if we have any ep operated on the same port.
    port  = EpAttr::netaddr_get_port(&map.rmp_addr);
    exist = EP_CAST_PTR(EpSvcImpl, ep_lookup_port(port));
    if (exist != NULL) {
        fds_verify(exist->ep_get_rcv_handler() == myep->ep_get_rcv_handler());
    }
    idx = ep_shm->ep_map_record(&map);

    // Add to the local mapping.
    ep_mtx.lock();
    ep_port_map[port] = ep;
    ep_uuid_map[ep->ep_my_uuid()] = idx;
    ep_mtx.unlock();
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

EpSvcHandle::pointer
EndPointMgr::svc_lookup(const fpi::SvcUuid &uuid, fds_uint32_t maj, fds_uint32_t min)
{
    return NULL;
}

// svc_lookup
// ----------
// Lookup a service handle based on uuid and correct version.
//
EpSvcHandle::pointer
NetMgr::svc_lookup(const ResourceUUID &peer, fds_uint32_t maj, fds_uint32_t min)
{
    return NULL;
}

// svc_lookup
// ----------
// Lookup a service handle based on its well-known name and correct version.
//
EpSvcHandle::pointer
NetMgr::svc_lookup(const char *peer_name, fds_uint32_t maj, fds_uint32_t min)
{
    return NULL;
}

EpSvc::pointer
NetMgr::ep_lookup_port(int port)
{
    return NULL;
}

}  // namespace fds
