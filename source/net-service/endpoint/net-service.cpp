/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <net/net-service.h>
#include <ep-map.h>

namespace fds {

EndPointMgr  gl_netService("EndPoint Mgr");

/*
 * -----------------------------------------------------------------------------------
 * EndPoint Manager
 * -----------------------------------------------------------------------------------
 */
EndPointMgr::EndPointMgr(const char *name) : Module(name), ep_map(NULL)
{
    static Module *ep_mgr_mods[] = {
        &gl_EpPlatLibMod,
        NULL
    };
    ep_map     = &gl_EpPlatLibMod;
    mod_intern = ep_mgr_mods;
}

// mod_init
// --------
//
int
EndPointMgr::mod_init(SysParams const *const p)
{
    Module::mod_init(p);
    return 0;
}

// mod_startup
// -----------
//
void
EndPointMgr::mod_startup()
{
}

// mod_shutdown
// ------------
//
void
EndPointMgr::mod_shutdown()
{
}

// ep_register
// -----------
// Register the endpoint to the local domain.
//
void
EndPointMgr::ep_register(EpSvc::pointer ep)
{
}

// ep_unregister
// -------------
// Unregister the endpoint.  This is lazy unregister.  Stale endpoint will be notified
// when the sender sends to the down endpoint.
//
void
EndPointMgr::ep_unregister(const fpi::SvcUuid &uuid)
{
}

// svc_lookup
// ----------
// Lookup a service handle based on uuid and correct version.
//
EpSvcHandle::pointer
EndPointMgr::svc_lookup(const ResourceUUID &peer, fds_uint32_t maj, fds_uint32_t min)
{
    return NULL;
}

// svc_lookup
// ----------
// Lookup a service handle based on its well-known name and correct version.
//
EpSvcHandle::pointer
EndPointMgr::svc_lookup(const char *peer_name, fds_uint32_t maj, fds_uint32_t min)
{
    return NULL;
}

}  // namespace fds
