/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

#ifndef SOURCE_INCLUDE_UTIL_DISK_UTILS_H_
#define SOURCE_INCLUDE_UTIL_DISK_UTILS_H_

#include "fds_types.h"

namespace fds {

// Disk capacity Thresholds
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
