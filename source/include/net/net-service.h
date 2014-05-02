/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_NET_NET_SERVICE_H_
#define SOURCE_INCLUDE_NET_NET_SERVICE_H_

#include <netinet/in.h>
#include <fds_module.h>
#include <fds_typedefs.h>
#include <shared/fds-constants.h>
#include <fdsp/fds_service_types.h>
#include <boost/intrusive_ptr.hpp>

namespace fds {

class EpSvc;
class EpEvtPlugin;
class EndPointMgr;

/**
 * -------------------
 * Theory of Operation
 * -------------------
 *
 * Concepts
 *   Endpoint provides RPC semantic.  With an endpoint, caller can make RPC call from the
 *   contract defined by an IDL file (Thrift).
 *
 *   Service provides message passing semantic to enable eligible module to communicate
 *   with the provider in the domain where the service registered.
 */
class EpAttr
{
  public:
    typedef boost::intrusive_ptr<EpAttr> pointer;
    typedef boost::intrusive_ptr<const EpAttr> const_ptr;

    // Physcial attributes of an endpoint.
    //
    struct sockaddr_in       ep_addr;

    // Server attributes.
    //

    // Methods.
    EpAttr();
    virtual ~EpAttr() {}

    EpAttr(fds_uint32_t ip, int port);  /**< ip = 0, default IP of the node */

  private:
    mutable boost::atomic<int>       ep_refcnt;

    friend void intrusive_ptr_add_ref(const EpAttr *x) {
        x->ep_refcnt.fetch_add(1, boost::memory_order_relaxed);
    }
    friend void intrusive_ptr_release(const EpAttr *x) {
        if (x->ep_refcnt.fetch_sub(1, boost::memory_order_release) == 1) {
            boost::atomic_thread_fence(boost::memory_order_acquire);
            delete x;
        }
    }
};

#define EP_CAST_PTR(T, ptr)  static_cast<T::pointer>(get_pointer(ptr))

/*
 * This event plugin object sparates error handling path from the main data flow path.
 * It provides hooks to enable an endpoint/service to participate in the error handling
 * path in the proper order defined by the model.
 */
class EpEvtPlugin
{
  public:
    typedef boost::intrusive_ptr<EpEvtPlugin> pointer;
    typedef boost::intrusive_ptr<const EpEvtPlugin> const_ptr;

    EpEvtPlugin();
    virtual ~EpEvtPlugin() {}

    // End-point and service events.
    //
    virtual void ep_connected() = 0;
    virtual void ep_down() = 0;
    virtual void svc_down() = 0;

    // Feedback to the error recovery flow.
    //
    void ep_cleanup_start();
    void ep_cleanup_finish();

    void svc_cleanup_start();
    void svc_cleanup_finish();

  protected:
    boost::intrusive_ptr<EpSvc>      ep_owner;

  private:
    friend class EpSvc;
    mutable boost::atomic<int>       ep_refcnt;

    friend void intrusive_ptr_add_ref(const EpEvtPlugin *x) {
        x->ep_refcnt.fetch_add(1, boost::memory_order_relaxed);
    }
    friend void intrusive_ptr_release(const EpEvtPlugin *x) {
        if (x->ep_refcnt.fetch_sub(1, boost::memory_order_release) == 1) {
            boost::atomic_thread_fence(boost::memory_order_acquire);
            delete x;
        }
    }
    void assign_ep_owner(boost::intrusive_ptr<EpSvc> owner) { ep_owner = owner; }
};

/*
 * Service handling object.  A service is identified by its UUID in the domain where
 * it registered.  Any software module in the domain can send messages that this
 * service understands (matching supported version) by looking at its UUID.
 */
class EpSvc
{
  public:
    typedef boost::intrusive_ptr<EpSvc> pointer;
    typedef boost::intrusive_ptr<const EpSvc> const_ptr;

    // Service bound to the local domain.
    EpSvc(const ResourceUUID &uuid, fds_uint32_t major, fds_uint32_t minor);

    // Service bound to a remote domain.
    EpSvc(const ResourceUUID &domain,
          const ResourceUUID &uuid,
          fds_uint32_t        major,
          fds_uint32_t        minor);

    virtual ~EpSvc() {}

    // When a message sent to this service handler arrives, the network layer will
    // call this function passing the message header.  It's up to the handler object
    // to intepret the remaining payload.
    //
    virtual void svc_receive_msg(const fpi::AsyncHdr &msg) = 0;

    // Control endpoint attributes.
    //
    void            ep_apply_attr();
    EpAttr::pointer ep_get_attr() { return ep_attr; }

  protected:
    friend class NetMgr;

    fpi::SvcID                       svc_id;
    fpi::SvcVer                      svc_ver;
    EpEvtPlugin::pointer             ep_evt;
    EpAttr::pointer                  ep_attr;

    fpi::DomainID                   *svc_domain;
    EndPointMgr                     *ep_mgr;

    // Service registration & lookup.
    //
    EpSvc(const NodeUuid       &mine,
          const NodeUuid       &peer,
          const EpAttr         &attr,
          EpEvtPlugin::pointer  ops);

