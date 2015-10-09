/*
 * Copyright 2015 Formation Data Systems, Inc.
 */
#include <set>
#include <string>
#include <boost/tokenizer.hpp>

#include "connector/scst/ScstConnector.h"
#include "connector/scst/ScstTarget.h"
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
        auto target_prefix = conf.get<std::string>("target_prefix", "iqn.2012-05.com.formationds:");
        instance_.reset(new ScstConnector(target_prefix, processor));
    });
}

void ScstConnector::stop() {
    // TODO(bszmyd): Sat 12 Sep 2015 03:56:58 PM GMT
    // Implement
}

ScstConnector::ScstConnector(std::string const& prefix,
                             std::weak_ptr<AmProcessor> processor)
        : amProcessor(processor),
          target_prefix(prefix)
{
    FdsConfigAccessor conf(g_fdsprocess->get_fds_config(), "fds.am.connector.scst.");
    auto threads = conf.get<uint32_t>("threads", 1);
    // TODO(bszmyd): Thu 24 Sep 2015 02:46:57 PM MDT
    // This is just for testing until we support dynamic volume loading
    ScstTarget* target {nullptr};
    try {
    target = new ScstTarget(target_prefix + "test",  threads, amProcessor);
    } catch (ScstError& e) {
        return;
    }

    LOGDEBUG << "Creating auto volumes for connector.";
    auto auto_volumes = conf.get<std::string>("auto_volumes", "scst_vol");
    {
        boost::tokenizer<boost::escaped_list_separator<char> > tok(auto_volumes);
        for (auto const& vol_name : tok) {
            try {
                target->addDevice(vol_name);
            } catch (ScstError& e) {
                LOGERROR << "Failed to create device for: " << vol_name;
            }
        }
    }

    target->enable();
}

}  // namespace fds
