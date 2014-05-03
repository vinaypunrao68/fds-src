/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#define GTEST_USE_OWN_TR1_TUPLE 0

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <RpcRequest.h>

using ::testing::AtLeast;
using ::testing::Return;
using namespace fds;  // NOLINT

namespace fds_test {
/**
 * Tests SyncLog add
 */
TEST(AsyncRpcRequest, invoke) {
    // EXPECT_TRUE(mdSet.size() == syncLog.size());
}

}  // namespace fds_test

int main(int argc, char** argv) {
    // The following line must be executed to initialize Google Mock
    // (and Google Test) before running the tests.
    ::testing::InitGoogleMock(&argc, argv);
    return RUN_ALL_TESTS();
}
