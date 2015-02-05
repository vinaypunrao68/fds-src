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

#include <AmAsyncService.h>
#include "AmAsyncXdi.h"
#include "AmAsyncDataApi_impl.h"
#include <responsehandler.h>

namespace fds {


class AsyncAmServiceRequestIfCloneFactory
    : virtual public apis::AsyncAmServiceRequestIfFactory
{
    using request_if = apis::AsyncAmServiceRequestIf;
 public:
    AsyncAmServiceRequestIfCloneFactory() {}
    ~AsyncAmServiceRequestIfCloneFactory() {}

    request_if* getHandler(const xdi_at::TConnectionInfo& connInfo);
    void releaseHandler(request_if* handler);
};

AsyncAmServiceRequestIfCloneFactory::request_if*
AsyncAmServiceRequestIfCloneFactory::getHandler(const xdi_at::TConnectionInfo& connInfo) {  // NOLINT
    // Get the underlying transport's socket so we can see what the host IP
    // address was of the incoming client.
    boost::shared_ptr<xdi_att::TSocket> sock =
        boost::dynamic_pointer_cast<xdi_att::TSocket>(connInfo.transport);
    fds_assert(sock.get());
    return new AmAsyncXdiRequest(boost::make_shared<AmAsyncXdiResponse>(sock->getPeerAddress()));
}

void
AsyncAmServiceRequestIfCloneFactory::releaseHandler(request_if* handler) {
    delete handler;
}

AsyncDataServer::AsyncDataServer(const std::string &name, fds_uint32_t instanceId)
: Module(name.c_str()),
    port(8899 + instanceId)
{
    serverTransport.reset(new xdi_att::TServerSocket(port));
    transportFactory.reset(new xdi_att::TFramedTransportFactory());
    protocolFactory.reset(new xdi_atp::TBinaryProtocolFactory());

    FdsConfigAccessor conf(g_fdsprocess->get_fds_config(), "fds.am.");
}

/**
 * Module initialization
 */
int
AsyncDataServer::mod_init(SysParams const *const p) {
    Module::mod_init(p);
    return 0;
}

/**
 * Module startup
 */
void
AsyncDataServer::mod_startup() {
}

/**
 * Module shutdown
 */
void
AsyncDataServer::mod_shutdown() {
}

/**
 * Initializes the server component
 */
void
AsyncDataServer::init_server() {
    // Setup API processor
    processorFactory.reset(new apis::AsyncAmServiceRequestProcessorFactory(
            boost::make_shared<AsyncAmServiceRequestIfCloneFactory>() ));

    // processor, protocolFactory, port, threadManager));
    ttServer.reset(new xdi_ats::TThreadedServer(processorFactory,
                                                serverTransport,
                                                transportFactory,
                                                protocolFactory));

    try {
        LOGNORMAL << "Starting the async data server at port " << port;
        listen_thread.reset(new boost::thread(&xdi_ats::TThreadedServer::serve,
                                              ttServer.get()));
    } catch(const xdi_att::TTransportException& e) {
        LOGERROR << "unable to start async data server : " << e.what();
        fds_panic("Unable to start async data server...bailing out");
    }
}

void
AsyncDataServer::deinit_server() {
    fds_verify(listen_thread != NULL);
    listen_thread->join();
    listen_thread.reset();
}
}  // namespace fds
