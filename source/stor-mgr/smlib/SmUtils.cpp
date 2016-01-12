/**
 * Copyright 2015 Formation Data Systems, Inc.
 */

#include <SmUtils.h>

namespace fds {

/**
 * This function returns the disk usage information,
 * given the disk mount point path.
 */
Error getDiskUsageInfo(DiskLocMap &diskLocMap,
                       const DiskId &diskId,
                       diskio::DiskStat &diskStat) {
    auto diskPath = diskLocMap[diskId].c_str();
    return getDiskUsageInfo(diskPath, diskStat);
}

Error getDiskUsageInfo(std::string diskPath,
                       diskio::DiskStat &diskStat) {
    Error err(ERR_OK);
    struct statvfs statbuf;
    if (statvfs(diskPath.c_str(), &statbuf) < 0) {
        return fds::Error(fds::ERR_DISK_READ_FAILED);
    }

    diskStat.dsk_tot_size = statbuf.f_blocks * statbuf.f_frsize;
    diskStat.dsk_avail_size = statbuf.f_bfree * statbuf.f_bsize;
    return err;
}

}  // namespace fds
