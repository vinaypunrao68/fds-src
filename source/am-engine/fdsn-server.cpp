/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <string>

#include <am-engine/fdsn-server.h>

namespace fds {

FdsnServer gl_FdsnServer("Global FDSN Server");

FdsnServer::FdsnServer(const std::string &name)
        : Module(name.c_str()),
          port(9988) {
    serverTransport.reset(new xdi_att::TServerSocket(port));
    transportFactory.reset(new xdi_att::TBufferedTransportFactory());
    protocolFactory.reset(new xdi_atp::TBinaryProtocolFactory());
    processor.reset(new xdi::AmShimProcessor(
        boost::shared_ptr<FdsnIf>(
            new FdsnIf())));
    // event_handler_.reset(new ServerEventHandler(*this));
    server.reset(new xdi_ats::TThreadedServer(processor,
                                              serverTransport,
                                              transportFactory,
                                              protocolFactory));

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
FdsnServer::init_server(FDS_NativeAPI *api) {
    fds_verify(api != NULL);
    eng_api = api;

    try {
        LOGNORMAL << "Starting the FDSN server...";
        listen_thread.reset(new boost::thread(&xdi_ats::TThreadedServer::serve,
                                              server.get()));
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
