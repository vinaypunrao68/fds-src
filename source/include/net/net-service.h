/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_NET_NET_SERVICE_H_
#define SOURCE_INCLUDE_NET_NET_SERVICE_H_

#include <netinet/in.h>
#include <string>
#include <fds_module.h>
#include <fds_process.h>
#include <fds_typedefs.h>
#include <shared/fds-constants.h>
#include <fdsp/fds_service_types.h>
#include <boost/intrusive_ptr.hpp>

// Forward declarations
namespace apache { namespace thrift { namespace transport {
class TTransport;
}}}  // namespace apache::thrift::transport

namespace fds {
namespace tt = apache::thrift::transport;

class EpSvc;
class EpSvcImpl;
class EpSvcHandle;
class EpEvtPlugin;
class NetMgr;
class EpPlatLibMod;
class NetPlatSvc;
class NetPlatform;

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
    struct sockaddr                  ep_addr;

    // Server attributes.
    //

    // Methods.
    EpAttr() : ep_refcnt(0) {}
    virtual ~EpAttr() {}

    EpAttr(fds_uint32_t ip, int port);        /**< ip = 0, default IP of the node */
    EpAttr &operator = (const EpAttr &rhs);

    int attr_get_port();
    static int  netaddr_get_port(const struct sockaddr *adr);

    /**
     * Get my IP from the network interface (e.g. eth0).  Pass NULL if want to get the
     * IP used to reach the default gateway.
     * @param adr, mask, flags (o): info about the IP.
     * @param iface (i): network interface (e.g. eth0, lo...).
     */
    static void netaddr_my_ip(struct sockaddr *adr,
                              struct sockaddr *mask,
                              unsigned int    *flags,
                              const char      *iface);

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

#define EP_CAST_PTR(T, ptr)  static_cast<T *>(get_pointer(ptr))

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

    /**
     * For list of service handles bound to this endpoint, notify them when the service
     * is up/down.
     *
     * @param svc (i) - same as EpSvc::pointer type, pointer to the local representation
     *            of the service's provider (e.g. server object).
     * @param handle (i) - same as EpSvcHandle::pointer type, pointer to the handle
     *            of a client of the service (e.g. client object).
     */
    virtual void svc_up(boost::intrusive_ptr<EpSvcHandle>   handle) = 0;
    virtual void svc_down(boost::intrusive_ptr<EpSvc>       svc,
                          boost::intrusive_ptr<EpSvcHandle> handle) = 0;

    /**
     * Feedback to the error recovery flow.  When the plugin code is ready to handle
     * errors comming back from async messages, notify the net service layer with these
     * calls to start the cleanup process.
     */
    void ep_cleanup_start();
    void ep_cleanup_finish();

    void svc_cleanup_start();
    void svc_cleanup_finish();

  protected:
    boost::intrusive_ptr<EpSvc>      ep_owner;

  private:
    friend class EpSvcImpl;
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
 * Service object provider.  A service is identified by its UUID in the domain where
 * it registered.  Any software module in the domain can send messages that this
 * service understands (matching supported version) by looking at its UUID.
 */
class EpSvc
{
  public:
    typedef boost::intrusive_ptr<EpSvc> pointer;
    typedef boost::intrusive_ptr<const EpSvc> const_ptr;

    // When a message sent to this service handler arrives, the network layer will
    // call this function passing the message header.  It's up to the handler object
    // to intepret the remaining payload.
    //
    virtual void svc_receive_msg(const fpi::AsyncHdr &msg) = 0;

    // Just return int for now
    //
    virtual int  ep_get_status() { return 0; }

    // Control endpoint/service attributes.
    //
    void            ep_apply_attr();
    EpAttr::pointer ep_get_attr() { return ep_attr; }

  protected:
    fpi::SvcID                       svc_id;
    fpi::SvcVer                      svc_ver;
    EpEvtPlugin::pointer             ep_evt;
    EpAttr::pointer                  ep_attr;

    fpi::DomainID                   *svc_domain;
    NetMgr                          *ep_mgr;

    // Service bound to the local domain.
    EpSvc(const ResourceUUID &uuid, fds_uint32_t major, fds_uint32_t minor);

    // Service bound to a remote domain.
    EpSvc(const ResourceUUID &domain,
          const ResourceUUID &uuid,
          fds_uint32_t        major,
          fds_uint32_t        minor);

    virtual ~EpSvc() {}

    // Return the raw receive handler for this service endpoint.
    //
    template <class RecvIf>
    boost::shared_ptr<RecvIf> ep_rpc_recv() {
        return boost::static_pointer_cast<RecvIf>(ep_get_rcv_handler());
    }

  private:
    friend class NetMgr;
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
    virtual fds_uint64_t ep_my_uuid() { return svc_id.svc_uuid.svc_uuid; }
    virtual fds_uint64_t ep_peer_uuid() { return INVALID_RESOURCE_UUID.uuid_get_val(); }
    virtual void        *ep_get_rcv_handler() { return NULL; }
};

/**
 * Service object handle (e.g. client side).  A service provider can service many
 * concurrent clients in the domain.  This service handle object governs the life
 * cycle of the handler at the client side.
 */
class EpSvcHandle
{
  public:
    typedef boost::intrusive_ptr<EpSvcHandle> pointer;
    typedef boost::intrusive_ptr<const EpSvcHandle> const_ptr;

