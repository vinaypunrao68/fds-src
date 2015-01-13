/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <limits>
#include <string>
#include <ostream>

#include "gtest/gtest.h"

#include "util/timeutils.h"

#include "lib/qos_tokbucket.h"

using std::cout;
using std::numeric_limits;
using std::ostream;
using std::string;
using std::to_string;

using ::testing::InitGoogleTest;
using ::testing::TestWithParam;
using ::testing::Values;

using fds::util::getTimeStampMicros;

class TestingTokenBucket : public fds::TokenBucket {
    public:
        using fds::TokenBucket::rate_;
        using fds::TokenBucket::burst_;

        using fds::TokenBucket::t_last_update_;
        using fds::TokenBucket::token_count_;

        using fds::TokenBucket::updateTokensOnly;
        using fds::TokenBucket::expireTokens;

        TestingTokenBucket(fds_uint64_t rate, fds_uint64_t burst, fds_uint64_t max_burst_us = 0)
                : TokenBucket(rate, burst, max_burst_us) {}
};

TEST(TokenBucket, initInternalInvariantsRateBurst) {
    fds_uint64_t const testRate = 1;
    fds_uint64_t const testBurst = 2;

    TestingTokenBucket testObject(testRate, testBurst);
    EXPECT_EQ(testRate, testObject.rate_) << "rate was not initialized properly.";
    EXPECT_EQ(testBurst, testObject.burst_) << "burst was not initialized properly.";
    EXPECT_EQ(0, testObject.token_count_) << "token_count was not initialized properly.";
    EXPECT_LT(0, testObject.t_last_update_) << "t_last_update was not initialized properly.";
    EXPECT_GE(getTimeStampMicros(), testObject.t_last_update_)
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
        EXPECT_EQ(testRate, testObject.burst_) << "burst was not initialized properly to 1 second.";
    }
    {
        TestingTokenBucket testObject(testRate, testBurst, testMaxBurstUsTrunc1Sec);
        EXPECT_EQ(testRate, testObject.burst_) << "burst was not initialized properly to 1 second.";
    }
    {
        TestingTokenBucket testObject(testRate, testBurst, testMaxBurstUsTrunc2Sec);
        EXPECT_EQ(testRate, testObject.burst_) << "burst was not initialized properly to 1 second.";
    }
    {
        TestingTokenBucket testObject(testRate, testBurst, testMaxBurstUsExact2Sec);
        EXPECT_EQ(testRate * 2, testObject.burst_)
            << "burst was not initialized properly to 2 seconds.";
    }
    {
        TestingTokenBucket testObject(testRate, testBurst, testMaxBurstUsExact3Sec);
        EXPECT_EQ(testBurst, testObject.burst_)
            << "burst was not initialized properly to burst parameter.";
    }
}

TEST(TokenBucket, initPreconditions) {
    fds_uint64_t const testRateMax = TestingTokenBucket::MAX_RATE;
    fds_uint64_t const testRateOverMax = testRateMax + 1;
    fds_uint64_t const testBurst = 1;

    {
        TestingTokenBucket testObject(testRateMax, testBurst);
        EXPECT_EQ(testRateMax, testObject.rate_) << "rate was not initialized properly.";
    }
    {
        EXPECT_DEATH(TestingTokenBucket testObject(testRateOverMax, testBurst), "");
    }
}

TEST(TokenBucket, modifyParams) {
    fds_uint64_t const initialRate = 1;
    fds_uint64_t const initialBurst = 1;
    fds_uint64_t const updatedRate = initialRate * 10;
    fds_uint64_t const updatedBurst = updatedRate * 2;
    fds_uint64_t const secondsToSimulate = 1;

    TestingTokenBucket testObject(initialRate, initialBurst);

    testObject.t_last_update_ = 0;
    testObject.modifyParams(updatedRate, updatedBurst, secondsToSimulate * 1000000);

    fds_uint64_t expectedTokens = initialRate * secondsToSimulate;
    EXPECT_EQ(expectedTokens, testObject.token_count_)
        << "tokens not correctly accumulated prior to update.";

    EXPECT_EQ(updatedRate, testObject.rate_) << "rate was not updated properly.";
    EXPECT_EQ(updatedBurst, testObject.burst_) << "burst was not updated properly.";
}

TEST(TokenBucket, modifyRateWithBurstInMicroseconds) {
    fds_uint64_t const initialRate = 1;
    fds_uint64_t const initialBurst = 1;
    fds_uint64_t const updatedRate = initialRate * 17;
    fds_uint64_t const updatedBurstDuration = 2 * 1000000;  // 2 seconds.
    fds_uint64_t const updatedBurst = updatedRate * updatedBurstDuration / 1000000;

    TestingTokenBucket testObject(initialRate, initialBurst);

    testObject.modifyRate(updatedRate, updatedBurstDuration);

    EXPECT_EQ(updatedRate, testObject.rate_) << "rate was not updated properly.";
    EXPECT_EQ(updatedBurst, testObject.burst_) << "burst was not updated properly.";
}

