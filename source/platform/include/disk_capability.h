/*
 * Copyright 2015 by Formation Data Systems, Inc.
 */

#ifndef SOURCE_PLATFORM_INCLUDE_DISK_CAPABILITY_H_
#define SOURCE_PLATFORM_INCLUDE_DISK_CAPABILITY_H_

#include <cstdint>
#include <cstdlib>

#include "shared/fds_types.h"
#include "cpplist.h"
#include "platform_disk_obj.h"

namespace fds
{

struct DiskCapability
{
    fds_disk_type_t disk_type {FDS_DISK_SATA};
    bool spinning {false};
    bool initialized {false};
    fds_uint64_t capacity_gb {0};

    ChainLink             dc_link;
    PmDiskObj::pointer    dc_owner;

    explicit DiskCapability(PmDiskObj::pointer disk);
    ~DiskCapability() = default;

    void dsk_capability_read();
};

}  // namespace fds

#endif  // SOURCE_PLATFORM_INCLUDE_DISK_CAPABILITY_H_
