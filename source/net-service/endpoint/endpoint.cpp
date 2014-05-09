/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <net/net-service.h>
#include <net/net-service-tmpl.hpp>

namespace fds {

/*
 * -----------------------------------------------------------------------------------
 * EndPoint Attributes
 * -----------------------------------------------------------------------------------
 */
EpAttr::EpAttr(fds_uint32_t ip, int port) : ep_refcnt(0)
{
    ep_addr.sa_family = AF_INET;
    ((struct sockaddr_in *)&ep_addr)->sin_port = port;
}

EpAttr &
EpAttr::operator = (const EpAttr &rhs)
{
    ep_addr = rhs.ep_addr;
    return *this;
}

int
EpAttr::attr_get_port()
{
    if (ep_addr.sa_family == AF_INET) {
        return (((struct sockaddr_in *)&ep_addr)->sin_port);
    }
    return (((struct sockaddr_in6 *)&ep_addr)->sin6_port);
}

/*
 * -----------------------------------------------------------------------------------
 * Base EndPoint Service Handler
 * -----------------------------------------------------------------------------------
 */
EpSvc::EpSvc(const ResourceUUID &uuid, fds_uint32_t major, fds_uint32_t minor)
    : ep_evt(NULL), ep_attr(NULL), svc_domain(NULL), ep_refcnt(0)
{
    svc_ver.ver_major = major;
    svc_ver.ver_minor = minor;
    uuid.uuid_assign(&svc_id.svc_uuid);

    ep_mgr = EndPointMgr::ep_mgr_singleton();
}

EpSvc::EpSvc(const ResourceUUID &domain,
             const ResourceUUID &uuid,
             fds_uint32_t        major,
             fds_uint32_t        minor) : EpSvc(uuid, major, minor)
{
    svc_domain = new fpi::DomainID();
    domain.uuid_assign(&svc_domain->domain_id);
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
 * Base EndPoint Implementation
 * -----------------------------------------------------------------------------------
 */
EpSvcImpl::~EpSvcImpl() {}

EpSvcImpl::EpSvcImpl(const NodeUuid       &mine,
                     const NodeUuid       &peer,
                     const EpAttr         &attr,
                     EpEvtPlugin::pointer  ops) : EpSvc(mine, 0, 0)
{
    peer.uuid_assign(&ep_peer_id.svc_uuid);
    ep_evt   = ops;
    ep_attr  = new EpAttr();
    *ep_attr = attr;
    if (ep_evt != NULL) {
        ep_evt->assign_ep_owner(this);
    }
}

EpSvcImpl::EpSvcImpl(const fpi::SvcID     &mine,
                     const fpi::SvcID     &peer,
                     const EpAttr         &attr,
                     EpEvtPlugin::pointer  ops)
    : EpSvcImpl(ResourceUUID(mine.svc_uuid), ResourceUUID(peer.svc_uuid), attr, ops)
{
    ep_peer_id = peer;
}

// ep_bind_service
// ---------------
//
void
EpSvcImpl::ep_bind_service(EpSvc::pointer svc)
{
}

// ep_unbind_service
// -----------------
//
EpSvc::pointer
EpSvcImpl::ep_unbind_service(const fpi::SvcID &id)
{
    return NULL;
}

// ep_lookup_service
// -----------------
//
EpSvc::pointer
EpSvcImpl::ep_lookup_service(const ResourceUUID &uuid)
{
    return NULL;
}

// ep_lookup_service
// -----------------
//
EpSvc::pointer
EpSvcImpl::ep_lookup_service(const char *name)
{
    return NULL;
}

/*
 * -----------------------------------------------------------------------------------
 * Base EP Event Plugin
 * -----------------------------------------------------------------------------------
 */
EpEvtPlugin::EpEvtPlugin() : ep_refcnt(0) {}

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

#if 0
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
EndPointMgr::svc_lookup(const fpi::SvcUuid &peer, fds_uint32_t maj, fds_uint32_t min)
{
    return NULL;
}
// TODO(Rao): Remove
EpSvcHandle::pointer
EndPointMgr::svc_lookup(const ResourceUUID &uuid, fds_uint32_t maj, fds_uint32_t min)
{
    return nullptr;
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

#endif
}  // namespace fds
