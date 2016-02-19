/*
 * Copyright 2013-2016 Formation Data Systems, Inc.
 */

#include "AmVolume.h"

namespace fds
{

AmVolume::AmVolume(VolumeDesc const& vol_desc)
    :  FDS_Volume(vol_desc),
       access_token()
{
}

AmVolume::~AmVolume() = default;

fds_int64_t
AmVolume::getToken() const {
    return access_token.getToken();
}

void
AmVolume::setToken(AmVolumeAccessToken const& token) {
    access_token = token;
}

bool
AmVolume::cacheable() const {
    return access_token.cacheAllowed();
}

bool
AmVolume::writable() const {
    return access_token.writeAllowed();
}

}  // namespace fds
