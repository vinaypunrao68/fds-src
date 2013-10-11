/*
 * Copyright 2013 Formation Data Systems, Inc.
 *
 *  Hierarchical token bucket algorithm
 * 
 */
#include "qos_htb.h"

#define INFINITE_BURST                        987654321
#define DEFAULT_ASSURED_WAIT_TIME_MICROSEC    100000   

namespace fds {

/***** QoSHTBDispatcher implementation ******/
QoSHTBDispatcher::QoSHTBDispatcher(FDS_QoSControl *ctrl, fds_uint64_t _total_rate)
  : FDS_QoSDispatcher(ctrl),
    total_rate(_total_rate),
    wait_time_microsec(DEFAULT_ASSURED_WAIT_TIME_MICROSEC),
    pool(0, _total_rate, DEFAULT_ASSURED_WAIT_TIME_MICROSEC)
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
				      FDS_VolumeQueue *queue,
				      fds_uint64_t q_min_rate,
				      fds_uint32_t q_priority)
{
  Error err(ERR_OK);
   
  /* we need a new state to control new queue */
  TBQueueState *qstate = new TBQueueState(q_min_rate, q_min_rate, default_burst_size);
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
  err = FDS_QoSDispatcher::registerQueueWithLockHeld(queue_id, queue, q_min_rate, q_priority);
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
    TBQueueState::tbStateType state = qstate->tryToConsumeAssuredTokens();

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


/******  TokenBucket implementation  ****/
TokenBucket::TokenBucket(fds_uint64_t _rate, fds_uint64_t _burst)
  :rate(_rate), burst(_burst)
{
  /* we start with 0 tokens */
  t_last_update = boost::posix_time::microsec_clock::universal_time();
  token_count = 0.0;
}

TokenBucket::~TokenBucket() 
{
}

/* generate tokens since t_last_update and add to token counter
 * but do not cap the tokens to burst size  */
inline void TokenBucket::updateTokensOnly(void)
{
  boost::posix_time::ptime now = boost::posix_time::microsec_clock::universal_time();
  boost::posix_time::time_duration elapsed = now - t_last_update;
  fds_uint64_t elapsed_microsec = elapsed.total_microseconds();

  if (elapsed_microsec > 0) {
    token_count += ((double)elapsed_microsec / 1000000.0) * (double) rate;  
    t_last_update = now; 
  }
}

/* 
 * Starting from now, token bucket will accumulate tokens with new rate
 * and cap them to new burst size 
 */
inline void TokenBucket::modifyParams(fds_uint64_t _rate, fds_uint64_t _burst)
{
  /* first update tokens using old rate, but do not cap to burst size now,
   * updateTokens() call will do it since it returns number of spilled tokens */
  updateTokensOnly();

  rate = _rate;
  burst = _burst;  
}

inline void TokenBucket::modifyRate(fds_uint64_t _rate)
{
  modifyParams(_rate, burst);
}

/* update tokens and cap to burst size, return number of tokens that are spilled over burst size */
inline fds_uint64_t TokenBucket::updateTBState(void)
{
  fds_uint64_t expired_tokens= 0;

  updateTokensOnly();

  /* cap token counter to burst size */
  if ( (fds_uint64_t)token_count > burst) {
     expired_tokens = burst - (fds_uint64_t)token_count;
     token_count = (double)burst;
  }
  return expired_tokens;
}

/* Assumes token bucket state is up to date */
inline fds_bool_t TokenBucket::tryToConsumeTokens(fds_uint64_t num_tokens)
{
   if ((double)num_tokens <= token_count) {
       token_count -= (double)num_tokens;
       return true;
   }
   return false;
}

/* Assumes token bucket state is up to date */
inline fds_uint64_t TokenBucket::tryToConsumeTokensOrGetDelay(fds_uint64_t num_tokens)
{
   if (!tryToConsumeTokens(num_tokens)) {
      double tokens_needed = token_count - (double)num_tokens;
      assert(tokens_needed > 0.0); /* otherwise tryToConsumeTokens would return false */

      double tdelta_microsec = (tokens_needed / (double) rate ) * 1000000.0;     
      return (fds_uint64_t)tdelta_microsec;
   }
   
   return 0;
}

/**** ControlledTokenBucket implementation *******/
ControlledTokenBucket::ControlledTokenBucket(fds_uint64_t _rate)
  :TokenBucket(_rate, INFINITE_BURST)
{
}

ControlledTokenBucket::~ControlledTokenBucket()
{
}

inline void ControlledTokenBucket::addTokens(fds_uint64_t num_tokens)
{
  token_count += (double)num_tokens;
  /* we still cap to 'infinite' burst size to not overflow the uint64 token_count */
  if (token_count > (double)burst) 
      token_count = (double)burst;
}

inline fds_uint64_t ControlledTokenBucket::removeTokens(fds_uint64_t num_tokens)
{
  if ((double)num_tokens < token_count) {
     token_count -= (double)num_tokens;
  } 
  else {
     token_count = 0.0;
  }
}

/******* TBQueueState implementation ***********/
TBQueueState::TBQueueState(fds_uint64_t _min_rate, 
			   fds_uint64_t _max_rate,
			   fds_uint64_t _burst_size)
  : min_rate(_min_rate), max_rate(_max_rate), burst_size(_burst_size),
  recent_ma_rate(0.0), tb_min(_min_rate, _burst_size), tb_max(_max_rate, _burst_size)
{
  queued_io_counter = ATOMIC_VAR_INIT(0);
}

TBQueueState::~TBQueueState() 
{
}

void TBQueueState::modifyParameters(fds_uint64_t _min_rate,
				    fds_uint64_t _max_rate,
				    fds_uint64_t _burst_size)
{
  tb_min.modifyParams(_min_rate, _burst_size);
  tb_max.modifyParams(_max_rate, _burst_size);
}

void TBQueueState::modifyMinRate(fds_uint64_t _min_rate)
{
  tb_min.modifyRate(_min_rate);
}

void TBQueueState::modifyMaxRate(fds_uint64_t _max_rate)
{
  tb_max.modifyRate(_max_rate);
}

/* for now assuming constant IO cost, so simple implementation  */
inline fds_uint32_t TBQueueState::handleIoEnqueue(FDS_IOType* /*io*/)
{
  return (std::atomic_fetch_add(&queued_io_counter, (unsigned int)1) + 1);
}

/* for now assuming constant IO cost */
inline fds_uint32_t TBQueueState::handleIoDispatch(FDS_IOType* /*io*/)
{
  return (std::atomic_fetch_sub(&queued_io_counter, (unsigned int)1) - 1);
}

fds_uint64_t TBQueueState::updateTokens(void)
{
  return 0; //TBD
}

TBQueueState::tbStateType TBQueueState::tryToConsumeAssuredTokens()
{
  unsigned int queued_ios = std::atomic_load(&queued_io_counter);
  if (queued_ios == 0) {
    return TBQUEUE_STATE_QUEUE_EMPTY;
  }

  return TBQUEUE_STATE_OK;
}

inline void TBQueueState::consumeTokens(void)
{
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
