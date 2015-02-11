/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#define GTEST_USE_OWN_TR1_TUPLE 0

#include <memory>
#include <sstream>
#include <thread>
#include <vector>
#include <fds_module_provider.h>
#include <Tracebuffer.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using ::testing::AtLeast;
using ::testing::Return;
using namespace fds;  // NOLINT

namespace fds {

struct TracebufferPoolProvider : CommonModuleProviderIf {
    explicit TracebufferPoolProvider(int poolEntryCnt) {
        pool_.reset(new TracebufferPool(poolEntryCnt));
    }
    virtual TracebufferPool* getTracebufferPool() {return pool_.get(); }

    std::unique_ptr<TracebufferPool> pool_;
};

/**
* @brief Helper function
*/
void traceFunc() {
    Tracebuffer tb(TB_RINGBUFFER);
    for (uint32_t i = 0; i < MAX_TRACEBUFFER_ENTRY_CNT+5; i++) {
        std::stringstream ss;
        ss << "hello " << i;
        tb.trace(ss.str());
    }
    EXPECT_TRUE(tb.size() == MAX_TRACEBUFFER_ENTRY_CNT);
}

/**
 *
 * 1. simple test case
 * 2. create global pool 100
 * 3. create trace buffer
 * 4. add 15 entries
 * 5. Make sure only last 10 entries are there;
 */
TEST(Tracebuffer, trace1)
{
    MODULEPROVIDER() = new TracebufferPoolProvider(100);
    traceFunc();
    delete MODULEPROVIDER();
}

/**
 * 1. Create pool of 5
 * 2. add 15 entries
 * 3. only last two should show up
 */
TEST(Tracebuffer, trace2)
{
    uint32_t maxTraceCnt = 5;
    MODULEPROVIDER() = new TracebufferPoolProvider(maxTraceCnt);

    Tracebuffer tb(TB_RINGBUFFER);
    for (uint32_t i = 0; i < maxTraceCnt+5; i++) {
        std::stringstream ss;
        ss << "hello " << i;
        tb.trace(ss.str());
    }
    EXPECT_TRUE(tb.size() == maxTraceCnt);

    delete MODULEPROVIDER();
}

/**
 * 1. Create brunch of thread
 * 2. On each thread trace a whole bunch of entries
 * 3. traces entires should be last few entries cached by local trace cache
 */
TEST(Tracebuffer, trace3)
{
    MODULEPROVIDER() = new TracebufferPoolProvider(100);

    std::thread t1(traceFunc);
    std::thread t2(traceFunc);
    std::thread t3(traceFunc);
    std::thread t4(traceFunc);
    t1.join();
    t2.join();
    t3.join();
    t4.join();

    delete MODULEPROVIDER();
}


}  // namespace fds

int main(int argc, char** argv) {
    // The following line must be executed to initialize Google Mock
    // (and Google Test) before running the tests.
    ::testing::InitGoogleMock(&argc, argv);
    return RUN_ALL_TESTS();
}
