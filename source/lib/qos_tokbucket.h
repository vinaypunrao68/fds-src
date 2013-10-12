/*
 * Copyright 2013 Formation Data Systems, Inc.
 *
 *  Basic Token Bucket implementation that 
 *  controls rate and and cannot accumulate
 *  more than burst size number of tokens 
 *  
 *  Currently using microsec granularity timers
 *  so would not support more than 1M IOPS that well.
 *  Will need to reconsider a finer granularity timers.
 */

#ifndef SOURCE_LIB_QOS_TOKBUCKET_H_
#define SOURCE_LIB_QOS_TOKBUCKET_H_

#include <boost/date_time/posix_time/posix_time.hpp>
#include "fds_types.h"

#define INFINITE_BURST                    987654321


namespace fds {

 /*  Demand-driven token bucket 
  *  For use by dispatchers that will drive the token bucket state, or 
  *  as a building block for a more complex token buckets. 
  * 
  *  Tokens are accumulated with rate 'rate', but never can exceed 
  *  burst size 'burst'. Thus, token bucket control rate 'rate' and 
  *  if tokens are not consumed and accumulate above burst size, 
  *  the tokens above burst size will be thrown away. 
  */
  class TokenBucket {
  public:
    TokenBucket(fds_uint64_t _rate, fds_uint64_t _burst)
      : rate(_rate), burst(_burst)
    {
       t_last_update = boost::posix_time::microsec_clock::universal_time();
       token_count = 0.0; /* start with 0 tokens */
    }
    ~TokenBucket() {}

    inline fds_uint64_t getRate() const { return rate; }
    inline fds_bool_t hasTokens(fds_uint32_t num_tokens) const { return ((double)num_tokens <= token_count); }

    /* Change rate and burst parameters 
     * Means that tokens will start accumulating with new rate, and capped 
     * to a new burst. This does not change the number of tokens that we 
     * accumulated with the old rate. */
    inline void modifyParams(fds_uint64_t _rate, fds_uint64_t _burst) {
        updateTokensOnly();
        /* do not cap to burst size now, updateState() will do it so it can 
         * the number of spilled tokens */
        rate = _rate;
        burst = _burst;
    }
    inline void modifyRate(fds_uint64_t _rate) {
        modifyParams(_rate, burst);
    }

    /* If token bucket has 'num_tokens', consume them and return true
     * otherwise return false. Assumes that token bucket state is current --
     * !! Must call updateTBState() first to update the state. */
    inline fds_bool_t tryToConsumeTokens(fds_uint32_t num_tokens) {
       if ((double)num_tokens <= token_count) {
          token_count -= (double)num_tokens;
          return true;
       }
       return false;
    }

    /* if tokens are not available, returns time delay in microseconds 
     * when tokens will become available, othewise returns 0
     * !! Must call updateTBState() first to update the state */
    inline fds_uint64_t getDelayMicrosec(fds_uint32_t num_tokens) {
       if (token_count < (double)num_tokens) {
          double delay_microsec = ( ((double)num_tokens - token_count)/(double)rate ) * 1000000.0;
          return (fds_uint64_t)delay_microsec;
       }
       return 0; /* otherwise enough tokens */
    }

    /* Update number of tokens in the token bucket
     * returns the number of tokens that spilled over the burst size */
    inline fds_uint64_t updateTBState(void) {
       fds_uint64_t expired_tokens = 0;
       updateTokensOnly();
    
       /* cap token counter to burst size */
       if ((fds_uint64_t)token_count > burst) {
          expired_tokens = burst - (fds_uint64_t)token_count;
          token_count = (double)burst;
       }
       return expired_tokens;
    } 

  protected: /* methods */
    inline void updateTokensOnly(void) {
        boost::posix_time::ptime now = boost::posix_time::microsec_clock::universal_time();
        boost::posix_time::time_duration elapsed = now - t_last_update;
        fds_uint64_t elapsed_microsec = elapsed.total_microseconds();

        if (elapsed_microsec > 0) {
           token_count += ((double)elapsed_microsec / 1000000.0) * (double)rate;
           t_last_update = now;
        }
    }

  protected:
    /* configurable parameters */
    fds_uint64_t rate; /* rate controlled by this token bucket */
    fds_uint64_t burst; /* maximum number of tokens that bucket can accumulate */

    /* dynamic state */
    boost::posix_time::ptime t_last_update; /* last time token bucket state was updated */
    double token_count; /* number of tokens in the token bucket since t_last_update */
  };

  /*
   * Demand-driven token bucket that allows to control its number of tokens
   * from outside (add and remove tokens explicitly). 
   * By default has an infinite burst size 
   */
  class ControlledTokenBucket: public TokenBucket {
  public: 
    ControlledTokenBucket(fds_uint64_t _rate, fds_uint64_t _burst=INFINITE_BURST)
      : TokenBucket(_rate, _burst) {
    }
    ~ControlledTokenBucket() {
    }

    /* add/remove tokens to/from token buucket  */
    inline void addTokens(fds_uint64_t num_tokens) {
        token_count += (double)num_tokens;
        if (token_count > (double)burst) {
           token_count = (double)burst; 
        }
    }

    /* returns actual number of tokens that was removed (since we cannot 
     * decrease the number of tokens below zero */
    inline fds_uint64_t removeTokens(fds_uint64_t num_tokens) {
        if ((double)num_tokens < token_count) {
            token_count -= (double)num_tokens;
        }
        else {
            token_count = 0.0;
        }
    }
    
    /* base class' update tokens will update tokens based on rate,
    *  and consume tokens are also the same */
  };


} //namespace fds

#endif /* SOURCE_LIB_QOS_TOKBUCKET_H_ */

