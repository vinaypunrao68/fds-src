/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#include <disk_partition.h>
#include "disk_op.h"
#include "disk_part_mgr.h"
#include "disk_partition.h"

namespace fds
{
    DiskPartition::DiskPartition() : dsk_link(this), dsk_obj(NULL), dsk_mgr(NULL), dsk_pdev(NULL),
        dsk_pe(NULL)
    {
    }

    DiskPartition::~DiskPartition()
    {
        if (dsk_pe != NULL)
        {
            ped_disk_destroy(dsk_pe);
            ped_device_close(dsk_pdev);
        }
    }

    // dsk_partition_init
    // ------------------
    //
    void DiskPartition::dsk_partition_init(DiskPartMgr *mgr, PmDiskObj::pointer disk)
    {
        PedPartition   *part;

        dsk_mgr = mgr;
        dsk_obj = disk;

        dsk_pdev = ped_device_get(disk->rs_get_name());

        if (dsk_pdev != NULL)
        {
            dsk_pe = ped_disk_new(dsk_pdev);

            part = NULL;
            while ((part = ped_disk_next_partition(dsk_pe, part)) != NULL)
            {
                std::cout << part << std::endl;
            }
        }

        if ((dsk_pdev == NULL) || (dsk_pe == NULL))
        {
            std::cout << "You don't have permission to read devices" << std::endl;
        }
    }

    // dsk_partition_check
    // -------------------
    //
    void DiskPartition::dsk_partition_check()
    {
    }
}  // namespace fds