TEST(TokenBucket, modifyRate) {
    fds_uint64_t const initialRate = 1;
    fds_uint64_t const burst = 1;
    fds_uint64_t const updatedRate = initialRate * 23;

    TestingTokenBucket testObject(initialRate, burst);

    testObject.modifyRate(updatedRate);

    EXPECT_EQ(updatedRate, testObject.rate_) << "rate was not updated properly.";
    EXPECT_EQ(burst, testObject.burst_) << "burst was modified inappropriately.";
}

TEST(TokenBucket, expireTokens) {
    fds_uint64_t const burst = 2;
    fds_uint64_t const overBurstTokens = 3;
    fds_uint64_t const initialTokens = burst + overBurstTokens;
    fds_uint64_t const expectedExpiredTokens = initialTokens - burst;

    TestingTokenBucket testObject(1, burst);
    testObject.token_count_ = initialTokens;

    auto expiredTokens = testObject.expireTokens();

    EXPECT_EQ(testObject.burst_, testObject.token_count_) << "token_count was not updated properly.";
    EXPECT_EQ(expectedExpiredTokens, expiredTokens) << "incorrect number of tokens expired.";
}

#if GTEST_HAS_PARAM_TEST
struct TokenBucketState {
    fds_uint64_t rate;
    fds_uint64_t burst;
    fds_uint64_t t_last_update;
    fds_uint64_t token_count;
};
ostream& operator <<(ostream& os, TokenBucketState const& rhs) {
    return os << "TokenBucketState("
              << "rate: " << rhs.rate
              << ", burst: " << rhs.burst
              << ", t_last_update: " << rhs.t_last_update
              << ", token_count: " << rhs.token_count
              << ")";
}

struct TokenBucketTestParams {
    TokenBucketState oldState;
    TokenBucketState newState;
    TokenBucketTestParams(TokenBucketState oldState, TokenBucketState newState)
            : oldState(oldState), newState(newState) { }
};
ostream& operator <<(ostream& os, TokenBucketTestParams const& rhs) {
    return os << "TokenBucketTestParams("
              << "oldState: " << rhs.oldState
              << ", newState: " << rhs.newState
              << ")";
}

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
ostream& operator <<(ostream& os, TokenBucketConsumeParams const& rhs) {
    return os << "TokenBucketConsumeParams("
              << "oldState: " << rhs.oldState
              << ", newState: " << rhs.newState
              << ", expectSuccess: " << rhs.expectSuccess
              << ", consume: " << rhs.consume
              << ")";
}

struct UpdateTokensOnlyParams : public TokenBucketTestParams {
    bool expectCrash;
    fds_uint64_t nowMicrosec;
    UpdateTokensOnlyParams(TokenBucketState oldState,
                           TokenBucketState newState,
                           fds_uint64_t nowMicrosec,
                           bool expectCrash)
            : TokenBucketTestParams(oldState, newState),
              nowMicrosec(nowMicrosec), expectCrash(expectCrash) { }
};
ostream& operator<<(ostream& os, UpdateTokensOnlyParams const& rhs) {
    return os << "UpdateTokensOnlyParams("
              << "oldState: " << rhs.oldState
              << "newState: " << rhs.newState
              << "nowMicrosec: " << rhs.nowMicrosec
              << "expectCrash: " << rhs.expectCrash
              << ")";
}

class TokenBucketConsume : public TestWithParam<TokenBucketConsumeParams> { };
class TokenBucketUpdate : public TestWithParam<UpdateTokensOnlyParams> { };

template <class T> string pluralize(T value, string singular, string plural) {
    return to_string(value) + " " + (value == 1 ? singular : plural);
}

TEST_P(TokenBucketConsume, tryToConsumeTokens) {
    auto param = GetParam();

    TestingTokenBucket testObject(param.oldState.rate, param.oldState.burst);
    testObject.token_count_ = param.oldState.token_count;

    if (param.expectSuccess) {
        EXPECT_TRUE(testObject.tryToConsumeTokens(param.consume))
                << "Unable to consume " << pluralize(param.consume, "token", "tokens")
                << " from " << testObject.token_count_;
        EXPECT_EQ(param.newState.token_count, testObject.token_count_)
                << "Incorrect number of tokens remaining after "
                << pluralize(param.consume, "token", "tokens") << " was removed from "
                << param.oldState.token_count << ".";
    } else {
        EXPECT_FALSE(testObject.tryToConsumeTokens(param.consume))
                << pluralize(param.consume, "token", "tokens") << " was allowed to be consumed "
                << "from " << param.oldState.token_count;
        EXPECT_EQ(param.oldState.token_count, testObject.token_count_)
                << "Token count should not be modified when the full count cannot be consumed.";
    }
}

