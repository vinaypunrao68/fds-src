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

#include <fds_resource.h>
#include <net/net-service.h>
#include <concurrency/ThreadPool.h>
#include <fds_typedefs.h>

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

  protected:
    friend class NetMgr;
    fpi::SvcID                       ep_peer_id;
    fpi::SvcVer                      ep_peer_ver;

    EpSvcImpl(const NodeUuid       &mine,
              const NodeUuid       &peer,
              const EpAttr         &attr,
              EpEvtPlugin::pointer  ops);

    EpSvcImpl(const fpi::SvcID     &mine,
              const fpi::SvcID     &peer,
              const EpAttr         &attr,
              EpEvtPlugin::pointer  ops);

    EpSvcImpl(const fpi::SvcID &mine, const fpi::SvcID &peer, EpEvtPlugin::pointer ops);

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
        if (ptr->ep_state == EP_ST_CONNECTED) {
            return;
        }
        if (ptr->ep_trans != NULL) {
            // Reset the old connection.
            ptr->ep_trans->close();
        } else {
            bo::shared_ptr<tt::TTransport> sock(new tt::TSocket(ip, port));
            ptr->ep_trans.reset(new tt::TFramedTransport(sock));

            bo::shared_ptr<tp::TProtocol> proto(new tp::TBinaryProtocol(ptr->ep_trans));
            bo::shared_ptr<SendIf> rpc(new SendIf(proto));
            ptr->ep_rpc = rpc;
        }
        try {
            ptr->ep_state = EP_ST_CONNECTING;
            ptr->ep_trans->open();
            ptr->ep_state = EP_ST_CONNECTED;
            ptr->ep_notify_plugin();
        } catch(...) {
            fds_threadpool *pool = NetMgr::ep_mgr_singleton()->ep_mgr_thrpool();
            pool->schedule(endpoint_retry_connect, ptr);
        }
    }
};

class NetPlatSvc;

/**
 * Endpoint is the logical RPC representation of a physical connection.
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
             EpEvtPlugin::pointer       ops)
        : EpSvcImpl(mine, peer, attr, ops), ep_rpc_recv(rcv_if) {
            ep_init_obj();
        }
    EndPoint(int                        port,
             const NodeUuid            &mine,
             const NodeUuid            &peer,
             boost::shared_ptr<RecvIf>  rcv_if,
             EpEvtPlugin::pointer       ops,
             const char                *iface = "lo")
        : EpSvcImpl(mine, peer, EpAttr(iface, port), ops), ep_rpc_recv(rcv_if) {
            ep_init_obj();
        }

    /**
     * ep_reconnect
     * ------------
     */
    void ep_reconnect()
    {
    }
    /**
     * ep_setup_server
     * ---------------
     */
    void ep_setup_server()
    {
        int port = ep_attr->attr_get_port();
        bo::shared_ptr<tp::TProtocolFactory>  proto(new tp::TBinaryProtocolFactory());
        bo::shared_ptr<tt::TServerTransport>  trans(new tt::TServerSocket(port));
        bo::shared_ptr<tt::TTransportFactory> tfact(new tt::TFramedTransportFactory());

        ep_server = bo::shared_ptr<ts::TThreadedServer>(
                new ts::TThreadedServer(ep_rpc_recv, trans, tfact, proto));
    }
    /**
     * Get the handle to communicate with the peer endpoint.
     */
    EpSvcHandle::pointer ep_send_handle()
    {
        if (ep_peer == NULL) {
            ep_peer = new EpSvcHandle(this, ep_evt);
            endpoint_connect_handle<SendIf>(ep_peer);
        }
        return ep_peer;
    }
    /**
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
    void ep_activate() {
        ep_setup_server();
        ep_server->serve();
    }
    void ep_run_server() {
        ep_server->serve();
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
        *clnt = new EpSvcHandle(ep, evt);
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

    if (peer == NullSvcUuid) {
        ptr->ep_peer_uuid(uuid);
    } else {
        uuid = peer;
    }
    port = NetMgr::ep_mgr_singleton()->ep_uuid_binding(uuid, &ip);
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
    }
}

}  // namespace fds
#endif  // SOURCE_INCLUDE_NET_NET_SERVICE_TMPL_H_
