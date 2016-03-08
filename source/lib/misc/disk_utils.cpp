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
extern "C" {
#include <sys/mount.h>
#include <fcntl.h>
}

namespace fds {
DiskUtils::CapacityPair
DiskUtils::getDiskConsumedSize(const std::string &mount_path, bool use_stat) {

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

    return DiskUtils::CapacityPair(consumedSize, totalSize);
}

fds_bool_t
DiskUtils::diskFileTest(const std::string& path) {
    int fd = open(path.c_str(), O_RDWR | O_CREAT | O_SYNC, S_IRUSR | S_IWUSR);
    if (fd == -1 || fsync(fd) ||close(fd)) {
        LOGNOTIFY << "File test for disk = " << path << " failed with errno = " << errno;
        return true;
    } else {
        return false;
    }
}

fds_bool_t
DiskUtils::isDiskUnreachable(const std::string& diskPath,
                             const std::string& diskDev,
                             const std::string& mountPnt) {
  std::string path = diskPath + "/.tempFlush";
  bool retVal = DiskUtils::diskFileTest(path);
  std::remove(path.c_str());
  if (mount(diskDev.c_str(), mountPnt.c_str(), "xfs", MS_RDONLY, nullptr)) {
    if (errno == ENODEV) {
      LOGNOTIFY << "Device " << diskDev << ", disk path " << diskPath << " is not accessible ";
      return  (retVal | true);
    }
  } else {
    umount2(mountPnt.c_str(), MNT_FORCE);
  }
  return (retVal | false);
}

/*
 * A function that returns the difference between the diskSet 1 and diskSet 2.
 * The difference is defined as element(s) in diskSet1, but not in diskSet2.
 */
DiskIdSet
DiskUtils::diffDiskSet(const DiskIdSet& diskSet1,
                       const DiskIdSet& diskSet2) {
    DiskIdSet deltaDiskSet;

    std::set_difference(diskSet1.begin(), diskSet1.end(),
                        diskSet2.begin(), diskSet2.end(),
                        std::inserter(deltaDiskSet, deltaDiskSet.begin()));

    return deltaDiskSet;
}

}  // namespace fds
