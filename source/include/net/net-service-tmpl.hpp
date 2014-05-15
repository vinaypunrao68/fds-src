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

#include <fds_typedefs.h>
#include <net/net-service.h>

namespace fds {

namespace at = apache::thrift;
namespace tc = apache::thrift::concurrency;
namespace tp = apache::thrift::protocol;
namespace ts = apache::thrift::server;
namespace tt = apache::thrift::transport;
namespace bo = boost;

// Forward declaration.
struct ep_map_rec;
template <class SendIf, class RecvIf> class EndPoint;

/*
 * -------------------------------------------------------------------------------------
 * Internal tempalate implementation:
 * -------------------------------------------------------------------------------------
 */

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

    void ep_fillin_binding(struct ep_map_rec *map);
    void ep_peer_uuid(fpi::SvcUuid &uuid) { uuid = ep_peer_id.svc_uuid; }
    fds_uint64_t ep_peer_uuid() { return ep_peer_id.svc_uuid.svc_uuid; }

    virtual void ep_connect_server(int port, const std::string &ip) = 0;
};

class NetPlatSvc;

// ep_connect_server
// -----------------
// Connect to a server by its known IP and port.  Thrift's connection handles are
// saved by the caller.
//
template <class SendIf>
void endpoint_connect_server(int port, const std::string &ip,
                             bo::shared_ptr<SendIf>         &out,
                             bo::shared_ptr<tt::TTransport> &trans)
{
    if (trans != NULL) {
        // Reset the old connection.
        trans->close();
    } else {
        bo::shared_ptr<tt::TTransport> sock(new tt::TSocket(ip, port));
        trans.reset(new tt::TFramedTransport(sock));

        bo::shared_ptr<tp::TProtocol>  proto(new tp::TBinaryProtocol(trans));
        out.reset(new SendIf(proto));
    }
    try {
        trans->open();
    } catch(at::TException &tx) {  // NOLINT
        std::cout << "Error: " << tx.what() << std::endl;
    }
}

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

    static inline EndPoint<SendIf, RecvIf>::pointer ep_cast_ptr(EpSvc::pointer ptr) {
        return static_cast<EndPoint<SendIf, RecvIf> *>(get_pointer(ptr));
    }
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

    // ep_reconnect
    // ------------
    //
    void ep_reconnect()
    {
    }
    // ep_server_listen
    // ----------------
    //
    void ep_setup_server()
    {
        int port = ep_attr->attr_get_port();
        bo::shared_ptr<tp::TProtocolFactory>  proto(new tp::TBinaryProtocolFactory());
        bo::shared_ptr<tt::TServerTransport>  trans(new tt::TServerSocket(port));
        bo::shared_ptr<tt::TTransportFactory> tfact(new tt::TFramedTransportFactory());

        ep_server = bo::shared_ptr<ts::TThreadedServer>(
                new ts::TThreadedServer(ep_rpc_recv, trans, tfact, proto));
    }
    // Connect to the server, return the net-service handle to represent the client
    // side of the connection.
    //
    EpSvcHandle::pointer ep_new_handle(int port, const std::string &ip)
    {
        bo::shared_ptr<SendIf> rpc;
        EpSvcHandle::pointer clnt = new EpSvcHandle(this, NULL, NULL);

        endpoint_connect_server<SendIf>(port, ip, rpc, clnt->ep_trans);
        clnt->ep_rpc = rpc;
        return clnt;
    }
    EpSvcHandle::pointer ep_server_handle() {
        return ep_clnt_ptr;
    }
    // Connect to the server.  Save connection handles to this endpoint.
    //
    void ep_connect_server(int port, const std::string &ip)
    {
        if (ep_clnt_ptr == NULL) {
            bo::shared_ptr<SendIf> rpc;

            ep_clnt_ptr = new EpSvcHandle(this, NULL, NULL);
            endpoint_connect_server<SendIf>(port, ip, rpc, ep_clnt_ptr->ep_trans);
            ep_clnt_ptr->ep_rpc = rpc;
        }
    }
    void ep_activate() {
        ep_setup_server();
    }
    void ep_run_server() {
        ep_server->serve();
    }
    // Synchronous send/receive handlers.
    //
    boost::shared_ptr<SendIf> ep_sync_rpc()
    {
        if (ep_clnt_ptr != NULL) {
            return boost::static_pointer_cast<SendIf>(ep_clnt_ptr->ep_rpc);
        }
        return NULL;
    }
    boost::shared_ptr<RecvIf> ep_rpc_handler() { return ep_rpc_recv; }

    // Async message passing.
    //
    //

  protected:
    friend class NetPlatSvc;
    bo::shared_ptr<SendIf>                 ep_rpc_send;
    bo::shared_ptr<RecvIf>                 ep_rpc_recv;
    bo::shared_ptr<tt::TTransport>         ep_trans;
    bo::shared_ptr<ts::TThreadedServer>    ep_server;
    bo::shared_ptr<ts::TNonblockingServer> ep_nb_srv;
    EpSvcHandle::pointer                   ep_clnt_ptr;

    int  ep_get_status() { return 0; }
    bool ep_is_connection() { return true; }
    void svc_receive_msg(const fpi::AsyncHdr &msg) {}

  private:
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
        ep_trans    = NULL;
        ep_server   = NULL;
        ep_nb_srv   = NULL;
        ep_clnt_ptr = NULL;
    }
};

}  // namespace fds
#endif  // SOURCE_INCLUDE_NET_NET_SERVICE_TMPL_H_
