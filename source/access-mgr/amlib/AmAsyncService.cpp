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
#include <responsehandler.h>

namespace fds {

AsyncDataServer::AsyncDataServer(const std::string &name,
                                 fds_uint32_t instanceId)
        : Module(name.c_str()),
          port(8899 + instanceId),
          numServerThreads(10) {
    serverTransport.reset(new xdi_att::TServerSocket(port));
    transportFactory.reset(new xdi_att::TFramedTransportFactory());
    protocolFactory.reset(new xdi_atp::TBinaryProtocolFactory());

    FdsConfigAccessor conf(g_fdsprocess->get_fds_config(), "fds.am.");
    numServerThreads = conf.get<fds_uint32_t>("async_server_threads");
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
    // Setup thrift threads
    threadManager = xdi_atc::ThreadManager::newSimpleThreadManager(numServerThreads);
    threadFactory.reset(new xdi_atc::PosixThreadFactory());
    threadManager->threadFactory(threadFactory);

    // Setup API processor
    processor.reset(new apis::AsyncAmServiceRequestProcessor(
        boost::make_shared<AmAsyncDataApi>()));
    // event_handler_.reset(new ServerEventHandler(*this));

    // nbServer.reset(new xdi_ats::TNonblockingServer(
    // processor, protocolFactory, port, threadManager));
    ttServer.reset(new xdi_ats::TThreadedServer(processor,
                                                serverTransport,
                                                transportFactory,
                                                protocolFactory));

    try {
        LOGNORMAL << "Starting the async data server with " << numServerThreads
                  << " server threads at port " << port;
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
    ttServer->stop();
    listen_thread->join();
    listen_thread.reset();
}
}  // namespace fds
