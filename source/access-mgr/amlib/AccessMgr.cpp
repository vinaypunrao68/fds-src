/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <string>

#include "fds_process.h"

#include "AccessMgr.h"
#include "AmProcessor.h"
#include <net/SvcMgr.h>

#include "connector/xdi/AmAsyncService.h"
#include "connector/block/NbdConnector.h"

namespace fds {

AccessMgr::AccessMgr(const std::string &modName,
                     CommonModuleProviderIf *modProvider)
        : Module(modName.c_str()),
          modProvider_(modProvider),
          stop_signal(),
          shutting_down(false),
          amProcessor(std::make_shared<fds::AmProcessor>(this, modProvider)),
          standalone_mode{false}
{
}

AccessMgr::~AccessMgr() = default;

int
AccessMgr::mod_init(SysParams const *const param) {
    FdsConfigAccessor conf(g_fdsprocess->get_fds_config(), "fds.am.");
    standalone_mode = conf.get<bool>("testing.standalone");
    return Module::mod_init(param);
}


void
AccessMgr::mod_startup() {
    LOGNOTIFY << "Starting processing layer ";
    amProcessor->start();
}

void
AccessMgr::mod_shutdown() {
    std::unique_lock<std::mutex> lk {stop_lock};
    shutting_down = true;
    stop_signal.notify_one();
}

void AccessMgr::mod_enable_service()
{
    /**
     * Before being able to serve I/O requests, must first pull DMT
     * and DLT information. At this time, we've already done the registration
     * with the OM so anything here is post-registration.
     */
    if (!asyncServer && amProcessor->haveTables()) {
        LOGNOTIFY << "Enabling services ";
        initilizeConnectors();
    }
}

void AccessMgr::initilizeConnectors() {
    auto weakProcessor = std::weak_ptr<AmProcessor>(amProcessor);

    fds_uint32_t pmPort = g_fdsprocess->get_fds_config()->get<int>(
        "fds.pm.platform_port");
    if (!standalone_mode) {
        pmPort = modProvider_->getSvcMgr()->getMappedSelfPlatformPort();
    }
    LOGTRACE << "Platform port " << pmPort;

    /**
     * Initialize the async server
     */
    asyncServer.reset(new AsyncDataServer(weakProcessor, pmPort));
    asyncServer->start();

    /**
     * Initialize the block connector
     */
    NbdConnector::start(weakProcessor);
}

void AccessMgr::mod_disable_service() {
    amProcessor->stop();
}

void
AccessMgr::run() {
    std::unique_lock<std::mutex> lk {stop_lock};
    stop_signal.wait(lk, [this]() { return this->shutting_down; });

    amProcessor->prepareForShutdownMsgRespCallCb();

    LOGDEBUG << "Processing layer has shutdown, stop external services.";
    asyncServer->stop();
    NbdConnector::stop();
}

}  // namespace fds
