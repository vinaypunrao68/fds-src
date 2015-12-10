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
    // TODO(bszmyd): Sat 12 Sep 2015 03:56:58 PM GMT
    // Implement
}

void ScstConnector::volumeAdded(VolumeDesc const& volDesc) {
    if (1 == volDesc.volType && !volDesc.isSnapshot() && instance_) {
        instance_->addTarget(volDesc);
    }
}

void ScstConnector::volumeRemoved(VolumeDesc const& volDesc) {
    if (instance_) {
        instance_->removeTarget(volDesc);
    }
}

void ScstConnector::addTarget(VolumeDesc const& volDesc) {
    std::lock_guard<std::mutex> lg(target_lock_);

    if (targets_.end() == targets_.find(volDesc.name)) {
        auto target = new ScstTarget(target_prefix + volDesc.name,
                                     threads,
                                     amProcessor);
        targets_[volDesc.name].reset(target);
        target->addDevice(volDesc.name);
        target->enable();
    }
}

void ScstConnector::removeTarget(VolumeDesc const& volDesc) {
    std::lock_guard<std::mutex> lg(target_lock_);

    auto it = targets_.find(volDesc.name);
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

void
ScstConnector::discoverTargets() {
    auto amProc = amProcessor.lock();
    if (!amProc) {
        GLOGERROR << "No processing layer, no targets.";
        return;
    }
    GLOGNORMAL << "Discovering iSCSI volumes to export.";
    std::vector<VolumeDesc> volumes;
    amProc->getVolumes(volumes);

    for (auto const& vol : volumes) {
        // FIXME(bszmyd): Mon 23 Nov 2015 05:27:02 PM MST
        // This is a magic value from thrift that i don't want to include
        // headers from
        if (1 != vol.volType || vol.isSnapshot()) continue;
        try {
            auto it = targets_.end();
            bool happened {false};
            auto target = new ScstTarget(target_prefix + vol.name,
                                         threads,
                                         amProcessor);
            targets_[vol.name].reset(target);
            target->addDevice(vol.name);
            target->enable();
        } catch (ScstError& e) {
            LOGERROR << "Failed to create device for: " << vol.name;
        }
    }
}

}  // namespace fds
