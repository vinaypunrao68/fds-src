/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

#ifndef SOURCE_INCLUDE_UTIL_DISK_UTILS_H_
#define SOURCE_INCLUDE_UTIL_DISK_UTILS_H_

#include "fds_types.h"

namespace fds {

// Disk capacity Thresholds
// We should define DISK_CAPACITY_ERROR_THRESHOLD as:
// (disk_capacity - (((number_bits_per_token / number_disks) / disk_capacity) * 2)) / disk_capacity
// Should also likely add a ~2% buffer for safety
// This will give us a value that is 2x the maximum size of a token file to allow for compaction in the worst case
// Subsequent thresholds should be set as WARNING - 10 and WARNING - 20




#define DISK_CAPACITY_WARNING_THRESHOLD 75.0
#define DISK_CAPACITY_ALERT_THRESHOLD 85.0
#define DISK_CAPACITY_ERROR_THRESHOLD 95.0

class DiskUtils {

public:
    // Type for storing (used capacity, total capacity) pairs
    typedef std::pair<fds_uint64_t, fds_uint64_t> capacity_tuple;

    static capacity_tuple getDiskConsumedSize(const std::string&  mount_path, bool use_stat = false);
    static fds_bool_t diskFileTest(const std::string& path);
    static fds_bool_t isDiskUnreachable(const std::string& diskPath,
                                        const std::string& diskDev,
                                        const std::string& mountPnt);
};

}  // namespace fds

#endif //SOURCE_INCLUDE_UTIL_DISK_UTILS_H_
