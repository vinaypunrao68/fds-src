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
    : FDS_QoSDispatcher(ctrl, log),
    total_rate(_total_rate),
    wait_time_microsec(DEFAULT_ASSURED_WAIT_MICROSEC),
    pool(0, _total_rate, DEFAULT_ASSURED_WAIT_MICROSEC)
{
  /* TODO: we need a good per-volume burst size value, sholdn't this be a max queue size at SM/DM? */
  default_burst_size = 100; 

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
  if (_total_rate > pool.getMinRate())
    {
      total_rate = _total_rate;
      /* only need to modify avail rate */
      pool.modifyAvailRate(total_rate - pool.getMinRate());
    }
  else 
    {
      /* for now we are not letting total rate go below volumes' total min rate */
      err = ERR_INVALID_ARG;
      return err;
    }

  return err;
}

Error QoSHTBDispatcher::registerQueue(fds_uint32_t queue_id,
				      FDS_VolumeQueue *queue)
{
  Error err(ERR_OK);
  fds_uint64_t q_min_rate = queue->iops_min;
  fds_uint64_t q_max_rate = queue->iops_max;
  fds_uint32_t q_priority = queue->priority;
   
  /* we need a new state to control new queue */
  TBQueueState *qstate = new TBQueueState(q_min_rate, q_max_rate, default_burst_size);
  if (!qstate) {
    FDS_PLOG(qda_log) << "QoSHTBDispatcher: failed to allocate memory for queue state for queue: " << queue_id;
    err = ERR_MAX;
    return err;
  }

  /* for now, do not allow total min rate to decrease below total rate -- return error
   *  TODO: should we scale down all volumes' min rates? */
  qda_lock.write_lock();
 
  if ((q_min_rate + pool.getMinRate()) > total_rate) {
    qda_lock.write_unlock();
    delete qstate;
    err = ERR_INVALID_ARG;
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
  pool.incMinRate(q_min_rate);  
  qstate_map[queue_id] = qstate;
  qda_lock.write_unlock();

  return err;
}

Error QoSHTBDispatcher::deregisterQueue(fds_uint32_t queue_id)
{
  Error err(ERR_OK);
  TBQueueState* qstate = NULL;

  qda_lock.write_lock();
  /* call base class to remove queue */
  err = FDS_QoSDispatcher::deregisterQueueWithLockHeld(queue_id);
  /* if error, still try to remove queue state first before returning */
  
  if (qstate_map.count(queue_id) == 0) {
    qda_lock.write_unlock();
    err = ERR_DUPLICATE; /* we probably got same error from base class, but still good to check if queue state exists */
    return err;
  }

  qstate = qstate_map[queue_id];
  qstate_map.erase(queue_id);

  /* make sure to update pool that our min rate decreased */
  pool.decMinRate(qstate->getMinRate());
  qda_lock.write_unlock();

  /* cleanup */
  delete qstate;

  return err;
}

void QoSHTBDispatcher::ioProcessForEnqueue(fds_uint32_t queue_id,
					   FDS_IOType *io)
{
  TBQueueState* qstate = qstate_map[queue_id];
  qstate->handleIoEnqueue(io);
}

void QoSHTBDispatcher::ioProcessForDispatch(fds_uint32_t queue_id,
					   FDS_IOType *io)
{
  /* we already updated tokens in getNextQueueForDispatch(), 
   * so only update number of queued ios  */
  TBQueueState* qstate = qstate_map[queue_id];
  qstate->handleIoDispatch(io);
}

/* find queue whose IO needs to be dispatched next */
fds_uint32_t QoSHTBDispatcher::getNextQueueForDispatch()
{
  fds_uint32_t qid = 0;
  fds_uint64_t toks = 0;  

  /* all token buckets are demand-driven, so make sure to update state first */
  toks = pool.updateState();
  if (toks == 0) {
    //TODO: need func for waiting, so continue
  }

  /* NOT a complete implementation yet, fow now doing round-robin */
  qstate_map_it_t end_it = qstate_map.find(last_dispatch_qid); /* the queue we got last time will be the last to check now */
  if (end_it == qstate_map.end()) {
    /* the queue we dispatched IO last time seems to be removed */
    end_it = qstate_map.begin();
  }
  qstate_map_it_t it = end_it;
  it++;
  
  while (it != end_it) {
    TBQueueState *qstate = it->second;
      /* before quering any state, update tokens */
    fds_uint64_t toks = qstate->updateTokens();
    TBQueueState::tbStateType state = qstate->tryToConsumeAssuredTokens(1);

    if (state != TBQueueState::TBQUEUE_STATE_QUEUE_EMPTY) {
      return it->first; /* basically find next non-empty qeueue -- TEMP */
    }

    /* next queue */
    ++it;
    if (it == qstate_map.end())
      it = qstate_map.begin();
  }
  
  /* we did not find any queue -- this should not happen, the base class method will assert  */
  return qid;
}

/******* TBQueueState implementation ***********/
TBQueueState::TBQueueState(fds_uint64_t _min_rate, 
			   fds_uint64_t _max_rate,
			   fds_uint64_t _burst_size)
  : burst_size(_burst_size),
    tb_min(_min_rate, _burst_size), 
    tb_max(_max_rate, _burst_size)
{
  queued_io_counter = ATOMIC_VAR_INIT(0);
  memset(recent_iops, 0, sizeof(fds_uint32_t) * HTB_WMA_LENGTH);
  next_hist_ts = boost::posix_time::microsec_clock::universal_time() + 
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
    next_hist_ts += boost::posix_time::microseconds(HTB_WMA_SLOT_SIZE_MICROSEC);
    do {
      hist_slotix = (hist_slotix + 1) % HTB_WMA_LENGTH;
      recent_iops[hist_slotix] = 0;
    } while (now > next_hist_ts);
    recent_iops[hist_slotix]++;    
  }
  
  return (std::atomic_fetch_sub(&queued_io_counter, (unsigned int)1) - 1);
}

