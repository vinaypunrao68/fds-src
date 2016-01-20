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
#include "AmProcessor.h"
#include "fds_process.h"
extern "C" {
#include "connector/scst/scst_user.h"
}

namespace fds {

static constexpr size_t minimum_chap_password_len {12};

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
        instance_->discoverTargets();
    });
}

void ScstConnector::stop() {
    if (instance_) {
        instance_->shutdown();
    }
}

void ScstConnector::shutdown() {
    std::lock_guard<std::mutex> lk(target_lock_);
    stopping = true;
    stopping_condition_.notify_all();
    for (auto& target_pair : targets_) {
        target_pair.second->shutdown();
    }
}

void ScstConnector::volumeAdded(VolumeDesc const& volDesc) {
    if ((fpi::FDSP_VOL_BLKDEV_TYPE == volDesc.volType || fpi::FDSP_VOL_ISCSI_TYPE == volDesc.volType)
        && !volDesc.isSnapshot()
        && instance_) {
        instance_->addTarget(volDesc);
    }
}

void ScstConnector::volumeRemoved(VolumeDesc const& volDesc) {
    if (instance_) {
        instance_->removeTarget(volDesc);
    }
}

void ScstConnector::targetDone(const std::string target_name) {
    std::lock_guard<std::mutex> lk(target_lock_);
    if (0 < targets_.erase(target_name)) {
        LOGNOTIFY << "Connector has removed target: " << target_name;
    }
}

void ScstConnector::addTarget(VolumeDesc const& volDesc) {
    std::lock_guard<std::mutex> lk(target_lock_);
    if (!stopping) {
        _addTarget(volDesc);
    }
}

void ScstConnector::_addTarget(VolumeDesc const& volDesc) {
    // Create target if it does not already exist
    auto target_name = target_prefix + volDesc.name;
    bool happened {false};
    auto it = targets_.end();
    std::tie(it, happened) = targets_.emplace(target_name, nullptr);
    if (happened) {
        try {
            it->second.reset(new ScstTarget(this,
                                            target_name,
                                            threads,
                                            amProcessor));
        } catch (ScstError& e) {
            LOGERROR << "Failed to initialize target [" << target_name << "], ensure that SCST is installed and running.";
            return;
        }
        it->second->addDevice(volDesc);
    }
    if (targets_.end() == it) {
        LOGERROR << "Failed to insert target into target map...";
        return;
    }
    auto& target = *it->second;

    // If we already had a target, and it's shutdown...wait for it to complete
    // before trying to apply the apparently new descriptor
    if (!happened && !target.enabled()) {
        LOGNOTIFY << "Waiting for existing target to complete shutdown: " << target_name;
        return;
    }

    // Setup initiator masking
    std::set<std::string> initiator_list;
    for (auto const& ini : volDesc.iscsiSettings.initiators) {
        initiator_list.emplace(ini.wwn_mask);
    }
    target.setInitiatorMasking(initiator_list);

    // Setup CHAP
    std::unordered_map<std::string, std::string> credentials;
    for (auto const& cred : volDesc.iscsiSettings.incomingUsers) {
        if (minimum_chap_password_len > cred.passwd.size()) {
            GLOGWARN << "User: [" << cred.name
                     << "] has an undersized password of length: [" << cred.passwd.size()
                     << "] where the minimum length is " << minimum_chap_password_len;
            continue;
        }
        auto cred_it = credentials.end();
        bool happened;
        std::tie(cred_it, happened) = credentials.emplace(cred.name, cred.passwd);
        if (!happened) {
            GLOGWARN << "Duplicate user: [" << cred.name << "]";
            continue;
        }
    }
    target.setCHAPCreds(credentials);

    target.enable();
}

void ScstConnector::removeTarget(VolumeDesc const& volDesc) {
    std::lock_guard<std::mutex> lg(target_lock_);
    auto target_name = target_prefix + volDesc.name;

    auto it = targets_.find(target_name);
    if (targets_.end() == it) return;

    it->second->removeDevice(volDesc.name);
}

ScstConnector::ScstConnector(std::string const& prefix,
                             std::weak_ptr<AmProcessor> processor)
        : amProcessor(processor),
          target_prefix(prefix)
{
    FdsConfigAccessor conf(g_fdsprocess->get_fds_config(), "fds.am.connector.scst.");
    threads = conf.get<uint32_t>("threads", threads);
}

static auto const rediscovery_delay = std::chrono::seconds(15);

void
ScstConnector::discoverTargets() {
    // We need to poll OM for the volume list every so often, as it isn't
    // guaranteed to be complete when initializing.
    std::unique_lock<std::mutex> lk(target_lock_);
    while (!stopping) {
        auto amProc = amProcessor.lock();
        if (!amProc) {
            GLOGERROR << "No processing layer, no targets.";
            break;
        }
        GLOGTRACE << "Discovering iSCSI volumes to export.";
        std::vector<VolumeDesc> volumes;
        amProc->getVolumes(volumes);

        for (auto const& vol : volumes) {
            if ((fpi::FDSP_VOL_BLKDEV_TYPE != vol.volType && fpi::FDSP_VOL_ISCSI_TYPE != vol.volType)
                || vol.isSnapshot()) continue;
            _addTarget(vol);
        }
        stopping_condition_.wait_for(lk, rediscovery_delay);
    }
}

}  // namespace fds
