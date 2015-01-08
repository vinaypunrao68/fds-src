/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_NET_NET_SERVICE_TMPL_H_
#define SOURCE_INCLUDE_NET_NET_SERVICE_TMPL_H_

#include <unistd.h>
#include <fcntl.h>
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
      boost::shared_ptr<tt::TFramedTransport> buf_transport =
          boost::static_pointer_cast<tt::TFramedTransport>(transport);

      boost::shared_ptr<tt::TSocket> sock =
          boost::static_pointer_cast<tt::TSocket>(
                  buf_transport->getUnderlyingTransport());

      ret << sock->getPeerAddress() << ":" << sock->getPeerPort();
      return ret.str();
  }
  /**
   * Called when a new client has connected and is about to being processing.
   */
  virtual void* createContext(
      boost::shared_ptr<tp::TProtocol> input,
      boost::shared_ptr<tp::TProtocol> output) override
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
                             boost::shared_ptr<tp::TProtocol>input,
                             boost::shared_ptr<tp::TProtocol>output)
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
};

class NetPlatSvc;

/**
 * endpoint_connect_server
 * -----------------------
 * Connect to a server by its known IP and port.  Thrift's connection handles are
 * saved by the caller.
 */
template <class SendIf> static void
endpoint_connect_server(int                   port,
                        const std::string    &ip,
                        EpSvcHandle::pointer *out,
                        EpSvc::pointer        owner,
                        fpi::SvcUuid          peer,
                        EpEvtPlugin::pointer  evt,
                        fds_uint32_t          maj,
                        fds_uint32_t          min)
{
    NetMgr               *net;
    EpSvcHandle::pointer  ptr;
    uint32_t bufSz = 512;

    net = NetMgr::ep_mgr_singleton();
    if (peer != NullSvcUuid) {
        ptr = net->svc_handler_lookup(peer, maj, min);
        if (ptr != NULL) {
            *out = ptr;
            return;
        }
    }
    bo::shared_ptr<tt::TTransport> sock(new tt::TSocket(ip, port));
    bo::shared_ptr<tt::TTransport> trans(new tt::TFramedTransport(sock, bufSz));
    bo::shared_ptr<tp::TProtocol>  proto(new tp::TBinaryProtocol(trans));
    // TODO(Rao): Change this to use alloc_rpc_client()
    bo::shared_ptr<SendIf>         rpc(new SendIf(proto));

    if (owner == NULL) {
        ptr = new EpSvcHandle(peer, evt, maj, min);
    } else {
        ptr = new EpSvcHandle(owner, evt, maj, min);
    }
    ptr->ep_state = EP_ST_DISCONNECTED | EP_ST_NO_PLUGIN;
    fds_verify((ptr->ep_trans == NULL) && (ptr->ep_rpc == NULL));

    ptr->ep_trans = trans;
    ptr->ep_rpc   = rpc;
    ptr->ep_sock  = bo::static_pointer_cast<tt::TSocket>(sock);

    ptr->ep_reconnect();
    if (peer == NullSvcUuid) {
        *out = ptr;
        ptr->ep_notify_plugin(true);
        LOGDEBUG <<  "[Svc] new svc client "
                 << " - ip:port: " << ip << ":" << port << " - ptr " << ptr;
        return;
    }
    net->ep_handler_register(ptr);

    // This is atomic assignment.
    *out = net->svc_handler_lookup(peer, maj, min);
    LOGDEBUG <<  "[Svc] new svc client - peer: " << peer.svc_uuid
             << " - ip:port: " << ip << ":" << port
             << " - ptr " << ptr << " - out " << (*out);
    fds_assert(*out != NULL);
}

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
             const char                *iface = NULL)
        : EpSvcImpl(mine, peer, EpAttr(iface, port), ops, major, minor),
          ep_rpc_recv(rcv_if) {
            ep_init_obj();
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
                boost::shared_ptr<FdsTProcessorEventHandler2>(
                    new FdsTProcessorEventHandler2()));
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
     * ep_register
     * -----------
     * Register the endpoint and make full duplex connection.  Use this method
     * instead of NetMgr::ep_register() because NetMgr can't do connection w/out
     * the SendIf template.
     */
    void ep_register(bool update_domain = true)
    {
        NetMgr::ep_mgr_singleton()->ep_register(this, update_domain);
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
        fds_uint32_t maj, min;

        maj = ep->ep_version(&min);
        endpoint_connect_server<SendIf>(port, ip, clnt, ep, NullSvcUuid, evt, maj, min);
    }
};

