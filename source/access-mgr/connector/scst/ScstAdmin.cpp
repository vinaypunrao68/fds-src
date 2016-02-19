/*
 * ScstAdmin.cpp
 *
 * Copyright (c) 2016, Brian Szmyd <szmyd@formationds.com>
 * Copyright (c) 2016, Formation Data Systems
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

#include "connector/scst/ScstAdmin.h"

extern "C" {
#include <glob.h>
#include <libgen.h>
#include <sys/types.h>
#include <sys/stat.h>
}
#include <chrono>
#include <fstream>
#include <thread>

#include "util/Log.h"

namespace fds
{

static std::string scst_iscsi_target_path   { "/sys/kernel/scst_tgt/targets/iscsi/" };
static std::string scst_iscsi_target_enable { "/enabled" };
static std::string scst_iscsi_target_mgmt   { "mgmt" };

static std::string scst_iscsi_cmd_add       { "add_target" };
static std::string scst_iscsi_cmd_remove    { "del_target" };

static std::string scst_iscsi_ini_path      { "/ini_groups/" };
static std::string scst_iscsi_ini_mgmt      { "/ini_groups/mgmt" };
static std::string scst_secure_group_name   { "secure" };
static std::string scst_secure_open_mask   { "*" };

static std::string scst_iscsi_lun_path      { "/luns/" };
static std::string scst_iscsi_lun_mgmt      { "mgmt" };

static std::string scst_iscsi_host_mgmt_path { "/initiators/" };
static std::string scst_iscsi_host_mgmt      { "mgmt" };

/**
 * Is the driver enabled
 */
