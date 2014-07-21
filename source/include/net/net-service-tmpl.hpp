/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_NET_NET_SERVICE_TMPL_H_
#define SOURCE_INCLUDE_NET_NET_SERVICE_TMPL_H_

#include <unistd.h>
#include <arpa/inet.h>
#include <thrift/transport/TSocket.h>
#include <thrift/transport/TBufferTransports.h>
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TNonblockingServer.h>
#include <thrift/server/TThreadedServer.h>
#include <thrift/concurrency/ThreadManager.h>
#include <thrift/transport/TBufferTransports.h>
#include <net/fdssocket.h>

#include <util/Log.h>
#include <fds_resource.h>
#include <net/net-service.h>
#include <concurrency/ThreadPool.h>
#include <fds_typedefs.h>
#include <net/RpcFunc.h>

namespace fds {

namespace at = apache::thrift;
namespace tc = apache::thrift::concurrency;
namespace tp = apache::thrift::protocol;
namespace ts = apache::thrift::server;
namespace tt = apache::thrift::transport;

// Forward declaration.
struct ep_map_rec;
template <class SendIf, class RecvIf> class EndPoint;

/*
 * -------------------------------------------------------------------------------------
 * Internal tempalate implementation:
 * -------------------------------------------------------------------------------------
 */
extern void endpoint_retry_connect(EpSvcHandle::pointer ptr);

// TODO(Rao):
// 1. Remove 2 from both classes below.  2 is there b/c might conflict with
// netsession version
// 2. Add more socket lifecycle managment stuff than just logs
/**
 * TProcessor event handler.  Override this class if you want to peform
 * actions around functions executed by TProcessor.  This class just
 * overrides handlerError() to catch any exceptions thrown by functions
 * executed by TProcessor
 */
class FdsTProcessorEventHandler2 : public apache::thrift::TProcessorEventHandler {
public:
    FdsTProcessorEventHandler2()
    {
    }
    virtual void handlerError(void* ctx, const char* fn_name) override
    {
        LOGCRITICAL << "TProcessor error at: " << fn_name;
        fds_assert(!"TProcessor error");
    }
};

/**
 * @brief Listener for events from Thrift Server
 */
class ServerEventHandler2 : public apache::thrift::server::TServerEventHandler
{
 public:
  ServerEventHandler2()
  {
  }
  static std::string getTransportKey(
      boost::shared_ptr<apache::thrift::transport::TTransport> transport)
  {
      std::stringstream ret;
      /* What we get is TFramedTransport.  We will extract TSocket from it */
      boost::shared_ptr<apache::thrift::transport::TFramedTransport> buf_transport =
          boost::static_pointer_cast<apache::thrift::transport::TFramedTransport>(transport);

      boost::shared_ptr<apache::thrift::transport::TSocket> sock =
          boost::static_pointer_cast<apache::thrift::transport::TSocket>\
          (buf_transport->getUnderlyingTransport());

      ret << sock->getPeerAddress() << ":" << sock->getPeerPort();
      return ret.str();
  }
  /**
   * Called when a new client has connected and is about to being processing.
   */
  virtual void* createContext(
      boost::shared_ptr<apache::thrift::protocol::TProtocol> input,
      boost::shared_ptr<apache::thrift::protocol::TProtocol> output) override
  {
      /* Add the new connection */
      LOGDEBUG << "New connection: " << getTransportKey(input->getTransport());
      return NULL;
  }
  /**
   * Called when a client has finished request-handling to delete server
   * context.
   */
  virtual void deleteContext(void* serverContext,
                             boost::shared_ptr<apache::thrift::protocol::TProtocol>input,
                             boost::shared_ptr<apache::thrift::protocol::TProtocol>output)
  {
      LOGDEBUG << "Removing connection: " << getTransportKey(input->getTransport());
  }
};

/**
 * Endpoint implementation class.
 */
class EpSvcImpl : public EpSvc
{
  public:
    typedef boost::intrusive_ptr<EpSvcImpl> pointer;
    typedef boost::intrusive_ptr<const EpSvcImpl> const_ptr;
    virtual ~EpSvcImpl();

