/*
 * Copyright 2015 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_NET_SVCSERVER_H_
#define SOURCE_INCLUDE_NET_SVCSERVER_H_

#include <boost/shared_ptr.hpp>
#include <thread>
#include <memory>
#include <atomic>
#include <set>
#include <boost/enable_shared_from_this.hpp>
#include <concurrency/Mutex.h>
#include <thrift/server/TServer.h>
#include <thrift/TProcessor.h>

/* Forward declarations */
namespace apache { namespace thrift { namespace transport {
    class TSocket;
    class TTransport;
    using TTransportPtr = boost::shared_ptr<TTransport>;
}}}  // namespace apache::thrift::transport

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
struct SvcServer : boost::enable_shared_from_this<SvcServer>,
    apache::thrift::server::TServerEventHandler,
    apache::thrift::TProcessorEventHandler
{
    SvcServer(int port, fpi::PlatNetSvcProcessorPtr processor);
    virtual ~SvcServer();
    void start();
    void stop();

    /**
     * @brief Called when new connection is established
     *
     * @param input
     * @param output
     *
     * @return 
     */
    virtual void* createContext(boost::shared_ptr<tp::TProtocol> input,
                                boost::shared_ptr<tp::TProtocol> output) override;
    /**
     * Called when a client has finished request-handling to delete server
     * context.
     */
    virtual void deleteContext(void* serverContext,
                               boost::shared_ptr<tp::TProtocol>input,
                               boost::shared_ptr<tp::TProtocol>output) override;
    /**
    * @brief Invoked when thrift process encounters an error
    *
    * @param ctx
    * @param fn_name
    */
    virtual void handlerError(void* ctx, const char* fn_name) override;

    std::string logString() const;

 protected:
    void serve_();
    void closeClientConnections_();
    
    int port_;
    boost::shared_ptr<tt::TServerTransport> serverTransport_;
    boost::shared_ptr<ts::TServer> server_;
    std::unique_ptr<std::thread> serverThread_;

    /* Connected transports */
    /**
     * Functor for comparing buffered transports
     */
    struct LtTransports {
        bool operator() (const tt::TTransportPtr &t1, const tt::TTransportPtr &t2) {
            return t1.get() < t2.get();
        }
    };
    std::set<tt::TTransportPtr, LtTransports> connections_;
    fds_mutex connectionsLock_;
    std::atomic<bool> stopped_;
};
}  // namespace fds

#endif  // SOURCE_INCLUDE_NET_SVCSERVER_H_
