/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <string>

#include "fds_process.h"

#include "AccessMgr.h"
#include "AmProcessor.h"
#include <net/SvcMgr.h>
#include <AmDataApi.h>

#include "connector/xdi/AmAsyncService.h"
#include "connector/xdi/fdsn-server.h"
#include "connector/block/NbdConnector.h"

namespace fds {

AccessMgr::AccessMgr(const std::string &modName,
                     CommonModuleProviderIf *modProvider)
        : Module(modName.c_str()),
          modProvider_(modProvider),
          stop_signal(),
          shutting_down(false),
          amProcessor(std::make_shared<fds::AmProcessor>(modProvider)),
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
    amProcessor->start([this]() mutable { this->stop(); });
}

void
AccessMgr::mod_shutdown() {
}

void AccessMgr::mod_enable_service()
{
    LOGNOTIFY << "Enabling services ";

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
    fdsnServer = std::unique_ptr<FdsnServer>(new FdsnServer(dataApi, pmPort));
    fdsnServer->start();

    /**
     * Initialize the async server
     */
    auto weakProcessor = std::weak_ptr<AmProcessor>(amProcessor);
    asyncServer.reset(new AsyncDataServer(weakProcessor, pmPort));
    asyncServer->start();

    /**
     * Initialize the block connector
     */
    blkConnector.reset(new NbdConnector(weakProcessor));
}

void AccessMgr::mod_disable_service() {
    stop();
}

void
AccessMgr::run() {
    std::unique_lock<std::mutex> lk {stop_lock};
    stop_signal.wait(lk, [this]() { return this->shutting_down; });

    amProcessor->prepareForShutdownMsgRespCallCb();

    LOGDEBUG << "Processing layer has shutdown, stop external services.";
    asyncServer->stop();
    fdsnServer->stop();
    blkConnector->stop();
}

void
AccessMgr::stop() {
    std::unique_lock<std::mutex> lk {stop_lock};
    shutting_down = true;
    stop_signal.notify_one();
}

}  // namespace fds
