/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

#ifndef SOURCE_DATA_MGR_INCLUDE_DM_TVC_VOLUMEACCESSTABLE_H_
#define SOURCE_DATA_MGR_INCLUDE_DM_TVC_VOLUMEACCESSTABLE_H_

#include <random>
#include <unordered_map>

#include "shared/fds_types.h"
#include "fdsp/common_types.h"
#include "fds_volume.h"

namespace fpi = FDS_ProtocolInterface;

namespace fds
{

/**
 * Structure used by the TvC to prevent multi-access to a volume.
 *
 * NOTE(bszmyd): Tue 07 Apr 2015 02:41:14 AM PDT
 * Not thread-safe, needs mutual exclusion at point of access
 */
struct DmVolumeAccessTable {
    explicit DmVolumeAccessTable(fds_volid_t const vol_uuid)
        : access_map(),
          random_generator(vol_uuid)
    {}
    DmVolumeAccessTable(DmVolumeAccessTable const&) = delete;
    DmVolumeAccessTable& operator=(DmVolumeAccessTable const&) = delete;
    ~DmVolumeAccessTable() = default;

    /**
     * Request an access token for the given policy
     */
    Error getToken(fds_int64_t& token, fpi::VolumeAccessPolicy const& policy);

    /**
     * Remove a previously received token
     */
    void removeToken(fds_int64_t const token);

  private:
    std::unordered_map<fds_int64_t, fpi::VolumeAccessPolicy> access_map;
    std::mt19937_64 random_generator;

    bool write_locked { false };
    bool read_locked { false };
};
}  // namespace fds

#endif  // SOURCE_DATA_MGR_INCLUDE_DM_TVC_VOLUMEACCESSTABLE_H_
