/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#define GTEST_USE_OWN_TR1_TUPLE 0
#include <cstdlib>
#include <ctime>
#include <set>
#include <iostream>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <concurrency/ThreadPool.h>
#include <concurrency/SynchronizedTaskExecutor.hpp>

using ::testing::AtLeast;
using ::testing::Return;
using namespace fds;  // NOLINT

namespace fds_test {
std::unordered_map<int, std::atomic<bool>> inprogMap;
std::atomic<int> completedCnt;

void taskFunc(int qid, int seqId)
{
    // std::cout << "qid: " << qid << " seqId: " << seqId << std::endl;
    fds_assert(inprogMap[qid] == false)

    inprogMap[qid] = true;

    for (uint32_t i = 0; i < 100000; i++) {}

    inprogMap[qid] = false;
    completedCnt++;
}

/**
 * Tests schedule function of SynchronizedTaskExecutor
 */
TEST(SynchronizedTaskExecutor, schedule) {
    int nQCnt = 20;
    int nReqs = nQCnt * 1500;
    completedCnt = 0;
    fds::fds_threadpool tp;
    fds::SynchronizedTaskExecutor<int> executor(tp);

    for (int i = 0; i < nQCnt; i++) {
        inprogMap[i] = false;
    }

    for (int i = 0; i < nReqs; i++) {
        auto qid = i%nQCnt;
        executor.schedule(qid, std::bind(taskFunc, qid, i));
    }

    while (completedCnt < nReqs) {
        sleep(1);
    }
}

}  // namespace fds_test

int main(int argc, char** argv) {
    // The following line must be executed to initialize Google Mock
    // (and Google Test) before running the tests.
    ::testing::InitGoogleMock(&argc, argv);
    return RUN_ALL_TESTS();
}
