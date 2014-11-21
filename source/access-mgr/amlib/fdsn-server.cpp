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
#include <fdsn-server.h>
#include <responsehandler.h>
#include <StorHvisorNet.h>

namespace fds {

FdsnServer::FdsnServer(const std::string &name,
                       AmDataApi::shared_ptr &_dataApi,
                       fds_uint32_t instanceId)
        : Module(name.c_str()),
          port(9988 + instanceId),
          dataApi(_dataApi),
          numFdsnThreads(10) {
    serverTransport.reset(new xdi_att::TServerSocket(port));
    transportFactory.reset(new xdi_att::TBufferedTransportFactory());
    protocolFactory.reset(new xdi_atp::TBinaryProtocolFactory());

    FdsConfigAccessor conf(g_fdsprocess->get_fds_config(), "fds.am.");
    numFdsnThreads = conf.get<fds_uint32_t>("fdsn_server_threads");

    // server_->setServerEventHandler(event_handler_);
}

/**
 * Module initialization
 */
int
FdsnServer::mod_init(SysParams const *const p) {
    Module::mod_init(p);
    return 0;
}

/**
 * Module startup
 */
void
FdsnServer::mod_startup() {
}

/**
 * Module shutdown
 */
void
FdsnServer::mod_shutdown() {
}

/**
 * Initializes the server component
 */
void
FdsnServer::init_server() {
    // Setup thrift threads
    threadManager = xdi_atc::ThreadManager::newSimpleThreadManager(numFdsnThreads);
    threadFactory.reset(new xdi_atc::PosixThreadFactory());
    threadManager->threadFactory(threadFactory);
    threadManager->start();

    // Setup API processor
    processor.reset(new apis::AmServiceProcessor(
        dataApi));
    // event_handler_.reset(new ServerEventHandler(*this));

    nbServer.reset(new xdi_ats::TNonblockingServer(
        processor, protocolFactory, port, threadManager));
    // server.reset(new xdi_ats::TThreadedServer(processor,
    //                                       serverTransport,
    //                                        transportFactory,
    //                                        protocolFactory));

    try {
        LOGNORMAL << "Starting the FDSN server with " << numFdsnThreads
                  << " threads at port " << port;
        listen_thread.reset(new boost::thread(&xdi_ats::TNonblockingServer::serve,
                                              nbServer.get()));
        // listen_thread.reset(new boost::thread(&xdi_ats::TThreadedServer::serve,
        //                                   server.get()));
    } catch(const xdi_att::TTransportException& e) {
        LOGERROR << "unable to start FDSN server : " << e.what();
        fds_panic("Unable to start FDSN server...bailing out");
    }
}

void
FdsnServer::deinit_server() {
    fds_verify(listen_thread != NULL);
    listen_thread->join();
}
}  // namespace fds