    // Service registration & lookup.
    //
    virtual void           ep_activate() {}
    virtual void           ep_reconnect();
    virtual void           ep_input_event(fds_uint32_t evt);
    virtual void           ep_bind_service(EpSvc::pointer svc);
    virtual EpSvc::pointer ep_unbind_service(const fpi::SvcID &id);
    virtual EpSvc::pointer ep_lookup_service(const ResourceUUID &uuid);
    virtual EpSvc::pointer ep_lookup_service(const char *name);

    static EpSvcImpl::pointer ep_impl_cast(EpSvc::pointer svc) {
        return static_cast<EpSvcImpl *>(get_pointer(svc));
    }
  protected:
    friend class NetMgr;
    fpi::SvcID                       ep_peer_id;
    fpi::SvcVer                      ep_peer_ver;

    EpSvcImpl(const NodeUuid       &mine,
              const NodeUuid       &peer,
              const EpAttr         &attr,
              EpEvtPlugin::pointer  ops,
              fds_uint32_t          major,
              fds_uint32_t          minor);

    EpSvcImpl(const fpi::SvcID     &mine,
              const fpi::SvcID     &peer,
              const EpAttr         &attr,
              EpEvtPlugin::pointer  ops,
              fds_uint32_t          major,
              fds_uint32_t          minor);

    EpSvcImpl(const fpi::SvcID     &mine,
              const fpi::SvcID     &peer,
              EpEvtPlugin::pointer  ops,
              fds_uint32_t          major,
              fds_uint32_t          minor);

    void         ep_handle_error(const Error &err);
    void         ep_connect_peer(int port, const std::string &ip);
    void         ep_fillin_binding(struct ep_map_rec *map);
    void         ep_peer_uuid(fpi::SvcUuid &uuid)  { uuid = ep_peer_id.svc_uuid; }
    fds_uint64_t ep_peer_uuid()  { return ep_peer_id.svc_uuid.svc_uuid; }

  public:
    /**
     * ep_connect_server
     * -----------------
     * Connect to a server by its known IP and port.  Thrift's connection handles are
     * saved by the caller.
     */
    template <class SendIf> static void
    ep_connect_server(int port, const std::string &ip, EpSvcHandle::pointer ptr)
    {
        if (ptr->ep_state == EP_ST_INIT) {
            bo::shared_ptr<net::Socket> sock(new net::Socket(ip, port));
            bo::shared_ptr<tt::TTransport> trans(new tt::TFramedTransport(sock));
            bo::shared_ptr<tp::TProtocol>  proto(new tp::TBinaryProtocol(trans));
            bo::shared_ptr<SendIf>         rpc(new SendIf(proto));

            NetMgr    *net = NetMgr::ep_mgr_singleton();
            fds_mutex *mtx = net->ep_obj_mutex(get_pointer(ptr));

            mtx->lock();
            if (ptr->ep_state == EP_ST_INIT) {
                ptr->ep_state = EP_ST_DISCONNECTED;
                fds_verify((ptr->ep_trans == NULL) && (ptr->ep_rpc == NULL));

                ptr->ep_trans = trans;
                ptr->ep_rpc   = rpc;
                sock->setEventHandler(ptr.get());
                ptr->ep_sock  = sock;
            }
            // else, these ptrs are deleted when we're out of scope.
            mtx->unlock();

            // It's ok to register twice.
            NetMgr::ep_mgr_singleton()->ep_handler_register(ptr);
        }
        ptr->ep_reconnect();
    }
};

class NetPlatSvc;

/**
 * Endpoint is the logical RPC representation of a physical connection.
 * There isn't any lock in EndPoint object because by design, it's setup by a module,
 * which is single threaded in initialization path.
 *
 * Thrift template implementation.
 */
template <class SendIf, class RecvIf>
class EndPoint : public EpSvcImpl
{
  public:
    virtual ~EndPoint() {}
    EndPoint(const fpi::SvcID          &mine,
             const fpi::SvcID          &peer,
             const EpAttr              &attr,
             boost::shared_ptr<RecvIf>  rcv_if,
             EpEvtPlugin::pointer       ops,
             fds_uint32_t               major = 0,
             fds_uint32_t               minor = 0)
        : EpSvcImpl(mine, peer, attr, ops, major, minor), ep_rpc_recv(rcv_if) {
            ep_init_obj();
        }
    EndPoint(int                        port,
             const NodeUuid            &mine,
             const NodeUuid            &peer,
             boost::shared_ptr<RecvIf>  rcv_if,
             EpEvtPlugin::pointer       ops,
             fds_uint32_t               major = 0,
             fds_uint32_t               minor = 0,
             const char                *iface = "lo")
        : EpSvcImpl(mine, peer, EpAttr(iface, port), ops, major, minor),
          ep_rpc_recv(rcv_if) {
            ep_init_obj();
        }

