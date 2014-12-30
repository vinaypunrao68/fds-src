/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#include <string>

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
