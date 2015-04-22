/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_ACCESS_MGR_INCLUDE_FDSN_SERVER_H_
#define SOURCE_ACCESS_MGR_INCLUDE_FDSN_SERVER_H_

#include <string>
#include <util/Log.h>
#include <fdsp/XdiService.h>
#include <concurrency/Thread.h>
#include <AmDataApi.h>

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

namespace xdi_at  = apache::thrift;
namespace xdi_att = apache::thrift::transport;
namespace xdi_atc = apache::thrift::concurrency;
namespace xdi_atp = apache::thrift::protocol;
namespace xdi_ats = apache::thrift::server;

class FdsnIf;

/**
 * RPC-based server for FDSN. Exposes FDSN interface via
 * RPC-endpoints.
 */
class FdsnServer {
  private:
    fds_uint32_t             port;
    AmDataApi::shared_ptr dataApi;
    fds_uint32_t numFdsnThreads;

    /*
     * Thrift endpoint related
     */
    boost::shared_ptr<xdi_att::TServerTransport>  serverTransport;
    boost::shared_ptr<xdi_att::TTransportFactory> transportFactory;
    boost::shared_ptr<xdi_atp::TProtocolFactory>  protocolFactory;
    boost::shared_ptr<apis::XdiServiceProcessor>   processor;

    boost::shared_ptr<xdi_atc::ThreadManager>      threadManager;
    boost::shared_ptr<xdi_atc::PosixThreadFactory> threadFactory;
    std::unique_ptr<xdi_ats::TNonblockingServer> nbServer;

    std::unique_ptr<std::thread> listen_thread;

  public:
    FdsnServer(AmDataApi::shared_ptr &_dataApi,
               fds_uint32_t pmPort);
    FdsnServer(FdsnServer const&) = delete;
    FdsnServer& operator=(FdsnServer const&) = delete;
    ~FdsnServer()
        { stop(); }

    void start();
    void stop();
};

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_FDSN_SERVER_H_
