/*
 * Copyright 2015 by Formation Data Systems, Inc.
 */
#include <arpa/inet.h>
#include <util/Log.h>
#include <fdsp/PlatNetSvc.h>
#include <thrift/server/TThreadedServer.h>
#include <thrift/transport/TBufferTransports.h>
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/TSocket.h>
#include <thrift/transport/TServerSocket.h>
#include <net/SvcServer.h>

namespace fds {

// TODO(Rao): Move this function into net_util.h
static std::string getTransportKey(
    boost::shared_ptr<apache::thrift::transport::TTransport> transport)
{
    std::stringstream ret;
    /* What we get is TFramedTransport.  We will extract TSocket from it */
    boost::shared_ptr<tt::TFramedTransport> buf_transport =
        boost::static_pointer_cast<tt::TFramedTransport>(transport);

    boost::shared_ptr<tt::TSocket> sock =
        boost::static_pointer_cast<tt::TSocket>(
            buf_transport->getUnderlyingTransport());

    ret << sock->getPeerAddress() << ":" << sock->getPeerPort();
    return ret.str();
}

SvcServer::SvcServer(int port, fpi::PlatNetSvcProcessorPtr processor)
{
    port_ = port;
    boost::shared_ptr<tp::TProtocolFactory>  proto(new tp::TBinaryProtocolFactory());
    serverTransport_.reset(new tt::TServerSocket(port));
    boost::shared_ptr<tt::TTransportFactory> tfact(new tt::TFramedTransportFactory());

    server_ = boost::shared_ptr<ts::TThreadedServer>(
        new ts::TThreadedServer(processor, serverTransport_, tfact, proto));
    stopped_ = false;
}

SvcServer::~SvcServer()
{
    GLOGNOTIFY << logString();
}

void SvcServer::start()
{
    GLOGNOTIFY << logString();
    server_->setServerEventHandler(shared_from_this());
    serverThread_.reset(new std::thread([this] {this->serve_();}));
}

void SvcServer::stop()
{
    GLOGNOTIFY << logString();

    /* Set stopped_ to true, so that any new connection/context creation after this flag is set
     * to true, we can close the connection.
     */
    stopped_ = true;

    /* Stop the server so that we will not accept any new connections */
    server_->stop();

#if 0
    /* NOTE: Commented.  Uncomment if needed */
    /* Thi sleep is introduced so that any race with stop() and new connection context created
     * see TThreadedServer::tasks_, gets settled(not certain).  After this sleep any new context
     * created should be both in connections_ and TThreadedServer::tasks_.  Without this sleep
     * connections context may be in TThreadedServer::tasks_ but not in connections_.  It would
     * have been ideal if we had access to TThreadedServer::tasks_, but it's made private
     * implementation.
     */
    sleep(1);
#endif

    closeClientConnections_();
    /* Not sure if the second stop is really needed */
    server_->stop();
    serverTransport_->close();
    /* Do a join on the thread */
    serverThread_->join();
}

void SvcServer::closeClientConnections_()
{
    /* Disconnect all the connected clients so that server can be shutdown */
    std::set<tt::TTransportPtr, LtTransports> tempConnections;
    {
        /*
         * NOTE:  Below we will invoke close connection on all connections
         * in connections_ member which invokes deleteContext which
         * removes the connection from connections_.  To not cause a deadlock
         * we will copy connection_ and invoke connection->close() without holding
         * lock
         */
        fds_mutex::scoped_lock l(connectionsLock_);
        tempConnections = connections_;
    }

    /* Close all the connections. */
    for (auto itr : tempConnections) {
        itr->close();
    }

    /* Wait until all connections are closed.  NOTE, not lockng. Should be ok */
    int slept_time = 0;
    while (connections_.size() > 0 &&
           slept_time < 1000) {
        usleep(10);
        slept_time += 10;
    }
    if (connections_.size() > 0) {
        GLOGNOTIFY << "We still have " << connections_.size() << " transports open";
    }
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

void* SvcServer::createContext(
    boost::shared_ptr<tp::TProtocol> input,
    boost::shared_ptr<tp::TProtocol> output)
{
    auto transport = input->getTransport();

    if (stopped_) {
        /* Once stopped_ is set to true, we will not cache connections.  We will close
         * them immediately, so that join() call in TThreadedServer::stop() will return.
         */
        transport->close();
        return nullptr;;
    }

    {
        fds_scoped_lock l(connectionsLock_);
        connections_.insert(transport);
    }
    LOGDEBUG << "New connection: " << getTransportKey(transport);

    return nullptr;
}

void SvcServer::deleteContext(void* serverContext,
                   boost::shared_ptr<tp::TProtocol>input,
                   boost::shared_ptr<tp::TProtocol>output)
{
    auto transport = input->getTransport();
    {
        fds_scoped_lock l(connectionsLock_);
        connections_.erase(transport);
    }
    LOGDEBUG << "Removing connection: " << getTransportKey(transport);
}

void SvcServer::handlerError(void* ctx, const char* fn_name)
{
    LOGCRITICAL << "TProcessor error at: " << fn_name;
}

std::string SvcServer::logString() const
{
    std::stringstream ss;
    ss << "Server at port: " << port_;
    return ss.str();
}

} // namespace fds
