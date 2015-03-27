/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <string>

#include "fds_process.h"

#include "AccessMgr.h"
#include "AmProcessor.h"
#include <net/SvcMgr.h>
#include <AmDataApi.h>
#include <OmConfigService.h>

#include "connector/xdi/AmAsyncService.h"
#include "connector/xdi/fdsn-server.h"
#include "connector/block/NbdConnector.h"

namespace fds {

AccessMgr::AccessMgr(const std::string &modName,
                     CommonModuleProviderIf *modProvider)
        : Module(modName.c_str()),
          modProvider_(modProvider),
          shuttingDown(false),
          amProcessor(std::make_shared<fds::AmProcessor>("AM Processor Module",
                                                       [this]() mutable { this->stop(); }))
{
}

AccessMgr::~AccessMgr() {
}

int
AccessMgr::mod_init(SysParams const *const param) {
    Module::mod_init(param);

    FdsConfigAccessor conf(g_fdsprocess->get_fds_config(), "fds.am.");
    auto standalone_mode = conf.get<bool>("testing.standalone");

    /**
     * Initialize the old synchronous Xdi interface
     */
    dataApi = boost::make_shared<AmDataApi>(amProcessor);
    fds_uint32_t pmPort = g_fdsprocess->get_fds_config()->get<int>(
        "fds.pm.platform_port");
    if (!standalone_mode) {
        pmPort = modProvider_->getSvcMgr()->getMappedSelfPlatformPort();
    }
    LOGTRACE << "Platform port " << pmPort;

    // Init the FDSN server to serve XDI data requests
    fdsnServer = FdsnServer::unique_ptr(new FdsnServer("AM FDSN Server", dataApi, pmPort));
    fdsnServer->init_server();

    /**
     * Initialize the async server
     */
    auto weakProcessor = std::weak_ptr<AmProcessor>(amProcessor);
    asyncServer = AsyncDataServer::unique_ptr(new AsyncDataServer("AM Async Server", pmPort));
    asyncServer->init_server(weakProcessor);

    if (!standalone_mode) {
        omConfigApi = boost::make_shared<OmConfigApi>();
    }

    /**
     * Initialize the block connector
     */
    blkConnector = std::unique_ptr<NbdConnector>(new NbdConnector(omConfigApi, weakProcessor));

    return 0;
}

void
AccessMgr::mod_startup() {
}

void
AccessMgr::mod_shutdown() {
    amProcessor->mod_shutdown();
}

void AccessMgr::mod_enable_service()
{
    LOGNOTIFY << "Enbaling services ";
    amProcessor->mod_startup();
}

void
AccessMgr::run() {
    // Run until the data server stops
    fdsnServer->deinit_server();
    asyncServer->deinit_server();
}

void
AccessMgr::stop() {
    LOGDEBUG << "Shutting down and no outstanding I/O's. Stop dispatcher and server.";
    asyncServer->getTTServer()->stop();
    fdsnServer->getNBServer()->stop();
}

}  // namespace fds
