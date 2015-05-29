/*
 * Copyright 2013 Formation Data Systems, Inc.
 *
 * Dispatcher that guarantees min iops for each 
 * queue and shares available resources by priority.
 * We have an available resource if 
 * 
 */

#ifndef SOURCE_LIB_QOS_MIN_PRIO_H
#define SOURCE_LIB_QOS_MIN_PRIO_H

#include "qos_htb.h" /* using TBQueueState class */
#include "fds_qos.h"

#define DEFAULT_MP_ASSURED_WAIT_MICROSEC    250000

namespace fds {

/*
 * Controls a set of queues to guarantee a min rate
 * and sharing available resources based on priorities
 *
 * Resources are considered available if the current number of 
 * outstanding IOs are less than maximum number of outstanding 
 * ios. If a queue has a min iops reservation, it holds a set
 * of tokens that are generated with min iops rate, but can hold
 * only up to a max number of tokens. The max number of tokens 
 * represent how long we can wait for a IO before the guaranteed
 * portion of resource expire. This allows workloads to arrive in 
 * burst but still get their guaranteed.
 *
 * The available resources are shared based on queue's priority.
 *
 * Configurable parameters:
 *  'wait_time" -- how long we wait for IOs to arrive before we 
 *                 expire queue's guaranteed token/credit.
 *  'outstand_count' -- number of outstanding IOs at the 
 *                 underlying storage system (in base class)
 *
 */
class QoSMinPrioDispatcher: public FDS_QoSDispatcher
{
public:
  QoSMinPrioDispatcher(FDS_QoSControl* ctrl, fds_log* log, fds_uint32_t _outstand_count);
  ~QoSMinPrioDispatcher() = default;

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

  using FDS_QoSDispatcher::dispatchIOs;

private:
  /********* configurable parameters ********/

  /* How long we wait before expiring 'guaranteed' tokens that represent 
   * assured rate of a queue. This means that a workload with delays between
   * IOs longer than this interva will not meet its guaranteed rate. The smaller
   * this interval, the smaller burst of 'guaranteed' iops will go ahead of 
   * other ios. */
  fds_uint64_t wait_time_microsec;

  /******** dynamic state **********/
  qstate_map_type qstate_map; /* min rate control and recent iops history for priority sharing */

  fds_qid_t last_dispatch_qid; /* queue id from which we dispatched last IO */
};

} // namespace fds


#endif  /* SOURCE_LIB_QOS_MIN_PRIO_H*/
