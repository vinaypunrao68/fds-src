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
#include <boost/tokenizer.hpp>

#include "connector/scst/ScstConnector.h"
#include "connector/scst/ScstConnection.h"
#include "fds_process.h"
extern "C" {
#include "connector/scst/scst_user.h"
}

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
        auto target_name = conf.get<std::string>("target_name",
                                                 "iqn.2015-08.com.formationds:block_storage");
        instance_.reset(new ScstConnector(target_name, processor, threads - 1));
        instance_->initializeTarget();
        // Start the main server thread
        auto t = std::thread(&ScstConnector::follow, instance_.get());
        t.detach();
    });
}

void ScstConnector::stop() {
    // TODO(bszmyd): Sat 12 Sep 2015 03:56:58 PM GMT
    // Implement
}

ScstConnector::ScstConnector(std::string const& name,
                             std::weak_ptr<AmProcessor> processor,
                             size_t const followers)
        : LeaderFollower(followers, false),
          amProcessor(processor),
          target_name(name) {
    LOGDEBUG << "Initialized server with: " << followers << " followers."
             << " Target: [" << target_name << "]";
}

void
ScstConnector::initializeTarget() {
    evLoop = std::make_shared<ev::dynamic_loop>(ev::NOENV | ev::POLL);
    if (!evLoop) {
        LOGERROR << "Failed to initialize lib_ev...SCST is not serving devices";
        return;
    }

    // TODO(bszmyd): Sat 12 Sep 2015 12:14:19 PM MDT
    // We can support other target-drivers than iSCSI...TBD
    // Create an iSCSI target in the SCST mid-ware for our handler
    LOGDEBUG << "Creating iSCSI target for connector.";
    auto scstTgtMgmt = open("/sys/kernel/scst_tgt/targets/iscsi/mgmt", O_WRONLY);
    if (0 > scstTgtMgmt) {
        LOGERROR << "Could not create target, no iSCSI devices will be presented!";
    } else {
        static std::string const add_tgt_cmd = "add_target " + target_name;
        auto i = write(scstTgtMgmt, add_tgt_cmd.c_str(), add_tgt_cmd.size());
        close(scstTgtMgmt);
    }
    // XXX(bszmyd): Sat 12 Sep 2015 10:18:54 AM MDT
    // Create a phony device at startup for testing
    auto processor = amProcessor.lock();
    if (!processor) {
        LOGNORMAL << "No processing layer, shutdown.";
        return;
    }

    LOGDEBUG << "Creating auto volumes for connector.";
    FdsConfigAccessor conf(g_fdsprocess->get_fds_config(), "fds.am.connector.scst.");
    auto auto_volumes = conf.get<std::string>("auto_volumes", "scst_vol");
    {
        boost::tokenizer<boost::escaped_list_separator<char> > tok(auto_volumes);
        for (auto const& vol_name : tok) {
            try {
            auto client = new ScstConnection(vol_name, this, evLoop, processor);
            } catch (ScstError& e) {
                LOGERROR << "Failed to create device for: " << vol_name;
            }
        }
    }

    LOGDEBUG << "Enabling iSCSI target.";
    scstTgtMgmt = open((std::string("/sys/kernel/scst_tgt/targets/iscsi/") + target_name + "/enabled").c_str(), O_WRONLY);
    if (0 > scstTgtMgmt) {
        LOGERROR << "Could not enable target, no iSCSI devices will be presented!";
    } else {
        auto i = write(scstTgtMgmt, "1", 1);
        close(scstTgtMgmt);
    }
    LOGNORMAL << "Scst Connector is running...";
}

void
ScstConnector::lead() {
    evLoop->run(0);
}

}  // namespace fds