    /**
     * ep_reconnect
     * ------------
     */
    void ep_reconnect()
    {
        fds_assert(ep_peer != NULL);
        ep_peer->ep_reconnect();
    }
    /**
     * ep_setup_server
     * ---------------
     * Provide different methods to setup a Thrift's server.
     */
    void ep_setup_server()
    {
        int port = ep_attr->attr_get_port();
        bo::shared_ptr<tp::TProtocolFactory>  proto(new tp::TBinaryProtocolFactory());
        bo::shared_ptr<tt::TServerTransport>  trans(new tt::TServerSocket(port));
        bo::shared_ptr<tt::TTransportFactory> tfact(new tt::TFramedTransportFactory());

        ep_server = bo::shared_ptr<ts::TThreadedServer>(
                new ts::TThreadedServer(ep_rpc_recv, trans, tfact, proto));

        /* Set up listeners for processor and server events */
        ep_rpc_recv->setEventHandler(
                boost::shared_ptr<FdsTProcessorEventHandler2>(new FdsTProcessorEventHandler2()));
        ep_server->setServerEventHandler(
                boost::shared_ptr<ServerEventHandler2>(new ServerEventHandler2()));
    }

    void ep_activate()
    {
        ep_setup_server();
        ep_server->serve();
    }
    void ep_run_server() { ep_server->serve(); }

    /**
     * ep_send_handle
     * --------------
     * Get the handle to communicate with the peer endpoint.  The init. path must call
     * this method to setup the full duplex mode for this endpoint.
     */
    EpSvcHandle::pointer ep_send_handle()
    {
        if (ep_peer == NULL) {
            bool       out = true;
            NetMgr    *net = NetMgr::ep_mgr_singleton();
            fds_mutex *mtx = net->ep_obj_mutex(this);

            mtx->lock();
            if (ep_peer == NULL) {
                out     = false;
                ep_peer = new EpSvcHandle(this, ep_evt, 0, 0);
            }
            mtx->unlock();
            if (out == false) {
                endpoint_connect_handle<SendIf>(ep_peer);
            }
        }
        return ep_peer;
    }
    /**
     * ep_register
     * -----------
     * Register the endpoint and make full duplex connection.  Use this method
     * instead of NetMgr::ep_register() because NetMgr can't do connection w/out
     * the SendIf template.
     */
    void ep_register(bool update_domain = true)
    {
        NetMgr::ep_mgr_singleton()->ep_register(this, update_domain);
        if (ep_peer_id.svc_uuid != NullSvcUuid) {
            EpSvcHandle::pointer ptr = ep_send_handle();
            fds_verify(ptr != NULL);
        }
    }
    /**
     * Synchronous send/receive handlers.
     */
    boost::shared_ptr<SendIf> ep_sync_rpc()
    {
        if (ep_peer != NULL) {
            return ep_peer->svc_rpc<SendIf>();
        }
        return NULL;
    }
    boost::shared_ptr<RecvIf> ep_rpc_handler() { return ep_rpc_recv; }

    /**
     * Async message passing.
     *
     */

  protected:
    friend class NetMgr;
    friend class NetPlatSvc;
    bo::shared_ptr<SendIf>                 ep_rpc_send;
    bo::shared_ptr<RecvIf>                 ep_rpc_recv;
    bo::shared_ptr<tt::TTransport>         ep_trans;
    bo::shared_ptr<ts::TThreadedServer>    ep_server;
    fds_mutex                              ep_mtx;

    int  ep_get_status()     { return 0; }
    bool ep_is_connection()  { return true; }
    void svc_receive_msg(const fpi::AsyncHdr &msg) {}

