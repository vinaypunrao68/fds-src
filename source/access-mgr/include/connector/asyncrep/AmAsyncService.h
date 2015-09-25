/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_ACCESS_MGR_INCLUDE_AMASYNCSERVICE_H_
#define SOURCE_ACCESS_MGR_INCLUDE_AMASYNCSERVICE_H_

#include <string>
#include <util/Log.h>
#include <fdsp/AsyncXdiServiceResponse.h>
#include <concurrency/Thread.h>
#include <AmAsyncDataApi.h>

#include <arpa/inet.h>
#include <thrift/concurrency/ThreadManager.h>
#include <thrift/concurrency/PosixThreadFactory.h>
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TThreadPoolServer.h>
#include <thrift/server/TThreadedServer.h>
#include <thrift/server/TNonblockingServer.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TBufferTransports.h>
#include <thrift/transport/TTransportUtils.h>

namespace fds {

struct AmProcessor;

namespace xdi_at  = apache::thrift;
namespace xdi_att = apache::thrift::transport;
namespace xdi_atc = apache::thrift::concurrency;
namespace xdi_atp = apache::thrift::protocol;
namespace xdi_ats = apache::thrift::server;

/**
 * RPC-based async server for XDI. Exposes AM data interface via
 * RPC-endpoints.
 */
class AsyncDataServer {
    fds_uint32_t               port;

    // Thrift endpoint related
    boost::shared_ptr<xdi_att::TServerTransport>  serverTransport;
    boost::shared_ptr<xdi_att::TTransportFactory> transportFactory;
    boost::shared_ptr<xdi_atp::TProtocolFactory>  protocolFactory;
    boost::shared_ptr<xdi_at::TProcessorFactory>  processorFactory;

    std::unique_ptr<xdi_ats::TThreadedServer>    ttServer;

    std::shared_ptr<std::thread> listen_thread;

  public:
    AsyncDataServer(std::weak_ptr<AmProcessor> processor, fds_uint32_t pmPort);
    AsyncDataServer(AsyncDataServer const&) = delete;
    AsyncDataServer& operator=(AsyncDataServer const&) = delete;
    virtual ~AsyncDataServer()
        { stop(); }

    void start();
    void stop();
};

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_AMASYNCSERVICE_H_
