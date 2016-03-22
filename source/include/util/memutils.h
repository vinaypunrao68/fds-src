/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#ifndef SOURCE_INCLUDE_UTIL_MEMUTILS_H_
#define SOURCE_INCLUDE_UTIL_MEMUTILS_H_

#include <shared/fds_types.h>
namespace fds {
namespace util {

// get the current memory usage in Kilo Bytes
fds_uint64_t getMemoryKB();

} // namespace util
} // namespace fds

#endif // SOURCE_INCLUDE_UTIL_MEMUTILS_H_
