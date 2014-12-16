/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#include "disk_label.h"
#include "disk_label_op.h"

namespace fds
{
    // ------------------------------------------------------------------------------------
    // Disk Label Op Router
    // ------------------------------------------------------------------------------------
    DiskLabelOp::DiskLabelOp(dlabel_op_e op, DiskLabelMgr *mgr) : dl_op(op), dl_mgr(mgr)
    {
    }
    DiskLabelOp::~DiskLabelOp()
    {
    }

    bool DiskLabelOp::dsk_iter_fn(DiskObject::pointer curr)
    {
        PmDiskObj::pointer    disk;

        disk = PmDiskObj::dsk_cast_ptr(curr);
        switch (dl_op)
        {
            case DISK_LABEL_READ:
                dl_mgr->dsk_read_label(PmDiskObj::dsk_cast_ptr(curr));
                return true;

            case DISK_LABEL_WRITE:
                return true;

            default:
                break;
        }
        return false;
    }
}  // namespace fds
