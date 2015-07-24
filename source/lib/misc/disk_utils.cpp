/*
 * Copyright 2015 Formation Data Systems, Inc.
 */
#include <set>
#include <sys/statvfs.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fiu-control.h>
#include <fiu-local.h>
#include <include/util/disk_utils.h>
#include <stor-mgr/include/SmTypes.h>

namespace fds {
DiskCapacityUtils::capacity_tuple
DiskCapacityUtils::getDiskConsumedSize(const std::string &mount_path, bool use_stat) {

    // Stat won't give us an accurate total size, so we always want to statvfs to get the totalSize
    // However, if use_stat == true then we need to use stat to get the consumed size
    struct statvfs statbuf;
    memset(&statbuf, 0, sizeof(statbuf));

    // Get the total size info for the disk regardless of type
    if (statvfs(mount_path.c_str(), &statbuf) < 0) {
        LOGERROR << "Could not read disk " << mount_path;
    }
    fds_uint64_t totalSize = statbuf.f_blocks * statbuf.f_frsize;
    fds_uint64_t consumedSize = totalSize - (statbuf.f_bfree * statbuf.f_bsize);


    if (use_stat) {
        struct stat statbuf;
        memset(&statbuf, 0, sizeof(statbuf));

        if (stat(mount_path.c_str(), &statbuf) < 0) {
            LOGERROR << "Could not stat file " << mount_path;
        }
        consumedSize = statbuf.st_size;
        LOGDEBUG << "use_stat was TRUE found " << consumedSize << " as consumed size.";
    }

    return DiskCapacityUtils::capacity_tuple(consumedSize, totalSize);
}

}  // namespace fds
