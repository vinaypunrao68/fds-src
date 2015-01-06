/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#include "disk_op.h"
#include "disk_partition.h"

namespace fds
{
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
}  // namespace fds
