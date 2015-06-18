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
#include "connector/xdi/fdsn-server.h"
#include <responsehandler.h>

namespace fds {

FdsnServer::FdsnServer(AmDataApi::shared_ptr &_dataApi,
                       fds_uint32_t pmPort)
        : dataApi(_dataApi),
          numFdsnThreads(10) {
    FdsConfigAccessor conf(g_fdsprocess->get_fds_config(), "fds.am.");
    numFdsnThreads = conf.get<fds_uint32_t>("fdsn_server_threads");
    int amServicePortOffset = conf.get<int>("am_service_port_offset");
    port = pmPort + amServicePortOffset;

    serverTransport.reset(new xdi_att::TServerSocket(port));
    transportFactory.reset(new xdi_att::TBufferedTransportFactory());
    protocolFactory.reset(new xdi_atp::TBinaryProtocolFactory());
}

/**
 * Initializes the server component
 */
void
FdsnServer::start() {
    // Setup thrift threads
    threadManager = xdi_atc::ThreadManager::newSimpleThreadManager(numFdsnThreads);
    threadFactory.reset(new xdi_atc::PosixThreadFactory());
    threadManager->threadFactory(threadFactory);
    threadManager->start();

    // Setup API processor
    processor.reset(new apis::XdiServiceProcessor(dataApi));

    nbServer.reset(new xdi_ats::TNonblockingServer(
                        processor, protocolFactory, port, threadManager));

    try {
        LOGNORMAL << "Starting the FDSN server with " << numFdsnThreads
                  << " threads at port " << port;
        listen_thread.reset(new std::thread(&xdi_ats::TNonblockingServer::serve,
                                            nbServer.get()));
    } catch(const xdi_att::TTransportException& e) {
        LOGERROR << "unable to start FDSN server : " << e.what();
        fds_panic("Unable to start FDSN server...bailing out");
    }
}

void
FdsnServer::stop() {
    nbServer->stop();
    if (listen_thread) {
        listen_thread->join();
        listen_thread.reset();
    }
}
}  // namespace fds
