/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_NET_NET_SERVICE_H_
#define SOURCE_INCLUDE_NET_NET_SERVICE_H_

#include <fds_module.h>
#include <fds_typedefs.h>
#include <shared/fds-constants.h>
#include <fdsp/fds_service_types.h>
#include <boost/intrusive_ptr.hpp>

namespace fds {

class EpEvtPlugin;
class NetMgr;

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
struct EpAttr
{
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
    virtual ~EpEvtPlugin();

    // End-point events.
    //
    virtual void ep_connected() = 0;
    virtual void ep_cleanup_start() = 0;
    virtual void ep_cleanup_finish() = 0;

    // Service events.
    //
    virtual void svc_down() = 0;
    virtual void svc_cleanup_start() = 0;
    virtual void svc_cleanup_finish() = 0;

  private:
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

    virtual ~EpSvc();

    // When a message sent to this service handler arrives, the network layer will
    // call this function passing the message header.  It's up to the handler object
    // to intepret the remaining payload.
    //
    virtual void svc_receive_msg(const fpi::AsyncHdr &msg) = 0;

  protected:
    fpi::SvcID                       svc_id;
    fpi::SvcVer                      svc_ver;
    fpi::DomainID                   *svc_domain;

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
template <class EndPointIf>
class EndPoint : public EpSvc
{
  public:
    typedef boost::intrusive_ptr<EndPoint<EndPointIf>> pointer;
    typedef boost::intrusive_ptr<const EndPoint<EndPointIf>> const_ptr;

    virtual ~EndPoint();
    EndPoint(const fpi::SvcID              &mine,
             const fpi::SvcID              &peer,
             boost::shared_ptr<EndPointIf>  snd_if,
             boost::shared_ptr<EndPointIf>  rcv_if,
             EpEvtPlugin::pointer           ops,
             const EpAttr                  &attr);

    static inline EndPoint<EndPointIf>::pointer ep_cast_ptr(EpSvc::pointer ptr) {
        return static_cast<EndPoint<EndPointIf> *>(get_pointer(ptr));
    }

    // Service registration & lookup.
    //
    virtual void           ep_bind_service(const fpi::SvcID &id, EpSvc::pointer svc);
    virtual EpSvc::pointer ep_unbind_service(const fpi::SvcUuid &svc);
    virtual EpSvc::pointer ep_lookup_service(const fpi::SvcUuid &svc);
    virtual EpSvc::pointer ep_lookup_service(const char *name);

    // Synchronous send.
    boost::shared_ptr<EndPointIf> ep_sync_rpc();

    // Async message passing.
    //

    // Endpoint attributes.
    virtual void ep_get_attr(EpAttr *attr) const;
    virtual void ep_set_attr(const EpAttr *attr);

  protected:
    friend class NetMgr;

    boost::shared_ptr<EndPointIf>  ep_rpc_send;
    boost::shared_ptr<EndPointIf>  ep_rpc_recv;
    EpEvtPlugin::pointer           ep_evt;
    EpAttr                         ep_attr;
    NetMgr                        *ep_mgr;
};

/**
 * Singleton module manages all endpoints.
 */
class NetMgr : public Module
{
  public:
    explicit NetMgr(const char *name);
    virtual ~NetMgr();

    // Module methods.
    //
    virtual int  mod_init(SysParams const *const p);
    virtual void mod_startup();
    virtual void mod_shutdown();

    // Endpoint registration and lookup.
    //
    virtual void           ep_register(EpSvc::pointer ep);
    virtual void           ep_register(const fpi::DomainID &domain, EpSvc::pointer ep);
    virtual void           ep_unregister(const fpi::SvcUuid &uuid);
    virtual EpSvc::pointer ep_lookup(const fpi::SvcUuid &peer);
    virtual EpSvc::pointer ep_lookup(const char *peer_name);

    // Local domain service lookup.
    //
    virtual EpSvc::pointer ep_svc_lookup(const fpi::SvcUuid &svc);

    // Hook up with domain membership to know which node belongs to which domain.
    //

    // Hook with with node/domain state machine events.
    //
};

}  // namespace fds
#endif  // SOURCE_INCLUDE_NET_NET_SERVICE_H_
