/*
 * ScstTarget.cpp
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

#include "connector/scst/ScstTarget.h"

#include <iterator>
#include <fstream>
#include <thread>

#include <ev++.h>

#include "connector/scst/ScstDevice.h"
#include "connector/scst/ScstCommon.h"
#include "util/Log.h"

namespace fds {

static std::string scst_iscsi_target_path   { "/sys/kernel/scst_tgt/targets/iscsi/" };
static std::string scst_iscsi_target_enable { "/enabled" };
static std::string scst_iscsi_target_mgmt   { "mgmt" };

static std::string scst_iscsi_cmd_add       { "add_target" };
static std::string scst_iscsi_cmd_remove    { "del_target" };

static std::string scst_iscsi_lun_mgmt      { "/luns/mgmt" };

ScstTarget::ScstTarget(std::string const& name, 
                       size_t const followers,
                       std::weak_ptr<AmProcessor> processor) :
    LeaderFollower(followers, false), 
    amProcessor(processor),
    target_name(name)
{
    // TODO(bszmyd): Sat 12 Sep 2015 12:14:19 PM MDT
    // We can support other target-drivers than iSCSI...TBD
    // Create an iSCSI target in the SCST mid-ware for our handler
    GLOGDEBUG << "Creating iSCSI target for connector: [" << target_name << "]";
    std::ofstream tgt_dev(scst_iscsi_target_path + scst_iscsi_target_mgmt, std::ios::out);

    if (!tgt_dev.is_open()) {
        GLOGERROR << "Could not create target, no iSCSI devices will be presented!";
        throw ScstError::scst_error;
    }

    // Add a new target for ourselves
    tgt_dev << scst_iscsi_cmd_add << " " << target_name << std::endl;
    tgt_dev.close();

    // Start watching the loop
    evLoop = std::make_shared<ev::dynamic_loop>(ev::NOENV | ev::POLL);
    if (!evLoop) {
        LOGERROR << "Failed to initialize lib_ev...SCST is not serving devices";
        throw ScstError::scst_error;
    }

    asyncWatcher = std::unique_ptr<ev::async>(new ev::async());
    asyncWatcher->set(*evLoop);
    asyncWatcher->set<ScstTarget, &ScstTarget::wakeupCb>(this);
    asyncWatcher->start();

    // Just we spawn a leader thread to start this target handling requests
    auto t = std::thread(&ScstTarget::follow, this);
    t.detach();
}

int32_t
ScstTarget::addDevice(std::string const& volume_name) {
    // Check if we have a device with this name already
    if (device_map.end() != device_map.find(volume_name)) {
        GLOGWARN << "Already have a device for volume: [" << volume_name << "]";
        return -1;
    }

    auto processor = amProcessor.lock();
    if (!processor) {
        GLOGNORMAL << "No processing layer, shutdown.";
        return -1;
    }

    // Get the next available LUN number
    auto lun_it = std::find_if_not(lun_table.begin(),
                                   lun_table.end(),
                                   [] (device_ptr& p) -> bool { return (!!p); });
    if (lun_table.end() == lun_it) {
        GLOGNOTIFY << "Target [" << target_name << "] has exhausted all LUNs.";
        return -1;
    }
    int32_t lun_number = std::distance(lun_table.begin(), lun_it);
    GLOGDEBUG << "Mapping [" << volume_name
              << "] to LUN [" << lun_number << "]";

    ScstDevice* device = nullptr;
    try {
    device = new ScstDevice(volume_name, this, processor);
    } catch (ScstError& e) {
        return -1;
    }

    lun_it->reset(device);
    device_map[volume_name] = lun_it;
    lunsToMap.push_back(lun_number);
    asyncWatcher->send();
    return lun_number;
}

void
ScstTarget::_mapReadyLUNs() {
    for (auto& lun: lunsToMap) {
        auto& device = lun_table[lun];

        // TODO(bszmyd): Sat 12 Sep 2015 12:14:19 PM MDT
        // We can support other target-drivers than iSCSI...TBD
        // Create an iSCSI target in the SCST mid-ware for our handler
        std::ofstream lun_mgmt(scst_iscsi_target_path + target_name + scst_iscsi_lun_mgmt,
                               std::ios::out);
        if (!lun_mgmt.is_open()) {
            GLOGERROR << "Could not map lun for [" << device->getName() << "]";
        }
        lun_mgmt << "add " + device->getName() + " " + std::to_string(lun) << std::endl;
        lun_mgmt.close();

        // Persist changes in the LUN table and device map
        device->start(evLoop);
    }
    lunsToMap.clear();
}

void
ScstTarget::toggle_state(bool const enable) {
    GLOGDEBUG << "Toggling iSCSI target.";
    std::ofstream dev(scst_iscsi_target_path + target_name + scst_iscsi_target_enable,
                      std::ios::out);
    if (!dev.is_open()) {
        GLOGERROR << "Could not toggle target, no iSCSI devices will be presented!";
        throw ScstError::scst_error;
    }

    // Enable target
    if (enable) {
        dev << "1" << std::endl;
    } else { 
        dev << "0" << std::endl;
    }
    dev.close();
    GLOGNORMAL << "iSCSI target [" << target_name
               << "] has been " << (enable ? "enabled" : "disabled");
}

void
ScstTarget::wakeupCb(ev::async &watcher, int revents) {
    _mapReadyLUNs();
}

void
ScstTarget::lead() {
    evLoop->run(0);
}

}  // namespace fds