bool
ScstAdmin::driverEnabled() {
    auto path = scst_iscsi_target_path + scst_iscsi_target_enable;
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
 * Is the driver enabled
 */
void
ScstAdmin::toggleDriver(bool const enable) {
    // Do nothing if already in that state
    if (enable == driverEnabled()) return;

    std::ofstream dev(scst_iscsi_target_path + scst_iscsi_target_enable,
                      std::ios::out);
    if (!dev.is_open()) {
        GLOGERROR << "Could not toggle driver, no iSCSI devices will be presented!";
        return;
    }

    // Enable target
    if (enable) {
        dev << "1" << std::endl;
    } else {
        dev << "0" << std::endl;
    }
    GLOGNORMAL << "iSCSI driver has been " << (enable ? "enabled" : "disabled");
}

/**
 * Is the target enabled
 */
bool
ScstAdmin::targetEnabled(std::string const& target_name) {
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

void ScstAdmin::toggleTarget(std::string const& target_name, bool const enable) {
    // Do nothing if already in that state
    if (enable == targetEnabled(target_name)) return;

    std::ofstream dev(scst_iscsi_target_path + target_name + scst_iscsi_target_enable,
                      std::ios::out);
    if (!dev.is_open()) {
        GLOGERROR << "Could not toggle target, no iSCSI devices will be presented!";
        return;
    }

    // Enable target
    if (enable) {
        dev << "1" << std::endl;
    } else { 
        dev << "0" << std::endl;
    }
    GLOGNORMAL << "iSCSI target [" << target_name
              << "] has been " << (enable ? "enabled" : "disabled");
}

/**
 * Build a map of the current users known to SCST for a given target
 */
ScstAdmin::credential_map
ScstAdmin::currentIncomingUsers(std::string const& target_name) {
    credential_map current_users;
    glob_t glob_buf {};
    auto pattern = scst_iscsi_target_path + target_name + "/IncomingUser*";
    auto res = glob(pattern.c_str(), GLOB_ERR, nullptr, &glob_buf);
    if (0 == res) {
        auto user = glob_buf.gl_pathv;
        while (nullptr != *user && 0 < glob_buf.gl_pathc) {
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
 * Build a set of the current initiators known to SCST for a given target
 */
void
ScstAdmin::currentInitiators(std::string const& target_name, initiator_set& current_set) {
    current_set.clear();
    glob_t glob_buf {};
    auto pattern = scst_iscsi_target_path + target_name + scst_iscsi_ini_path + scst_secure_group_name + scst_iscsi_host_mgmt_path + "*";
    auto res = glob(pattern.c_str(), GLOB_ERR, nullptr, &glob_buf);
    if (0 == res) {
        auto inititator = glob_buf.gl_pathv;
        while (nullptr != *inititator && 0 < glob_buf.gl_pathc) {
            auto initiator_base = strdup(*inititator);
            auto initiator_name = std::string(basename(initiator_base));
            free(initiator_base);
            if (scst_iscsi_host_mgmt != initiator_name) {
                LOGDEBUG << "Found initiator: " << initiator_name;
                current_set.emplace(std::move(initiator_name));
            }
            ++inititator; --glob_buf.gl_pathc;
        }
    }
    globfree(&glob_buf);
}

/**
 * Add the given credential to the target's IncomingUser attributes
 */
void ScstAdmin::addIncomingUser(std::string const& target_name,
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

/**
 * When we're done with the target from it from SCST to cause
 * any existing sessions to be terminated.
 */
void
ScstAdmin::addToScst(std::string const& target_name) {
    GLOGDEBUG << "Adding iSCSI target: [" << target_name << "]";
    std::ofstream tgt_dev(scst_iscsi_target_path + scst_iscsi_target_mgmt, std::ios::out);

    if (tgt_dev.is_open()) {
        // Add a new target
        tgt_dev << scst_iscsi_cmd_add << " " << target_name << std::endl;
        return;
    }
    GLOGERROR << "Could not create target, no iSCSI devices will be presented!";
    throw ScstError::scst_error;
}

bool ScstAdmin::groupExists(std::string const& target_name, std::string const& group_name) {
    struct stat info;
    auto ini_dir = scst_iscsi_target_path + target_name + scst_iscsi_ini_path + group_name;
    return ((0 == stat(ini_dir.c_str(), &info)) && (info.st_mode & S_IFDIR));
}

/**
 * Remove the given credential to the target's IncomingUser attributes
 */
void ScstAdmin::removeIncomingUser(std::string const& target_name,
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

bool ScstAdmin::mapDevices(std::string const& target_name,
                           device_map_type const& device_map,
                           lun_iterator const lun_start) {
    // Remove any lun in the default group, we always operate with a group
    // whether we have any assigned initiators or not.
    auto default_lun_path = scst_iscsi_target_path + target_name + scst_iscsi_lun_path + scst_iscsi_lun_mgmt;
    std::ofstream lun_mgmt(default_lun_path, std::ios::out);
    if (!lun_mgmt.is_open()) {
        GLOGERROR << "Could not remove default luns for [" << target_name << "]";
    } else {
        auto lun_path = scst_iscsi_target_path + target_name + scst_iscsi_lun_path;
        for (auto const& device : device_map) {
            auto lun_number = std::distance(lun_start, device.second);
            auto lun_dev = lun_path + std::to_string(lun_number);
            struct stat info;
            if ((0 == stat(lun_dev.c_str(), &info)) && (info.st_mode & S_IFDIR)) {
                lun_mgmt << "del " << lun_number << std::endl;
            }
        }
        lun_mgmt.close();
    }
    
    // add to default group
    auto lun_mgmt_path = scst_iscsi_target_path +
                         target_name +
                         scst_iscsi_ini_path +
                         scst_secure_group_name +
                         scst_iscsi_lun_path;

    lun_mgmt.open(lun_mgmt_path + scst_iscsi_lun_mgmt, std::ios::out);
    if (!lun_mgmt.is_open()) {
        GLOGERROR << "Could not map luns for [" << target_name << "]";
        return false;
    }

    for (auto const& device : device_map) {
        auto lun_number = std::distance(lun_start, device.second);
        auto lun_dev = lun_mgmt_path + std::to_string(lun_number);
        struct stat info;
        if ((0 != stat(lun_dev.c_str(), &info))) {
            lun_mgmt << "add " << device.first
                     << " " << lun_number
                     << std::endl;
        }
    }
    return true;
}

bool ScstAdmin::applyMasking(std::string const& target_name, initiator_set const& new_set) {
    // Ensure ini group exists
    initiator_set current_set;
    if (!groupExists(target_name, scst_secure_group_name)) {
        std::ofstream ini_mgmt(scst_iscsi_target_path + target_name + scst_iscsi_ini_mgmt,
                              std::ios::out);
        if (!ini_mgmt.is_open()) {
            GLOGWARN << "Could not open mgmt interface to create masking group.";
            return false;
        }
        ini_mgmt << "create " << scst_secure_group_name << std::endl;
    } else {
        currentInitiators(target_name, current_set);
    }

    // If there are no masks, add a global one
    auto copy_set = new_set;
    if (copy_set.empty()) {
        copy_set.emplace(scst_secure_open_mask);
    }

    {
        std::ofstream host_mgmt(scst_iscsi_target_path
                                   + target_name
                                   + scst_iscsi_ini_path
                                   + scst_secure_group_name
                                   + scst_iscsi_host_mgmt_path
                                   + scst_iscsi_host_mgmt,
                               std::ios::out);
        if (!host_mgmt.is_open()) {
            GLOGWARN << "Could not open mgmt interface to add initiators to group.";
            return false;
        }
        std::vector<std::string> manip_list;
        // For each ini that is new to the list, add to the group
        std::set_difference(copy_set.begin(), copy_set.end(),
                            current_set.begin(), current_set.end(),
                            std::inserter(manip_list, manip_list.begin()));
        for (auto const& host : manip_list) {
            host_mgmt << "add " << host << std::endl;
        }
        manip_list.clear();

        // For each ini that is no longer in the list, remove from group
        std::set_difference(current_set.begin(), current_set.end(),
                            copy_set.begin(), copy_set.end(),
                            std::inserter(manip_list, manip_list.begin()));

        for (auto const& host : manip_list) {
            host_mgmt << "del " << host << std::endl;
        }
    }
    return true;
}

/**
 * Before a target can be removed, all sessions must be closed
 */
void ScstAdmin::removeInitiators(std::string const& target_name) {
    GLOGDEBUG << "Closing active sessions for: " << target_name;
    glob_t glob_buf {};
    auto pattern = scst_iscsi_target_path + target_name + "/sessions/*/force_close";
    auto res = glob(pattern.c_str(), GLOB_ERR, nullptr, &glob_buf);
    if (0 == res) {
        auto session = glob_buf.gl_pathv;
        while (nullptr != *session && 0 < glob_buf.gl_pathc) {
            std::ofstream session_close(*session, std::ios::out);

            if (session_close.is_open()) {
                session_close << "1" << std::endl;
            }
            if (!session_close.good()) {
                GLOGWARN << "Could not close session: [" << *session
                         << "] may not be able to remove target.";
            }
            ++session; --glob_buf.gl_pathc;
        }
    }
    globfree(&glob_buf);
}

/**
 * When we're done with the target from it from SCST to cause
 * any existing sessions to be terminated.
 */
void ScstAdmin::removeFromScst(std::string const& target_name) {
    removeInitiators(target_name);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    GLOGDEBUG << "Removing iSCSI target: [" << target_name << "]";
    std::ofstream tgt_dev(scst_iscsi_target_path + scst_iscsi_target_mgmt, std::ios::out);

    if (tgt_dev.is_open()) {
        // Remove new target
        tgt_dev << scst_iscsi_cmd_remove << " " << target_name << std::endl;
        if (tgt_dev.good()) {
            return;
        }
    }
    GLOGWARN << "Could not remove target.";

}

/**
 * Remove the given credential to the target's IncomingUser attributes
 */
void ScstAdmin::setQueueDepth(std::string const& target_name, uint8_t const queue_depth) {
    GLOGDEBUG << "Setting iSCSI target queue depth [" << (uint32_t)queue_depth
              << "] for: [" << target_name << "]";
    std::ofstream tgt_dev(scst_iscsi_target_path + target_name + "/QueuedCommands", std::ios::out);
    if (!tgt_dev.is_open()) {
        GLOGERROR << "Could not set queued depth for: " << target_name;
        return;
    }
    tgt_dev << (uint32_t) queue_depth << std::endl;
}

}  // namespace fds
