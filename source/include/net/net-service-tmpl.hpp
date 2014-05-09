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

  protected:
    fpi::SvcID                       ep_peer_id;
    fpi::SvcVer                      ep_peer_ver;

  private:
    friend class NetMgr;

    void ep_fillin_binding(struct ep_map_rec *map);
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
    friend class NetMgr;
    bo::shared_ptr<SendIf>                 ep_rpc_send;
    bo::shared_ptr<tt::TTransport>         ep_sock;
    bo::shared_ptr<tt::TTransport>         ep_trans;
    bo::shared_ptr<tp::TProtocol>          ep_proto;

    bo::shared_ptr<RecvIf>                 ep_rpc_recv;
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
#endif  // SOURCE_INCLUDE_NET_NET_SERVICE_TMPL_H_
