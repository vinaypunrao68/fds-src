/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#include <sys/resource.h>
#include <util/memutils.h>

namespace fds {
namespace util {

fds_uint64_t getMemBytes() {
    struct rusage usage;
    getrusage (RUSAGE_SELF, &usage);
    return usage.ru_maxrss * 1024;
}

}  // namespace util
}  // namespace fds
