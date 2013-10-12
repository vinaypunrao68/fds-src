/*
 * Copyright 2013 Formation Data Systems, Inc.
 *
 *  Hierarchical token bucket algorithm
 * 
 */

#ifndef SOURCE_LIB_QOS_HTB_H_
#define SOURCE_LIB_QOS_HTB_H_

#include <unordered_map>
#include <atomic>
#include <boost/date_time/posix_time/posix_time.hpp>

#include "qos_tokbucket.h"
#include "fds_qos.h"

#define DEFAULT_ASSURED_WAIT_MICROSEC     100000
#define HTB_WMA_SLOT_SIZE_MICROSEC        250000   /* the length of stat slot for gathering recent iops performance */   
/* If you modify HTB_WMA_LENTH, yoy must update TBQueueState::kweights constant array  */
#define HTB_WMA_LENGTH                    20       /* the length of recent iops performance window in HTB_WMA_SLOT_SIZE_MICROSEC slots */

namespace fds {

  class TBQueueState;
  typedef std::unordered_map<fds_uint32_t, TBQueueState* > qstate_map_t;
  typedef std::unordered_map<fds_uint32_t, TBQueueState*>::iterator qstate_map_it_t;

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
    inline void modifyParameters(fds_uint64_t _min_rate, fds_uint64_t _max_rate, fds_uint64_t _burst_size) {
      tb_min.modifyParams(_min_rate, _burst_size);
      tb_max.modifyParams(_max_rate, _burst_size);
      burst_size = _burst_size;
    }
    inline void modifyMinRate(fds_uint64_t _min_rate) { tb_min.modifyRate(_min_rate); }
    inline void modifyMaxRate(fds_uint64_t _max_rate) { tb_max.modifyRate(_max_rate); }
    inline void modifyBurstSize(fds_uint64_t _burst_size) { modifyParameters(getMinRate(), getMaxRate(), _burst_size); }

    inline fds_uint64_t getMinRate() const { return tb_min.getRate();}
    inline fds_uint64_t getMaxRate() const { return tb_max.getRate();}

    /* Notification that IO 'io' is queued or dispatched from the queue (dequeued)
     * uses atomic operations to update state of the queue 
     * both functions return resulting number of queued IOs */
    fds_uint32_t handleIoEnqueue(FDS_IOType */*io*/);
    fds_uint32_t handleIoDispatch(FDS_IOType */*io*/);

    /* returns moving average of IOPS performance */
    double getIOPerfWMA();

    /* Update number of tokens and returns the number of tokens that spilled over burst_size */
    /* from tb_max token bucket  */
    fds_uint64_t updateTokens(void);
    
    /* Uses the state from the last call to updateTokens().
     * For the IO at the head of the queue, try to consume tokens that were created with minRate 
     * Otherwise, returns:
     *        TBQUEUE_STATE_QUEUE_EMPTY: queue is empty so no IOs to consume a token 
     *        TBQUEUE_STATE_NO_TOKENS: queue is not empty but reached max rate 
     *        TBQUEUE_STATE_NO_ASSURED_TOKENS: queue not empty, there are tokens, but no 'assured tokens' */
    tbStateType tryToConsumeAssuredTokens(fds_uint32_t io_cost);

    /* For the IO at the head of the queue, consume tokens 
     * Call this method only when it is known that there are queued IOs and available tokens;
     * this function will assert if this is not true 
     */
    inline void consumeTokens(fds_uint32_t io_cost) {
      fds_bool_t bret = tb_max.tryToConsumeTokens(io_cost);
      assert(bret);
    }

  private:
    /***** configurable parameters *****/
    /* min_rate is tb_min.rate */
    /* max_rate is tb_max.rate */
    fds_uint64_t burst_size;

    /***** dynamic state *****/

   /*  Moving average of recent dispatched IO rate. 
    *  This is used (by the parent dispatcher) to fairly share available tokens among 
    *  among queues with the same priority */
    fds_uint32_t recent_iops[HTB_WMA_LENGTH];  /* a window of the recent iops performance */
    boost::posix_time::ptime next_hist_ts;
    fds_uint32_t hist_slotix;
    static const double kweights[HTB_WMA_LENGTH];
  
    TokenBucket tb_min;  /* token bucket to ensure min_rate */
    TokenBucket tb_max; /* token bucket to control we do not exceed max_rate */

