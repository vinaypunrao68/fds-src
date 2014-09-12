/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_NET_NET_SERVICE_H_
#define SOURCE_INCLUDE_NET_NET_SERVICE_H_

#include <netinet/in.h>
#include <list>
#include <string>
#include <concurrency/SynchronizedTaskExecutor.hpp>

#include <fds_ptr.h>
#include <fds_module.h>
#include <fds_resource.h>
#include <fds_error.h>
#include <shared/fds-constants.h>
#include <fdsp/fds_service_types.h>
#include <fdsp/BaseAsyncSvc.h>
#include <fds_typedefs.h>
#include <util/hash-util.h>
#include <net/fdssocket.h>

// Forward declarations
namespace apache { namespace thrift { namespace transport {
    class TSocket;
    class TTransport;
}}}  // namespace apache::thrift::transport

namespace FDS_ProtocolInterface {
    class UuidBindMsg;
    class NodeInfoMsg;
    class PlatNetSvcClient;
    typedef boost::shared_ptr<BaseAsyncSvcClient> BaseAsyncSvcClientPtr;
}  // namespace FDS_ProtocolInterface

namespace fpi = FDS_ProtocolInterface;

namespace fds {
namespace bo  = boost;
namespace tt  = apache::thrift::transport;
namespace tp  = apache::thrift::protocol;

class EpSvc;
class EpSvcImpl;
class EpSvcHandle;
class EpEvtPlugin;
class EpPlatLibMod;
class NetMgr;
class NetPlatSvc;
class NetPlatform;
class NetPlatSvc;
class fds_threadpool;
class DomainAgent;
class Platform;
class FdsTimer;
class PlatUuidBind;

struct ep_map_rec;
template <class SendIf, class RecvIf> class EndPoint;

// Global constants used by net-service layer.
//
extern const fpi::SvcUuid NullSvcUuid;

typedef enum
{
    EP_ST_INIT         = 0x0000,
    EP_ST_DISCONNECTED = 0x0001,
    EP_ST_CONNECTING   = 0x0002,
    EP_ST_CONNECTED    = 0x0004,
    EP_ST_NO_PLUGIN    = 0x8000,
    EP_ST_MAX
} ep_state_e;

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
    typedef bo::intrusive_ptr<EpAttr> pointer;
    typedef bo::intrusive_ptr<const EpAttr> const_ptr;

    // Physcial attributes of an endpoint.
    //
    struct sockaddr               ep_addr;

    // Server attributes.
    //

    // Methods.
    EpAttr() : ep_refcnt(0) {}
    virtual ~EpAttr() {}

    EpAttr(fds_uint32_t ip, int port);        /**< ipv4 net format.             */
    EpAttr(const char *iface, int port);      /**< get local ip from the iface. */
    EpAttr &operator = (const EpAttr &rhs);

    int attr_get_port();

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

    static int  netaddr_get_port(const struct sockaddr *adr);
    static void netaddr_set_port(struct sockaddr *adr, int port);
    static void netaddr_to_str(const struct sockaddr *adr, char *ip, int ip_len);
    static void netaddr_frm_str(struct sockaddr *adr,
                                int port, const char *ip, bool v4 = true);

  private:
    INTRUSIVE_PTR_DEFS(EpAttr, ep_refcnt);
};

/*
 * This event plugin object sparates error handling path from the main data flow path.
 * It provides hooks to enable an endpoint/service to participate in the error handling
 * path in the proper order defined by the model.
 */
class EpEvtPlugin
{
  public:
    typedef bo::intrusive_ptr<EpEvtPlugin> pointer;
    typedef bo::intrusive_ptr<const EpEvtPlugin> const_ptr;

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
    virtual void svc_up(bo::intrusive_ptr<EpSvcHandle>   handle) = 0;
    virtual void svc_down(bo::intrusive_ptr<EpSvc>       svc,
                          bo::intrusive_ptr<EpSvcHandle> handle) = 0;

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
    bo::intrusive_ptr<EpSvc>       ep_owner;
    bo::intrusive_ptr<EpSvcHandle> ep_handle;

