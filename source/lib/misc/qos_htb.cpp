/*
 * Copyright 2013 Formation Data Systems, Inc.
 *
 *  Hierarchical token bucket algorithm
 * 
 */
#include "qos_htb.h"

namespace fds {

/***** QoSHTBDispatcher implementation ******/
QoSHTBDispatcher::QoSHTBDispatcher(FDS_QoSControl *ctrl, fds_log *log, fds_uint64_t _total_rate)
  : FDS_QoSDispatcher(ctrl, log, _total_rate),
    total_rate(_total_rate),
    total_min_rate(0),
    max_burst_size(20),
    wait_time_microsec(DEFAULT_ASSURED_WAIT_MICROSEC),
    avail_pool(_total_rate, max_burst_size),
    current_throttle_level(11)
{
  /* TODO: we need a good per-volume burst size value, sholdn't this be a max queue size at SM/DM? */
  default_que_burst_size = 20; 

  last_dispatch_qid = 0; /* this should be an invalid id */
}

QoSHTBDispatcher::~QoSHTBDispatcher()
{
  qda_lock.write_lock();
  for (qstate_map_it_t it = qstate_map.begin();
       it != qstate_map.end();
       ++it)
    {
      TBQueueState *qstate = it->second;
      delete qstate;
    }
  qstate_map.clear();
  qda_lock.write_unlock();  
}

/* Returns ERR_INVALID_ARG if _total_rate < sum of all volumes' minimum rates */
Error QoSHTBDispatcher::modifyTotalRate(fds_uint64_t _total_rate)
{
  Error err(ERR_OK);

  /* do not modify if _total_rate is below total min_rate of all volumes */
  if (_total_rate > total_min_rate)
    {
      total_rate = _total_rate;
      /* only need to modify avail rate */
      avail_pool.modifyRate(total_rate - total_min_rate);
    }
  else 
    {
      /* for now we are not letting total rate go below volumes' total min rate */
      err = ERR_INVALID_ARG;
      return err;
    }

  return err;
}

Error QoSHTBDispatcher::registerQueue(fds_qid_t queue_id,
				      FDS_VolumeQueue *queue)
{
  Error err(ERR_OK);
  fds_uint64_t new_total_min_rate = 0;
  fds_uint64_t new_total_avail_rate = 0;
  fds_uint64_t q_min_rate = queue->iops_min;
  fds_uint64_t q_max_rate = queue->iops_max;

  /* If iops_max == 0, means no max, set max_rate to a very high value */
  if (q_max_rate == 0) 
     q_max_rate = HTB_QUEUE_RATE_INFINITE_MAX;
   
  /* since highest priority queues will get all the slack, give some minimum
   * iops to queues iops_min == 0 */
  if (q_min_rate < HTB_QUEUE_RATE_MIN) 
    q_min_rate = HTB_QUEUE_RATE_MIN;

  /* we need a new state to control new queue */
  TBQueueState *qstate = new TBQueueState(queue_id, 
                                          q_min_rate, 
                                          q_max_rate,
                                          queue->priority,
                                          wait_time_microsec, 
                                          default_que_burst_size);
  if (!qstate) {
    LOGERROR
      << "QoSHTBDispatcher: failed to allocate memory for queue state for queue: " << queue_id;
    err = ERR_MAX;
    return err;
  }

  /* for now, do not allow total min rate to increase above total rate -- return error
   *  TODO: should we scale down all volumes' min rates? */
  qda_lock.write_lock();
 
  if ((q_min_rate + total_min_rate) > total_rate) {
    qda_lock.write_unlock();
    delete qstate;
    LOGERROR << "QoSHTBDispatcher: invalid qos rates.  q_min_rate: "
             << q_min_rate << " total_min_rate: " << total_min_rate << " total_rate: "
             << total_rate;
    err = Error(ERR_EXCEED_MIN_IOPS);
    return err;
  }

  /* call base class to actually add queue */
  err = FDS_QoSDispatcher::registerQueueWithLockHeld(queue_id, queue);
  if (!err.ok()) {
    qda_lock.write_unlock();
    delete qstate;
    return err;
  }
  
  /* base class already checked that queue_id is valid and not already registered */
  /* update total min rate and avail rate */
  total_min_rate += q_min_rate;
  new_total_min_rate = total_min_rate;
  new_total_avail_rate = avail_pool.getRate();
  if (new_total_avail_rate > q_min_rate) {
     new_total_avail_rate -= q_min_rate;
  }
  else {
     /* for now we allow total_min_rate to exceed total_rate, means avail rate is 0,
      * Even if avail_pool does not generate any tokens, it still gets unused 
      * assured tokens (from min_iops reservation) and shares them with other queues */
     new_total_avail_rate = 0;
  }
  avail_pool.modifyRate(new_total_avail_rate);

  /* add queue state to map */  
  qstate_map[queue_id] = qstate;
  qda_lock.write_unlock();

  LOGNOTIFY << "QosHTBDispatcher: registered queue 0x" << std::hex << queue_id << std::dec
            << "with min_iops=" << queue->iops_min
            << "; max_iops=" << queue->iops_max
            << "; prio=" << queue->priority
            << "; total_min_rate " << new_total_min_rate 
            << ", total_avail_rate " << new_total_avail_rate;

  return err;
}

Error QoSHTBDispatcher::modifyQueueQosParams(fds_qid_t queue_id,
					     fds_uint64_t iops_min,
					     fds_uint64_t iops_max,
					     fds_uint32_t prio)
{
  Error err(ERR_OK);
  fds_uint64_t new_total_min_rate = 0;
  fds_uint64_t new_total_avail_rate = 0;
  fds_uint64_t q_min_rate = iops_min;
  fds_uint64_t q_max_rate = iops_max;
  TBQueueState* qstate = NULL;

  /* If iops_max == 0, means no max, set max_rate to a very high value */
  if (q_max_rate == 0) 
     q_max_rate = HTB_QUEUE_RATE_INFINITE_MAX;

  /* since highest priority queues will get all the slack, give some minimum
   * iops to queues iops_min == 0 */
  if (q_min_rate < HTB_QUEUE_RATE_MIN) 
    q_min_rate = HTB_QUEUE_RATE_MIN;

  qda_lock.write_lock();

  /* first check if we know about this queue */
  if (qstate_map.count(queue_id) == 0) {
    qda_lock.write_unlock();
    err = Error(ERR_NOT_FOUND);
    return err;
  }
  qstate = qstate_map[queue_id];

  /* for now, do not allow total min rate to increase above total rate -- return error
   *  TODO: should we scale down all volumes' min rates? */
  if ((q_min_rate > qstate->min_rate) && ((q_min_rate - qstate->min_rate + total_min_rate) > total_rate)) {
    qda_lock.write_unlock();
    LOGERROR << "QoSHTBDispatcher: invalid qos rates.  q_min_rate: "
             << q_min_rate << " total_min_rate: " 
             << total_min_rate-qstate->min_rate+q_min_rate 
             << "  total_rate: " << total_rate;
    err = Error(ERR_EXCEED_MIN_IOPS);
    return err;
  }

  /* call base class to actually modify queue qos params */
  err = FDS_QoSDispatcher::modifyQueueQosWithLockHeld(queue_id, q_min_rate, q_max_rate, prio);
  if (!err.ok()) {
    qda_lock.write_unlock();
    return err;
  }

  /* update total min rate and avail rate */
  fds_verify(total_min_rate >= qstate->min_rate);
  total_min_rate = total_min_rate - qstate->min_rate + q_min_rate;
  new_total_min_rate = total_min_rate;
  new_total_avail_rate = avail_pool.getRate() + qstate->min_rate;
  if (new_total_avail_rate > q_min_rate) {
     new_total_avail_rate -= q_min_rate;
  }
  else {
     /* for now we allow total_min_rate to exceed total_rate, means avail rate is 0,
      * Even if avail_pool does not generate any tokens, it still gets unused 
      * assured tokens (from min_iops reservation) and shares them with other queues */
     new_total_avail_rate = 0;
  }
  avail_pool.modifyRate(new_total_avail_rate);

  /* modify queue state params */
  qstate->min_rate = q_min_rate;
  qstate->max_rate = q_max_rate;
  qstate->priority = prio;
  /* since we may also have a throttle level set, this func will change effective min/max rates
   * of token bucket based on iops min and max but also a current throttle level */
  setQueueThrottleLevel(qstate, current_throttle_level);
  qda_lock.write_unlock();

  LOGNOTIFY << "QosHTBDispatcher: modified queue " << queue_id
            << "new min_iops=" << q_min_rate
            << "; max_iops=" << q_max_rate
            << "; prio=" << prio
            << "; total_min_rate " << new_total_min_rate 
            << ", total_avail_rate " << new_total_avail_rate;

  return err;
}

Error QoSHTBDispatcher::deregisterQueue(fds_qid_t queue_id)
{
  Error err(ERR_OK);
  TBQueueState* qstate = NULL;
  fds_uint64_t new_total_min_rate = 0;
  fds_uint64_t new_total_avail_rate = 0;

  qda_lock.write_lock();
  /* call base class to remove queue */
  err = FDS_QoSDispatcher::deregisterQueueWithLockHeld(queue_id);
  /* if error, still try to remove queue state first before returning */
  
  if (qstate_map.count(queue_id) == 0) {
    qda_lock.write_unlock();
    err = Error(ERR_DUPLICATE); /* we probably got same error from base class, but still good to check if queue state exists */
    return err;
  }

  qstate = qstate_map[queue_id];
  qstate_map.erase(queue_id);

  /* update total min and avail rates */
  assert(qstate->min_rate <= total_min_rate);
  total_min_rate -= qstate->min_rate;

  new_total_min_rate = total_min_rate;
  if (total_rate > total_min_rate)
     new_total_avail_rate = total_rate - total_min_rate;
  
  avail_pool.modifyRate(new_total_avail_rate);
  qda_lock.write_unlock();

  /* cleanup */
  delete qstate;
 
  LOGNOTIFY << "QosHTBDispatcher: deregistered queue 0x" << std::hex << queue_id << std::dec
            << "; total_min_rate " << new_total_min_rate
            << ", total_avail_rate " << new_total_avail_rate; 
  
  return err;
}

void QoSHTBDispatcher::setQueueThrottleLevel(TBQueueState *qstate, float throttle_level)
{
  assert((throttle_level >= -10) && (throttle_level <= 11));
  fds_int32_t tlevel_x = (fds_int32_t) throttle_level;
  double tlevel_frac = fabs((double)throttle_level - (double)tlevel_x);
  setQueueThrottleLevel(qstate, tlevel_x, tlevel_frac); 
}

void QoSHTBDispatcher::setQueueThrottleLevel(TBQueueState *qstate, fds_uint32_t tlevel_x, double tlevel_frac)
{
  if (tlevel_x >= 0) {
    if (qstate->priority > tlevel_x) {
      /* throttle to min rate */
      qstate->setEffectiveMinMaxRates(qstate->min_rate, qstate->min_rate, wait_time_microsec);
    }
    else if (qstate->priority < tlevel_x) {
      /* ok to go up to max rate limit */
      qstate->setEffectiveMinMaxRates(qstate->min_rate, qstate->max_rate, wait_time_microsec);
    }
    else {
      /* ok to go up to min_rate + Y/10*(max_rate - min_rate) */
      double new_max_rate = qstate->min_rate + tlevel_frac * (qstate->max_rate - qstate->min_rate);
      qstate->setEffectiveMinMaxRates(qstate->min_rate, (fds_uint64_t)new_max_rate, wait_time_microsec);
    }
  }
  else { /* tlevel_x < 0 */
    double share = (10.0 - (double)abs(tlevel_x))/10.0;
    fds_uint64_t new_min_rate = (fds_uint64_t)(share * (double)qstate->min_rate);
    /* we don't want to go to absolute 0 rate, so set some very small rate as min */
    if (new_min_rate < HTB_QUEUE_RATE_MIN) 
      new_min_rate = HTB_QUEUE_RATE_MIN;
    qstate->setEffectiveMinMaxRates(new_min_rate, new_min_rate, wait_time_microsec);
    /* note that we are not going to change total_min_rate when changing queue's effective
     * min rate (meaning we are not increasing total available rate), because in this case
     * all volumes are throttled down, and no sharing is happening */
  }

  LOGNOTIFY << "QosHTBDispatcher: setThrottleLevel(X=" 
            << tlevel_x <<", Y/10=" << tlevel_frac 
            << ") queue " << qstate->queue_id 
            << " Policy (" << qstate->min_rate 
            << "," << qstate->max_rate << "," << qstate->priority
            << "), effective min_rate=" << qstate->getEffectiveMinRate() 
            << ", effective max_rate=" << qstate->getEffectiveMaxRate();

}

void QoSHTBDispatcher::setThrottleLevel(float throttle_level)
{
  assert((throttle_level >= -10) && (throttle_level <= 10));
  fds_int32_t tlevel_x = (fds_int32_t) throttle_level;
  double tlevel_frac = fabs((double)throttle_level - (double)tlevel_x);

  LOGNOTIFY << "QosHTBDispatcher: set throttle level to " 
            << throttle_level
            << "; X=" << tlevel_x 
            << ", Y/10=" << tlevel_frac;

  qda_lock.write_lock();
  for (qstate_map_it_t it = qstate_map.begin();
       it != qstate_map.end();
       ++it)
    {
      TBQueueState* qstate = it->second;
      assert(qstate);
      setQueueThrottleLevel(qstate, tlevel_x, tlevel_frac);
    }
  current_throttle_level = throttle_level;
  qda_lock.write_unlock();
}

void QoSHTBDispatcher::ioProcessForEnqueue(fds_qid_t queue_id,
					   FDS_IOType *io)
{
  TBQueueState* qstate = qstate_map[queue_id];
  assert(qstate);
  fds_uint32_t queued_ios = qstate->handleIoEnqueue(io);
  LOGDEBUG << "QoSHTBDispatcher: handling enqueue IO to queue 0x"
           << std::hex << queue_id << std::dec
           << " ; # of queued_ios " << (queued_ios+1);
}

void QoSHTBDispatcher::ioProcessForDispatch(fds_qid_t queue_id,
					   FDS_IOType *io)
{
  /* we already updated tokens in getNextQueueForDispatch(), 
   * so only update number of queued ios  */
  TBQueueState* qstate = qstate_map[queue_id];
  assert(qstate);
  qstate->handleIoDispatch(io);
}

/* find queue whose IO needs to be dispatched next */
fds_qid_t QoSHTBDispatcher::getNextQueueForDispatch()
{
  fds_qid_t ret_qid = 0;
  TBQueueState *min_wma_qstate = NULL;
  double min_wma;  
  uint min_wma_hiprio;
 
  while (1) {

    /* reset qstate with min wma, since we start searching from beginning */
    min_wma_qstate = NULL;

    /* all tokens are demand-driven, so make sure to update state first */
    avail_pool.updateTBState();

    /**** search if any queues has IOs that need to be dispatched to meet min_iops ****/
    /* we will check the queue that we serviced last time last */
    qstate_map_it_t end_it = qstate_map.find(last_dispatch_qid);
    if (end_it == qstate_map.end()) {
      /* the queue we dispatched IO last time seems to be removed */
      end_it = qstate_map.begin();
    }
    qstate_map_it_t it = end_it;
    it++;
    if (it == qstate_map.end()) {
      it = qstate_map.begin();
    }
  
    for (uint i = 0; i < qstate_map.size(); ++i) {
      TBQueueState *qstate = it->second;
      assert(qstate != NULL);

      /* before querying any state, update tokens */
      fds_uint64_t exp_assured_toks = qstate->updateTokens();
      if (exp_assured_toks > 0) {
	/* first put expired assured tokens to the avail_pool */
	avail_pool.addTokens(exp_assured_toks);
	LOGDEBUG << "QoSHTVDispatcher: moving " 
                 << exp_assured_toks << " expired assured toks from "
                 << "queue 0x" << std::hex << qstate->queue_id 
                 << std::dec << " to the pool of available tokens";
      }

      /* try to see if we can serve the io from the head of queue with assured tokens */
      TBQueueState::tbStateType state = qstate->tryToConsumeAssuredTokens(1);
      if (state == TBQueueState::TBQUEUE_STATE_OK) {
        /* we found a queue whose IO we will dispatch to meet its min_ios */
        last_dispatch_qid = it->first;
        LOGDEBUG << "QoSHTBDispatcher: dispatch (min_iops) io from queue 0x"
                 << std::hex << it->first << std::dec;
        return it->first;
      }
      else if (state == TBQueueState::TBQUEUE_STATE_NO_ASSURED_TOKENS) {
        /* queue has at least one io and available tokens (but no assured tokens) */
        double q_wma = qstate->getIOPerfWMA();
        if (!min_wma_qstate) {
           min_wma = q_wma;
           min_wma_qstate = qstate;
           min_wma_hiprio = qstate->priority;
        }
        else if (qstate->priority < min_wma_hiprio) {
           /* assuming higher priority is a lower the number (highest = 1) */
           min_wma_hiprio = qstate->priority;
           min_wma = q_wma;
           min_wma_qstate = qstate;
        }
        else if ((qstate->priority == min_wma_hiprio) && (q_wma < min_wma)) {
           min_wma = q_wma;
           min_wma_qstate = qstate;
        }
      }

      /* next queue */
      ++it;
      if (it == qstate_map.end())
	it = qstate_map.begin();

    }

    /* we did not find any queue with IOs that need to meet it min_iops */
    if (min_wma_qstate) {
     /* we found queue that has IO, available tokens, and lowest recent ave performance */
     /* dispatch this io if we have available tokens in the resource pool */
     fds_bool_t bHasToks = avail_pool.tryToConsumeTokens(1);
     if (bHasToks) {
         min_wma_qstate->consumeTokens(1);
         ret_qid = min_wma_qstate->queue_id;
         LOGDEBUG << "QoSHTBDispatcher: dispatch (avail) io from queue 0x"
                  << std::hex << ret_qid << std::dec;
         break;
     }
    }

    /* we did not find any IOs to dispatch */
    { /* wait for next guaranteed token or few non-guaranteed tokens to be created */
     const double num_toks = 1.0;
     fds_uint64_t assured_delay_microsec = (total_min_rate > 0) ? (num_toks/(double)total_min_rate)*1000000.0 : 0;
     fds_uint64_t delay_microsec = (avail_pool.getRate() > 0) ? ((num_toks+1.0)/(double)avail_pool.getRate())*1000000.0 : 0;
     if ((assured_delay_microsec > 0) && (delay_microsec > assured_delay_microsec))
        delay_microsec = assured_delay_microsec;

     LOGDEBUG << "QoSHTBDispatcher: no tokens available, will sleep for " << delay_microsec;
     if (delay_microsec > 0) {
        qda_lock.read_unlock();
        boost::this_thread::sleep(boost::posix_time::microseconds(delay_microsec));
        qda_lock.read_lock();
     }
    }

  } // while (1)

  last_dispatch_qid = ret_qid;
  return ret_qid;
}

/******* TBQueueState implementation ***********/
TBQueueState::TBQueueState(fds_qid_t _queue_id,
                           fds_uint64_t _min_rate, 
			   fds_uint64_t _max_rate,
                           fds_uint32_t _priority,
			   fds_uint64_t _assured_wait_microsec,
			   fds_uint64_t _burst_size)
  : queue_id(_queue_id),
    min_rate(_min_rate),
    max_rate(_max_rate),
    priority(_priority),
    tb_min(_min_rate, _burst_size, _assured_wait_microsec), 
    tb_max(_max_rate, _burst_size)
{
  assert(_min_rate <= _max_rate);
  queued_io_counter = ATOMIC_VAR_INIT(0);
  memset(recent_iops, 0, sizeof(fds_uint32_t) * HTB_WMA_LENGTH);

  /* align next_hist_ts to a second boundary, so that all volumes histories are aligned */
  next_hist_ts = boost::posix_time::second_clock::universal_time() +
    boost::posix_time::microseconds(HTB_WMA_SLOT_SIZE_MICROSEC);
  hist_slotix = 0;
}

TBQueueState::~TBQueueState() 
{
}

/* for now assuming constant IO cost, so simple implementation  */
fds_uint32_t TBQueueState::handleIoEnqueue(FDS_IOType* /*io*/)
{
  return (std::atomic_fetch_add(&queued_io_counter, (unsigned int)1) + 1);
}

/* for now assuming constant IO cost */
fds_uint32_t TBQueueState::handleIoDispatch(FDS_IOType* /*io*/)
{
  /* update recent iops performance */
  /* since this implementation assumes single dispatcher thread, each IO
   * dispatch happens one after another, if that assumption does not hold anymore
   * will need to change wma calculation  */  
  boost::posix_time::ptime now = boost::posix_time::microsec_clock::universal_time();
  if (now < next_hist_ts) {
    /* we are still filling the same slot */
    recent_iops[hist_slotix]++;
  }
  else {
    /* will start the next slot, possibly skipping idle */
    do {
      next_hist_ts += boost::posix_time::microseconds(HTB_WMA_SLOT_SIZE_MICROSEC);
      hist_slotix = (hist_slotix + 1) % HTB_WMA_LENGTH;
      recent_iops[hist_slotix] = 0;
    } while (now > next_hist_ts);
    recent_iops[hist_slotix]++;    
  }
  
  return (std::atomic_fetch_sub(&queued_io_counter, (unsigned int)1) - 1);
}

/* This is an array of powers of 0.9 */
const double TBQueueState::kweights[HTB_MAX_WMA_LENGTH] = 
  {1.0000, 0.9000, 0.8100, 0.7290, 0.6561, 0.5905, 0.5314, 0.4783, 0.4305, 0.3874, 
   0.3487, 0.3138, 0.2824, 0.2541, 0.2288, 0.2059, 0.1853, 0.1668, 0.1501, 0.1351};
double TBQueueState::getIOPerfWMA()
{
  /* calculating weighted average over recent history using 
   * formula from Zygaria paper [RTAS 2006] */
  double wma = 0.0;
  int ix = (hist_slotix + 1) % HTB_WMA_LENGTH; // oldest history slot  
  for (int i = (HTB_WMA_LENGTH-1); i >= 0; --i)
    {
      wma += (recent_iops[ix] * kweights[i]);
      ix = (ix + 1) % HTB_WMA_LENGTH;
    }
  return wma;
}

/* Updates both token buckets and returns number of expired 'assured' tokens */   
fds_uint64_t TBQueueState::updateTokens(void)
{
  /* TODO: we can make it more efficient by getting the current time here
   * and adding token bucket methods that take current time (since 
   * get time seems quite expensive), will do later, we may anyway try 
   * to use chrono nanosec timers (cannot compile for some reason). */
  boost::posix_time::ptime now = boost::posix_time::microsec_clock::universal_time();
  moveToNextHistTs(now);

  tb_max.updateTBState();
  return tb_min.updateTBState();
}

/* will consume 'io_cost' tokens from tb_min if they are available, there is at 
 * least one queued IO, and we are not over the max rate limit, otherwise 
 * return one of the states */
TBQueueState::tbStateType TBQueueState::tryToConsumeAssuredTokens(fds_uint32_t io_cost)
{
  unsigned int queued_ios = std::atomic_load(&queued_io_counter);
  if (queued_ios == 0) {
    return TBQUEUE_STATE_QUEUE_EMPTY;
  }

  /* check if we reached max_rate */
  if ( !tb_max.hasTokens(io_cost) ) {
    return TBQUEUE_STATE_NO_TOKENS;
  }
  
  /* finally, try to consume tokens from tb_min */
  if ( !tb_min.tryToConsumeTokens(io_cost) ) {
    return TBQUEUE_STATE_NO_ASSURED_TOKENS;
  }

  /* we consumed to assured tokens */
  tb_max.tryToConsumeTokens(io_cost);
  /* it's expected that we should always have tokens in tb_max
  * when there are tokens in tb_min -- not asserting for now */
  return TBQUEUE_STATE_OK;
}


} // namespace fds
