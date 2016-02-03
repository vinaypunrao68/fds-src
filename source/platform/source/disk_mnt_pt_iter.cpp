/*
 * Copyright 2016 by Formation Data Systems, Inc.
 */

#include "disk_mnt_pt_iter.h"
#include "platform_disk_obj.h"

namespace fds
{
    // ------------------------------------------------------------------------------------
    // Disk Data Mount Point Finder
    // ------------------------------------------------------------------------------------
    bool DiskMntPtIter::dsk_iter_fn(DiskObject::pointer curr)
    {
        PmDiskObj::pointer disk = PmDiskObj::dsk_cast_ptr(curr);

        if (disk->dsk_get_parent() == disk || disk->dsk_get_parent() == NULL)
        {   // shouldn't happen; it's not a slice, so return
            LOGWARN << "Bad disk slice in mount point iteration, skipping" << disk->rs_get_name();
            return true;
        }

        std::string mnt_pt = disk->dsk_get_mount_point();
        if (!mnt_pt.empty())
        {
            bool reset_mnt_pt = true;

            if (prev != NULL && (prev->rs_get_name() != NULL))
            {
                if (strcmp(prev->rs_get_name(), disk->rs_get_name()) > 0)
                {   // only reset it's the next slice (e.g. previous is /dev/sdb2, current is /dev/sdb3)
                    reset_mnt_pt = false;
                }
            }
            if (reset_mnt_pt)
            {
                LOGDEBUG << "Setting data mount point " << mnt_pt.c_str() << " for " << disk->dsk_get_parent()->rs_get_name();
                disk->dsk_get_parent()->dsk_set_mount_point(mnt_pt.c_str());
                prev = disk;
            }
        }

        return true;
    }
}  // namespace fds
