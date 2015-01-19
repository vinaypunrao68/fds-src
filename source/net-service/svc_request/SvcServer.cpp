/*
 * Copyright 2015 by Formation Data Systems, Inc.
 */
#include <arpa/inet.h>
#include <util/Log.h>
#include <fdsp/PlatNetSvc.h>
#include <thrift/server/TThreadedServer.h>
#include <thrift/transport/TBufferTransports.h>
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/TServerSocket.h>
#include <net/SvcServer.h>

namespace fds {

SvcServer::SvcServer(int port, fpi::PlatNetSvcProcessorPtr processor)
{
    port_ = port;
    boost::shared_ptr<tp::TProtocolFactory>  proto(new tp::TBinaryProtocolFactory());
    boost::shared_ptr<tt::TServerTransport>  trans(new tt::TServerSocket(port));
    boost::shared_ptr<tt::TTransportFactory> tfact(new tt::TFramedTransportFactory());

    server_ = boost::shared_ptr<ts::TThreadedServer>(
        new ts::TThreadedServer(processor, trans, tfact, proto));

}

SvcServer::~SvcServer()
{
    GLOGNOTIFY << logString();
}

void SvcServer::start()
{
    GLOGNOTIFY << logString();
    serverThread_.reset(new std::thread([this] {this->serve_();}));
}

void SvcServer::stop() {
    GLOGNOTIFY << logString();
    server_->stop();
}

void SvcServer::serve_() {
    GLOGNOTIFY << logString();
    try {
        server_->serve();
    } catch (std::exception &e) {
        GLOGWARN << "Stopping " << logString() << " Exception: " << e.what();
    } catch (...) {
        GLOGWARN << "Stopping " << logString() << " unknown exception";
    }
}

std::string SvcServer::logString() const
{
    std::stringstream ss;
    ss << "Server at port: " << port_;
    return ss.str();
}

} // namespace fds
