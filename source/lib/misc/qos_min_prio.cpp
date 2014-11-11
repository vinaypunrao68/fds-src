/*
 * Copyright 2013 Formation Data Systems, Inc.
 * 
 */

#include "qos_min_prio.h"

namespace fds {

QoSMinPrioDispatcher::QoSMinPrioDispatcher(FDS_QoSControl *ctrl, fds_log* log, fds_uint32_t _total_rate)
  : FDS_QoSDispatcher(ctrl, log, _total_rate),
    wait_time_microsec(DEFAULT_MP_ASSURED_WAIT_MICROSEC)
{
  last_dispatch_qid = 0; /* this should be an invalid id */ 
  max_outstanding_ios = 20;
}

QoSMinPrioDispatcher::~QoSMinPrioDispatcher()
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

Error QoSMinPrioDispatcher::registerQueue(fds_qid_t queue_id,
					  FDS_VolumeQueue *queue)
{
  Error err(ERR_OK);

  /* we need a new queue state to control new queue */
  TBQueueState *qstate = new TBQueueState(queue_id,
					  queue->iops_min,
					  10000,
					  queue->priority,
					  wait_time_microsec,
					  20);

  if (!qstate) {
    FDS_PLOG_SEV(qda_log, fds::fds_log::error) 
      << "QoSMinPrioDispatcher: failed to create queue state for queue " << queue_id;
    err = ERR_MAX;
    return err;
  }

  qda_lock.write_lock();

  /* call base class to actually add queue */
  err = FDS_QoSDispatcher::registerQueueWithLockHeld(queue_id, queue);
  if (!err.ok()) {
    qda_lock.write_unlock();
    delete qstate;
    return err;
  }

  /* base class already checked that queue_id is valid and not already registered */
  /* add queue state to map */
  qstate_map[queue_id] = qstate;
  qda_lock.write_unlock();

  FDS_PLOG_SEV(qda_log, fds::fds_log::notification) 
    << "QoSMinPrioDispatcher: registered queue " << queue_id
    << " with min_iops=" << queue->iops_min
    << " ; prio=" << queue->priority;

  return err;
}

Error QoSMinPrioDispatcher::deregisterQueue(fds_qid_t queue_id)
{
  Error err(ERR_OK);
  TBQueueState *qstate = NULL;

  qda_lock.write_lock();

  /* call base class to remove queue first */
  err = FDS_QoSDispatcher::deregisterQueueWithLockHeld(queue_id);
  /* if error, still try to remove queue state first before returning */

  if (qstate_map.count(queue_id) == 0) {
    qda_lock.write_unlock();
    err = ERR_DUPLICATE;  /* we probably got same error from base class, but still good to check if queue state exists */
    return err;
  }
  qstate = qstate_map[queue_id];
  qstate_map.erase(queue_id);
  qda_lock.write_unlock();

  delete qstate;

  FDS_PLOG_SEV(qda_log, fds::fds_log::notification) 
    << "QoSMinPrioDispatcher: deregistered queue " << queue_id;

  return err;
}

void QoSMinPrioDispatcher::ioProcessForEnqueue(fds_qid_t queue_id, 
					       FDS_IOType *io)
{
  TBQueueState* qstate = qstate_map[queue_id];
  assert(qstate);
  FDS_PLOG(qda_log) << "QoSMinPrioDispatcher: handling enqueue IO to queue " << queue_id;
  qstate->handleIoEnqueue(io);
}

void QoSMinPrioDispatcher::ioProcessForDispatch(fds_qid_t queue_id,
						FDS_IOType *io)
{
  TBQueueState* qstate = qstate_map[queue_id];
  assert(qstate);

  /* this will update performance history and number of queued ios */
  qstate->handleIoDispatch(io);

  /* number of pending ios is updated in the base class, at this point
   * it's not substracted yet */
}

/* Finds queue whose IO needs to be dispatched next 
 * Assumption -- if this function is called, it means that 
 * current number of outstanding IOs is less than maximum,
 * AND there is at least one IO pending in at least one queue */
fds_qid_t QoSMinPrioDispatcher::getNextQueueForDispatch()
{
  fds_qid_t ret_qid = 0;
  TBQueueState *dispatch_qstate = NULL;
  double min_wma;
  uint min_wma_hiprio;

  /* this is work-conserving dispatcher, since this function is called only when:
   * 1) we have at least one pending IO (in any queue); AND
   * 2) the current number of outstanding IOs < max */  

  qstate_map_it_t end_it = qstate_map.find(last_dispatch_qid);
  if (end_it == qstate_map.end()) {
    /* the queue we dispatched IO from last time seems to be removed */
    end_it = qstate_map.begin();
  }
  qstate_map_it_t it = end_it;
  it++;
  if (it == qstate_map.end()) {
    it = qstate_map.begin();
  }

  last_dispatch_qid = it->second->queue_id;

  for (uint i = 0; i < qstate_map.size(); ++i) {
    TBQueueState *qstate = it->second;
    assert(qstate != NULL);

    /* before querying any state, update assured tokens */
    fds_uint64_t now = util::getTimeStampMicros();
    fds_uint64_t exp_assured_toks = qstate->updateTokens(now);
    /* we only getting expired assured tokens for debugging */
    if (exp_assured_toks > 0) {
      FDS_PLOG_SEV(qda_log, fds::fds_log::debug) 
	<< "QoSMinPrioDispatcher: queue " 
	<< qstate->queue_id 
	<< " lost " << exp_assured_toks << " assured tokens";
    }

    /* check if we can serve the IO from the head of queue with assured tokens */
    TBQueueState::tbStateType state = qstate->tryToConsumeAssuredTokens(1);
    if (state == TBQueueState::TBQUEUE_STATE_OK) {
      /* we found a queue whose IO we need to dispatch to meet its min_iops */
      last_dispatch_qid = it->first;
      FDS_PLOG(qda_log) << "QoSMinPrioDispatcher:: dispatch (min_iops) io from queue " << it->first;
      return it->first;
    }
    else if (state == TBQueueState::TBQUEUE_STATE_NO_ASSURED_TOKENS) {
      /* this queue has at least one IO */
      double q_wma = qstate->getIOPerfWMA();
      if (!dispatch_qstate) {
	min_wma = q_wma;
	dispatch_qstate = qstate;
	min_wma_hiprio = qstate->priority;
      }
      else if (qstate->priority < min_wma_hiprio) {
	/* assuming higher priority has a lower priority number (highest == 1) */
	min_wma_hiprio = qstate->priority;
	min_wma = q_wma;
	dispatch_qstate = qstate;
      }
      else if ((qstate->priority == min_wma_hiprio) && (q_wma < min_wma)) {
	min_wma = q_wma;
	dispatch_qstate = qstate;
      }
    }

    /* next queue */
    ++it;
    if (it == qstate_map.end())
      it = qstate_map.begin();
  }

  /* we did not find any queue that has IOs and need to meet its iops_min */
  /* however, if we did not find any other queue to dispatch from, then we are in wrong state */
  assert(dispatch_qstate);
  ret_qid = dispatch_qstate->queue_id;
  FDS_PLOG(qda_log) << "QoSMinPrioDispatcher: dispatch (avail) io from queue " << ret_qid;

  last_dispatch_qid = ret_qid;
  return ret_qid;
}

} // namespace fds
