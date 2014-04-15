/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <string>

#include <am-engine/xdi-server.h>

namespace fds {

XdiServer gl_XdiServer("Global XDI Server");

XdiServer::XdiServer(const std::string &name)
        : Module(name.c_str()),
          port(9988) {
    serverTransport.reset(new xdi_att::TServerSocket(port));
    transportFactory.reset(new xdi_att::TBufferedTransportFactory());
    protocolFactory.reset(new xdi_atp::TBinaryProtocolFactory());
    processor.reset(new xdi::AmShimProcessor(
        boost::shared_ptr<XdiIf>(
            new XdiIf())));
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
XdiServer::mod_init(SysParams const *const p) {
    Module::mod_init(p);
    return 0;
}

/**
 * Module startup
 */
void
XdiServer::mod_startup() {
}

/**
 * Module shutdown
 */
void
XdiServer::mod_shutdown() {
}

/**
 * Initializes the server component
 */
void
XdiServer::init_server(FDS_NativeAPI *api) {
    fds_verify(api != NULL);
    eng_api = api;

    try {
        LOGNORMAL << "Starting the xdi server...";
        listen_thread.reset(new boost::thread(&xdi_ats::TThreadedServer::serve,
                                              server.get()));
      } catch(const xdi_att::TTransportException& e) {
          LOGERROR << "unable to start server : " << e.what();
          fds_panic("Unable to start a server...bailing out");
      }
}

void
XdiServer::deinit_server() {
    fds_verify(listen_thread != NULL);
    listen_thread->join();
}

}  // namespace fds
