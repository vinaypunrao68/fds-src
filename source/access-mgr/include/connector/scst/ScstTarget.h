/*
 * scst/ScstTarget.h
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

#ifndef SOURCE_ACCESS_MGR_INCLUDE_CONNECTOR_SCST_SCSTTARGET_H_
#define SOURCE_ACCESS_MGR_INCLUDE_CONNECTOR_SCST_SCSTTARGET_H_

#include <array>
#include <condition_variable>
#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "connector/scst/ScstCommon.h"
#include "concurrency/LeaderFollower.h"

namespace fds {

struct AmProcessor;
struct ScstDevice;

/**
 * ScstTarget contains a list of devices (LUNs) and configures the Scst target
 */
struct ScstTarget
    : public LeaderFollower
{
    ScstTarget(std::string const& name,
               size_t const followers,
               std::weak_ptr<AmProcessor> processor);
    ScstTarget(ScstTarget const& rhs) = delete;
    ScstTarget& operator=(ScstTarget const& rhs) = delete;

    ~ScstTarget() override;

    bool enabled() const;
    void disable() { toggle_state(false); }
    void enable() { toggle_state(true); }

    std::string targetName() const { return target_name; }

    void addDevice(std::string const& volume_name);
    void deviceDone(std::string const& volume_name);
    void removeDevice(std::string const& volume_name);
    void setCHAPCreds(std::unordered_map<std::string, std::string> const& credentials);
    void setInitiatorMasking(std::vector<std::string> const& ini_members);

    void mapDevices();

 protected:
    void lead() override;

 private:
    template<typename T>
    using unique = std::unique_ptr<T>;
    using device_ptr = unique<ScstDevice>;

    using lun_table_type = std::array<device_ptr, 255>;

    template<typename T>
    using map_type = std::unordered_map<std::string, T>;
    using device_map_type = map_type<lun_table_type::iterator>;

    /// Max LUNs is 255 per target
    device_map_type device_map;
    lun_table_type lun_table;

    std::condition_variable deviceStartCv;
    std::mutex deviceLock;
    std::deque<int32_t> devicesToStart;

    /// Initiator masking
    std::vector<std::string> ini_members;

    // Async event to add/remove/modify luns
    unique<ev::async> asyncWatcher;

    // To hand out to devices
    std::shared_ptr<ev::dynamic_loop> evLoop;

    std::weak_ptr<AmProcessor> amProcessor;

    std::string const target_name;

    void clearMasking();

    void startNewDevices();

    void toggle_state(bool const enable);

    void wakeupCb(ev::async &watcher, int revents);
};

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_CONNECTOR_SCST_SCSTTARGET_H_
