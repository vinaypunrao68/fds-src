/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

#ifndef SOURCE_DATA_MGR_INCLUDE_DM_TVC_VOLUMEACCESSTABLE_H_
#define SOURCE_DATA_MGR_INCLUDE_DM_TVC_VOLUMEACCESSTABLE_H_

#include <chrono>
#include <mutex>
#include <random>
#include <unordered_map>
#include <utility>

#include "fdsp/common_types.h"

#include "shared/fds_types.h"
#include "fds_volume.h"

namespace fpi = FDS_ProtocolInterface;

namespace fds
{

struct FdsTimer;
struct FdsTimerTask;

/**
 * Structure used by the TvC to prevent multi-access to a volume.
 */
struct DmVolumeAccessTable {
    explicit DmVolumeAccessTable(fds_volid_t const vol_uuid);
    DmVolumeAccessTable(DmVolumeAccessTable const&) = delete;
    DmVolumeAccessTable& operator=(DmVolumeAccessTable const&) = delete;
    ~DmVolumeAccessTable() = default;

    /**
     * Request an access token for the given policy
     */
    Error getToken(fds_int64_t& token,
                   fpi::VolumeAccessPolicy const& policy,
                   std::chrono::duration<fds_uint32_t> const lease_time);

    /**
     * Remove a previously received token
     */
    void removeToken(fds_int64_t const token);

  private:
    using map_value_type = std::pair<fpi::VolumeAccessPolicy, boost::shared_ptr<FdsTimerTask>>;
    std::unordered_map<fds_int64_t, map_value_type> access_map;
    std::mt19937_64 random_generator;

    bool write_locked { false };
    bool read_locked { false };
    std::mutex lock;

    /**
     * A timer to expire tokens
     */
    static std::shared_ptr<FdsTimer> getTimer();

    std::shared_ptr<FdsTimer> timer;
};
}  // namespace fds

#endif  // SOURCE_DATA_MGR_INCLUDE_DM_TVC_VOLUMEACCESSTABLE_H_
