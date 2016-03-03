/*
 * Copyright 2015 by Formation Data Systems, Inc.
 */
#include <arpa/inet.h>
#include <util/Log.h>
#include <fds_assert.h>
#include <thrift/server/TThreadedServer.h>
#include <thrift/transport/TBufferTransports.h>
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/TSocket.h>
#include <thrift/transport/TServerSocket.h>
#include <net/SvcServer.h>
#include <fds_error.h>
#include "fdsp/common_constants.h"

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

SvcServer::SvcServer(int port, boost::shared_ptr<at::TProcessor> currentProcessor,
    const std::string &strThriftServiceName, CommonModuleProviderIf *moduleProvider)
{
    port_ = port;
    boost::shared_ptr<tp::TProtocolFactory>  proto(new tp::TBinaryProtocolFactory());
    serverTransport_.reset(new tt::TServerSocket(port));
    boost::shared_ptr<tt::TTransportFactory> tfact(new tt::TFramedTransportFactory());

    /**
     * FEATURE TOGGLE: enable multiplexed services
     * Tue Feb 23 15:04:17 MST 2016
     */
    bool enableMultiplexedServices = false;
    if (moduleProvider) {
        // The module provider might not supply the base path we want,
        // so make our own config access. This is libConfig data, not
        // to be confused with FDS configuration (platform.conf).
        FdsConfigAccessor configAccess(moduleProvider->get_fds_config(), "fds.feature_toggle.");
        enableMultiplexedServices = configAccess.get<bool>("common.enable_multiplexed_services", false);
    }
    if (enableMultiplexedServices) {
        // Note on Thrift service compatibility:
        // If a backward incompatible change arises, pass additional pairs of
        // processor and Thrift service name to SvcProcess::init(). Similarly,
        // if the Thrift service API wants to be broken up.
        processor_.reset(new ::apache::thrift::TMultiplexedProcessor());
        processor_->registerProcessor(strThriftServiceName, currentProcessor);
        processor_->registerProcessor(fpi::commonConstants().PLATNET_SERVICE_NAME, currentProcessor);
        server_ = boost::shared_ptr<ts::TThreadedServer>(
            new ts::TThreadedServer(processor_, serverTransport_, tfact, proto));
    } else {
        server_ = boost::shared_ptr<ts::TThreadedServer>(
            new ts::TThreadedServer(currentProcessor, serverTransport_, tfact, proto));
    }
    stopped_ = true;
}

SvcServer::~SvcServer()
{
    GLOGNOTIFY << logString();
}

void SvcServer::setListener(SvcServerListener *listener)
{
    listener_ = listener;
}

Error SvcServer::start()
{
    GLOGNOTIFY << logString();
    fds_verify(stopped_ == true);

    stopped_ = false;
    server_->setServerEventHandler(shared_from_this());
    serverThread_.reset(new std::thread([this] {this->serve_();}));

    /* Wait until server starts listening i.e preserve() callback is invoked form thrift */
    startWaiter_.await();
    return startWaiter_.status;
}

void SvcServer::stop()
{
    GLOGNOTIFY << logString();

    if (stopped_) {
        GLOGWARN << "Server is already stopped..ignoring";
        return;
    }

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
    Error e = ERR_OK;

    GLOGNOTIFY << logString();

    try {
        server_->serve();
    } catch (std::exception &ex) {
        GLOGWARN << "Stopping " << logString() << " Exception: " << ex.what();
        e = ERR_SVC_SERVER_CRASH;
    } catch (...) {
        GLOGWARN << "Stopping " << logString() << " unknown exception";
        e = ERR_SVC_SERVER_CRASH;
    }

    if (startWaiter_.getNumTasks() > 0) {
        /* This typically happens when TServer::listen() throws and exception.
         * We returned from serve() even before waiting in start() finished.  If we are here
         * because server crashed there is no need to notify the listener as waiting for start
         * hans't finished yet.
         * now we will notify with an error
         */
        startWaiter_.status = e;
        startWaiter_.done();
    } else if (e != ERR_OK) {
        listener_->notifyServerDown(ERR_SVC_SERVER_CRASH);
    }
}

void SvcServer::preServe() {
    fds_assert(startWaiter_.getNumTasks() == 1);
    startWaiter_.status = ERR_OK;
    startWaiter_.done();
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
