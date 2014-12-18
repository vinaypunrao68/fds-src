/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

/*
#include <disk_partition.h>

#include "disk_partition.h"
*/

#include "disk_op.h"
#include "disk_part_mgr.h"

namespace fds
{
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
