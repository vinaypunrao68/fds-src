/// @file
/// @copyright  2013 Formation Data Systems, Inc.
///
/// Basic Token Bucket implementation.
///
/// Accumulates tokens at a specified rate into a bucket of a specified capacity. Once the bucket is
/// full, no more tokens may be accumulated until tokens are removed.
///
/// @remark  Currently using microsecond-granularity times, so this will not support more than 1M
/// @remark  IOPS.
///
#ifndef SOURCE_LIB_QOS_TOKBUCKET_H_
#define SOURCE_LIB_QOS_TOKBUCKET_H_

#include <algorithm>
#include <limits>
#include <typeinfo>

#include "fds_assert.h"
#include "fds_types.h"
#include "util/timeutils.h"

namespace fds {

///
/// Demand-driven token bucket.
///
/// For use by dispatchers that will drive the token bucket state, or as a building block for more
/// complex token buckets.
///
/// Tokens are accumulated at a rate of ::rate_ tokens per second. The maximum number of tokens that
/// may accumulate at one time is specified by ::burst_.
///
class TokenBucket {
public:
    ///
    /// Maximum token accumulation rate, set to allow 100 years of accumulation before overflow.
    /// This still allows 5,861,588,978 tokens per second.
    ///
    static fds_uint64_t const MAX_RATE =
            std::numeric_limits<fds_uint64_t>::max() / (  // Distribute max holdable tokens over
                (100ull         // years
                * 365ull        // to days
                + 24ull)        // and leap days in 100 years
                * 24ull         // to hours
                * 60ull         // to minutes
                * 60ull);       // to seconds

    ///
    /// Constructor.
    ///
    /// @param  rate          The number of tokens accumulated per second. Must not be greater than
    ///                         ::MAX_RATE.
    /// @param  burst         The maximum number of tokens that can accumulate at one time.
    /// @param  max_burst_us  @p burst will be clamped to the number of tokens that accumulate in
    ///                         this many microseconds.
    ///
    TokenBucket(fds_uint64_t rate, fds_uint64_t burst, fds_uint64_t max_burst_us = 0)
            : rate_(rate), burst_(burst) {
        if (rate > MAX_RATE) {
            fds_panic("Rate %llu cannot be greater than MAX_RATE %llu.", rate, MAX_RATE);
        }
        if (rate_ == 0) {
            // set to 1, since we are dividing by rate_
            rate_ = 1;
        }

        if (max_burst_us > 0) {
            TokenBucket::burst_ = std::min(
                    // TODO(DAC): Is truncation the desired behavior?
                    max_burst_us * rate / 1000000, burst);
        }
        t_last_update_ = util::getTimeStampMicros();  // current time in microseconds
        token_count_ = 0;
    }

    ///
    /// Destructor.
    ///
    ~TokenBucket() {}

    ///
    /// Get the number of tokens that are accumulated per second.
    ///
    /// @return  The current property value.
    ///
    inline fds_uint64_t getRate() const {
        return rate_;
    }

    ///
    /// Check whether there are at least a specified number of tokens accumulated.
    ///
    /// @param  num_tokens  The number of tokens to check for.
    ///
    /// @return  Whether there are @p num_tokens currently in the bucket.
    ///
    inline fds_bool_t hasTokens(fds_uint32_t num_tokens) const {
        return num_tokens <= token_count_;
    }

    ///
    /// Change the ::rate_ and ::burst_ parameters at the same time.
    ///
    /// Tokens since the last update will be accumulated at the old rate, but will not be clamped to
    /// either the new or old ::burst_.
    ///
    /// @param  rate         The new number of tokens to be accumulated per second.
    /// @param  burst        The new maximum number of tokens that may be accumulated.
    /// @param  nowMicrosec  The current number of microseconds since epoch.
    ///
    inline void modifyParams(fds_uint64_t rate, fds_uint64_t burst, fds_uint64_t nowMicrosec = 0) {
        if (nowMicrosec == 0) {
            nowMicrosec = util::getTimeStampMicros();
        }
        // FIXME(DAC): Tokens are accumulated with the old rate, but will be clamped with the new
        //             burst. This could be problematic when, for example, lowering the rate and
        //             raising the burst, many more tokens than what is probably expected will be
        //             present in the bucket.
        updateTokensOnly(nowMicrosec);

        TokenBucket::rate_ = rate;
        if (TokenBucket::rate_ == 0) {
            TokenBucket::rate_ = 1;   // minimum value that is not 0, since we divide by rate_
        }
        TokenBucket::burst_ = burst;
    }

    ///
    /// Change the ::rate_ and ::burst_ parameters at the same time.
    ///
    /// Tokens since the last update will be accumulated at the new rate, but will not be clamped to
    /// either the new or old ::burst_.
    ///
    /// @param  rate      The new number of tokens to be accumulated per second.
    /// @param  burst_us  ::burst_ will be set so that, at @p rate, this bucket will fill in
    ///                     @p burst_us microseconds.
    ///
    inline void modifyRate(fds_uint64_t rate, fds_uint64_t burst_us) {
        modifyParams(rate, burst_us * rate / 1000000);
    }

    ///
    /// Change the ::rate_ parameter.
    ///
    /// Tokens since the last update will be accumulated at the new rate, but will not be clamped to
    /// ::_burst.
    ///
    /// @param  rate  The new number of tokens to be accumulated per second.
    ///
    inline void modifyRate(fds_uint64_t rate) {
        modifyParams(rate, burst_);
    }

