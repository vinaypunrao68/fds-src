
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#define GTEST_USE_OWN_TR1_TUPLE 0

#include <fds_assert.h>
#include <concurrency/HashedLocks.hpp>
#include <vector>
#include <unordered_map>
#include <thread>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <iostream>
#include <util/timeutils.h>
#include <testlib/DataGen.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using ::testing::AtLeast;
using ::testing::Return;
using namespace fds;  // NOLINT

namespace fds {


TEST(Serialzer, memcpy)
{
    uint32_t bufSz = 150 << 10;
    uint32_t nBufs = 3;
    uint64_t startTime;
    uint64_t endTime;

    DataGenIf *g = new RandDataGenerator<>(bufSz, bufSz);
    /* Time single buffer copy */
    auto d = g->nextItem();
    std::string serialized(d->size(), '\0');
    startTime = util::getTimeStampNanos();
    memcpy(&serialized[0], &(*d)[0], d->size());
    endTime = util::getTimeStampNanos();
    std::cout << "Single buffer copy time: " << endTime-startTime << std::endl;
    serialized.clear();
    delete g;

    /* Time hashtable copy */
    bufSz = bufSz / nBufs;
    g = new RandDataGenerator<>(bufSz, bufSz);
    uint32_t totalSz = 0;
    uint32_t copyIdx = 0;
    std::unordered_map<int, StringPtr> bufMap;
    for (uint32_t i = 0; i < nBufs; i++) {
        bufMap[i] = g->nextItem();
        totalSz += bufMap[i]->size();
    }

    startTime = util::getTimeStampNanos();
    serialized.resize(totalSz);
    for (uint32_t i = 0; i < nBufs; i++) {
        auto s = bufMap[i];
        memcpy(&serialized[copyIdx], &(*s)[0], s->size());
        copyIdx += s->size();
    }
    endTime = util::getTimeStampNanos();
    std::cout << "Hashtable memcpy time: " << endTime-startTime << std::endl;
    delete g;
}

}  // namespace fds

int main(int argc, char** argv) {
    // The following line must be executed to initialize Google Mock
    // (and Google Test) before running the tests.
    ::testing::InitGoogleMock(&argc, argv);
    return RUN_ALL_TESTS();
}
