/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#ifndef RPCEP_H
#define RPCEP_H

#include <iostream>
#include <functional>
#include <unordered_map>

#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <thrift/concurrency/ThreadManager.h>
#include <thrift/server/TThreadedServer.h>
#include <thrift/transport/TTransportUtils.h>
#include <fdsp/FDSP_DataPathReq.h>
#include <fds_typedefs.h>
#include <net/net-service.h>
#include <RpcRequest.h>
#include <RpcRequestPool.h>

using namespace FDS_ProtocolInterface;  // NOLINT
using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::concurrency;
using namespace ::apache::thrift::transport;
using namespace ::apache::thrift::server;

namespace fds {

typedef uint64_t EpId;
class RpcEpBase {
public:
    RpcEpBase(const EpId& id)
    : id_(id) {}
    EpId getId() {return id_;}

protected:
    EpId id_;
};
typedef boost::shared_ptr<RpcEpBase> RpcEpBasePtr;

template <class ClientT>
class RpcEp : public RpcEpBase {
public:
    RpcEp(const EpId id, std::string ip, uint32_t port)
    : RpcEpBase(id),
      socket_(new TSocket(ip, port)),
      transport_(new apache::thrift::transport::TBufferedTransport(socket_)),
      protocol_(new TBinaryProtocol(transport_))
    {
          req_client_.reset(new ClientT(protocol_));
    }
    void connect() {
        transport_->open();
    }
    boost::shared_ptr<ClientT> getClient() {return req_client_;}
protected:
    boost::shared_ptr<TSocket> socket_;
    boost::shared_ptr<TTransport> transport_;
    boost::shared_ptr<TProtocol> protocol_;
    std::string session_id_;

    /* Thrift generated client interface */
    boost::shared_ptr<ClientT> req_client_;
};

class RpcEpMgr {
public:
    RpcEpMgr() {
        tracker_.reset(new AsyncRpcRequestTracker());
    }
    void addEp(const EpId& id, RpcEpBasePtr ep) {
        epMap_[id] = ep;
    }

    template<class ClientT>
    boost::shared_ptr<ClientT> getEpClient(const EpId &id) {
        auto ret = epMap_[id];
        // TODO(Rao): I don't believe static_pointer_cast works here.  Need to use dynamic pointer cast
        return boost::static_pointer_cast<RpcEp<ClientT>>(ret)->getClient();
    }

    template<class PayloadT>
    void sendAsyncResponse(const AsyncHdr& reqHeader, const PayloadT& payload) {
        auto rspHeader = swapHeader(reqHeader);

        boost::shared_ptr<TMemoryBuffer> buffer(new TMemoryBuffer());
        boost::shared_ptr<TProtocol> binary_buf(new TBinaryProtocol(buffer));

        auto written = payload.write(binary_buf.get());
        fds_verify(written > 0);

        auto client = getEpClient<AsyncRspSvcClient>(rspHeader.msg_dst_uuid.svc_uuid);

        client->asyncResponse(rspHeader, buffer->getBufferAsString());
    }

    AsyncRpcRequestTrackerPtr getAsyncRpcRequestTracker()
    {
        return tracker_;
    }

    static AsyncHdr swapHeader(const AsyncHdr &reqHeader) {
        auto rspHeader = reqHeader;
        rspHeader.msg_src_uuid = reqHeader.msg_dst_uuid;
        rspHeader.msg_dst_uuid = reqHeader.msg_src_uuid;
        return rspHeader;
    }

protected:
    std::unordered_map<EpId, RpcEpBasePtr> epMap_;
    AsyncRpcRequestTrackerPtr tracker_;
};

template <class ProcessorT, class HandlerT>
class RpcServer {
public:
    RpcServer(const uint32_t &port) {

        boost::shared_ptr<TServerTransport> serverTransport;
        boost::shared_ptr<TTransportFactory> transportFactory;
        boost::shared_ptr<TProtocolFactory> protocolFactory;
        boost::shared_ptr<TProcessor> processor;
        handler_.reset(new HandlerT());

        serverTransport.reset(new apache::thrift::transport::TServerSocket(port));
        transportFactory.reset(new apache::thrift::transport::TBufferedTransportFactory());
        protocolFactory.reset( new TBinaryProtocolFactory());

        processor.reset(new ProcessorT(handler_));
        server_.reset(new TThreadedServer(processor,
                serverTransport, transportFactory, protocolFactory));
    }
    ~RpcServer() {
      server_->stop();
      if (listen_thread_) {
          listen_thread_->join();
      }
    }
    void start()
    {
      std::cerr << "Starting the server on a separate thread..." << std::endl;
      listen_thread_.reset(new boost::thread(&TThreadedServer::serve, server_.get()));
    }
    void setEpMgr(RpcEpMgr *mgr) {
        handler_->setEpMgr(mgr);
    }

protected:
  boost::shared_ptr<TThreadedServer> server_;
  boost::shared_ptr<HandlerT> handler_;
  /* For running the server listen on a seperate thread.  This way we don't
   * block the caller.
   */
  boost::shared_ptr<boost::thread> listen_thread_;
};

}  // namespace fds
#endif
