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
    // TODO(bszmyd): Sat 12 Sep 2015 03:56:58 PM GMT
    // Implement
}

ScstConnector::ScstConnector(std::weak_ptr<AmProcessor> processor,
                           size_t const followers)
        : LeaderFollower(followers, false),
          amProcessor(processor) {
    LOGDEBUG << "Initialized server with: " << followers << " followers.";
}

void
ScstConnector::lead() {
    if (!evLoop) {
        evLoop = std::make_shared<ev::dynamic_loop>();
        if (!evLoop) {
            LOGERROR << "Failed to initialize lib_ev...SCST is not serving devices";
            return;
        }

        // XXX(bszmyd): Sat 12 Sep 2015 10:18:54 AM MDT
        // Create a phony device at startup for testing
        auto processor = amProcessor.lock();
        if (!processor) {
            LOGNORMAL << "No processing layer, shutdown.";
            return;
        }
        auto client = new ScstConnection("scst_vol", this, evLoop, processor);
    }
    evLoop->run(0);
}

}  // namespace fds
