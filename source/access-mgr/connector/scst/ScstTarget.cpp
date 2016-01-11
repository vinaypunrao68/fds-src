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

#include <algorithm>
#include <iterator>
#include <fstream>
#include <thread>

extern "C" {
#include <glob.h>
#include <sys/types.h>
#include <sys/stat.h>
}

#include <ev++.h>

#include "connector/scst/ScstDisk.h"
#include "connector/scst/ScstCommon.h"
#include "util/Log.h"

namespace fds {

static std::string scst_iscsi_target_path   { "/sys/kernel/scst_tgt/targets/iscsi/" };
static std::string scst_iscsi_target_enable { "/enabled" };
static std::string scst_iscsi_target_mgmt   { "mgmt" };

static std::string scst_iscsi_cmd_add       { "add_target" };
static std::string scst_iscsi_cmd_remove    { "del_target" };

static std::string scst_iscsi_ini_path      { "/ini_groups/" };
static std::string scst_iscsi_ini_mgmt      { "/ini_groups/mgmt" };
static std::string scst_iscsi_lun_mgmt      { "/luns/mgmt" };

static std::string scst_iscsi_host_mgmt      { "/initiators/mgmt" };

using credential_map = std::unordered_map<std::string, std::string>;

/**
 * Is the target enabled
 */
bool
targetEnabled(std::string const& target_name) {
    auto path = scst_iscsi_target_path + target_name + scst_iscsi_target_enable;
    std::ifstream scst_enable(path, std::ios::in);
    if (scst_enable.is_open()) {
        std::string line;
        if (std::getline(scst_enable, line)) {
            std::istringstream iss(line);
            uint32_t enable_value;
            if (iss >> enable_value) {
                return 1 == enable_value;
            }
        }
    }
    return false;
}

/**
 * Build a map of the current users known to SCST for a given target
 */
credential_map
currentIncomingUsers(std::string const& target_name) {
    credential_map current_users;
    glob_t glob_buf {};
    auto pattern = scst_iscsi_target_path + target_name + "/IncomingUser*";
    auto res = glob(pattern.c_str(), GLOB_ERR, nullptr, &glob_buf);
    if (0 == res) {
        auto user = glob_buf.gl_pathv;
        while (nullptr != user && 0 < glob_buf.gl_pathc) {
            std::ifstream scst_user(*user, std::ios::in);
            if (scst_user.is_open()) {
                std::string line;
                if (std::getline(scst_user, line)) {
                    std::istringstream iss(line);
                    std::string user, password;
                    if (iss >> user >> password) {
                        current_users.emplace(user, password);
                    }
                }
            } else {
                GLOGERROR << "Could not open: [" << *user
                          << "] may have tainted credential list";
            }
            ++user; --glob_buf.gl_pathc;
        }
    }
    globfree(&glob_buf);
    return current_users;
}

/**
 * Add the given credential to the target's IncomingUser attributes
 */
void addIncomingUser(std::string const& target_name,
                     std::string const& user_name,
                     std::string const& password) {
    GLOGDEBUG << "Adding iSCSI target attribute IncomingUser [" << user_name
              << "] for: [" << target_name << "]";
    std::ofstream tgt_dev(scst_iscsi_target_path + scst_iscsi_target_mgmt, std::ios::out);
    if (!tgt_dev.is_open()) {
        GLOGERROR << "Could not add attribute to: " << target_name;
        return;
    }
    tgt_dev << "add_target_attribute "  << target_name
            << " IncomingUser "         << user_name
            << " "                      << password << std::endl;
}

bool groupExists(std::string const& target_name, std::string const& group_name) {
    struct stat info;
    auto ini_dir = scst_iscsi_target_path + target_name + scst_iscsi_ini_path + group_name;
    return ((0 == stat(ini_dir.c_str(), &info)) && (info.st_mode & S_IFDIR));
}

/**
 * Remove the given credential to the target's IncomingUser attributes
 */
void removeIncomingUser(std::string const& target_name,
                        std::string const& user_name) {
    GLOGDEBUG << "Removing iSCSI target attribute IncomingUser [" << user_name
              << "] for: [" << target_name << "]";
    std::ofstream tgt_dev(scst_iscsi_target_path + scst_iscsi_target_mgmt, std::ios::out);
    if (!tgt_dev.is_open()) {
        GLOGERROR << "Could not remove attribute from: " << target_name;
        return;
    }
    tgt_dev << "del_target_attribute "  << target_name
            << " IncomingUser "         << user_name << std::endl;
}

/**
 * Remove the given credential to the target's IncomingUser attributes
 */
void setQueueDepth(std::string const& target_name, uint8_t const queue_depth) {
    GLOGDEBUG << "Setting iSCSI target queue depth [" << (uint32_t)queue_depth
              << "] for: [" << target_name << "]";
    std::ofstream tgt_dev(scst_iscsi_target_path + target_name + "/QueuedCommands", std::ios::out);
    if (!tgt_dev.is_open()) {
        GLOGERROR << "Could not set queued depth for: " << target_name;
        return;
    }
    tgt_dev << (uint32_t) queue_depth << std::endl;
}


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

