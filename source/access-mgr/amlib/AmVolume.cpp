/*
 * Copyright 2013-2015 Formation Data Systems, Inc.
 */

#include "AmVolume.h"
#include "AmVolumeAccessToken.h"

namespace fds
{

AmVolume::AmVolume(VolumeDesc const& vol_desc,
                   FDS_VolumeQueue* queue,
                   boost::shared_ptr<AmVolumeAccessToken> _access_token)
    :  FDS_Volume(vol_desc),
       volQueue(queue),
       access_token(_access_token)
{
    volQueue->activate();
}

AmVolume::~AmVolume() = default;

fds_int64_t
AmVolume::getToken() const {
    if (access_token)
        return access_token->getToken();
    return invalid_vol_token;
}

std::pair<bool, bool>
AmVolume::getMode() const {
    return std::make_pair(access_token->writeAllowed(),
                          access_token->cacheAllowed());
}

}  // namespace fds
