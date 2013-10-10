/*
 * Copyright 2013 Formation Data Systems, Inc.
 *
 *  Hierarchical token bucket algorithm
 * 
 */

#ifndef SOURCE_LIB_QOS_HTB_H_
#define SOURCE_LIB_QOS_HTB_H_

#include <boost/date_time/posix_time/posix_time.hpp>

#include "fds_types.h"

namespace fds {

  class TBTQueueState;
  typedef std::unordered_map<fds_uint32_t, TBTQueueState* > qstate_map_t;

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
    TokenBucket(fds_uint64_t _rate, fds_uint64_t _burst);
    ~TokenBucket();

    /* change rate and burst parameters */
    void reset(fds_uint64_t _rate, fds_uint64_t _burst);
    void reset(fds_uint64_t _rate);

    /* If token bucket has 'num_tokens', consume them and return true
     * otherwise return false */
    inline fds_bool_t tryToConsumeTokens(fds_uint64_t num_tokens);

    /* Update number of tokens in the token bucket
     * returns the number of tokens that spilled over the burst size */
    inline fds_uint64_t updateTokens(); 

  protected:
    /* configurable parameters */
    fds_uint64_t rate; /* rate controlled by this token bucket */
    fds_uint64_t burst; /* maximum number of tokens that bucket can accumulate */

