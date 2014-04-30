/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_AM_ENGINE_FDSN_SERVER_H_
#define SOURCE_INCLUDE_AM_ENGINE_FDSN_SERVER_H_

#include <string>
#include <util/Log.h>
#include <fds_module.h>
#include <native_api.h>
#include <apis/AmService.h>
#include <concurrency/Thread.h>

#include <arpa/inet.h>
#include <thrift/concurrency/ThreadManager.h>
#include <thrift/concurrency/PosixThreadFactory.h>
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TThreadPoolServer.h>
#include <thrift/server/TThreadedServer.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TBufferTransports.h>
#include <thrift/transport/TTransportUtils.h>

namespace fds {

namespace xdi_at  = apache::thrift;
namespace xdi_att = apache::thrift::transport;
namespace xdi_atp = apache::thrift::protocol;
namespace xdi_ats = apache::thrift::server;

class FdsnIf;

/**
 * RPC-based server for FDSN. Exposes FDSN interface via
 * RPC-endpoints.
 */
class FdsnServer : public Module {
  private:
    FDS_NativeAPI::ptr       am_api;
    fds_uint32_t             port;
    typedef boost::shared_ptr<FdsnIf> FdsnIfPtr;
    FdsnIfPtr fdsnInterface;

    /*
     * Thrift endpoint related
     */
    boost::shared_ptr<xdi_att::TServerTransport>  serverTransport;
    boost::shared_ptr<xdi_att::TTransportFactory> transportFactory;
    boost::shared_ptr<xdi_atp::TProtocolFactory>  protocolFactory;
    boost::shared_ptr<apis::AmServiceProcessor>    processor;
    boost::shared_ptr<xdi_ats::TThreadedServer>   server;
    // boost::shared_ptr<xdi::AmShimIf>  handler;

    boost::shared_ptr<boost::thread> listen_thread;

  public:
    explicit FdsnServer(const std::string &name);
    virtual ~FdsnServer() {
    }
    typedef boost::shared_ptr<FdsnServer> ptr;

    int  mod_init(SysParams const *const param);
    void mod_startup();
    void mod_shutdown();

    virtual void init_server(FDS_NativeAPI::ptr api);
    virtual void deinit_server();
};

extern FdsnServer gl_FdsnServer;

}  // namespace fds

#endif  // SOURCE_INCLUDE_AM_ENGINE_FDSN_SERVER_H_
