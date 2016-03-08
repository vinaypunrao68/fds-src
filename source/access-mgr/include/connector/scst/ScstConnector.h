/*
 * scst/ScstConnector.h
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

#ifndef SOURCE_ACCESS_MGR_INCLUDE_CONNECTOR_SCST_SCSTCONNECTOR_H_
#define SOURCE_ACCESS_MGR_INCLUDE_CONNECTOR_SCST_SCSTCONNECTOR_H_

#include <condition_variable>
#include <map>
#include <memory>
#include <mutex>
#include <string>

#include "connector/scst/ScstCommon.h"
#include "fds_volume.h"

namespace fds {

struct AmProcessor;
struct ScstTarget;

struct ScstConnector
{
    ScstConnector(ScstConnector const& rhs) = delete;
    ScstConnector& operator=(ScstConnector const& rhs) = delete;
    ~ScstConnector() = default;

    static void start(std::weak_ptr<AmProcessor> processor);
    static void stop();
    static void volumeAdded(VolumeDesc const& volDesc);
    static void volumeRemoved(VolumeDesc const& volDesc);

    std::string targetPrefix() const { return target_prefix; }

    /***
     * Used by the ScstTarget to tell the Connector
     * it is safe to remove
     */
    void targetDone(const std::string target_name);

 private:
    template<typename T>
    using unique = std::unique_ptr<T>;

    static unique<ScstConnector> instance_;

    size_t threads {1};
    bool stopping {false};

    std::mutex target_lock_;
    std::condition_variable stopping_condition_;
    std::map<std::string, std::unique_ptr<ScstTarget>> targets_;

    ScstConnector(std::string const& prefix,
                  std::weak_ptr<AmProcessor> processor);

    std::weak_ptr<AmProcessor> amProcessor;

    std::string target_prefix;

    void addTarget(VolumeDesc const& volDesc);
    bool _addTarget(VolumeDesc const& volDesc);
    void discoverTargets();
    void removeTarget(VolumeDesc const& volDesc);
    void shutdown();
};

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_CONNECTOR_SCST_SCSTCONNECTOR_H_
