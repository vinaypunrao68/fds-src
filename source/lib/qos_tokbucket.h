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

#include <fds_types.h>
#include <util/timeutils.h>

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
    /* if _burst_length_microsec > 0, calculates burst based on time interval that we 
     * wait before we start throwing away tokens, and assigns burst as min of this value and _burst */
    TokenBucket(fds_uint64_t _rate, fds_uint64_t _burst, fds_uint64_t _burst_length_microsec=0)
      : rate(_rate)
    {
       burst = _burst;
       if (_burst_length_microsec > 0) {
          fds_uint64_t wburst = (fds_uint64_t)( (double)_burst_length_microsec/1000000.0 * (double)rate);
          if (wburst < burst) burst = wburst;
       }
       t_last_update = util::getTimeStampMicros();   // current time in microseconds
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
        fds_uint64_t nowMicrosec = util::getTimeStampMicros();
        updateTokensOnly(nowMicrosec);
        /* do not cap to burst size now, updateState() will do it so it can 
         * the number of spilled tokens */
        rate = _rate;
        burst = _burst;
    }
    inline void modifyRate(fds_uint64_t _rate, fds_uint64_t _burst_length_microsec) {
        modifyParams(_rate, (fds_uint64_t)( (double)_burst_length_microsec/1000000.0 * (double)_rate));
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
    inline fds_uint64_t updateTBState(fds_uint64_t nowMicrosec) {
       fds_uint64_t expired_tokens = 0;
       updateTokensOnly(nowMicrosec);
    
       /* cap token counter to burst size */
       if ((fds_uint64_t)token_count > burst) {
          expired_tokens = (fds_uint64_t)token_count - burst;
          /* important not to assign burst size, because if token_count is over burst by 
           * fraction of a token we will lose a fraction of a token. */
          token_count -= (double)expired_tokens;
       }
       return expired_tokens;
    } 

  protected: /* methods */
    inline void updateTokensOnly(fds_uint64_t nowMicrosec) {
        fds_uint64_t elapsed_microsec = nowMicrosec - t_last_update;
        if (elapsed_microsec > 0) {
           token_count += ((double)elapsed_microsec / 1000000.0) * (double)rate;
           t_last_update = nowMicrosec;
        }
    }

  protected:
    /* configurable parameters */
    fds_uint64_t rate; /* rate controlled by this token bucket */
    fds_uint64_t burst; /* maximum number of tokens that bucket can accumulate */

    /* dynamic state */
    fds_uint64_t t_last_update; /* last time in microsec token bucket state was updated */
    double token_count; /* number of tokens in the token bucket since t_last_update */
  };

  /*
   * Demand-driven token bucket that can get tokens
   * from outside (in addition to tokens generated with configured rate)
   */
  class RecvTokenBucket: public TokenBucket {
  public: 
    RecvTokenBucket(fds_uint64_t _rate, fds_uint64_t _burst)
      : TokenBucket(_rate, _burst) {
    }
    ~RecvTokenBucket() {
    }

    /* add tokens to the token bucket, burst size still applies */
    inline void addTokens(fds_uint64_t add_tokens) {
        token_count += (double)add_tokens;
        if ((fds_uint64_t)token_count > burst) {
           fds_uint64_t expired_toks = (fds_uint64_t)token_count - burst;
           token_count -= (double)expired_toks;
        }  
    } 
    
    /* base class' update tokens will update tokens based on rate,
    *  and consume tokens are also the same */
  };


} //namespace fds

#endif /* SOURCE_LIB_QOS_TOKBUCKET_H_ */

