/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

#ifndef SOURCE_PLATFORM_INCLUDE_PLATFORM_DISK_CAPABILITIES_H_
#define SOURCE_PLATFORM_INCLUDE_PLATFORM_DISK_CAPABILITIES_H_

#include <string>
#include <utility>
#include <vector>

#include "fds_error.h"
#include "platform_disk_obj.h"

namespace fds
{

struct DiskCapability;

struct DiskCapabilitiesMgr {
    void dsk_capability_read(PmDiskObj::pointer disk);

    /// The aggregate capacities for storage disks <hdd, sdd>
    std::pair<size_t, size_t> disk_capacities();

    /// The aggregate counts for storage disks <hdd, sdd>
    std::pair<size_t, size_t> disk_counts();

    /// Disk interface type
    fds_disk_type_t disk_type();

  protected:
   DiskCapability *dc_master;
   ChainList    dc_capabilities;
   mutable fds_mutex    dc_mtx;

  private:
};


}  // namespace fds


#endif  // SOURCE_PLATFORM_INCLUDE_PLATFORM_DISK_CAPABILITIES_H_
