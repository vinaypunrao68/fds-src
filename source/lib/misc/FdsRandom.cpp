/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <FdsRandom.h>
#include <chrono>

namespace fds {

RandNumGenerator::RandNumGenerator(fds_uint64_t seed) {
    generator.seed(seed);
}

RandNumGenerator::~RandNumGenerator() {
}

fds_uint64_t
RandNumGenerator::genNumSafe() {
    fds_scoped_lock scopeLock(rngLock);
    return genNum();
}

fds_uint64_t
RandNumGenerator::genNum() {
    return generator();
}

fds_uint64_t
RandNumGenerator::getRandSeed() {
    // This seed implementation returns the current
    // time since epoch
    auto dur = std::chrono::high_resolution_clock::now().time_since_epoch();
    return dur.count();
}

}  // namespace fds