    /*  For initial implementation where cost of IO is const, we just need 
     *  to maintain a counter of queued IOs to keep track if queue is empty or not 
     *  This value is the only variable of this class that is accessed by multiple threads  */
    std::atomic<unsigned int> queued_io_counter; 
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
    void modifyAvailRate(fds_uint64_t _avail_rate);
    /* assumes that all checks are done by parent class. If increasing min rate 
     * decreases avail_rate below 0, just sets avail_rate to 0 */
    void incMinRate(fds_uint64_t add_min_rate);
    void decMinRate(fds_uint64_t dec_min_rate);

    /* getters */
    inline fds_uint64_t getMinRate() const { return min_rate; }
    inline fds_uint64_t getAvailRate() const {return avail_rate; } 

    /* Update state : update number of 'assured' and 'available' tokens, and spill
     * expired 'assured' tokens to 'available' token bucket 
     * returns total number of tokens ('assured' + 'available') */
    fds_uint64_t updateState();

    inline fds_uint64_t getAvailableTokens();

    /* assumes appropriate tokens are available */
    void consumeAssuredTokens();
    void consumeAvailableTokens();

  private: /* data */
    fds_uint64_t min_rate;  /* total min (assured) rate */
    fds_uint64_t avail_rate; /* total available rate */

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
  class QoSHTBDispatcher: public FDS_QoSDispatcher {
  public:
    QoSHTBDispatcher(FDS_QoSControl* ctrl, fds_log *log, fds_uint64_t _total_rate);
    ~QoSHTBDispatcher();

    /***** implementation of base class functions *****/
    /* handle notification that IO was just queued */
    virtual void ioProcessForEnqueue(fds_uint32_t queue_id,
				     FDS_IOType *io);

    /* handle notification that IO will be dispatched */
    virtual void ioProcessForDispatch(fds_uint32_t queue_id,
				      FDS_IOType *io);				    

    /* returns queue is whose IO needs to be dispatched next 
     * this implementation consumes tokens required to dispatch IO */
    virtual fds_uint32_t getNextQueueForDispatch();

    /* this implementation calls based class registerQueue first */
    virtual Error registerQueue(fds_uint32_t queue_id,
				FDS_VolumeQueue *queue);

    /* this implementation calls base class deregisterQueue first */
    virtual Error deregisterQueue(fds_uint32_t queue_id);


    /**** extra methods that we could also add to base class ***/

    /* modify configurable parameters 
     * currently: returns ERR_INVALID_ARG id _total_rate < sum of all volumes' min rates 
     * TODO(?): If we want to be able to decrease total rate below sum of volumes min rates, we need
     * an ability to notify OM that some volumes will not be able to meet their reserved rate */
    Error modifyTotalRate(fds_uint64_t _total_rate);
  
    /* modify queue's configurable parameters */
    /*    Error modifyQueueParams(fds_uint32_t queue_id, 
			    fds_uint64_t q_min_rate,
			    fds_uint64_t q_max_rate,
			    fds_uint32_t q_priority);*/


  private:
    /****** configurable parameters *****/

    /* The total rate of IOs that will be dispatched from all the queues
     * given there are enough IOs queued up to consume this rate */ 
    fds_uint64_t total_rate;

    /*** Parameters that we set during construction for now, but 
     *** but could be dynamic based on some observations, etc. ****/

    /* How long we wait before giving tokens that represent assured rate 
     * for a queue to other queues. This means that a workload with delays
     * between IOs longer than this interval, they will not meet their
     * guaranteed rate. The smaller this interval, faster the 'reserved' 
     * but unused IOPS are given away to other volumes/queues */
    fds_uint64_t wait_time_microsec;

    /* Each queue will be configured with this burst size, unless 
     * it's explicitly set to another value. This specifies the maximim
     * number of IOs that will be dispatched from any given queue at the same time  */
    fds_uint64_t default_burst_size; /* in number of IOs */   

    /***** dynamic state ******/
    HTBPoolState pool;  /* pool of resource = 'assured' + 'available' */
    qstate_map_t qstate_map;  /* min and max rate control for each queue, and other queue state */  

    fds_uint32_t last_dispatch_qid; /* queue id from which we dispatch last IO */
  };


} //namespace fds

#endif /* SOURCE_LIB_QOS_HTB_H_ */

