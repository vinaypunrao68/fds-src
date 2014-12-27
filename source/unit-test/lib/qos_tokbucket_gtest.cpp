/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <limits>
#include <string>

#include "gtest/gtest.h"

#include "util/timeutils.h"

#include "lib/qos_tokbucket.h"

using std::string;
using std::to_string;

using ::testing::InitGoogleTest;
using ::testing::TestWithParam;
using ::testing::Values;

using fds::util::getTimeStampMicros;

class TestingTokenBucket : public fds::TokenBucket {
    public:
        using fds::TokenBucket::rate;
        using fds::TokenBucket::burst;

        using fds::TokenBucket::t_last_update;
        using fds::TokenBucket::token_count;

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

TEST(TokenBucket, modifyRateWithBurstInMicroseconds) {
    fds_uint64_t const initialRate = 1;
    fds_uint64_t const initialBurst = 1;
    fds_uint64_t const updatedRate = initialRate * 17;
    fds_uint64_t const updatedBurstDuration = 2 * 1000000;  // 2 seconds.
    fds_uint64_t const updatedBurst = updatedRate * updatedBurstDuration / 1000000;

    TestingTokenBucket testObject(initialRate, initialBurst);

    testObject.modifyRate(updatedRate, updatedBurstDuration);

    EXPECT_EQ(testObject.rate, updatedRate) << "rate was not updated properly.";
    EXPECT_EQ(testObject.burst, updatedBurst) << "burst was not updated properly.";
}

TEST(TokenBucket, modifyRate) {
    fds_uint64_t const initialRate = 1;
    fds_uint64_t const burst = 1;
    fds_uint64_t const updatedRate = initialRate * 23;

    TestingTokenBucket testObject(initialRate, burst);

    testObject.modifyRate(updatedRate);

    EXPECT_EQ(testObject.rate, updatedRate) << "rate was not updated properly.";
    EXPECT_EQ(testObject.burst, burst) << "burst was modified inappropriately.";
}

#if GTEST_HAS_PARAM_TEST
struct TokenBucketState {
    fds_uint64_t rate;
    fds_uint64_t burst;
    fds_uint64_t t_last_update;
    double token_count;
};

struct TokenBucketTestParams {
    TokenBucketState oldState;
    TokenBucketState newState;
    TokenBucketTestParams(TokenBucketState oldState, TokenBucketState newState)
            : oldState(oldState), newState(newState) { }
};

struct TokenBucketConsumeParams : public TokenBucketTestParams {
    bool expectSuccess;
    fds_uint32_t consume;
    TokenBucketConsumeParams(TokenBucketState oldState,
                             TokenBucketState newState,
                             bool expectSuccess,
                             fds_uint32_t consume)
            : TokenBucketTestParams(oldState, newState),
              expectSuccess(expectSuccess),
              consume(consume) { }
};

class TokenBucketConsume : public TestWithParam<TokenBucketConsumeParams> {
};

template <class T> string pluralize(T value, string singular, string plural) {
    return to_string(value) + " " + (value == 1 ? singular : plural);
}

TEST_P(TokenBucketConsume, tryToConsumeTokens) {
    auto param = GetParam();
    fds_uint64_t const rate = 1;
    fds_uint64_t const burst = 1;
    double testTokens;
    fds_uint32_t consumeTokens;

    TestingTokenBucket testObject(param.oldState.rate, param.oldState.burst);
    testObject.token_count = param.oldState.token_count;

    if (param.expectSuccess) {
        EXPECT_TRUE(testObject.tryToConsumeTokens(param.consume))
                << "Unable to consume " << pluralize(param.consume, "token", "tokens")
                << " from " << testObject.token_count;
        EXPECT_EQ(testObject.token_count, param.newState.token_count)
                << "Incorrect number of tokens remaining after "
                << pluralize(param.consume, "token", "tokens") << " was removed from "
                << param.oldState.token_count << ".";
    } else {
        EXPECT_FALSE(testObject.tryToConsumeTokens(param.consume))
                << pluralize(param.consume, "token", "tokens") << " was allowed to be consumed "
                << "from " << param.oldState.token_count;
        EXPECT_EQ(testObject.token_count, param.oldState.token_count)
                << "Token count should not be modified when the full count cannot be consumed.";
    }
}
#endif  // GTEST_HAS_PARAM_TEST

#if GTEST_HAS_PARAM_TEST
INSTANTIATE_TEST_CASE_P(ManualEdges, TokenBucketConsume, ::testing::Values(
        TokenBucketConsumeParams {
            TokenBucketState { 1, 1, 0, 0.0 },
            TokenBucketState { 1, 1, 0, 0.0 },
            false, 1
        },
        TokenBucketConsumeParams {
            TokenBucketState { 1, 1, 0, 1.0 },
            TokenBucketState { 1, 1, 0, 0.0 },
            true, 1
        }
));
#endif

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
