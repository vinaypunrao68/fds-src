/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#include <string>

/*
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <mntent.h>
#include <libudev.h>

#include <fds_uuid.h>
#include <fds_process.h>
#include <disk_partition.h>
#include <disk-constants.h>
#include <shared/fds-constants.h>

#include "disk_common.h"

#include "disk_inventory.h"
#include "disk_label.h"
#include "disk_label_op.h"
#include "disk_label_mgr.h"
#include "disk_part_mgr.h"
#include "disk_op.h"

#include "disk_plat_module.h"
#include "file_disk_inventory.h"
#include "file_disk_obj.h"
*/

#include "disk_common.h"

namespace fds
{

    DiskCommon::DiskCommon(const std::string &blk) : dsk_blk_path(blk)
    {
    }

    DiskCommon::~DiskCommon()
    {
    }

}  // namespace fds
