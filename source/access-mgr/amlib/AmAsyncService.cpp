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
#include <handlermappings.h>
#include <responsehandler.h>
#include <StorHvisorNet.h>

namespace fds {

AsyncDataServer::AsyncDataServer(const std::string &name,
                                 AmAsyncDataApi::shared_ptr &_dataApi)
        : Module(name.c_str()),
          port(8899),
          asyncDataApi(_dataApi),
          numServerThreads(10) {
    serverTransport.reset(new xdi_att::TServerSocket(port));
    // serverTransport.reset(new xdi_att::TServerSocket("/tmp/am-req-sock"));
    transportFactory.reset(new xdi_att::TFramedTransportFactory());
    protocolFactory.reset(new xdi_atp::TBinaryProtocolFactory());

    FdsConfigAccessor conf(g_fdsprocess->get_fds_config(), "fds.am.");
    numServerThreads = conf.get<fds_uint32_t>("async_server_threads");

    // server_->setServerEventHandler(event_handler_);
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
    threadManager->start();

    // Setup API processor
    processor.reset(new apis::AsyncAmServiceRequestProcessor(
        asyncDataApi));
    // event_handler_.reset(new ServerEventHandler(*this));

    // nbServer.reset(new xdi_ats::TNonblockingServer(
    // processor, protocolFactory, port, threadManager));
    ttServer.reset(new xdi_ats::TThreadedServer(processor,
                                                serverTransport,
                                                transportFactory,
                                                protocolFactory));

    try {
        LOGNORMAL << "Starting the async data server with " << numServerThreads
                  << " server threads...";
        // listen_thread.reset(new boost::thread(&xdi_ats::TNonblockingServer::serve,
        //                                   nbServer.get()));
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
}
}  // namespace fds
