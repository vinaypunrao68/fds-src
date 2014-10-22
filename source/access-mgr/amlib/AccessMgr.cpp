/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <string>
#include <AccessMgr.h>

extern StorHvCtrl *storHvisor;

namespace fds {

AccessMgr::AccessMgr(const std::string &modName,
                     CommonModuleProviderIf *modProvider)
        : Module(modName.c_str()),
          modProvider_(modProvider) {
}

AccessMgr::~AccessMgr() {
}

int
AccessMgr::mod_init(SysParams const *const param) {
    Module::mod_init(param);

    // Init the storHvisor global object. It takes a bunch of arguments
    // but doesn't really need them so we just create stock values.
    fds::Module *io_dm_vec[] = {
        nullptr
    };
    int argc = 0;
    char **argv = NULL;
    fds::ModuleVector io_dm(argc, argv, io_dm_vec);
    storHvisor = new StorHvCtrl(argc, argv, io_dm.get_sys_params(),
                                StorHvCtrl::NORMAL);

    dataApi = boost::make_shared<AmDataApi>();

    // Init the FDSN server to serve XDI data requests
    fdsnServer = FdsnServer::unique_ptr(new FdsnServer("AM FDSN Server", dataApi));
    fdsnServer->init_server();
    return 0;
}

void
AccessMgr::mod_startup() {
}

void
AccessMgr::mod_shutdown() {
}

void
AccessMgr::mod_lockstep_start_service() {
    storHvisor->StartOmClient();
    storHvisor->qos_ctrl->runScheduler();
    this->mod_lockstep_done();
}

void
AccessMgr::run() {
    // Run until the data server stops
    fdsnServer->deinit_server();
}

Error
AccessMgr::registerVolume(const VolumeDesc& volDesc) {
    return storHvisor->vol_table->registerVolume(volDesc);
}

}  // namespace fds