  private:
    friend class EpSvcImpl;
    friend class EpSvcHandle;
    INTRUSIVE_PTR_DEFS(EpEvtPlugin, ep_refcnt);

    void assign_ep_owner(bo::intrusive_ptr<EpSvc> owner) { ep_owner = owner; }
    void assign_ep_handle(bo::intrusive_ptr<EpSvcHandle> handle) { ep_handle = handle; }
};

/*
 * Service object provider.  A service is identified by its UUID in the domain where
 * it registered.  Any software module in the domain can send messages that this
 * service understands (matching supported version) by looking at its UUID.
 */
class EpSvc
{
  public:
    typedef bo::intrusive_ptr<EpSvc> pointer;
    typedef bo::intrusive_ptr<const EpSvc> const_ptr;

    /*
     * When a message sent to this service handler arrives, the network layer will
     * call this function passing the message header.  It's up to the handler object
     * to intepret the remaining payload.
     */
    virtual void svc_receive_msg(const fpi::AsyncHdr &msg) = 0;

    /**
     * Apply new attributes to this endpoint/service.
     */
    virtual void ep_apply_attr();

    /**
     * Format the uuid binding from this obj.
     */
    virtual void ep_fmt_uuid_binding(fpi::UuidBindMsg *msg, fpi::DomainID *domain);

    /**
     * Force the reconnection of this end-point.
     */
    virtual int  ep_get_status() { return 0; }
    virtual void ep_input_event(fds_uint32_t evt) {}
    virtual void ep_reconnect() {}

    /**
     * Acessor functions.
     */
    inline EpAttr::pointer      ep_get_attr() { return ep_attr; }
    inline EpEvtPlugin::pointer ep_evt_plugin() { return ep_evt; }

    inline fds_uint32_t ep_version(fds_uint32_t *minor) {
        *minor = svc_ver.ver_minor;
        return svc_ver.ver_major;
    }

    inline void ep_my_uuid(fpi::SvcUuid &uuid) { uuid = svc_id.svc_uuid; }
    inline fds_uint64_t ep_my_uuid() { return svc_id.svc_uuid.svc_uuid; }

    virtual void ep_peer_uuid(fpi::SvcUuid &uuid) { uuid.svc_uuid = 0; }
    virtual fds_uint64_t ep_peer_uuid() { return INVALID_RESOURCE_UUID.uuid_get_val(); }

    /**
     * Cast to the correct EndPoint type.  Return NULL if this is pure service object.
     */
    template <class SendIf, class RecvIf>
    inline bo::intrusive_ptr<EndPoint<SendIf, RecvIf>> ep_cast() {
        if (ep_is_connection()) {
            return static_cast<EndPoint<SendIf, RecvIf> *>(this);
        }
        return NULL;
    }
    inline bo::intrusive_ptr<EpSvcHandle> ep_send_handle()  { return ep_peer; }

  protected:
    fpi::SvcID                       svc_id;
    fpi::SvcVer                      svc_ver;
    fpi::FDSP_MgrIdType              svc_type;
    EpEvtPlugin::pointer             ep_evt;
    EpAttr::pointer                  ep_attr;
    bo::intrusive_ptr<EpSvcHandle>   ep_peer;

    fpi::DomainID                   *svc_domain;
    NetMgr                          *ep_mgr;

    // Service bound to the local domain.
    EpSvc(const ResourceUUID &uuid, fds_uint32_t major, fds_uint32_t minor);
    EpSvc(const ResourceUUID &uuid,
          fpi::FDSP_MgrIdType type,
          fds_uint32_t        major = 0,
          fds_uint32_t        minor = 0);

