/*
 * Copyright 2015 by Formation Data Systems, Inc.
 */

#include "util/Log.h"
#include "disk_capability_op.h"
#include "platform/disk_capabilities.h"

namespace fds
{
    // ------------------------------------------------------------------------------------
    // Disk Capability Op Router
    // ------------------------------------------------------------------------------------
    DiskCapabilityOp::DiskCapabilityOp(dcapablitiy_op_e op, DiskCapabilitiesMgr *mgr) : dc_op(op), dc_mgr(mgr)
    {
    }

    DiskCapabilityOp::~DiskCapabilityOp()
    {
    }

    bool DiskCapabilityOp::dsk_iter_fn(DiskObject::pointer curr)
    {
        PmDiskObj::pointer disk = PmDiskObj::dsk_cast_ptr(curr);
        // The capability resides on the first partition
        if (disk == disk->dsk_get_parent()) {
            disk->dsk_dev_foreach(this);
            // Read the rest of the physical disks
            return true;
        }

        switch (dc_op)
        {
            case DISK_CAPABILITIES_READ:
                dc_mgr->dsk_capability_read(PmDiskObj::dsk_cast_ptr(curr));
                // Read only the first partition
                return false;

            default:
                break;
        }
        return false;
    }
}  // namespace fds
