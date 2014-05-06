/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_NET_NET_SERVICE_H_
#define SOURCE_INCLUDE_NET_NET_SERVICE_H_

#include <netinet/in.h>
#include <string>
#include <fds_module.h>
#include <fds_typedefs.h>
#include <shared/fds-constants.h>
#include <fdsp/fds_service_types.h>
#include <boost/intrusive_ptr.hpp>

#include <unistd.h>
#include <arpa/inet.h>
#include <thrift/transport/TSocket.h>
#include <thrift/transport/TBufferTransports.h>
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TNonblockingServer.h>
#include <thrift/server/TThreadedServer.h>
#include <thrift/concurrency/ThreadManager.h>

namespace fds {

namespace at = apache::thrift;
namespace tc = apache::thrift::concurrency;
namespace tp = apache::thrift::protocol;
namespace ts = apache::thrift::server;
namespace tt = apache::thrift::transport;
namespace bo = boost;

class EpSvc;
class EpSvcImpl;
class EpSvcHandle;
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
    struct sockaddr                  ep_addr;

    // Server attributes.
    //

    // Methods.
    EpAttr() {}
    virtual ~EpAttr() {}

    EpAttr(fds_uint32_t ip, int port);        /**< ip = 0, default IP of the node */
    EpAttr &operator = (const EpAttr &rhs);

    int attr_get_port();

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
    EndPointMgr                     *ep_mgr;

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

  protected:
    friend class NetMgr;
    friend class EpSvc;

    EpSvcHandle() {}
    virtual ~EpSvcHandle() {}

    virtual int  ep_get_status() { return 0; }

    template <class SendIf>
    boost::shared_ptr<SendIf> svc_rpc() { return NULL; }

  private:
    mutable boost::atomic<int>       ep_refcnt;

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

    /**
     * Endpoint registration and lookup.
     */
    virtual void  ep_register(EpSvc::pointer ep);
    virtual void  ep_unregister(const fpi::SvcUuid &uuid);

    /**
     * Look up a service handler based on its uuid/name and major/minor version.
     * Services are only available in the local domain.
     *
     * @param uuid (i) - uuid of the service.
     * @param maj (i)  - the service's major version number.
     * @param min (i)  - the service's minor version number.
     */
    virtual EpSvcHandle::pointer
    svc_lookup(const ResourceUUID &uuid, fds_uint32_t maj, fds_uint32_t min);

    virtual EpSvcHandle::pointer
    svc_lookup(const char *name, fds_uint32_t maj, fds_uint32_t min);

    // Hook up with domain membership to know which node belongs to which domain.
    //

    // Hook with with node/domain state machine events.
    //
};

/*
 * -------------------------------------------------------------------------------------
 * Internal Implementation: we need it here because of template.
 * -------------------------------------------------------------------------------------
 */

/**
 * Endpoint implementation class.
 */
class EpSvcImpl : public EpSvc
{
  public:
    EpSvcImpl(const NodeUuid       &mine,
              const NodeUuid       &peer,
              const EpAttr         &attr,
              EpEvtPlugin::pointer  ops);

    EpSvcImpl(const fpi::SvcID     &mine,
              const fpi::SvcID     &peer,
              const EpAttr         &attr,
              EpEvtPlugin::pointer  ops);

    virtual ~EpSvcImpl();

    // Service registration & lookup.
    //
    virtual void           ep_bind_service(EpSvc::pointer svc);
    virtual EpSvc::pointer ep_unbind_service(const fpi::SvcID &id);
    virtual EpSvc::pointer ep_lookup_service(const ResourceUUID &uuid);
    virtual EpSvc::pointer ep_lookup_service(const char *name);

  protected:
    fpi::SvcID                       ep_peer_id;
    fpi::SvcVer                      ep_peer_ver;
};

/**
 * Endpoint is the logical RPC representation of a physical connection.
 * Thrift template implementation.
 */
template <class SendIf, class RecvIf>
class EndPoint : public EpSvcImpl
{
  public:
    typedef boost::intrusive_ptr<EndPoint<SendIf, RecvIf>> pointer;
    typedef boost::intrusive_ptr<const EndPoint<SendIf, RecvIf>> const_ptr;

