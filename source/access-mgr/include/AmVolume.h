/*
 * Copyright 2013-2015 Formation Data Systems, Inc.
 */

#ifndef SOURCE_ACCESS_MGR_INCLUDE_AMVOLUME_H_
#define SOURCE_ACCESS_MGR_INCLUDE_AMVOLUME_H_

#include <memory>
#include <util/Log.h>
#include <fds_volume.h>

namespace fds
{
struct AmVolume : public FDS_Volume , public HasLogger
{
    AmVolume(const VolumeDesc& vdesc, fds_log *parent_log);
    AmVolume(AmVolume const& rhs) = delete;
    AmVolume& operator=(AmVolume const& rhs) = delete;

    ~AmVolume();

    /*
     * per volume queue
     */
    std::unique_ptr<FDS_VolumeQueue> volQueue;
};

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_AMVOLUME_H_
