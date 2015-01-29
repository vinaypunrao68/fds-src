/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <string>

#include "fds_process.h"

#include "am-platform.h"

#include "AmCache.h"
#include "AccessMgr.h"
#include "StorHvCtrl.h"
#include "StorHvQosCtrl.h"
#include "StorHvVolumes.h"

extern AmPlatform gl_AmPlatform;

namespace fds {

AccessMgr::AccessMgr(const std::string &modName,
                     CommonModuleProviderIf *modProvider)
        : Module(modName.c_str()),
          modProvider_(modProvider),
          shuttingDown(false) {
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
    FdsConfigAccessor conf(g_fdsprocess->get_fds_config(), "fds.am.");
    instanceId = conf.get<fds_uint32_t>("instanceId");
    LOGNOTIFY << "Initializing AM instance " << instanceId;

    int argc = 0;
    char **argv = NULL;
    fds::ModuleVector io_dm(argc, argv, io_dm_vec);
    storHvisor = new StorHvCtrl(argc, argv, io_dm.get_sys_params(),
                                StorHvCtrl::NORMAL, instanceId);
    dataApi = boost::make_shared<AmDataApi>();

    // Init the FDSN server to serve XDI data requests
    fdsnServer = FdsnServer::unique_ptr(
        new FdsnServer("AM FDSN Server", dataApi, instanceId));
    fdsnServer->init_server();

    // Init the async server
    asyncServer = AsyncDataServer::unique_ptr(
        new AsyncDataServer("AM Async Server", instanceId));
    asyncServer->init_server();

    if (!conf.get<bool>("testing.toggleStandAlone")) {
        omConfigApi = boost::make_shared<OmConfigApi>();
    }

    blkConnector = boost::shared_ptr<NbdConnector>(
        boost::make_shared<NbdConnector>(omConfigApi));

    // Update the AM's platform with our instance ID so that
    // common fields (e.g., ports) can be updated
    gl_AmPlatform.setInstanceId(instanceId);
    return 0;
}

void
AccessMgr::mod_startup() {
}

void
AccessMgr::mod_shutdown() {
    delete storHvisor;
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
    asyncServer->deinit_server();
}

Error
AccessMgr::registerVolume(const VolumeDesc& volDesc) {
    // TODO(Andrew): Create cache separately since
    // the volume data doesn't do it. We should converge
    // on a single volume add location.
    storHvisor->amCache->createCache(volDesc);
    return storHvisor->vol_table->registerVolume(volDesc);
}

// Set AM in shutdown mode.
void
AccessMgr::setShutDown() {
    shuttingDown = true;
}

// Check whether AM is in shutdown mode.
bool
AccessMgr::isShuttingDown() {
    return shuttingDown;
}

/// Allow queues to drain and outstanding requests to complete.
void
AccessMgr::prepareToShutDown() {

}

}  // namespace fds