    virtual ~EndPoint() {}
    EndPoint(const fpi::SvcID          &mine,
             const fpi::SvcID          &peer,
             const EpAttr              &attr,
             boost::shared_ptr<RecvIf>  rcv_if,
             EpEvtPlugin::pointer       ops)
        : EpSvcImpl(mine, peer, attr, ops), ep_rpc_recv(rcv_if) {
            ep_init_obj();
        }
    EndPoint(int                        port,
             const NodeUuid            &mine,
             const NodeUuid            &peer,
             boost::shared_ptr<RecvIf>  rcv_if,
             EpEvtPlugin::pointer       ops)
        : EpSvcImpl(mine, peer, EpAttr(0, port), ops), ep_rpc_recv(rcv_if) {
            ep_init_obj();
        }

    static inline EndPoint<SendIf, RecvIf>::pointer ep_cast_ptr(EpSvc::pointer ptr) {
        return static_cast<EndPoint<SendIf, RecvIf> *>(get_pointer(ptr));
    }
    void ep_activate() {
        ep_setup_server();
        ep_client_connect();
    }
    // Synchronous send/receive handlers.
    //
    boost::shared_ptr<SendIf> ep_sync_rpc() { return ep_rpc_send; }
    boost::shared_ptr<RecvIf> ep_rpc_handler() { return ep_rpc_recv; }

    // Async message passing.
    //
    //

  protected:
    bo::shared_ptr<SendIf>                 ep_rpc_send;
    bo::shared_ptr<tt::TTransport>         ep_sock;
    bo::shared_ptr<tt::TTransport>         ep_trans;
    bo::shared_ptr<tp::TProtocol>          ep_proto;

    bo::shared_ptr<RecvIf>                 ep_rpc_recv;
    bo::shared_ptr<ts::TThreadedServer>    ep_server;
    bo::shared_ptr<ts::TNonblockingServer> ep_nb_srv;

    void svc_receive_msg(const fpi::AsyncHdr &msg) {}

  private:
    // ep_client_connect
    // -----------------
    //
    void ep_client_connect()
    {
        int         port;
        const char *host = "localhost";

        port = ep_attr->attr_get_port();
        if (port == 6000) {
            port = 7000;
        } else {
            port = 6000;
        }
        ep_sock  = bo::shared_ptr<tt::TTransport>(new tt::TSocket(host, port));
        ep_trans = bo::shared_ptr<tt::TTransport>(new tt::TFramedTransport(ep_sock));
        ep_proto = bo::shared_ptr<tp::TProtocol>(new tp::TBinaryProtocol(ep_trans));
        ep_rpc_send = bo::shared_ptr<SendIf>(new SendIf(ep_proto));
    }
    // ep_server_listen
    // ----------------
    //
    void ep_setup_server()
    {
        int     port;

        port = ep_attr->attr_get_port();
        bo::shared_ptr<tp::TProtocolFactory>  proto(new tp::TBinaryProtocolFactory());
        bo::shared_ptr<tt::TServerTransport>  trans(new tt::TServerSocket(port));
        bo::shared_ptr<tt::TTransportFactory> tfact(new tt::TFramedTransportFactory());

        ep_server = bo::shared_ptr<ts::TThreadedServer>(
                new ts::TThreadedServer(ep_rpc_recv, trans, tfact, proto));
    }
#if 0
    void ep_setup_server_nb()
    {
        boost::shared_ptr<tp::TProtocolFactory> proto(new tp::TBinaryProtocolFactory());
        boost::shared_ptr<tc::ThreadManager>
            thr_mgr(tc::ThreadManager::newSimpleThreadManager(15));

        boost::shared_ptr<tc::PosixThreadFactory> thr_fact(new tc::PosixThreadFactory());
        thr_mgr->threadFactory(thr_fact);

        ep_nb_srv = boost::shared_ptr<ts::TNonblockingServer>(
                new ts::TNonblockingServer(ep_rpc_recv, proto, 8888, thr_mgr));

        thr_mgr->start();
        ep_nb_srv->serve();
    }
#endif
    // ep_init_obj
    // -----------
    //
    void ep_init_obj()
    {
        ep_rpc_send = NULL;
        ep_sock     = NULL;
        ep_trans    = NULL;
        ep_proto    = NULL;
        ep_server   = NULL;
    }
};

}  // namespace fds
#endif  // SOURCE_INCLUDE_NET_NET_SERVICE_H_
