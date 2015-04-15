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
    return access_token->getToken();
}

void
AmVolume::swapToken(boost::shared_ptr<AmVolumeAccessToken>& _token) {
    access_token.swap(_token);
}

}  // namespace fds
