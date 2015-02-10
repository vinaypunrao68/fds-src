/*
 * Copyright 2015 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_NET_SVCSERVER_H_
#define SOURCE_INCLUDE_NET_SVCSERVER_H_

#include <boost/shared_ptr.hpp>
#include <thread>
#include <memory>

/* Forward declarations */
namespace apache { namespace thrift { namespace transport {
    class TSocket;
    class TTransport;
}}}  // namespace apache::thrift::transport

namespace apache { namespace thrift { namespace server {
    class TServer;
}}}  // namespace apache::thrift::server

namespace FDS_ProtocolInterface {
    class PlatNetSvcProcessor;
    using PlatNetSvcProcessorPtr = boost::shared_ptr<PlatNetSvcProcessor>;
}  // namespace FDS_ProtocolInterface

namespace fpi = FDS_ProtocolInterface;

namespace fds {
namespace at = apache::thrift;
namespace tc = apache::thrift::concurrency;
namespace tp = apache::thrift::protocol;
namespace ts = apache::thrift::server;
namespace tt = apache::thrift::transport;

/**
* @brief Server for the service.  Every service will have an instance of this server
*/
struct SvcServer {
    SvcServer(int port, fpi::PlatNetSvcProcessorPtr processor);
    virtual ~SvcServer();
    void start();
    void stop();
    std::string logString() const;

 protected:
    void serve_();
    
    int port_;
    boost::shared_ptr<ts::TServer> server_;
    std::unique_ptr<std::thread> serverThread_;
};
}  // namespace fds

#endif  // SOURCE_INCLUDE_NET_SVCSERVER_H_
