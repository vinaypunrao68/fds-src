/*
 * Copyright 2013-2016 Formation Data Systems, Inc.
 */

#include "AmVolume.h"

namespace fds
{

AmVolume::AmVolume(VolumeDesc const& vol_desc)
    :  FDS_Volume(vol_desc),
       access_token(nullptr)
{
}

AmVolume::~AmVolume() = default;

fds_int64_t
AmVolume::getToken() const {
    return (isOpen() ? access_token->getToken() : invalid_vol_token);
}

void
AmVolume::setToken(AmVolumeAccessToken const& token) {
    access_token.reset(new AmVolumeAccessToken(token));
}

fds_int64_t
AmVolume::clearToken() {
    auto token = (isOpen() ? access_token->getToken() : invalid_vol_token);
    access_token.reset();
    return token;
}

bool
AmVolume::isCacheable() const {
    return (isOpen() ? access_token->cacheAllowed() : false);
}

bool
AmVolume::isWritable() const {
    return (isOpen() ? access_token->writeAllowed() : false);
}

bool
AmVolume::isOpen() const {
    return (!!access_token);
}

}  // namespace fds