    // Service bound to a remote domain.
    EpSvc(const ResourceUUID &domain,
          const ResourceUUID &uuid,
          fds_uint32_t        major,
          fds_uint32_t        minor);

    virtual ~EpSvc() {}
    virtual bool ep_is_connection() { return false; }
    virtual void ep_handle_error(const Error &err);

  private:
    friend class NetMgr;
    friend class EpSvcHandle;
    INTRUSIVE_PTR_DEFS(EpSvc, ep_refcnt);
};

/**
 * Down cast an endpoint intrusive pointer.
 */
template <class T>
bo::intrusive_ptr<T> ep_cast_ptr(EpSvc::pointer ep) {
    return static_cast<T *>(get_pointer(ep));
}

/**
 * Service object handle (e.g. client side).  A service provider can service many
 * concurrent clients in the domain.  This service handle object governs the life
 * cycle of the handler at the client side.
 */
class EpSvcHandle : public net::SocketEventHandler
{
  public:
    typedef bo::intrusive_ptr<EpSvcHandle> pointer;
    typedef bo::intrusive_ptr<const EpSvcHandle> const_ptr;

    virtual ~EpSvcHandle();
    EpSvcHandle(EpSvc::pointer svc, EpEvtPlugin::pointer, fds_uint32_t, fds_uint32_t);
    EpSvcHandle(const fpi::SvcUuid   &peer,
                EpEvtPlugin::pointer  evt,
                fds_uint32_t          maj,
                fds_uint32_t          min)
        : EpSvcHandle(NULL, evt, maj, min) { ep_peer_id = peer; }

    ep_state_e ep_reconnect();
    ep_state_e ep_get_status()  { return static_cast<ep_state_e>(ep_state); }

    std::string logString()
    {
        std::ostringstream oss;
        oss << " peer uuid: " << std::hex << ep_peer_id.svc_uuid << std::dec << " ";
        if (ep_sock) {
            oss << ep_sock->getPeerAddress() << ":" << ep_sock->getPeerPort();
        }
        return oss.str();
    }
    virtual void onSocketConnect() override
    {
        LOGDEBUG << logString();
    }
    virtual void onSocketDisconnect() override
    {
        LOGDEBUG << logString();
    }
    virtual void onSocketClose() override
    {
        LOGDEBUG << logString();
    }

    template <class SendIf>
    bo::shared_ptr<SendIf> svc_rpc() {
        return bo::static_pointer_cast<SendIf>(ep_rpc);
    }
    /**
     * @return the major and minor version of this handle.
     */
    inline fds_uint32_t ep_version(fds_uint32_t *minor) const {
        *minor = ep_minor;
        return ep_major;
    }
    inline const bo::shared_ptr<tt::TSocket> ep_debug_sock() const {
        return ep_sock;
    }

    void ep_notify_plugin(bool always);
    void ep_handle_net_error();
    void ep_my_uuid(fpi::SvcUuid &uuid)    { ep_owner->ep_my_uuid(uuid); }
    void ep_peer_uuid(fpi::SvcUuid &uuid)  { uuid = ep_peer_id; }

    bo::shared_ptr<tt::TSocket> ep_get_socket() { return ep_sock; }

  protected:
    friend class NetMgr;
    template <class SendIf> friend void
    endpoint_connect_server(int, const std::string &,
                            bo::intrusive_ptr<EpSvcHandle> *,
                            bo::intrusive_ptr<EpSvc>, fpi::SvcUuid,
                            EpEvtPlugin::pointer, fds_uint32_t, fds_uint32_t);

    int                            ep_state;
    fds_uint32_t                   ep_major;
    fds_uint32_t                   ep_minor;
    fpi::SvcUuid                   ep_peer_id;
    EpSvc::pointer                 ep_owner;
    EpEvtPlugin::pointer           ep_plugin;
    bo::shared_ptr<void>           ep_rpc;
    bo::shared_ptr<tt::TSocket>    ep_sock;
    bo::shared_ptr<tt::TTransport> ep_trans;

