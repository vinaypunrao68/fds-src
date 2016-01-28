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
 private:
  /**
   * Class for storing capacity information. Templated to allow for atomic capacity information when needed, and
   * non-atomic when it isn't.
   */
  template<typename T>
  class DiskCapacityPair {
   public:
    DiskCapacityPair(T used, fds_uint64_t total): usedCapacity(used), totalCapacity(total) {}
    DiskCapacityPair(): usedCapacity(0), totalCapacity(0) {}
    T usedCapacity;
    fds_uint64_t totalCapacity;
  };

 public:
  // Type for storing (used capacity, total capacity) pairs
  // Capacity pair where usedCapacity is atomic
  typedef DiskCapacityPair<std::atomic<unsigned long long>> AtomicCapacityPair;
  // Non-atomic version
  typedef DiskCapacityPair<fds_uint64_t> CapacityPair;

  static CapacityPair getDiskConsumedSize(const std::string&  mount_path, bool use_stat = false);
  static fds_bool_t diskFileTest(const std::string& path);
  static fds_bool_t isDiskUnreachable(const std::string& diskPath,
                                      const std::string& diskDev,
                                      const std::string& mountPnt);
};

}  // namespace fds

#endif //SOURCE_INCLUDE_UTIL_DISK_UTILS_H_