    setQueueDepth(target_name, 64);

    // Clear any mappings we might have left from before
    clearMasking();

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

    // Spawn a leader thread to start this target handling requests
    auto t = std::thread(&ScstTarget::follow, this);
    t.detach();
}

ScstTarget::~ScstTarget() = default;

void
ScstTarget::addDevice(VolumeDesc const& vol_desc) {
    std::unique_lock<std::mutex> l(deviceLock);

    // Check if we have a device with this name already
    if (device_map.end() != device_map.find(vol_desc.name)) {
        GLOGDEBUG << "Already have a device for volume: [" << vol_desc.name << "]";
        return;
    }

    auto processor = amProcessor.lock();
    if (!processor) {
        GLOGERROR << "No processing layer, shutdown.";
        return;
    }

    // Get the next available LUN number
    auto lun_it = std::find_if_not(lun_table.begin(),
                                   lun_table.end(),
                                   [] (device_ptr& p) -> bool { return (!!p); });
    if (lun_table.end() == lun_it) {
        GLOGNOTIFY << "Target [" << target_name << "] has exhausted all LUNs.";
        return;
    }
    int32_t lun_number = std::distance(lun_table.begin(), lun_it);
    GLOGDEBUG << "Mapping [" << vol_desc.name
              << "] to LUN [" << lun_number << "]";

    ScstDevice* device = nullptr;
    try {
    device = new ScstDisk(vol_desc, this, processor);
    } catch (ScstError& e) {
        return;
    }

    lun_it->reset(device);
    device_map[vol_desc.name] = lun_it;

    devicesToStart.push_back(lun_number);
    asyncWatcher->send();
    // Wait for devices to start before continuing
    deviceStartCv.wait(l, [this] () -> bool { return devicesToStart.empty(); });
}

void
ScstTarget::deviceDone(std::string const& volume_name) {
    std::lock_guard<std::mutex> g(deviceLock);
    auto it = device_map.find(volume_name);
    if (device_map.end() == it) return;

    it->second->reset();
    device_map.erase(it);
    if (device_map.empty()) {
        toggle_state(false);
    }
}

void ScstTarget::removeDevice(std::string const& volume_name) {
    std::lock_guard<std::mutex> g(deviceLock);
    auto it = device_map.find(volume_name);
    if (device_map.end() == it) return;
    (*it->second)->shutdown();
}

void ScstTarget::mapDevices() {
    // Map the luns to either the security group or default group depending on
    // whether we have any assigned initiators.
    
    // add to default group
    auto lun_mgmt_path = (ini_members.empty() ?
                              (scst_iscsi_target_path + target_name + scst_iscsi_lun_mgmt) :
                              (scst_iscsi_target_path + target_name + scst_iscsi_ini_path
                                    + "allowed" + scst_iscsi_lun_mgmt));

    std::ofstream lun_mgmt(lun_mgmt_path, std::ios::out);
    if (!lun_mgmt.is_open()) {
        GLOGERROR << "Could not map luns for [" << target_name << "]";
        return;
    }

    for (auto const& device : device_map) {
        lun_mgmt << "add " << device.first
                 << " " << std::distance(lun_table.begin(), device.second)
                 << std::endl;
    }
    luns_mapped = true;
}

