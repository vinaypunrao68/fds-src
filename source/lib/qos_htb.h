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

#define HTB_QUEUE_RATE_MIN                20         /* protection from queues not going to absolute 0 rate, eg. when 
						     * setting throttle levels, etc. */
#define HTB_QUEUE_RATE_INFINITE_MAX       500000    /* a token bucket max rate when iops_max policy is set to 0 */

#define DEFAULT_ASSURED_WAIT_MICROSEC     1000000
#define HTB_WMA_SLOT_SIZE_MICROSEC        250000   /* the length of stat slot for gathering recent iops performance */   
/* If you modify HTB_MAX_WMA_LENGTH, yoy must update TBQueueState::kweights constant array  */
#define HTB_MAX_WMA_LENGTH                20
#define HTB_WMA_LENGTH                    20       /* the length of recent iops performance window in HTB_WMA_SLOT_SIZE_MICROSEC slots */

namespace fds {

  class TBQueueState;
  typedef std::unordered_map<fds_qid_t, TBQueueState* > qstate_map_t;
  typedef std::unordered_map<fds_qid_t, TBQueueState*>::iterator qstate_map_it_t;

  /* Queue state that is controlled by two demand-driven token buckets: one that 
   * ensures minimum rate, and another ensures that tokens are never consumed 
   * with rate higher than max rate. 
   *
   * Configurable parameters:
   * min_rate = minimum rate (also can be called assured rate)
   * max_rate = maximum rate (also can be called ceil rate)
   * assured_wait_microsec = time before assured tokens expire, this is directly 
   *                         translated to the burst size of token bucket controlling
   *                         min rate.
   * burst_size = maximum number of IOs that can be dispatched from this queue
   *              at the same time. E.g., when IOs do not arrive to the queue, 
   *              tokens will be accumulated (basically waiting for IOs to arrive).
   *              However, when number of tokens exceeds burst_size, they are 
   *              thrown away to ensure that there are never more than burstSize
   *              number of tokens that queue can consume. 
   * 
   * This initial implementation assumes any IO costs 1 token (for IOPS control).
   * Needs to be extended for variable IO cost. 
   *
   * This class is thread-safe assuming that multiple threads call handleIoEnqueue()
   * and a single thread calling other methods -- basically multiple threads queueing 
   * IOs and a single dispatcher thread that consumes tokens. 
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

    TBQueueState(fds_qid_t _queue_id, 
                 fds_uint64_t _min_rate, 
                 fds_uint64_t _max_rate,
                 fds_uint32_t _priority,
                 fds_uint64_t _assured_wait_microsec, 
                 fds_uint64_t _burst_size);
    ~TBQueueState();    

    /* Modify effective min and max rates  */
    inline void setEffectiveMinMaxRates(fds_uint64_t _min_rate, 
					fds_uint64_t _max_rate, 
					fds_uint64_t _assured_wait_microsec) {
      assert(_min_rate <= _max_rate);
      tb_min.modifyRate(_min_rate, _assured_wait_microsec);
      tb_max.modifyRate(_max_rate);
    }

    inline fds_uint64_t getEffectiveMinRate() const { return tb_min.getRate();}
    inline fds_uint64_t getEffectiveMaxRate() const { return tb_max.getRate();}

    /* Notification that IO 'io' is queued or dispatched from the queue (dequeued)
     * uses atomic operations to update state of the queue 
     * both functions return resulting number of queued IOs */
    fds_uint32_t handleIoEnqueue(FDS_IOType * /*io*/);
    fds_uint32_t handleIoDispatch(FDS_IOType * /*io*/);

    /* returns moving average of IOPS performance */
    double getIOPerfWMA();

    /* Update number of tokens and returns the number of expired 'assured' tokens */         
    fds_uint64_t updateTokens(fds_uint64_t nowMicrosec);
    
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
    inline void moveToNextHistTs(fds_uint64_t nowMicrosec) {
      if (nowMicrosec > next_hist_ts) {
	do {
	  next_hist_ts += HTB_WMA_SLOT_SIZE_MICROSEC;
          hist_slotix = (hist_slotix + 1) % HTB_WMA_LENGTH;
	  recent_iops[hist_slotix] = 0;
	} while (nowMicrosec > next_hist_ts);
      }
    }

  public: /* data */
    fds_qid_t queue_id;
    /* Queue policy settings. 
     * The queue may operate at different max and min rates depending on throttle 
     * level (the effective min and max are setting of tb_min and tb_max token buckets */
    fds_uint64_t min_rate; 
    fds_uint64_t max_rate;
    fds_uint32_t priority;

  private: /* data */

    /***** dynamic state *****/

   /*  Moving average of recent dispatched IO rate. 
    *  This is used (by the parent dispatcher) to fairly share available tokens among 
    *  among queues with the same priority */
    fds_uint32_t recent_iops[HTB_WMA_LENGTH];  /* a window of the recent iops performance */
    fds_uint64_t next_hist_ts;
    fds_uint32_t hist_slotix;
    static const double kweights[HTB_MAX_WMA_LENGTH];
  
