/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <net/net-service.h>

namespace fds {

EndPointMgr  gl_EndPointMgr("EndPoint Mgr");

/*
 * -----------------------------------------------------------------------------------
 * EndPoint Attributes
 * -----------------------------------------------------------------------------------
 */
EpAttr::EpAttr(fds_uint32_t ip, int port)
{
}

/*
 * -----------------------------------------------------------------------------------
 * Base EndPoint Service Handler
 * -----------------------------------------------------------------------------------
 */
EpSvc::EpSvc(const ResourceUUID &uuid, fds_uint32_t major, fds_uint32_t minor)
{
}

EpSvc::EpSvc(const ResourceUUID &domain,
             const ResourceUUID &uuid,
             fds_uint32_t        major,
             fds_uint32_t        minor)
{
}

EpSvc::EpSvc(const NodeUuid       &mine,
             const NodeUuid       &peer,
             const EpAttr         &attr,
             EpEvtPlugin::pointer  ops)
{
}

// ep_bind_service
// ---------------
//
void
EpSvc::ep_bind_service(EpSvc::pointer svc)
{
}

// ep_unbind_service
// -----------------
//
EpSvc::pointer
EpSvc::ep_unbind_service(const fpi::SvcID &id)
{
    return NULL;
}

// ep_lookup_service
// -----------------
//
EpSvc::pointer
EpSvc::ep_lookup_service(const ResourceUUID &uuid)
{
    return NULL;
}

// ep_lookup_service
// -----------------
//
EpSvc::pointer
EpSvc::ep_lookup_service(const char *name)
{
    return NULL;
}

// ep_apply_attr
// -------------
//
void
EpSvc::ep_apply_attr()
{
}

/*
 * -----------------------------------------------------------------------------------
 * Base EP Event Plugin
 * -----------------------------------------------------------------------------------
 */
EpEvtPlugin::EpEvtPlugin() {}

// ep_cleanup_start
// ----------------
//
void
EpEvtPlugin::ep_cleanup_start()
{
}

// ep_cleanup_finish
// -----------------
//
void
EpEvtPlugin::ep_cleanup_finish()
{
}

// svc_cleanup_start
// -----------------
//
void
EpEvtPlugin::svc_cleanup_start()
{
}

// svc_cleanup_finish
// ------------------
//
void
EpEvtPlugin::svc_cleanup_finish()
{
}

/*
 * -----------------------------------------------------------------------------------
 * EndPoint Manager
 * -----------------------------------------------------------------------------------
 */
EndPointMgr::EndPointMgr(const char *name) : Module(name) {}

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