  private:
    // ep_init_obj
    // -----------
    //
    void ep_init_obj()
    {
        ep_rpc_send = NULL;
        ep_trans    = NULL;
        ep_server   = NULL;
    }

  public:
    typedef boost::intrusive_ptr<EndPoint<SendIf, RecvIf>> pointer;
    typedef boost::intrusive_ptr<const EndPoint<SendIf, RecvIf>> const_ptr;

    static inline EndPoint<SendIf, RecvIf>::pointer ep_cast_ptr(EpSvc::pointer ptr) {
        return static_cast<EndPoint<SendIf, RecvIf> *>(get_pointer(ptr));
    }
    /**
     * Connect to the server, return the net-service handle to represent the client
     * side of the connection.
     */
    static void ep_new_handle(EpSvc::pointer        ep,
                              int                   port,
                              const std::string    &ip,
                              EpSvcHandle::pointer *clnt,
                              EpEvtPlugin::pointer  evt)
    {
        *clnt = new EpSvcHandle(ep, evt, port, 0);
        EpSvcImpl::ep_connect_server<SendIf>(port, ip, *clnt);
    }
};

// ep_svc_handle_connect
// ---------------------
// Connect the serice handle to its peer.  The binding is taken from the peer uuid.
// Note that we must pass by value here because we'll dispatch the threadpool request.
//
template <class SendIf> void
endpoint_connect_handle(EpSvcHandle::pointer ptr,
                        fpi::SvcUuid peer = NullSvcUuid, int retry = 0)
{
    int          port;
    std::string  ip;
    fpi::SvcUuid uuid;
    fds_uint32_t maj, min;

    if (peer == NullSvcUuid) {
        ptr->ep_peer_uuid(uuid);
    } else {
        uuid = peer;
    }
    maj  = ptr->ep_version(&min);
    port = NetMgr::ep_mgr_singleton()->ep_uuid_binding(uuid, maj, min, &ip);
    if (port != -1) {
        EpSvcImpl::ep_connect_server<SendIf>(port, ip, ptr);
    } else {
        fds_threadpool *pool = NetMgr::ep_mgr_singleton()->ep_mgr_thrpool();

        if (retry > 0) {
            sleep(1);
        } else if (retry > 100) {
            return;
        }
        retry++;
        pool->schedule(endpoint_connect_handle<SendIf>, ptr, uuid, retry);
        LOGDEBUG << "EP retry conn " << ip << ", port " << port << ", retry " << retry;
    }
}

template <class SendIf>
EpSvcHandle::pointer NetMgr::svc_get_handle(const fpi::SvcUuid   &peer,
                                            fds_uint32_t          maj,
                                            fds_uint32_t          min)
{
    UuidIntKey key(peer.svc_uuid, maj, min);

    /* TODO(Rao): This lock is expensive when we need to create a new client
     * We need to have finer locking between lookup and creation
     */
    fds_scoped_lock l(ep_handle_map_mtx);
    try {
        auto ep = ep_handle_map.at(key);
        return ep;
    } catch(const std::out_of_range &e) {
        std::string ip;
        auto port = ep_uuid_binding(peer, maj, min, &ip);
        LOGDEBUG <<  "New svc client.  peer: " << peer.svc_uuid
            << " ip:port: " << ip << ":" << port;

        auto ep = new EpSvcHandle(peer, NULL, maj, min);
        ep->ep_sock = bo::make_shared<net::Socket>(ip, port);
        ep->ep_trans = bo::make_shared<tt::TFramedTransport>(ep->ep_sock);
        auto proto = bo::make_shared<apache::thrift::protocol::TBinaryProtocol>(ep->ep_trans);
        ep->ep_rpc = bo::make_shared<SendIf>(proto);
        try {
            ep->ep_sock->open();
            ep_handle_map[key] = ep;
            return ep;
        } catch(std::exception &open_e) {
            LOGERROR <<  "Exception opening socket: " << e.what()
                << " peer: " << peer.svc_uuid
                << " ip:port: " << ip << ":" << port;
            return nullptr;
        }
    }
}


namespace net {


/**
 * Get or allocate a handle to communicate with the peer endpoint.  The 'mine' uuid
 * can be taken from the platform library to get the default uuid.
 */
template <class SendIf> void
svc_get_handle(const fpi::SvcUuid   &mine,
               const fpi::SvcUuid   &peer,
               EpSvcHandle::pointer *out,
               fds_uint32_t          maj,
               fds_uint32_t          min)
{
    NetMgr         *net;
    EpSvc::pointer  ep;
    // static std::vector<EpSvcHandle::pointer> ep_list;

    net  = NetMgr::ep_mgr_singleton();
    // TODO(Andrew/Rao): Make the correct lookup call when
    // we know what that is...
    *out = NULL;
    *out = net->svc_handler_lookup(peer, maj, min);
    if (*out == NULL) {
        // TODO(Vy): must suppy default values here.
        *out = new EpSvcHandle(peer, NULL, maj, min);
        // ep_list.push_back(*out);
        endpoint_connect_handle<SendIf>(*out);
        fds_assert((*out)->ep_get_socket()->isOpen());
    }
}

template <class SendIf> void
svc_get_handle(const fpi::SvcUuid   &peer,
               EpSvcHandle::pointer *out,
               fds_uint32_t          maj,
               fds_uint32_t          min)
{
    svc_get_handle<SendIf>(NullSvcUuid, peer, out, maj, min);
}

// TODO(Rao): Delete this code once Net::svc_get_handle() implementation is solid
#if 0
template <class SendIf>
bo::shared_ptr<SendIf> svc_get_client2(const fpi::SvcUuid   &peer,
                                      fds_uint32_t          maj,
                                      fds_uint32_t          min)
{
    auto net  = NetMgr::ep_mgr_singleton();
    UuidIntKey key(peer.svc_uuid, maj, min);

    fds_scoped_lock l(net->ep_client_map_mtx);
    try {
        return bo::static_pointer_cast<SendIf>(net->ep_client_map.at(key));
    } catch(const std::out_of_range &e) {
        std::string ip;
        auto port = net->ep_uuid_binding(peer, maj, min, &ip);

        LOGDEBUG <<  "New svc client.  peer: " << peer.svc_uuid
            << " ip:port: " << ip << ":" << port;

        bo::shared_ptr<net::Socket> sock(new net::Socket(ip, port));
        bo::shared_ptr<tt::TTransport> trans(new tt::TFramedTransport(sock));
        bo::shared_ptr<tp::TProtocol>  proto(new tp::TBinaryProtocol(trans));
        bo::shared_ptr<SendIf>         client(new SendIf(proto));
        try {
            sock->open();
            net->ep_client_map[key] = client;
            return client;
        } catch(std::exception &open_e) {
            LOGERROR <<  "Exception opening socket: " << e.what()
                << " peer: " << peer.svc_uuid
                << " ip:port: " << sock->getPeerAddress() << ":" << sock->getPeerPort();
            return nullptr;
        }
    }
}
#endif

#define MSG_DESERIALIZE(msgtype, error, payload) \
    net::ep_deserialize<fpi::msgtype>(const_cast<Error&>(error), payload)

template<class PayloadT> boost::shared_ptr<PayloadT>
ep_deserialize(Error &e, boost::shared_ptr<std::string> payload)
{
    DBG(GLOGDEBUG);

    if (e != ERR_OK) {
        return nullptr;
    }
    try {
        bo::shared_ptr<tt::TMemoryBuffer> memory_buf(
                new tt::TMemoryBuffer(reinterpret_cast<uint8_t*>(
                        const_cast<char*>(payload->c_str())), payload->size()));
        bo::shared_ptr<tp::TProtocol> binary_buf(new tp::TBinaryProtocol(memory_buf));

        boost::shared_ptr<PayloadT> result(boost::make_shared<PayloadT>());
        auto read = result->read(binary_buf.get());
        fds_verify(read > 0);
        return result;
    } catch(std::exception& ex) {
        GLOGWARN << "Failed to deserialize. Exception: " << ex.what();
        e = ERR_SERIALIZE_FAILED;
        return nullptr;
    }
}

}  // namespace net
}  // namespace fds
#endif  // SOURCE_INCLUDE_NET_NET_SERVICE_TMPL_H_