    virtual int  ep_reconnect() { return 0; }
    virtual int  ep_get_status() { return 0; }

    template <class SendIf>
    boost::shared_ptr<SendIf> svc_rpc() {
        return boost::static_pointer_cast<SendIf>(ep_rpc);
    }

  protected:
    friend class EpSvc;
    friend class NetPlatSvc;

    boost::shared_ptr<void>           ep_rpc;
    boost::shared_ptr<tt::TTransport> ep_trans;

    virtual ~EpSvcHandle() {}
    EpSvcHandle() : ep_refcnt(0), ep_rpc(NULL), ep_trans(NULL) {}
    EpSvcHandle(boost::shared_ptr<void> rpc, boost::shared_ptr<tt::TTransport> trans)
        : ep_rpc(rpc), ep_trans(trans) {}

  private:
    mutable boost::atomic<int>        ep_refcnt;

    friend void intrusive_ptr_add_ref(const EpSvcHandle *x) {
        x->ep_refcnt.fetch_add(1, boost::memory_order_relaxed);
    }
    friend void intrusive_ptr_release(const EpSvcHandle *x) {
        if (x->ep_refcnt.fetch_sub(1, boost::memory_order_release) == 1) {
            boost::atomic_thread_fence(boost::memory_order_acquire);
            delete x;
        }
    }
};

/**
 * Module vector hookup
 */
extern NetMgr                gl_netService;
extern NetPlatform           gl_netPlatform;

typedef std::unordered_map<fds_uint64_t, int>             UuidShmMap;
typedef std::unordered_map<fds_uint64_t, EpSvc::pointer>  UuidSvcMap;
typedef std::unordered_map<int, EpSvc::pointer>           PortSvcMap;

struct ep_map_rec;

/**
 * Singleton module manages all endpoints.
 */
class NetMgr : public Module
{
  public:
    explicit NetMgr(const char *name);
    virtual ~NetMgr() {}

    // Module methods.
    //
    virtual int  mod_init(SysParams const *const p);
    virtual void mod_startup();
    virtual void mod_shutdown();

    static NetMgr *ep_mgr_singleton() { return &gl_netService; }
    static fds_threadpool *ep_mgr_thrpool() { return g_fdsprocess->proc_thrpool(); }

    /**
     * Return all uuid binding records.  The RO array may have hole(s) where uuids
     * are 0 for invalid record.  Return the number of elements in the output map.
     */
    virtual int  ep_uuid_bindings(const struct ep_map_rec **map);

    /**
     * Endpoint registration and lookup.
     */
    virtual void ep_register(EpSvc::pointer ep, bool update_domain = true);
    virtual void ep_unregister(const fpi::SvcUuid &uuid);

    /**
     * Look up a service handler based on its uuid/name and major/minor version.
     * Services are only available in the local domain.
     *
     * @param uuid (i) - uuid of the service.
     * @param maj (i)  - the service's major version number.
     * @param min (i)  - the service's minor version number.
     */
    virtual EpSvcHandle::pointer
    svc_lookup(const fpi::SvcUuid &uuid, fds_uint32_t maj, fds_uint32_t min);

    virtual EpSvcHandle::pointer
    svc_lookup(const char *name, fds_uint32_t maj, fds_uint32_t min);

    // TODO(Vy): Please refactor the following as needed
    /**
     * Returns true if error e is actionable on the endpoint
     * @param e
     * @return
     */
    bool ep_actionable_error(const Error &e) const;

    /**
     * Handles endpoint error
     * TODO(Rao):  It's probably better for error handling to be scheduled
     * on a threadpool and not on calling thread
     * @param e
     */
    void ep_handle_error(const Error &e);

    // Hook up with domain membership to know which node belongs to which domain.
    //

    // Hook with with node/domain state machine events.
    //
  protected:
    EpPlatLibMod            *ep_shm;
    UuidSvcMap               ep_svc_map;
    UuidShmMap               ep_uuid_map;
    PortSvcMap               ep_port_map;
    fds_mutex                ep_mtx;

    virtual EpSvc::pointer ep_lookup_port(int port);
};

/**
 * Singleton module linked with platform daemon.
 */
class NetPlatform : public Module
{
  public:
    explicit NetPlatform(const char *name);
    virtual ~NetPlatform() {}

    // Module methods.
    //
    virtual int  mod_init(SysParams const *const p);
    virtual void mod_startup();
    virtual void mod_shutdown();

    static NetPlatform *nplat_singleton() { return &gl_netPlatform; }
    EpSvcHandle::pointer nplat_domain_rpc(const fpi::DomainID &id);
};

}  // namespace fds
#endif  // SOURCE_INCLUDE_NET_NET_SERVICE_H_