    EpSvc(const fpi::SvcID     &mine,
          const fpi::SvcID     &peer,
          const EpAttr         &attr,
          EpEvtPlugin::pointer  ops);

    virtual void           ep_bind_service(EpSvc::pointer svc);
    virtual EpSvc::pointer ep_unbind_service(const fpi::SvcID &id);
    virtual EpSvc::pointer ep_lookup_service(const ResourceUUID &uuid);
    virtual EpSvc::pointer ep_lookup_service(const char *name);

  private:
    mutable boost::atomic<int>       ep_refcnt;

    friend void intrusive_ptr_add_ref(const EpSvc *x) {
        x->ep_refcnt.fetch_add(1, boost::memory_order_relaxed);
    }
    friend void intrusive_ptr_release(const EpSvc *x) {
        if (x->ep_refcnt.fetch_sub(1, boost::memory_order_release) == 1) {
            boost::atomic_thread_fence(boost::memory_order_acquire);
            delete x;
        }
    }
};

/*
 * Endpoint is the logical representation of a physical connection.  It provides RPC
 * semantic.
 */
template <class SendIf, class RecvIf>
class EndPoint : public EpSvc
{
  public:
    typedef boost::intrusive_ptr<EndPoint<SendIf, RecvIf>> pointer;
    typedef boost::intrusive_ptr<const EndPoint<SendIf, RecvIf>> const_ptr;

    virtual ~EndPoint() {}
    EndPoint(const fpi::SvcID          &mine,
             const fpi::SvcID          &peer,
             const EpAttr              &attr,
             boost::shared_ptr<SendIf>  snd_if,
             boost::shared_ptr<RecvIf>  rcv_if,
             EpEvtPlugin::pointer       ops)
        : EpSvc(mine, peer, attr, ops), ep_rpc_send(snd_if), ep_rpc_recv(rcv_if) {}

    EndPoint(int                        port,
             const NodeUuid            &mine,
             const NodeUuid            &peer,
             boost::shared_ptr<SendIf>  snd_if,
             boost::shared_ptr<RecvIf>  rcv_if,
             EpEvtPlugin::pointer       ops)
        : EpSvc(mine, peer, EpAttr(0, port), ops),
          ep_rpc_send(snd_if), ep_rpc_recv(rcv_if) {}

    static inline EndPoint<SendIf, RecvIf>::pointer ep_cast_ptr(EpSvc::pointer ptr) {
        return static_cast<EndPoint<SendIf, RecvIf> *>(get_pointer(ptr));
    }
    // Synchronous send/receive handlers.
    //
    boost::shared_ptr<SendIf> ep_sync_rpc() { return ep_rpc_send; }
    boost::shared_ptr<RecvIf> ep_rpc_handler() { return ep_rpc_recv; }

    // Async message passing.
    //
    //

    /// Service binding and lookup.
    //
    virtual void ep_bind_service(EpSvc::pointer svc) {
        EpSvc::ep_bind_service(svc);
    }
    virtual EpSvc::pointer ep_unbind_service(const fpi::SvcID &id) {
        return EpSvc::ep_unbind_service(id);
    }
    virtual EpSvc::pointer ep_lookup_service(const ResourceUUID &uuid) {
        return EpSvc::ep_lookup_service(uuid);
    }
    virtual EpSvc::pointer ep_lookup_service(const char *name) {
        return EpSvc::ep_lookup_service(name);
    }

  protected:
    boost::shared_ptr<SendIf>      ep_rpc_send;
    boost::shared_ptr<RecvIf>      ep_rpc_recv;

    void svc_receive_msg(const fpi::AsyncHdr &msg) {}
};

/**
 * Module vector hookup
 */
extern EndPointMgr  gl_EndPointMgr;

/**
 * Singleton module manages all endpoints.
 */
class EndPointMgr : public Module
{
  public:
    explicit EndPointMgr(const char *name);
    virtual ~EndPointMgr() {}

    // Module methods.
    //
    virtual int  mod_init(SysParams const *const p);
    virtual void mod_startup();
    virtual void mod_shutdown();

    static EndPointMgr *ep_mgr_singleton() { return &gl_EndPointMgr; }

    // Endpoint registration and lookup.
    //
    virtual void           ep_register(EpSvc::pointer ep);
    virtual void           ep_register(const fpi::DomainID &domain, EpSvc::pointer ep);
    virtual void           ep_unregister(const fpi::SvcUuid &uuid);
    virtual EpSvc::pointer ep_lookup(const fpi::SvcUuid &peer);
    virtual EpSvc::pointer ep_lookup(const char *peer_name);

    // Local domain service lookup.
    //
    virtual EpSvc::pointer ep_svc_lookup(const ResourceUUID &uuid);

    // Hook up with domain membership to know which node belongs to which domain.
    //

    // Hook with with node/domain state machine events.
    //
};

}  // namespace fds
#endif  // SOURCE_INCLUDE_NET_NET_SERVICE_H_
