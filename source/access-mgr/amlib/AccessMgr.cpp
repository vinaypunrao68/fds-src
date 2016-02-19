/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <string>
#include <thread>

#include "fds_process.h"

#include "AccessMgr.h"
#include "AmProcessor.h"
#include <net/SvcMgr.h>

#include "connector/xdi/AmAsyncService.h"
#include "connector/nbd/NbdConnector.h"
#include "connector/scst/ScstConnector.h"

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
    FdsConfigAccessor features(g_fdsprocess->get_fds_config(), "fds.feature_toggle.");
    nbd_enabled = features.get<bool>("am.nbd_connector", nbd_enabled);
    scst_enabled = features.get<bool>("am.scst_connector", scst_enabled);
    return Module::mod_init(param);
}


void
AccessMgr::mod_startup() {
    LOGNOTIFY << "Starting processing layer ";
    amProcessor->start();
}

void
AccessMgr::mod_shutdown() {
    LOGNOTIFY << "Stopping processing layer ";
    amProcessor->stop();
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
    if (!asyncServer) {
        // Pretty useless till we have the tables, just keep retrying I guess.
        while (!amProcessor->haveTables()) {
            LOGWARN << "Failed to get the distribution tables...will try again.";
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }
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
     * Initialize the Nbd block connector
     */
    if (nbd_enabled) {
        NbdConnector::start(weakProcessor);
    }

    /**
     * FEATURE TOGGLE: Scst Connector
     * Fri 11 Sep 2015 09:31:21 AM MDT
     */
    if (scst_enabled) {
        /**
         * Initialize the Scst connector
         */
        auto t = std::thread(&ScstConnector::start, weakProcessor);
        t.detach();
    }
}

void AccessMgr::mod_disable_service() {
    LOGNOTIFY << "Stopping connectors";
    asyncServer->stop();
    if (nbd_enabled) {
        NbdConnector::stop();
    }
    /**
     * FEATURE TOGGLE: Scst Connector
     * Fri 11 Sep 2015 09:31:21 AM MDT
     */
    if (scst_enabled) {
        ScstConnector::stop();
    }
}

void
AccessMgr::run() {
    std::unique_lock<std::mutex> lk {stop_lock};
    stop_signal.wait(lk, [this]() { return this->shutting_down; });
    LOGNORMAL << "Processing layer has shutdown, stop external services.";
}

void
AccessMgr::volumeAdded(VolumeDesc const& volDesc) {
    if (nbd_enabled) {
        NbdConnector::volumeAdded(volDesc);
    }
    if (scst_enabled) {
        ScstConnector::volumeAdded(volDesc);
    }
}

void
AccessMgr::volumeRemoved(VolumeDesc const& volDesc) {
    if (nbd_enabled) {
        NbdConnector::volumeRemoved(volDesc);
    }
    if (scst_enabled) {
        ScstConnector::volumeRemoved(volDesc);
    }
}

}  // namespace fds
