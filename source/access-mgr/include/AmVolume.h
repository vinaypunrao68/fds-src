/*
 * Copyright 2013-2016 Formation Data Systems, Inc.
 */

#ifndef SOURCE_ACCESS_MGR_INCLUDE_AMVOLUME_H_
#define SOURCE_ACCESS_MGR_INCLUDE_AMVOLUME_H_

#include "fds_volume.h"
#include "AmVolumeAccessToken.h"

namespace fds
{

struct AmVolume : public FDS_Volume {
    explicit AmVolume(VolumeDesc const& vol_desc);

    AmVolume(AmVolume const& rhs) = delete;
    AmVolume& operator=(AmVolume const& rhs) = delete;

    ~AmVolume() override;

    bool cacheable() const;
    bool writable() const;

    bool startOpen() { if (opening) { return false; } return opening = true; }
    void stopOpen() { opening = false; }

    fds_int64_t getToken() const;
    void setToken(AmVolumeAccessToken const& token);

 private:

    /** access token */
    AmVolumeAccessToken access_token;

    bool opening {false};
};

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_AMVOLUME_H_
