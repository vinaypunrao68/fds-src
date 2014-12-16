/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#ifndef SOURCE_PLATFORM_INCLUDE_DISK_PARTITION_H_
#define SOURCE_PLATFORM_INCLUDE_DISK_PARTITION_H_

#include <parted/parted.h>

#include "cpplist.h"
#include "platform_disk_obj.h"

namespace fds
{
    class DiskPartMgr;

    /**
     * Handle jobs related to partition of a disk.
     */

    class DiskPartition
    {
        protected:
            friend class DiskPartMgr;

            ChainLink             dsk_link;
            PmDiskObj::pointer    dsk_obj;
            DiskPartMgr          *dsk_mgr;
            PedDevice            *dsk_pdev;
            PedDisk              *dsk_pe;

        public:
            DiskPartition();
            virtual ~DiskPartition();

            virtual void dsk_partition_init(DiskPartMgr *mgr, PmDiskObj::pointer disk);
            virtual void dsk_partition_check();
    };
}  // namespace fds

#endif  // SOURCE_PLATFORM_INCLUDE_DISK_PARTITION_H_
