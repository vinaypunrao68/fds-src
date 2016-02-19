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
#include "fds_module_provider.h"

namespace fpi = FDS_ProtocolInterface;

namespace fds
{

struct FdsTimer;
struct FdsTimerTask;

struct DmVolumeAccessEntry {
    /** Defines access type permitted to volume */
    fpi::VolumeAccessMode               access_mode;

    /** Service that "owns" this token */
    fpi::SvcUuid                        client_uuid;

    /** Timer that wakes up expiration task */
    boost::shared_ptr<FdsTimerTask>     timer_task;

};

/**
 * Structure used by the TvC to prevent multi-access to a volume.
 */
struct DmVolumeAccessTable {
    DmVolumeAccessTable(fds_volid_t const vol_uuid, const FdsTimerPtr &timer);
    DmVolumeAccessTable(DmVolumeAccessTable const&) = delete;
    DmVolumeAccessTable& operator=(DmVolumeAccessTable const&) = delete;
    ~DmVolumeAccessTable() = default;

    /**
     * Request an access token for the given policy
     */
    Error getToken(fpi::SvcUuid const& client_uuid,
                   fds_int64_t& token,
                   fpi::VolumeAccessMode const& mode,
                   std::chrono::duration<fds_uint32_t> const lease_time);

    /**
     * Remove a previously received token
     */
    void removeToken(fds_int64_t const token);

  private:
    std::unordered_map<fds_int64_t, DmVolumeAccessEntry> access_map;
    std::mt19937_64 random_generator;

    bool cached { false };
    std::mutex lock;

    boost::shared_ptr<FdsTimer> timer;
};
}  // namespace fds

#endif  // SOURCE_DATA_MGR_INCLUDE_DM_TVC_VOLUMEACCESSTABLE_H_
