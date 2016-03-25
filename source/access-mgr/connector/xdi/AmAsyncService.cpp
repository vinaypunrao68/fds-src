/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <map>
#include <unordered_map>
#include <utility>
#include <string>
#include <vector>

#include <FdsCrypto.h>
#include <fds_uuid.h>
#include <concurrency/Mutex.h>
#include <fds_process.h>

#include "AmProcessor.h"

#include "connector/xdi/AmAsyncService.h"
#include "connector/xdi/AmAsyncXdi.h"
#include "connector/xdi/XdiRestfulInterface.h"

namespace fds {

using ip_list = std::unordered_set<std::string>;

struct AmProcessor;

class AsyncAmServiceRequestIfCloneFactory
    : public apis::AsyncXdiServiceRequestIfFactory
{
    using request_if = apis::AsyncXdiServiceRequestIf;
    std::weak_ptr<AmProcessor> processor;
 public:

    explicit AsyncAmServiceRequestIfCloneFactory(std::weak_ptr<AmProcessor> _processor) :
        processor(_processor)
    { }

    ~AsyncAmServiceRequestIfCloneFactory() = default;

    request_if* getHandler(const xdi_at::TConnectionInfo& connInfo);
    void releaseHandler(request_if* handler);

    void getIps(ip_list& ip);
 private:
    // Stores a list of IPs of currently connected XDIs
    std::map<request_if*, std::string>       _connectedXdi;
};

AsyncAmServiceRequestIfCloneFactory::request_if*
AsyncAmServiceRequestIfCloneFactory::getHandler(const xdi_at::TConnectionInfo& connInfo) {  // NOLINT
    /** First see if we even have a processing layer */
    auto amProcessor = processor.lock();
    if (!amProcessor) {
        LOGNORMAL << "no processing layer";
        return nullptr;
    }

    // Get the underlying transport's socket so we can see what the host IP
    // address was of the incoming client.
    boost::shared_ptr<xdi_att::TSocket> sock =
        boost::dynamic_pointer_cast<xdi_att::TSocket>(connInfo.transport);
    fds_assert(sock.get());
    LOGNORMAL << "peer:" << sock->getPeerAddress() << " asynchronous Xdi connection";
    request_if* ri = new AmAsyncXdiRequest(amProcessor,
            boost::make_shared<AmAsyncXdiResponse>(sock->getPeerAddress()));
    _connectedXdi.insert(std::make_pair(ri, sock->getPeerAddress()));
    return ri;
}

void
AsyncAmServiceRequestIfCloneFactory::releaseHandler(request_if* handler) {
    _connectedXdi.erase(handler);
    delete handler;
}

void AsyncAmServiceRequestIfCloneFactory::getIps(std::unordered_set<std::string>& ip) {
    // loop through all the handles and by inserting into an unordered_set we will
    // get a list of all the unique IPs from the map.
    for (auto const& i : _connectedXdi) {
        ip.insert(i.second);
    }
}

AsyncDataServer::AsyncDataServer(std::weak_ptr<AmProcessor> processor,
                                 fds_uint32_t pmPort) : _processor(processor), _pmPort(pmPort)
{
    FdsConfigAccessor conf(g_fdsprocess->get_fds_config(), "fds.am.");
    int xdiServicePortOffset = conf.get<int>("xdi_service_port_offset");
    port = pmPort + xdiServicePortOffset;

    serverTransport.reset(new xdi_att::TServerSocket(port));
    transportFactory.reset(new xdi_att::TFramedTransportFactory());
    protocolFactory.reset(new xdi_atp::TBinaryProtocolFactory());
    // Setup API processor
    cloneFactory = boost::make_shared<AsyncAmServiceRequestIfCloneFactory>(processor);
    processorFactory.reset(new apis::AsyncXdiServiceRequestProcessorFactory(cloneFactory));

    // Timeout for flush request to xdi in ms
    XdiRestfulInterface::TIMEOUT = conf.get_abs<uint32_t>(
        "fds.am.svc.timeout.coordinator_switch", XdiRestfulInterface::TIMEOUT);
}

/**
 * Start serving any clients
 */
void
AsyncDataServer::start() {
    ttServer.reset(new xdi_ats::TThreadedServer(processorFactory,
                                                serverTransport,
                                                transportFactory,
                                                protocolFactory));

    try {
        LOGNORMAL << "port:" << port << " starting async data server";
        listen_thread.reset(new std::thread(&xdi_ats::TThreadedServer::serve,
                                            ttServer.get()));
    } catch(const xdi_att::TTransportException& e) {
        LOGERROR << "unable to start async data server:" << e.what();
        fds_panic("Unable to start async data server...bailing out");
    }
}

void
AsyncDataServer::stop() {
    ttServer->stop();
    if (listen_thread) {
        listen_thread->join();
        listen_thread.reset();
    }
}

void
AsyncDataServer::flushVolume(AmRequest* req, std::string const& vol) {
    ip_list ip;
    cloneFactory->getIps(ip);

    FdsConfigAccessor conf(g_fdsprocess->get_fds_config(), "fds.xdi.");
    int xdiControlPortOffset = conf.get<int>("control_port_offset");
    int xdiRestfulPort = _pmPort + xdiControlPortOffset;

    for (auto const& i : ip) {
        LOGDEBUG << "vol:" << vol
                 << " ip:" << i
                 << " port:" << xdiRestfulPort
                 << " flushing volume";
        auto x = XdiRestfulInterface(i, xdiRestfulPort);
        x.flushVolume(vol);
    }

    auto amProcessor = _processor.lock();
    if (!amProcessor) {
        LOGNORMAL << "vol:" << vol << " no processing layer, unable to flush";
        return;
    }
    amProcessor->enqueueRequest(req);
}
}  // namespace fds
