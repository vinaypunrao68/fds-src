/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <net/net-service-tmpl.hpp>
#include <ep-map.h>

namespace fds {

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

    ep_mgr = NetMgr::ep_mgr_singleton();
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

// ep_fillin_binding
// -----------------
//
void
EpSvcImpl::ep_fillin_binding(ep_map_rec_t *rec)
{
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

}  // namespace fds