void
ScstTarget::clearMasking() {
    GLOGDEBUG << "Clearing initiator mask for: " << target_name;
    // Remove the security group
    if (groupExists(target_name, "allowed")) {
        std::ofstream ini_mgmt(scst_iscsi_target_path + target_name + scst_iscsi_ini_mgmt,
                              std::ios::out);
        if (ini_mgmt.is_open()) {
            ini_mgmt << "del allowed" << std::endl;
        }
    }
    ini_members.clear();
}

void
ScstTarget::setCHAPCreds(std::unordered_map<std::string, std::string>& credentials) {
    std::unique_lock<std::mutex> l(deviceLock);

    // Remove all existing IncomingUsers if not in new creds
    // or secret is different
    for (auto const& cred : currentIncomingUsers(target_name)) {
        auto it = credentials.find(cred.first);
        if ((credentials.end() != it) && (it->second == cred.second)) {
            // Already have this login
            credentials.erase(it);
            continue;
        }
        removeIncomingUser(target_name, cred.first);
    }

    // Apply all credentials
    for (auto const& cred : credentials) {
        addIncomingUser(target_name, cred.first, cred.second);
    }
}

void
ScstTarget::setInitiatorMasking(std::set<std::string> const& new_members) {
    std::unique_lock<std::mutex> l(deviceLock);

    if ((ini_members == new_members) && luns_mapped) {
        return;
    } else if (new_members.empty()) {
        clearMasking();
    } else {

        // Ensure ini group exists
        if (!groupExists(target_name, "allowed")) {
            std::ofstream ini_mgmt(scst_iscsi_target_path + target_name + scst_iscsi_ini_mgmt,
                                  std::ios::out);
            if (!ini_mgmt.is_open()) {
                throw ScstError::scst_error;
            }
            ini_mgmt << "create allowed" << std::endl;
        }

        {
            std::ofstream host_mgmt(scst_iscsi_target_path
                                       + target_name
                                       + scst_iscsi_ini_path
                                       + "allowed"
                                       + scst_iscsi_host_mgmt,
                                   std::ios::out);
            if (! host_mgmt.is_open()) {
                throw ScstError::scst_error;
            }
            // For each ini that is no longer in the list, remove from group
            std::vector<std::string> manip_list;
            std::set_difference(ini_members.begin(), ini_members.end(),
                                new_members.begin(), new_members.end(),
                                std::inserter(manip_list, manip_list.begin()));

            for (auto const& host : manip_list) {
                host_mgmt << "del " << host << std::endl;
            }
            manip_list.clear();

            // For each ini that is new to the list, add to the group
            std::set_difference(new_members.begin(), new_members.end(),
                                ini_members.begin(), ini_members.end(),
                                std::inserter(manip_list, manip_list.begin()));
            for (auto const& host : manip_list) {
                host_mgmt << "add " << host << std::endl;
            }

            ini_members = new_members;
        }
    }
    mapDevices();
}

// Starting the devices has to happen inside the ev-loop
void
ScstTarget::startNewDevices() {
    {
        std::lock_guard<std::mutex> g(deviceLock);
        for (auto& lun: devicesToStart) {
            auto& device = lun_table[lun];

            // Persist changes in the LUN table and device map
            device->start(evLoop);
        }
        devicesToStart.clear();
    }
    deviceStartCv.notify_all();
}

void
ScstTarget::toggle_state(bool const enable) {
    // Do nothing if already in that state
    if (enable == targetEnabled(target_name)) return;

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
    startNewDevices();
}

void
ScstTarget::lead() {
    evLoop->run(0);
}

}  // namespace fds
