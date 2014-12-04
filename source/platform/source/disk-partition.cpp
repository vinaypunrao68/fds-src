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

    // -------------------------------------------------------------------------------------
    // Disk Op
    // -------------------------------------------------------------------------------------
    DiskOp::DiskOp(disk_op_e op, DiskPartMgr *mgr) : dsk_op(op), dsk_mgr(mgr)
    {
    }
    DiskOp::~DiskOp()
    {
    }

    // dsk_iter_fn
    // -----------
    //
    bool DiskOp::dsk_iter_fn(DiskObject::pointer curr)
    {
        DiskPartition        *part;
        PmDiskObj::pointer    disk;

        disk = PmDiskObj::dsk_cast_ptr(curr);

        switch (dsk_op)
        {
            case DISK_OP_CHECK_PARTITION:
                part = new DiskPartition();
                part->dsk_partition_init(dsk_mgr, disk);
                return true;

            case DISK_OP_DO_PARTITION:
                return true;

            default:
                break;
        }
        return false;
    }

    // -------------------------------------------------------------------------------------
    // Disk Partition Mgr
    // -------------------------------------------------------------------------------------
    DiskPartMgr::DiskPartMgr() : DiskObjIter(), dsk_mtx("dsk_mtx")
    {
    }
    DiskPartMgr::~DiskPartMgr()
    {
    }

    // dsk_iter_fn
    // -----------
    //
    bool DiskPartMgr::dsk_iter_fn(DiskObject::pointer curr)
    {
        return false;
    }

    // dsk_iter_fn
    // -----------
    //
    bool DiskPartMgr::dsk_iter_fn(DiskObject::pointer curr, DiskObjIter *arg)
    {
        DiskOp   *op;

        op = static_cast<DiskOp *>(arg);
        return op->dsk_iter_fn(curr);
    }

}  // namespace fds
