/*
 * Copyright 2013-2015 Formation Data Systems, Inc.
 */

#ifndef SOURCE_ACCESS_MGR_INCLUDE_AMVOLUME_H_
#define SOURCE_ACCESS_MGR_INCLUDE_AMVOLUME_H_

#include <memory>
#include "fds_volume.h"

namespace fds
{

struct AmVolume : public FDS_Volume {
    AmVolume(VolumeDesc const& vol_desc, FDS_VolumeQueue* queue);

    AmVolume(AmVolume const& rhs) = delete;
    AmVolume& operator=(AmVolume const& rhs) = delete;

    ~AmVolume();

    /*
     * per volume queue
     */
    FDS_VolumeQueue* volQueue;
};

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_AMVOLUME_H_