/* This is an array of powers of 0.9 */
const double TBQueueState::kweights[HTB_WMA_LENGTH] = 
  {1.0000, 0.9000, 0.8100, 0.7290, 0.6561, 0.5905, 0.5314, 0.4783, 0.4305, 0.3874, 
   0.3487, 0.3138, 0.2824, 0.2541, 0.2288, 0.2059, 0.1853, 0.1668, 0.1501, 0.1351};
double TBQueueState::getIOPerfWMA()
{
  /* calculating weighted average over recent history using 
   * formula from Zygaria paper [RTAS 2006] */
  double wma = 0.0;
  int ix = hist_slotix;
  for (int i = 0; i < HTB_WMA_LENGTH; ++i)
    {
      wma += (recent_iops[ix] * kweights[i]);
      ix = (ix + 1) % HTB_WMA_LENGTH;
    }
  return wma;
}

/* Updates both token buckets and returns number of spilled tokens from tb_max */
fds_uint64_t TBQueueState::updateTokens(void)
{
  /* TODO: we can make it more efficient by getting the current time here
   * and adding token bucket methods that take current time (since 
   * get time seems quite expensive), will do later, we may anyway try 
   * to use chrono nanosec timers (cannot compile for some reason). */
  tb_min.updateTBState();
  return tb_max.updateTBState();
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

/***** HTBPoolState implementation *****/
HTBPoolState::HTBPoolState(fds_uint64_t _min_rate,
			   fds_uint64_t _avail_rate,
			   fds_uint64_t _wait_time_microsec)
  : min_rate(_min_rate),
    avail_rate(_avail_rate),
    tb_min(_min_rate, (fds_uint64_t)((double)_wait_time_microsec / 1000000.0 * (double)_min_rate)),
    tb_avail(_avail_rate)
{
}

HTBPoolState::~HTBPoolState()
{
}

void HTBPoolState::modifyParameters(fds_uint64_t _min_rate,
				    fds_uint64_t _avail_rate,
				    fds_uint64_t _wait_time_microsec)
{
  tb_min.modifyParams(_min_rate, (fds_uint64_t)( (double)_wait_time_microsec / 1000000.0 * (double)_min_rate));
  tb_avail.modifyRate(_avail_rate);
}

void HTBPoolState::modifyAvailRate(fds_uint64_t _avail_rate)
{
  avail_rate = _avail_rate;
  tb_avail.modifyRate(avail_rate);
}

/* assumes all checks are done by parent class, if increasing min rate 
 * decreases avail_rate below zero, just sets avail_rate to 0 */
void HTBPoolState::incMinRate(fds_uint64_t add_min_rate)
{
  min_rate += add_min_rate;
  if (avail_rate > add_min_rate) {
    avail_rate -= add_min_rate;
  }  
  else {
    avail_rate = 0;
  }

  /* update token buckets */
  tb_min.modifyRate(min_rate);
  tb_avail.modifyRate(avail_rate);
}

/* assumes that all check are done by parent class. Assers if total min rate will decrease below 0 */
void HTBPoolState::decMinRate(fds_uint64_t dec_min_rate)
{
  assert(dec_min_rate <= min_rate);
  min_rate -= dec_min_rate;
  avail_rate += dec_min_rate;

  /* update token buckets */
  tb_min.modifyRate(min_rate);
  tb_avail.modifyRate(avail_rate);
}


fds_uint64_t HTBPoolState::updateState()
{
  return 0;
}

inline fds_uint64_t HTBPoolState::getAvailableTokens()
{
  return 0;
}

void HTBPoolState::consumeAssuredTokens()
{
}

void HTBPoolState::consumeAvailableTokens()
{
}


} // namespace fds
