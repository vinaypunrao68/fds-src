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

void
AmVolume::setSequenceId(fds_uint64_t const sequence_id) {
    // FIXME(bszmyd): Mon 15 Jun 2015 11:36:01 AM MDT
    // This only works if we can assume only one AM will be
    // using the volume at a time...we don't expect to set this
    // other than the initial volume open
    if (vol_sequence_id.load(std::memory_order_relaxed) < sequence_id) {
        vol_sequence_id.store(sequence_id, std::memory_order_relaxed);
    }
}

}  // namespace fds
