/*
 * Copyright 2015 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_ACCESS_MGR_INCLUDE_CONNECTOR_SCST_SCSTTARGET_H_
#define SOURCE_ACCESS_MGR_INCLUDE_CONNECTOR_SCST_SCSTTARGET_H_

#include <array>
#include <deque>
#include <memory>
#include <string>
#include <unordered_map>

#include "connector/scst/common.h"
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

    ~ScstTarget() = default;

    bool enabled() const;
    void disable() { toggle_state(false); }
    void enable() { toggle_state(true); }

    std::string targetName() const { return target_name; }

    int32_t addDevice(std::string const& volume_name);

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

    std::deque<int32_t> lunsToMap;
    void _mapReadyLUNs();

    // Async event to add/remove/modify luns
    unique<ev::async> asyncWatcher;

    // To hand out to devices
    std::shared_ptr<ev::dynamic_loop> evLoop;

    std::weak_ptr<AmProcessor> amProcessor;

    std::string const target_name;

    void toggle_state(bool const enable);

    void wakeupCb(ev::async &watcher, int revents);
};

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_CONNECTOR_SCST_SCSTTARGET_H_