INSTANTIATE_TEST_CASE_P(ManualEdges, TokenBucketConsume, ::testing::Values(
        // Simple failure case.
        TokenBucketConsumeParams {
            TokenBucketState { 1, 1, 0, 0 },
            TokenBucketState { 1, 1, 0, 0 },
            false, 1
        },
        // Simple success case.
        TokenBucketConsumeParams {
            TokenBucketState { 1, 1, 0, 1 },
            TokenBucketState { 1, 1, 0, 0 },
            true, 1
        }
));

TEST_P(TokenBucketUpdate, updateTokensOnly) {
    auto param = GetParam();

    TestingTokenBucket testObject(param.oldState.rate, param.oldState.burst);
    testObject.token_count_ = param.oldState.token_count;
    testObject.t_last_update_ = param.oldState.t_last_update;

    if (param.expectCrash) {
        EXPECT_DEATH(testObject.updateTokensOnly(param.nowMicrosec), "");
    }
    else
    {
        testObject.updateTokensOnly(param.nowMicrosec);

        EXPECT_EQ(param.newState.token_count, testObject.token_count_)
                << "token_count was not updated properly.";
        EXPECT_EQ(param.newState.t_last_update, testObject.t_last_update_)
                << "t_last_update was not updated properly.";
    }
}

INSTANTIATE_TEST_CASE_P(ManualEdges, TokenBucketUpdate, ::testing::Values(
    // Simple 1 token accumulation.
    UpdateTokensOnlyParams {
        TokenBucketState { 1, 1, 0, 0 },
        TokenBucketState { 1, 1, 1000000, 1 },
        1000000, false
    },
    // One microsecond away from getting a token.
    UpdateTokensOnlyParams {
        TokenBucketState { 1, 1, 0, 0 },
        TokenBucketState { 1, 1, 0, 0 },
        999999, false
    },
    // One microsecond past getting a token.
    UpdateTokensOnlyParams {
        TokenBucketState { 1, 1, 0, 0 },
        TokenBucketState { 1, 1, 1000000, 1 },
        1000001, false
    },
    // Try primes!
    UpdateTokensOnlyParams {
        TokenBucketState { 13, 1, 1298173, 0 },
        TokenBucketState { 13, 1, 2528942, 16 },
        1298173 + 1299709, false
    },
    // Just under new_tokens boundary.
    UpdateTokensOnlyParams {
        TokenBucketState {
            2836651403,  // Big number, under MAX_RATE, that divides evenly into
                         // ULONG_MAX / 1000000, the max theoretical # of tokens
            numeric_limits<fds_uint64_t>::max(),
            0,
            0
        },
        TokenBucketState {
            2836651403,
            numeric_limits<fds_uint64_t>::max(),
            numeric_limits<fds_uint64_t>::max() / 2836651403,
            numeric_limits<fds_uint64_t>::max() / 1000000
        },
        numeric_limits<fds_uint64_t>::max() / 2836651403, false
    },
    // Just over the new_tokens boundary.
    UpdateTokensOnlyParams {
        TokenBucketState {
            2836651403,
            numeric_limits<fds_uint64_t>::max(),
            0,
            0
        },
        TokenBucketState {
            2836651403,
            numeric_limits<fds_uint64_t>::max(),
            0,
            0
        },
        numeric_limits<fds_uint64_t>::max() / 2836651403 + 1, true
    },
    // Just under the token_count boundary.
    UpdateTokensOnlyParams {
        TokenBucketState {
            1, numeric_limits<fds_uint64_t>::max(),
            0, numeric_limits<fds_uint64_t>::max() - 1
        },
        TokenBucketState {
            1, numeric_limits<fds_uint64_t>::max(),
            1000000, numeric_limits<fds_uint64_t>::max()
        },
        1000000, false
    },
    // Just over the token_count boundary.
    UpdateTokensOnlyParams {
        TokenBucketState {
            1, numeric_limits<fds_uint64_t>::max(),
            0, numeric_limits<fds_uint64_t>::max()
        },
        TokenBucketState {
            1, numeric_limits<fds_uint64_t>::max(),
            0, numeric_limits<fds_uint64_t>::max()
        },
        1000000, true
    },
    // Make sure going back in time is caught.
    UpdateTokensOnlyParams {
        TokenBucketState { 1, 1, 1, 0 },
        TokenBucketState { 1, 1, 1, 0 },
        0, true
    }
));
#endif

int main(int argc, char** argv) {
    ::testing::FLAGS_gtest_death_test_style = "threadsafe";

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
