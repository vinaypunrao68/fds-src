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

#include "connector/xdi/AmAsyncService.h"
#include "connector/xdi/AmAsyncXdi.h"

namespace fds {

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
};

AsyncAmServiceRequestIfCloneFactory::request_if*
AsyncAmServiceRequestIfCloneFactory::getHandler(const xdi_at::TConnectionInfo& connInfo) {  // NOLINT
    /** First see if we even have a processing layer */
    auto amProcessor = processor.lock();
    if (!amProcessor) {
        LOGNORMAL << "No processing layer, shutdown.";
        return nullptr;
    }

    // Get the underlying transport's socket so we can see what the host IP
    // address was of the incoming client.
    boost::shared_ptr<xdi_att::TSocket> sock =
        boost::dynamic_pointer_cast<xdi_att::TSocket>(connInfo.transport);
    fds_assert(sock.get());
    LOGNORMAL << "Asynchronous Xdi connection being made from: " << sock->getPeerAddress();
    return new AmAsyncXdiRequest(amProcessor,
                                 boost::make_shared<AmAsyncXdiResponse>(sock->getPeerAddress()));
}

void
AsyncAmServiceRequestIfCloneFactory::releaseHandler(request_if* handler) {
    delete handler;
}

AsyncDataServer::AsyncDataServer(std::weak_ptr<AmProcessor> processor,
                                 fds_uint32_t pmPort)
{
    FdsConfigAccessor conf(g_fdsprocess->get_fds_config(), "fds.am.");
    int xdiServicePortOffset = conf.get<int>("xdi_service_port_offset");
    port = pmPort + xdiServicePortOffset;

    serverTransport.reset(new xdi_att::TServerSocket(port));
    transportFactory.reset(new xdi_att::TFramedTransportFactory());
    protocolFactory.reset(new xdi_atp::TBinaryProtocolFactory());
    // Setup API processor
    processorFactory.reset(new apis::AsyncXdiServiceRequestProcessorFactory(
            boost::make_shared<AsyncAmServiceRequestIfCloneFactory>(processor) ));
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
        LOGNORMAL << "Starting the async data server at port " << port;
        listen_thread.reset(new std::thread(&xdi_ats::TThreadedServer::serve,
                                            ttServer.get()));
    } catch(const xdi_att::TTransportException& e) {
        LOGERROR << "unable to start async data server : " << e.what();
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
}  // namespace fds
