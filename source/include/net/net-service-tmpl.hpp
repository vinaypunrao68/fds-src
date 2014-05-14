/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_NET_NET_SERVICE_TMPL_H_
#define SOURCE_INCLUDE_NET_NET_SERVICE__TMPLH_

#include <unistd.h>
#include <arpa/inet.h>
#include <thrift/transport/TSocket.h>
#include <thrift/transport/TBufferTransports.h>
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TNonblockingServer.h>
#include <thrift/server/TThreadedServer.h>
#include <thrift/concurrency/ThreadManager.h>
#include <net/net-service.h>

namespace fds {

struct ep_map_rec;

namespace at = apache::thrift;
namespace tc = apache::thrift::concurrency;
namespace tp = apache::thrift::protocol;
namespace ts = apache::thrift::server;
namespace tt = apache::thrift::transport;
namespace bo = boost;

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

    // Endpoint event input.
    //
    virtual void ep_input_event(fds_uint32_t evt);

  protected:
    fpi::SvcID                       ep_peer_id;
    fpi::SvcVer                      ep_peer_ver;

  private:
    friend class NetMgr;

    void ep_fillin_binding(struct ep_map_rec *map);
    virtual fds_uint64_t ep_peer_uuid() { return ep_peer_id.svc_uuid.svc_uuid; }
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
    // ep_connect_server
    // -----------------
    // Connect to a server by its known IP and port.  Thrift's connection handles are
    // saved by the caller.
    //
    void ep_connect_server(int port, const std::string &ip,
                           bo::shared_ptr<SendIf>         *out,
                           bo::shared_ptr<tt::TTransport> *trans)
    {
        if ((*trans) != NULL) {
            // Reset the old connection.
            (*trans)->close();
        } else {
            bo::shared_ptr<tt::TTransport> sock(new tt::TSocket(ip, port));
            *trans = bo::shared_ptr<tt::TTransport>(new tt::TFramedTransport(sock));

            bo::shared_ptr<tp::TProtocol>  proto(new tp::TBinaryProtocol(*trans));
            *out = bo::shared_ptr<SendIf>(new SendIf(proto));
        }
        try {
            (*trans)->open();
        } catch(at::TException &tx) {  // NOLINT
            std::cout << "Error: " << tx.what() << std::endl;
        }
    }
    // Connect to the server, return the net-service handle to represent the client
    // side of the connection.
    //
    EpSvcHandle::pointer ep_server_handle(int port, const std::string &ip)
    {
        EpSvcHandle::pointer clnt = new EpSvcHandle();
        ep_connect_server(port, ip, &clnt->ep_rpc, &clnt->ep_trans);
        return clnt;
    }
    // Connect to the server.  Save connection handles to this endpoint.
    //
    void ep_connect_server(int port, const std::string &ip) {
        ep_connect_server(port, ip, &ep_rpc_send, &ep_trans);
    }
    void ep_activate() {
        ep_setup_server();
        ep_client_connect();
    }
    void ep_run_server() {
        ep_server->serve();
    }
    // Synchronous send/receive handlers.
    //
    boost::shared_ptr<SendIf> ep_sync_rpc() { return ep_rpc_send; }
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

    void svc_receive_msg(const fpi::AsyncHdr &msg) {}
    void *ep_get_rcv_handler() { return static_cast<void *>(ep_rpc_recv.get()); }

  private:
    // ep_client_connect
    // -----------------
    //
    void ep_client_connect()
    {
#if 0
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
#endif
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
    void ep_init_obj() {
        ep_trans    = NULL;
        ep_server   = NULL;
        ep_rpc_send = NULL;
    }
};

}  // namespace fds
#endif  // SOURCE_INCLUDE_NET_NET_SERVICE_TMPL_H_