    /* dynamic state */
    boost::posix_time::ptime t_last_update; /* last time token bucket state was updated */
    fds_uint64_t token_count; /* number of tokens in the token bucket since t_last_update */
  };

  /*
   * Demand-driven token bucket that allows to control its number of tokens
   * from outside (add and remove tokens explicitly). Does not have burst size.
   */
  class ControlledTokenBucket: public TokenBucket {
  public: 
    ControlledTokenBucket(fds_uint64_t _rate);
    ~ControlledTokenBucket();

    /* add/remove tokens to/from token buucket  */
    void addTokens(fds_uint32_t num_tokens);
    fds_uint32_t removeTokens(fds_uint32_t num_tokens);    
    
    /* base class' update tokens will update tokens based on rate,
    *  and consume tokens are also the same */
  };


  /* Queue state that is controlled by a demand-driven token bucket that 
   * ensures minimum rate, but also ensures that tokens are never consumed 
   * with rate higher than max rate. 
   *
   * Configurable parameters:
   * min_rate = minimum rate (also can be called assured rate)
   * max_rate = maximum rate (also can be called ceil rate)
   * burst_size = maximum number of IOs that can be dispatched from this queue
   *              at the same time. E.g., when IOs do not arrive to the queue, 
   *              tokens will be accumulated (basically waiting for IOs to arrive).
   *              However, when number of tokens exceeds burst_size, they are 
   *              thrown away to ensure that there are never more than burstSize
   *              number of tokens that queue can consume. 
   * 
   * This token bucket is constructed with two token buckets: one which tracks
   * minRate and another that tracks maxRate.
   *
   * This initial implementation assumes any IO costs 1 token (for IOPS control).
   * Needs to be extended for variable IO cost. 
   *
   */
  class TBQueueState {
  public:
    /* this state will be returned from some methods */
    typedef enum {
      TBQUEUE_STATE_QUEUE_EMPTY,        /* queue is idle */
      TBQUEUE_STATE_NO_TOKENS,          /* means we are over max rate */
      TBQUEUE_STATE_NO_ASSURED_TOKENS,  /* no tokens created with minRate, meaning assured rate is met */
      TBQUEUE_STATE_OK                  /* otherwise */
    } tbStateType;

    TBQueueState(fds_uint64_t _min_rate, fds_uint64_t _max_rate, fds_uint64_t _burst_size);
    ~TBQueueState();    

    /* Modify configurable parameters */
    void modifyParameters(fds_uint64_t _min_rate, fds_uint32_t _max_rate, fds_uint64_t _burst_size);
    void modifyMinRate(fds_uint64_t _min_rate);
    void modifyMaxRate(fds_uint64_t _max_rate);
    void modifyBurstSize(fds_uint64_t _burst_size);

    /* Notification that IO 'io' is queued  
     * uses atomic operations to update state of the queue */
    inline void handleIoEnqueue(fds_io_t *io);

    /* Update number of tokens and returns the number of tokens that spilled over burst_size */
    fds_uint64_t updateTokens();
    
    /* For the IO at the head of the queue, try to consume tokens that were created with minRate 
    *  Otherwise, returns:
    *        TBQUEUE_STATE_QUEUE_EMPTY: queue is empty so no IOs to consume a token 
    *        TBQUEUE_STATE_NO_TOKENS: queue reached max rate 
    *        TBQUEUE_STATE_NO_ASSURED_TOKENS: there are no tokens created with minRate */
    tbStateType tryToConsumeAssuredTokens();

    /* For the IO at the head of the queue, consume tokens 
     * Call this method only when it is known that there are queued IOs and available tokens;
     * this function will assert if this is not true 
     */
    inline void consumeTokens();

  private:
    /***** configurable parameters *****/
    fds_uint64_t min_rate;
    fds_uint64_t max_rate;
    fds_uint64_t burst_size;

    /***** dynamic state *****/

   /*  Moving average of recent dispatched IO rate. 
    *  This is used (by the parent dispatcher) to fairly share available tokens among 
    *  among queues with the same priority */
    double recent_ma_rate;
  
    TokenBucket tb_min;  /* token bucket to ensure min_rate */
    TokenBucket tb_max; /* token bucket to control we do not exceed max_rate */

    /*  For initial implementation where cost of IO is const, we just need 
     *  to maintain a counter of queued IOs to keep track if queue is empty or not 
     *  This value is always updated using atomic operations */
    fds_uint64_t queued_io_counter; 
  };

  /* Pool of a performance resource, which controls 'assured' tokens (generated
   * to achieve total minimum rate) and 'available' tokens. 'assured' tokens
   * become 'available' after some expiration time (see 'wait_time' description
   * in TBT_Control class). 
   * This is the 'root' of HTB 
   */
  class HTBPoolState {
  public:
    HTBPoolState(fds_uint64_t _min_rate, fds_uint64_t _avail_rate, fds_uint64_t _wait_time_microsec);
    ~HTBPoolState();

    /* modify configurable parameters */
    void modifyParameters(fds_uint64_t _min_rate, fds_uint64_t _avail_rate, fds_uint64_t _wait_time_microsec);

    /* Update state : update number of 'assured' and 'available' tokens, and spill
     * expired 'assured' tokens to 'available' token bucket 
     * returns total number of tokens ('assured' + 'available') */
    fds_uint64_t updateState();

    inline fds_uint64_t getAvailableTokens();

    /* assumes appropriate tokens are available */
    void consumeAssuredTokens();
    void consumeAvailableTokens();

  private:
    /***** configurable parameters ****/
    fds_uint64_t wait_time_microsec;
    /* min and avail rate params are directly passed to appropriate token buckets */

    /***** dynamic state *****/
    TokenBucket tb_min;    /* generates tokens with min_rate */
    ControlledTokenBucket tb_avail; /* generates tokens with avail_rate */
  };

  /* Controls a set of queues with HTB mechamism
   * 
   * A pool of a performance resource represented as 'total_rate' (in IOPS) 
   * shared among a set of queues. HTB_Control ensures that each queue 
   * receives its minimum rate (also called assured rate). All other available
   * resources 'total_rate' - sum of minimum queue rates are shared among 
   * all the queues based on their priority. At any given time, one of the queues 
   * of highest priority that has IOs queued up will get the resource (token) to 
   * dispatch it IO. If there are multiple queues of the same priority, then 
   * available tokens are distributed fairly among those queues.
   * 
   * If IOs do not arrive, the HTB mechanism will wait 'wait_time' before giving
   * away assured portion of a queue's resources (tokens) to other queues. 
   *
   * Each queue also has a burst_size which ensures that at most burst_size number 
   * of IOs can be dispatched from this queue at the same time. 
   * 
   */
  class HTB_Control {
  public:
    HTBPool(fds_uint64_t _total_rate, 
	    fds_uint64_t _wait_time_microsec, 
	    fds_uint64_t _default_burst_size);
    ~HTBPool();

    /* modify configurable parameters */
    void modifyParameters(fds_uint64_t _total_rate, 
			  fds_uint64_t _wait_time_microsec,
			  fds_uint64_t _default_burst_size);
    void modifyTotalRate(fds_uint64_t _total_rate);

    /* handle adding/removing queues */
    Error handleRegisterQueue(fds_uint32_t queue_id, 
			      fds_uint64_t q_min_rate,
			      fds_uint64_t q_max_rate,
			      fds_uint32_t q_priority);
    Error handleDeregisterQueue(fds_uint32_t queue_id);
  
    /* modify queue's configurable parameters */
    Error modifyQueueParams(fds_uint32_t queue_id, 
			    fds_uint64_t q_min_rate,
			    fds_uint64_t q_max_rate,
			    fds_uint32_t q_priority);

    /* handle notification that IOs will be  queued */    
    inline void handleIoEnqueued(fds_uint32_t queue_id, fds_io_t *io);

    /* Get queue whose IO we will dispatch next
     * This also consumes tokens requied to dispatch an IO */
    fds_uint32_t getNextQueueForDispatch();

    /* Handle notification that IO will be dispatched 
     * We will just compare number of tokens we consumed with actual 
     * number of tokens this IO required. For now, this is just for sanity check
     * In the future, we may need to do more complex handling, eg. for variable IO sizes 
     */
    inline void handleIoDispatch(fds_uint32_t queue_id, fds_io_t *io);
   
  private:
    /****** configurable parameters *****/

    /* The total rate of IOs that will be dispatched from all the queues
     * given there are enough IOs queued up to consume this rate */ 
    fds_uint64_t total_rate;

    /* How long we wait before giving tokens that represent assured rate 
     * for a queue to other queues. This means that a workload with delays
     * between IOs longer than this interval, they will not meet their
     * guaranteed rate. The smaller this interval, faster the 'reserved' 
     * but unused IOPS are given away to other volumes/queues */
    fds_uint64_t wait_time_microsec;

    /* Each queue will be configured with this burst size, unless 
     * it's explicitly set to another value. This specifies the maximim
     * number of IOs that will be dispatched from any given queue at the same time  */
    fds_uint64_t default_burst_size;    

    /***** dynamic state ******/
    HTBPoolState pool;  /* pool of resource = 'assured' + 'available' */
    qstate_map_t qstate_map;  /* min and max rate control for each queue, and other queue state */  
  };



} //namespace fds

#endif /* SOURCE_LIB_QOS_HTB_H_ */

