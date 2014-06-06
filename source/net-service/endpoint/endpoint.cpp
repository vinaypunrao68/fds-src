/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <string>
#include <net/net-service-tmpl.hpp>
#include <ep-map.h>

namespace fds {

/*
 * -----------------------------------------------------------------------------------
 * Base EndPoint Service Handler
 * -----------------------------------------------------------------------------------
 */
EpSvc::EpSvc(const ResourceUUID &uuid, fds_uint32_t major, fds_uint32_t minor)
    : ep_refcnt(0), ep_evt(NULL), ep_attr(NULL), svc_domain(NULL)
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

// ep_fmt_uuid_binding
// -------------------
//
void
EpSvc::ep_fmt_uuid_binding(fpi::UuidBindMsg *msg, fpi::DomainID *domain)
{
    char ip[INET6_ADDRSTRLEN + 1];

    msg->svc_port = EpAttr::netaddr_get_port(&ep_attr->ep_addr);
    msg->svc_id   = svc_id;

    EpAttr::netaddr_to_str(&ep_attr->ep_addr, ip, INET6_ADDRSTRLEN);
    msg->svc_addr.assign(ip);

    if ((domain != NULL) && (svc_domain != NULL)) {
        *domain = *svc_domain;
    }
}

/*
 * -----------------------------------------------------------------------------------
 * EndPoint Handler
 * -----------------------------------------------------------------------------------
 */
EpSvcHandle::EpSvcHandle()
    : ep_refcnt(0), ep_state(EP_ST_INIT),
      ep_owner(NULL), ep_plugin(NULL), ep_rpc(NULL), ep_trans(NULL) {}

EpSvcHandle::EpSvcHandle(EpSvc::pointer svc, EpEvtPlugin::pointer evt) : EpSvcHandle()
{
    ep_plugin = evt;
    ep_owner  = svc;
    if (svc != NULL) {
        svc->ep_peer_uuid(ep_peer_id);
    } else {
        ep_peer_id = NullSvcUuid;
    }
    if (evt != NULL) {
        evt->assign_ep_handle(this);
    }
}

EpSvcHandle::~EpSvcHandle()
{
    if (ep_trans != NULL) {
        ep_trans->close();
    }
}

// ep_reconnect
// ------------
//
int
EpSvcHandle::ep_reconnect()
{
    NetMgr    *net;
    fds_mutex *mtx;

    fds_verify((ep_rpc != NULL) && (ep_trans != NULL));
    if ((ep_state == EP_ST_CONNECTED) || (ep_state == EP_ST_CONNECTING)) {
        return 0;
    }
    net = NetMgr::ep_mgr_singleton();
    mtx = net->ep_obj_mutex(this);

    mtx->lock();
    if (ep_state != EP_ST_CONNECTING) {
        ep_state = EP_ST_CONNECTING;
        mtx->unlock();

        try {
            ep_trans->open();
            ep_state = EP_ST_CONNECTED;
            ep_notify_plugin();
        } catch(...) {
            ep_state = EP_ST_DISCONNECTED;
            // TODO(Vy): Fix this
            return 0;
            sleep(1);
            net->ep_mgr_thrpool()->schedule(&EpSvcHandle::ep_reconnect, this);
        }
    } else {
        mtx->unlock();
    }
    return 0;
}

// ep_notify_plugin
// ----------------
//
void
EpSvcHandle::ep_notify_plugin()
{
    // TODO(Vy): Enable this verify
    // fds_verify(ep_plugin != NULL);
    if (ep_state == EP_ST_CONNECTED) {
        if (ep_plugin) {
            ep_plugin->ep_connected();
        }
    }
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
    ep_peer  = NULL;
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

EpSvcImpl::EpSvcImpl(const fpi::SvcID     &mine,
                     const fpi::SvcID     &peer,
                     EpEvtPlugin::pointer  ops)
    : EpSvc(ResourceUUID(mine.svc_uuid), 0, 0)
{
    ep_peer_id = peer;
    ep_evt     = ops;
    ep_attr    = NULL;
    if (ep_evt != NULL) {
        ep_evt->assign_ep_owner(this);
    }
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

// ep_input_event
// --------------
//
void
EpSvcImpl::ep_input_event(fds_uint32_t evt)
{
}

// ep_reconnect
// ------------
//
void
EpSvcImpl::ep_reconnect()
{
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
    rec->rmp_uuid = svc_id.svc_uuid.svc_uuid;
    memcpy(&rec->rmp_addr, &ep_attr->ep_addr, sizeof(rec->rmp_addr));
    strncpy(rec->rmp_name, svc_id.svc_name.c_str(), MAX_SVC_NAME_LEN - 1);
}

void
EpSvcImpl::ep_connect_peer(int port, const std::string &ip)
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
