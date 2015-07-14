/*
 * Copyright 2015 Formation Data Systems, Inc.
 */
#include <set>
#include <sys/statvfs.h>
#include <fiu-control.h>
#include <fiu-local.h>
#include <include/util/disk_utils.h>
#include <stor-mgr/include/SmTypes.h>

namespace fds {
DiskCapacityUtils::capacity_tuple
DiskCapacityUtils::getDiskConsumedSize(const std::string &mount_path) {

    // Cause method to return capacity
    fiu_do_on("diskUtils.cause_used_capacity_error", \
              fiu_disable("diskUtils.cause_used_capacity_alert"); \
              fiu_disable("diskUtils.cause_used_capacity_warn"); \
              LOGDEBUG << "Err injection: (" << DISK_CAPACITY_ERROR_THRESHOLD
              << ", 100). This should cause an alert."; \
              DiskCapacityUtils::capacity_tuple retVals(DISK_CAPACITY_ERROR_THRESHOLD, 100); \
              return retVals; \
    );

    fiu_do_on("diskUtils.cause_used_capacity_alert", \
              fiu_disable("diskUtils.cause_used_capacity_error"); \
              fiu_disable("diskUtils.cause_used_capacity_warn"); \
              LOGDEBUG << "Err injection: (" << DISK_CAPACITY_ALERT_THRESHOLD + 1
              << ", 100). This should cause an alert."; \
              DiskCapacityUtils::capacity_tuple retVals(DISK_CAPACITY_ALERT_THRESHOLD + 1, 100); \
              return retVals; \
    );

    fiu_do_on("diskUtils.cause_used_capacity_warn", \
              fiu_disable("diskUtils.cause_used_capacity_error"); \
              fiu_disable("diskUtils.cause_used_capacity_alert"); \
              LOGDEBUG << "Err injection: (" << DISK_CAPACITY_WARNING_THRESHOLD + 1
              << ", 100). This should cause a warning."; \
              DiskCapacityUtils::capacity_tuple retVals(DISK_CAPACITY_WARNING_THRESHOLD + 1, 100); \
              return retVals; \
    );

    struct statvfs statbuf;
    memset(&statbuf, 0, sizeof(statbuf));

    // Get the total size info for the disk regardless of type
    if (statvfs(mount_path.c_str(), &statbuf) < 0) {
        LOGERROR << "Could not read disk " << mount_path;
    }
    fds_uint64_t totalSize = statbuf.f_blocks * statbuf.f_frsize;
    fds_uint64_t consumedSize = totalSize - (statbuf.f_bfree * statbuf.f_bsize);

    return DiskCapacityUtils::capacity_tuple(consumedSize, totalSize);
}

}  // namespace fds
