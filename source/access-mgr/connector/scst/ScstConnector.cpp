/*
 * ScstConnector.cpp
 *
 * Copyright (c) 2015, Brian Szmyd <szmyd@formationds.com>
 * Copyright (c) 2015, Formation Data Systems
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
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
    LOGDEBUG << "Creating auto volumes for connector.";
    auto auto_volumes = conf.get<std::string>("auto_volumes", "scst_vol");
    {
        boost::tokenizer<boost::escaped_list_separator<char> > tok(auto_volumes);
        for (auto const& vol_name : tok) {
            try {
                ScstTarget* target {nullptr};
                target = new ScstTarget(target_prefix + vol_name,  threads, amProcessor);
                target->addDevice(vol_name);
                target->enable();
            } catch (ScstError& e) {
                LOGERROR << "Failed to create device for: " << vol_name;
            }
        }
    }
}

}  // namespace fds
