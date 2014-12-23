/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <limits>

#include "gtest/gtest.h"

#include "util/timeutils.h"

#include "qos_tokbucket.h"

using fds::TokenBucket;
using fds::util::getTimeStampMicros;

class TestingTokenBucket : public TokenBucket {
    public:
        using TokenBucket::rate;
        using TokenBucket::burst;

        using TokenBucket::t_last_update;
        using TokenBucket::token_count;

        TestingTokenBucket(fds_uint64_t rate, fds_uint64_t burst, fds_uint64_t max_burst_us = 0)
                : TokenBucket(rate, burst, max_burst_us) {}
};

TEST(TokenBucket, initInternalInvariantsRateBurst) {
    fds_uint64_t const testRate = 1;
    fds_uint64_t const testBurst = 2;

    TestingTokenBucket testObject(testRate, testBurst);
    EXPECT_EQ(testObject.rate, testRate) << "rate was not initialized properly.";
    EXPECT_EQ(testObject.burst, testBurst) << "burst was not initialized properly.";
    EXPECT_EQ(testObject.token_count, 0.0) << "token_count was not initialized properly.";
    EXPECT_GT(testObject.t_last_update, 0) << "t_last_update was not initialized properly.";
    EXPECT_LE(testObject.t_last_update, getTimeStampMicros())
        << "t_last_update was not initialized properly.";
}

TEST(TokenBucket, initInternalInvariantsMaxBurst) {
    fds_uint64_t const testRate = 1;
    fds_uint64_t const testBurst = testRate * 2;
    fds_uint64_t const testMaxBurstUsExact1Sec = testRate * 1000000;
    fds_uint64_t const testMaxBurstUsTrunc1Sec = testRate * 1000000 + 1;
    fds_uint64_t const testMaxBurstUsTrunc2Sec = testRate * 1000000 * 2 - 1;
    fds_uint64_t const testMaxBurstUsExact2Sec = testRate * 1000000 * 2;
    fds_uint64_t const testMaxBurstUsExact3Sec = testRate * 1000000 * 3;

    {
        TestingTokenBucket testObject(testRate, testBurst, testMaxBurstUsExact1Sec);
        EXPECT_EQ(testObject.burst, testRate) << "burst was not initialized properly to 1 second.";
    }
    {
        TestingTokenBucket testObject(testRate, testBurst, testMaxBurstUsTrunc1Sec);
        EXPECT_EQ(testObject.burst, testRate) << "burst was not initialized properly to 1 second.";
    }
    {
        TestingTokenBucket testObject(testRate, testBurst, testMaxBurstUsTrunc2Sec);
        EXPECT_EQ(testObject.burst, testRate) << "burst was not initialized properly to 1 second.";
    }
    {
        TestingTokenBucket testObject(testRate, testBurst, testMaxBurstUsExact2Sec);
        EXPECT_EQ(testObject.burst, testRate * 2)
            << "burst was not initialized properly to 2 seconds.";
    }
    {
        TestingTokenBucket testObject(testRate, testBurst, testMaxBurstUsExact3Sec);
        EXPECT_EQ(testObject.burst, testBurst)
            << "burst was not initialized properly to burst parameter.";
    }
}

TEST(TokenBucket, modifyParams) {
    fds_uint64_t const initialRate = 1;
    fds_uint64_t const initialBurst = 1;
    fds_uint64_t const updatedRate = initialRate * 10;
    fds_uint64_t const updatedBurst = updatedRate * 2;
    size_t const secondsToSimulate = 1;

    TestingTokenBucket testObject(initialRate, initialBurst);

    testObject.t_last_update = 0;
    testObject.modifyParams(updatedRate, updatedBurst, secondsToSimulate * 1000000);

    double expectedTokens = initialRate * secondsToSimulate;
    EXPECT_EQ(testObject.token_count, expectedTokens)
        << "tokens not correctly accumulated prior to update.";

    EXPECT_EQ(testObject.rate, updatedRate) << "rate was not updated properly.";
    EXPECT_EQ(testObject.burst, updatedBurst) << "burst was not updated properly.";
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
