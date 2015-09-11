/*
 * Copyright 2015 Formation Data Systems, Inc.
 */
#include <set>
#include <string>
#include <thread>

extern "C" {
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/uio.h>
}

#include <ev++.h>
#include <boost/shared_ptr.hpp>

#include "connector/scst/ScstConnector.h"
#include "connector/scst/ScstConnection.h"
#include "fds_process.h"
#include "connector/scst/scst_user.h"

namespace fds {

// The singleton
std::unique_ptr<ScstConnector> ScstConnector::instance_ {nullptr};

void ScstConnector::start(std::weak_ptr<AmProcessor> processor) {
    static std::once_flag init;
    // Initialize the singleton
    std::call_once(init, [processor] () mutable
    {
        FdsConfigAccessor conf(g_fdsprocess->get_fds_config(), "fds.am.connector.scst.");
        auto threads = conf.get<uint32_t>("threads", 1);
        instance_.reset(new ScstConnector(processor, threads - 1));
        // Start the main server thread
        auto t = std::thread(&ScstConnector::follow, instance_.get());
        t.detach();
    });
}

void ScstConnector::stop() {
    if (instance_ && instance_->asyncWatcher) {
       instance_->asyncWatcher->send();
    }
}

ScstConnector::ScstConnector(std::weak_ptr<AmProcessor> processor,
                           size_t const followers)
        : LeaderFollower(followers, false),
          amProcessor(processor) {
    LOGDEBUG << "Initialized server with: " << followers << " followers.";
    initialize();
}

void ScstConnector::initialize() {
    // Close the SCST device if we are re-initializing
    if (0 <= scstDev)
        { reset(); }

    // Open the SCST user device
    if (0 > (scstDev = openScst())) {
        return;
    }

    // This is our async event watcher for shutdown
    if (!asyncWatcher) {
        asyncWatcher = std::unique_ptr<ev::async>(new ev::async());
        asyncWatcher->set<ScstConnector, &ScstConnector::reset>(this);
        asyncWatcher->start();
    }

    // Setup event loop
    if (!evLoop && !evIoWatcher) {
        LOGNORMAL << "Accepting SCST connections";
        evLoop = std::unique_ptr<ev::dynamic_loop>(new ev::dynamic_loop());
        if (!evLoop || !evIoWatcher) {
            LOGERROR << "Failed to initialize lib_ev...";
            return;
        }
        evIoWatcher = std::unique_ptr<ev::io>(new ev::io());
        evIoWatcher->set<ScstConnector, &ScstConnector::scstEvent>(this);
    }
    evIoWatcher->set(scstDev, ev::READ);
    evIoWatcher->start(scstDev, ev::READ);
}

void ScstConnector::reset() {
    if (0 <= scstDev) {
        evIoWatcher->stop();
        shutdown(scstDev, SHUT_RDWR);
        close(scstDev);
        scstDev = -1;
    }
}

void
ScstConnector::scstEvent(ev::io &watcher, int revents) {
    if (EV_ERROR & revents) {
        LOGERROR << "Got invalid libev event";
        return;
    }

    // TODO(bszmyd): Fri 11 Sep 2015 05:32:03 AM MDT
    // IMPLEMENT SCST event handler

    /** First see if we even have a processing layer */
//    auto processor = amProcessor.lock();
//    if (!processor) {
//        LOGNORMAL << "No processing layer, shutdown.";
//        reset();
//        return;
//    }

    // Create a handler for this SCST connection
    // Will delete itself when connection dies
//    ScstConnection *client = new ScstConnection(this, clientsd, processor);
//    LOGNORMAL << "Created client connection...";
}

int
ScstConnector::openScst() {
    int dev = open(DEV_USER_PATH DEV_USER_NAME, O_RDWR | O_NONBLOCK);
    if (0 > dev) {
        LOGERROR << "Opening the SCST device failed: " << strerror(errno);
    }
    return dev;
}

void
ScstConnector::lead() {
    evLoop->run(0);
}

}  // namespace fds