    ///
    /// Attempt to consume a specified number of tokens.
    ///
    /// If this bucket does not have sufficient tokens to fulfill the requested number, no tokens
    /// will be consumed.
    ///
    /// @pre  updateTBState() must be called if the state (number of accumulated tokens) is not
    ///       current.
    ///
    /// @param  num_tokens  The number of tokens to attempt to consume.
    ///
    /// @return  Whether the requested number of tokens were consumed.
    ///
    inline fds_bool_t tryToConsumeTokens(fds_uint32_t num_tokens) {
        if (num_tokens <= token_count_) {
            token_count_ -= num_tokens;
            return true;
        }
        return false;
    }

    ///
    /// Get the number of microseconds it will take for a specified number of tokens to be
    /// available.
    ///
    /// @pre  updateTBState() must be called if the state (number of accumulated tokens) is not
    ///       current.
    ///
    /// @param  num_tokens  The number of tokens to get the delay for.
    ///
    /// @return  The number of microseconds before @p num_tokens are available.
    ///
    inline fds_uint64_t getDelayMicrosec(fds_uint32_t num_tokens) {
        if (token_count_ < num_tokens) {
            fds_uint64_t needed_tokens = num_tokens - token_count_;
            fds_uint64_t delay_microsec = needed_tokens * 1000000 / rate_;
            return delay_microsec;
        }
        return 0; /* otherwise enough tokens */
    }

    ///
    /// Update the number of tokens in the bucket.
    ///
    /// @param  nowMicrosec  The current number of microseconds since epoch.
    ///
    /// @return  The number of tokens that spilled over ::burst_.
    ///
    inline fds_uint64_t updateTBState(fds_uint64_t nowMicrosec) {
        updateTokensOnly(nowMicrosec);
        auto expired_tokens = expireTokens();
        return expired_tokens;
    }

protected:
    ///
    /// Accumulate tokens without clamping to ::burst_.
    ///
    /// @param  nowMicrosec  The current number of microseconds since epoch.
    ///
    inline void updateTokensOnly(fds_uint64_t nowMicrosec) {
        // Going back in time is not supported. Add it if you need it, but the code hasn't been
        // designed with that in mind.
        fds_verify(nowMicrosec >= t_last_update_);

        // FIXME(DAC): Should we freak out if we go back in time?
        fds_uint64_t elapsed_microsec = nowMicrosec - t_last_update_;
        // FIXME(DAC): Should we only do this if we're actually getting more tokens?
        if (elapsed_microsec > 0) {

            // Crash on overflow. We don't do >= rate / 1000000 because the intermediate result is
            // where we can get overflow.
            fds_verify(std::numeric_limits<fds_uint64_t>::max() / elapsed_microsec >= rate_);
            fds_uint64_t new_tokens = elapsed_microsec * rate_ / 1000000;

            // Very low likelihood of an overflow here, but if a misconfiguration or other special
            // circumstances lead to > 100 years passing (see MAX_RATE), at least the error won't
            // cause something hard to debug.
            fds_verify(std::numeric_limits<fds_uint64_t>::max() - token_count_ >= new_tokens);
            token_count_ += new_tokens;

            // token_microsec will be strictly less than or equal to elapsed_microsec, which can't
            // possibly cause overflow since it's calculated as the difference between two unsigned
            // fds_uint64_t. The new_tokens multiplication can't itself overflow due to the check
            // on MAX_RATE.
            auto token_microsec = new_tokens * 1000000 / rate_;
            t_last_update_ += token_microsec;
        }
    }

    ///
    /// Clamp the number of tokens in this bucket to ::burst_.
    ///
    /// @return  The number of tokens that were expired.
    ///
    inline fds_uint64_t expireTokens() {
        fds_uint64_t expired_tokens = 0;

        if (token_count_ > burst_) {
            expired_tokens = token_count_ - burst_;
            token_count_ -= expired_tokens;
        }

        return expired_tokens;
    }

protected:
public:
    /// @defgroup  Config  Configurable parameters.
    ///@{
    fds_uint64_t rate_;   ///< Number of tokens accumulated per second.
    fds_uint64_t burst_;  ///< Maximum number of tokens that this bucket can accumulate.
    ///@}

    /// @defgroup  State  Dynamic state.
    ///@{
    fds_uint64_t t_last_update_;  ///< Microseconds since epoch on which this bucket last
                                  ///< accumulated tokens.
    fds_uint64_t token_count_;    ///< Number of tokens currently in the bucket.
    ///@}
};

///
/// Demand-driven token bucket that can receive tokens as well as generating them at the configured
/// rate.
///
class RecvTokenBucket : public TokenBucket {
public:
    ///
    /// Constructor.
    ///
    /// @param  rate   The number of tokens accumulated per second.
    /// @param  burst  The maximum number of tokens that can accumulate at one time.
    ///
    RecvTokenBucket(fds_uint64_t rate, fds_uint64_t burst) : TokenBucket(rate, burst) {}

    ///
    /// Destructor.
    ///
    ~RecvTokenBucket() {}

    /* add tokens to the token bucket, burst size still applies */
    ///
    /// Add tokens to this bucket.
    ///
    /// After adding, tokens above ::burst_ will be expired.
    ///
    /// @param  add_tokens  The number of tokens to add.
    ///
    inline void addTokens(fds_uint64_t add_tokens) {
        token_count_ += add_tokens;
        (void)expireTokens();
    }
};

}  //namespace fds

#endif  // SOURCE_LIB_QOS_TOKBUCKET_H_
