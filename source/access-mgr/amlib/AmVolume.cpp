/*
 * Copyright 2013-2015 Formation Data Systems, Inc.
 */

#include "AmVolume.h"

namespace fds
{

AmVolume::AmVolume(VolumeDesc const& vol_desc, FDS_VolumeQueue* queue, fds_int64_t _token)
    :  FDS_Volume(vol_desc),
       volQueue(queue),
       token(_token)
{
    volQueue->activate();
}

AmVolume::~AmVolume() = default;

}  // namespace fds
