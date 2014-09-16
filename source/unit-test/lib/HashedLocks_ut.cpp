
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#define GTEST_USE_OWN_TR1_TUPLE 0

#include <fds_assert.h>
#include <concurrency/HashedLocks.hpp>
#include <vector>
#include <thread>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <iostream>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using ::testing::AtLeast;
using ::testing::Return;
using namespace fds;  // NOLINT

namespace fds {

void lockFunc(HashedLocks<int> &hl)
{
    srand(time(nullptr));
    for (int i = 0; i < 100; i++) {
       ScopedHashedLock<int, HashedLocks<int>> l(hl, i);
       std::cout << "Processing " << i << std::endl;
    }
}

TEST(HashedLocks, lock)
{
    HashedLocks<int> hl(20);
    std::thread t1(std::bind(lockFunc, std::ref(hl)));
    std::thread t2(std::bind(lockFunc, std::ref(hl)));
    std::thread t3(std::bind(lockFunc, std::ref(hl)));
    std::thread t4(std::bind(lockFunc, std::ref(hl)));
    t1.join();
    t2.join();
    t3.join();
    t4.join();
}

}  // namespace fds

int main(int argc, char** argv) {
    // The following line must be executed to initialize Google Mock
    // (and Google Test) before running the tests.
    ::testing::InitGoogleMock(&argc, argv);
    return RUN_ALL_TESTS();
}