    TokenBucket tb_min;  /* token bucket to ensure min_rate */
    TokenBucket tb_max; /* token bucket to control we do not exceed max_rate */

    /*  For initial implementation where cost of IO is const, we just need 
     *  to maintain a counter of queued IOs to keep track if queue is empty or not 
     *  This value is the only variable of this class that is accessed by multiple threads  */
    std::atomic<unsigned int> queued_io_counter; 
  };


  /* Controls a set of queues with HTB mechanism
   * 
   * A pool of a performance resource represented as 'total_rate' (in IOPS) 
   * shared among a set of queues. HTB_Control ensures that each queue 
   * receives its minimum rate (also called assured rate). All other available
   * resources 'total_rate' - sum of minimum queue rates are shared among 
   * all the queues based on their priority. At any given time, one of the queues 
   * of highest priority that has IOs queued up will get the resource (token) to 
   * dispatch its IO. If there are multiple queues of the same priority, then 
   * available tokens are distributed fairly among those queues.
   * 
   * If IOs do not arrive, the HTB mechanism will wait 'wait_time' before giving
   * away assured portion of a queue's resources (tokens) to other queues. 
   *
   * Each queue also has a burst_size which ensures that at most burst_size number 
   * of IOs can be dispatched from this queue at the same time. 
   *
   * The dispatcher has a total burst size -- the maximum number of IOs that can 
   * be dispatched with this dispatcher at the same time. The total burst size 
   * never exceeds the sum of of queues burst sizes. 
   * 
   */
  class QoSHTBDispatcher: public FDS_QoSDispatcher {
  public:
    QoSHTBDispatcher(FDS_QoSControl* ctrl, fds_log *log, fds_uint64_t _total_rate);
    virtual ~QoSHTBDispatcher();

    /***** implementation of base class functions *****/
    /* handle notification that IO was just queued */
    virtual void ioProcessForEnqueue(fds_qid_t queue_id,
				     FDS_IOType *io);

    /* handle notification that IO will be dispatched */
    virtual void ioProcessForDispatch(fds_qid_t queue_id,
				      FDS_IOType *io);				    

    /* returns queue is whose IO needs to be dispatched next 
     * this implementation consumes tokens required to dispatch IO */
    virtual fds_qid_t getNextQueueForDispatch();

    /* this implementation calls based class registerQueue first */
    virtual Error registerQueue(fds_qid_t queue_id,
				FDS_VolumeQueue *queue);

    /* this implementation calls base class deregisterQueue first */
    virtual Error deregisterQueue(fds_qid_t queue_id);

    virtual Error modifyQueueQosParams(fds_qid_t queue_id,
				       fds_uint64_t iops_min,
				       fds_uint64_t iops_max,
				       fds_uint32_t prio);

    virtual void setThrottleLevel(float throttle_level);

    /**** extra methods that we could also add to base class ***/

    /* modify configurable parameters 
     * currently: returns ERR_INVALID_ARG id _total_rate < sum of all volumes' min rates 
     * TODO(?): If we want to be able to decrease total rate below sum of volumes min rates, we need
     * an ability to notify OM that some volumes will not be able to meet their reserved rate */
    Error modifyTotalRate(fds_uint64_t _total_rate);
  
   using FDS_QoSDispatcher::dispatchIOs;

  private:
   void setQueueThrottleLevel(TBQueueState* qstate, float throttle_level); 
   void setQueueThrottleLevel(TBQueueState* qstate, fds_uint32_t tlevel_x, double tlevel_frac);

  private:
    /****** configurable parameters *****/

    /* The total rate of IOs that will be dispatched from all the queues
     * given there are enough IOs queued up to consume this rate */ 
    fds_uint64_t total_rate;

    /* sum of min rates of all queues */
    fds_uint64_t total_min_rate; 

    /* How long we wait before giving tokens that represent assured rate 
     * for a queue to other queues. This means that a workload with delays
     * between IOs longer than this interval, they will not meet their
     * guaranteed rate. The smaller this interval, faster the 'reserved' 
     * but unused IOPS are given away to other volumes/queues */
    fds_uint64_t wait_time_microsec;

    /* Each queue will be configured with this burst size, unless 
     * it's explicitly set to another value. This specifies the maximim
     * number of IOs that will be dispatched from any given queue at the same time  */
    fds_uint64_t default_que_burst_size; /* in number of IOs */   

    /* total burst size from dispatcher */
    fds_uint64_t max_burst_size; /* in number of IOs */

    /* current throttle level */
    float current_throttle_level;

    /***** dynamic state ******/
    RecvTokenBucket avail_pool; /* pool of available tokens (non-guaranteed tokens + expired guaranteed tokens) */
    qstate_map_t qstate_map;  /* min and max rate control for each queue, and other queue state */  

    fds_qid_t last_dispatch_qid; /* queue id from which we dispatch last IO */
  };


} //namespace fds

#endif /* SOURCE_LIB_QOS_HTB_H_ */

