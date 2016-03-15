/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#include <sys/resource.h>
#include <util/memutils.h>

namespace fds {
namespace util {

fds_uint64_t getMemoryKB() {
    struct rusage usage;
    getrusage (RUSAGE_SELF, &usage);
    return usage.ru_maxrss;
}

}  // namespace util
}  // namespace fds