    EpSvcHandle();
    virtual void ep_handle_error(const Error &err);

  private:
    INTRUSIVE_PTR_DEFS(EpSvcHandle, ep_refcnt);
};

template <class SendIf> extern void
endpoint_connect_handle(EpSvcHandle::pointer eh,
                        fpi::SvcUuid uuid = NullSvcUuid, int retry = 0);

/**
 * Module vector hookup
 */
extern NetMgr                gl_NetService;
extern NetPlatform          *gl_NetPlatSvc;

typedef std::list<EpSvc::pointer>                              EpSvcList;
typedef std::unordered_map<fds_uint64_t, EpSvcList>            UuidEpMap;
typedef std::unordered_map<fds_uint64_t, EpSvc::pointer>       UuidSvcMap;
typedef std::unordered_map<UuidIntKey, int, UuidIntKeyHash>    UuidShmMap;
typedef std::unordered_map<UuidIntKey, EpSvcHandle::pointer, UuidIntKeyHash> EpHandleMap;
typedef std::unordered_map<UuidIntKey, fpi::BaseAsyncSvcClientPtr, UuidIntKeyHash> EpClientMap;

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
    virtual void mod_enable_service();
    virtual void mod_shutdown();

    static NetMgr *ep_mgr_singleton() { return &gl_NetService; }
    static fds_threadpool *ep_mgr_thrpool();
    boost::shared_ptr<boost::thread> ep_register_thread;
    SynchronizedTaskExecutor<uint64_t>* ep_get_task_executor();

    /**
     * Return all uuid binding records.  The RO array may have hole(s) where uuids
     * are 0 for invalid record.  Return the number of elements in the output map.
     * The caller doesn't have to free the RO array.
     */
    virtual int  ep_uuid_bindings(const struct ep_map_rec **map);
    virtual int  ep_uuid_binding(const fpi::SvcUuid &uuid,
                                 fds_uint32_t        maj,
                                 fds_uint32_t        min,
                                 std::string        *ip);

    /**
     * Endpoint registration and lookup.
     */
    virtual void ep_register(EpSvc::pointer ep, bool update_domain = true);
    virtual void ep_unregister(const fpi::SvcUuid &peer, fds_uint32_t, fds_uint32_t);

    virtual void ep_handler_register(EpSvcHandle::pointer handle);
    virtual void ep_handler_unregister(EpSvcHandle::pointer handle);
    virtual void ep_handler_unregister(const fpi::SvcUuid &peer);
    virtual void ep_register_binding(const struct ep_map_rec *rec, int idx);

    /**
     * Lookup a service handler based on its uuid/name and major/minor version.
     * Services are only available in the local domain.
     *
     * @param uuid (i) - uuid of the service.
     * @param maj (i)  - the service's major version number.
     * @param min (i)  - the service's minor version number.
     */
    virtual EpSvc::pointer
    svc_lookup(const fpi::SvcUuid &uuid, fds_uint32_t maj, fds_uint32_t min);

    virtual EpSvc::pointer
    svc_lookup(const char *name, fds_uint32_t maj, fds_uint32_t min);

    virtual EpSvcHandle::pointer
    svc_handler_lookup(const fpi::SvcUuid &uuid, fds_uint32_t maj, fds_uint32_t min);

    template <class SendIf>
    EpSvcHandle::pointer svc_get_handle(const fpi::SvcUuid   &peer,
                                        fds_uint32_t          maj,
                                        fds_uint32_t          min);

    /**
     * Returns true if error e is actionable on the endpoint
     * @param e
     * @return
     */
    virtual bool ep_actionable_error(/*const fpi::SvcUuid &uuid,*/ const Error &e) const;

    /**
     * Handles endpoint error on a thread from threadpool.
     * @param e
     */
    virtual void ep_handle_error(const fpi::SvcUuid &uuid, const Error &e,
                                 fds_uint32_t        maj = 0,
                                 fds_uint32_t        min = 0);

    /**
     * Lookup an endpoint.  The caller must call EpSvc::ep_cast<SendIf, RecvIf> to
     * cast it to the correct type.
     * @param s (i) - the source uuid.
     * @param d (i) - the destination uuid.
     */
    virtual EpSvc::pointer endpoint_lookup(const char *name);
    virtual EpSvc::pointer endpoint_lookup(const fpi::SvcUuid &d);
    virtual EpSvc::pointer endpoint_lookup(const fpi::SvcUuid &s, const fpi::SvcUuid &d);

    /**
     * Return the handle to the domain master.  From the handle, the caller can make
     * RPC calls, send async messages to it.
     */
    virtual EpSvcHandle::pointer
    svc_domain_master(const fpi::DomainID &id,
                      bo::shared_ptr<fpi::PlatNetSvcClient> &rpc);

    bo::shared_ptr<FdsTimer> ep_get_timer() const;

    /**
     * Utility function for swapping the endpoints in the header
     */
    static fpi::AsyncHdr ep_swap_header(const fpi::AsyncHdr &req_hdr);

    /**
     * Return mutex hashed by a pointer object.
     */
    inline fds_mutex *ep_obj_mutex(void *ptr) {
        return &ep_obj_mtx[(reinterpret_cast<fds_uint64_t>(ptr) >> 4) % ep_mtx_arr];
    }

    // Hook up with domain membership to know which node belongs to which domain.
    //

    // Hook with with node/domain state machine events.
    //

    fds_mutex                      ep_client_map_mtx;
    EpClientMap                    ep_client_map;

  protected:
    static const int               ep_mtx_arr = 16;

    Platform                      *plat_lib;
    NetPlatSvc                    *plat_net;
    EpPlatLibMod                  *ep_shm;

    SynchronizedTaskExecutor<uint64_t> *ep_task_executor;

    UuidEpMap                      ep_map;
    UuidSvcMap                     ep_svc_map;
    UuidShmMap                     ep_uuid_map;
    EpHandleMap                    ep_handle_map;
    fds_mutex                      ep_handle_map_mtx;
    fds_mutex                      ep_mtx;
    fds_mutex                      ep_obj_mtx[ep_mtx_arr];

    EpSvcHandle::pointer           ep_domain_clnt;
    bo::intrusive_ptr<DomainAgent> ep_domain_agent;

    ResourceUUID const *const ep_my_platform_uuid();
    void ep_register_thr(EpSvc::pointer ep, bool update_domain);
    void ep_handle_error_thr(fpi::SvcUuid *uuid, Error *e, fds_uint32_t, fds_uint32_t);

    virtual EpSvc::pointer ep_lookup_port(int port);
};

/**
 * Singleton module providing platform network services.
 */
class NetPlatform : public Module
{
  public:
    explicit NetPlatform(const char *name);
    virtual ~NetPlatform() {}

    inline static NetPlatform   *nplat_singleton() { return gl_NetPlatSvc; }
    virtual void                 nplat_register_node(const fpi::NodeInfoMsg *msg) = 0;
    virtual EpSvcHandle::pointer nplat_domain_rpc(const fpi::DomainID &id) = 0;

    virtual bo::intrusive_ptr<DomainAgent> nplat_self() = 0;
    virtual bo::intrusive_ptr<DomainAgent> nplat_master() = 0;

    virtual void nplat_set_my_ep(bo::intrusive_ptr<EpSvcImpl> ep) = 0;
    virtual bo::intrusive_ptr<EpSvcImpl> nplat_my_ep() = 0;
    virtual std::string const *const nplat_domain_master(int *port) = 0;

  protected:
    // Dependent module.
    //
    NetMgr                        *netmgr;
    Platform                      *plat_lib;
};

}  // namespace fds
#endif  // SOURCE_INCLUDE_NET_NET_SERVICE_H_