template <class SendIf>
EpSvcHandle::pointer NetMgr::svc_get_handle(const fpi::SvcUuid   &peer,
                                            fds_uint32_t          maj,
                                            fds_uint32_t          min)
{
    UuidIntKey key(peer.svc_uuid, maj, min);

    try {
        fds_scoped_lock l(ep_handle_map_mtx);
        auto ep = ep_handle_map.at(key);
        int fd = ep->ep_sock->getSocketFD();
        do {
            if (fcntl(fd, F_GETFL) != -1) {
                return (ep); 
            }
        } while (errno == EINTR);
        if (errno != EBADF) {
            return (ep);
        }
        try {
            ep->ep_sock->close();
            ep->ep_sock->open();
            LOGNORMAL << " connection failed and redone successfully. ";
            return (ep);
        } catch (...) {
            return (nullptr);
        }
    } catch(const std::out_of_range &e) {
        /* Create the endpoint client
         * NOTE: We let go of lock here because socket connection
         * is expensive.  We reaquire the lock when adding to map below
         */
        std::string ip;
        auto port = ep_uuid_binding(peer, maj, min, &ip);

        if (port < 0) {
            LOGWARN <<  "Endpoint address not found for peer: " << peer.svc_uuid;
            return nullptr;
        }

        LOGDEBUG <<  "New svc client.  peer: " << peer.svc_uuid
            << " ip:port: " << ip << ":" << port;

        EpSvcHandle::pointer ep = new EpSvcHandle(peer, NULL, maj, min);
        ep->ep_sock = bo::make_shared<net::Socket>(ip, port);
        ep->ep_trans = bo::make_shared<tt::TFramedTransport>(ep->ep_sock);
        auto proto = bo::make_shared<tp::TBinaryProtocol>(ep->ep_trans);
        ep->ep_rpc = alloc_rpc_client(peer, maj, min, proto);
        try {
            ep->ep_sock->open();
            {
                fds_scoped_lock l(ep_handle_map_mtx);
                /* Look up again.  ep may hanve been added while we weren't under lock */
                auto ret_ep_itr = ep_handle_map.find(key);
                if (ret_ep_itr == ep_handle_map.end()) {
                    ep_handle_map[key] = ep;
                    return ep;
                } else {
                    LOGDEBUG <<  "Ignoring extra endpoint.  peer: " << peer.svc_uuid
                        << " ip:port: " << ip << ":" << port;
                    return ret_ep_itr->second;
                }
            }
        } catch(std::exception &open_e) {
            LOGERROR <<  "Exception opening socket: " << e.what()
                << " peer: " << peer.svc_uuid
                << " ip:port: " << ip << ":" << port;
            return nullptr;
        }
    }
}

/**
* @brief Retuns the thrift client rpc
*
* @tparam ClientIf one of PlatNetSvc, SMSvc, etc.
* @param peer
* @param maj
* @param min
*
* @return 
*/
template <class ClientIf>
bo::shared_ptr<ClientIf> NetMgr::client_rpc(const fpi::SvcUuid   &peer,
                                    fds_uint32_t          maj,
                                    fds_uint32_t          min)
{
    EpSvcHandle::pointer ep = NetMgr::ep_mgr_singleton()->svc_get_handle<ClientIf>(peer, 0 , min);
    if (!ep) {
        return nullptr;
    }
    return ep->svc_rpc<ClientIf>();
    // return bo::shared_ptr<ClientIf>();
}

namespace net {

/**
 * svc_get_handle
 * --------------
 * Get or allocate a handle to communicate with the peer endpoint.  The 'mine' uuid
 * can be taken from the platform library to get the default uuid.
 */
template <class SendIf> void
svc_get_handle(const fpi::SvcUuid   &peer,
               EpSvcHandle::pointer *out,
               fds_uint32_t          maj,
               fds_uint32_t          min,
               EpEvtPlugin::pointer  evt = NULL)
{
    NetMgr         *net;
    int             port;
    std::string     ip;
    EpSvc::pointer  ep;

    net  = NetMgr::ep_mgr_singleton();
    *out = NULL;
    *out = net->svc_handler_lookup(peer, maj, min);
    if (*out != NULL) {
        if ((*out)->ep_get_status() != EP_ST_CONNECTED) {
            (*out)->ep_reconnect();
        }
        return;
    }
    port = NetMgr::ep_mgr_singleton()->ep_uuid_binding(peer, maj, min, &ip);
    fds_verify(port != -1);

    endpoint_connect_server<SendIf>(port, ip, out, NULL, peer, evt, maj, min);
}

template <class SendIf> void
svc_get_handle(const std::string    &ip,
               fds_uint32_t          port,
               EpSvcHandle::pointer *out,
               const fpi::SvcUuid   &peer,
               fds_uint32_t          maj,
               fds_uint32_t          min,
               EpEvtPlugin::pointer  evt = NULL)
{
    endpoint_connect_server<SendIf>(port, ip, out, NULL, peer, evt, maj, min);
}

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
