#ifndef __FDS_FDS_QOS_H__
#define __FDS_FDS_QOS_H__
#include <fds_types.h>
#include <fds_err.h>
#include <util/Log.h>
#include <concurrency/RwLock.h>
#include <unordered_map>
#include <mutex> // std::mutex, std::unique_lock
#include <condition_variable> 
#include <atomic>
#include "qos_ctrl.h"


namespace fds {

  typedef std::unordered_map<fds_uint32_t,FDS_VolumeQueue *> queue_map_t;
  class FDS_QoSControl;

  class FDS_QoSDispatcher {

  public:

    fds_log *qda_log;
    queue_map_t queue_map;
    FDS_QoSControl *parent_ctrlr;
    float current_throttle_level;
    fds_uint64_t total_svc_rate;
    fds_rwlock qda_lock; // Protects queue_map (and any high level structures in derived class) 
                         // from events like volumes being inserted or removed during enqueue IO or dispatchIO.

    // std::mutex ios_pending_mtx; // to protect the counter num_pending_ios
    // std::condition_variable ios_pending_cv; // Condn variable to signal dispatcher when there is an IO available, from enqueueIO thread.
    // fds_uint32_t num_pending_ios; // ios pending, aggregated across all queues, protected by ios_pending_mtx;
    std::atomic<unsigned int> num_pending_ios;
 
    virtual fds_uint32_t getNextQueueForDispatch() = 0;

    FDS_QoSDispatcher() {}
    FDS_QoSDispatcher(FDS_QoSControl *ctrlr, fds_log *log, fds_uint64_t total_server_rate) {
      parent_ctrlr = ctrlr;
      qda_log = log;
      total_svc_rate = total_server_rate;
      num_pending_ios = ATOMIC_VAR_INIT(0);
    }
    ~FDS_QoSDispatcher() {
    }

    Error registerQueueWithLockHeld(fds_uint32_t queue_id, FDS_VolumeQueue *queue) {

      Error err(ERR_OK);
      
      if (queue_map.count(queue_id) != 0) {	
	err = ERR_DUPLICATE;
	return err;
      }
      queue_map[queue_id] = queue;

      return err;
    }

    virtual Error registerQueue(fds_uint32_t queue_id, FDS_VolumeQueue *queue ) {
      Error err(ERR_OK);
      qda_lock.write_lock();
      err = registerQueueWithLockHeld(queue_id, queue);
      qda_lock.write_unlock();
      return err;
    }

    Error deregisterQueueWithLockHeld(fds_uint32_t queue_id) {
      Error err(ERR_OK);
      if  (queue_map.count(queue_id) == 0) {
	err = ERR_INVALID_ARG;
	return err;
      }
      FDS_VolumeQueue *que = queue_map[queue_id];
      // Assert here that queue is empty, when one such API is available.
      queue_map.erase(queue_id);
      return err;
    }

    virtual Error deregisterQueue(fds_uint32_t queue_id) {
      Error err(ERR_OK);      
      qda_lock.write_lock();
      err = deregisterQueueWithLockHeld(queue_id);
      qda_lock.write_unlock();
      return err;
    }

    // Assumes caller has the qda read lock
    virtual void ioProcessForEnqueue(fds_uint32_t queue_id, FDS_IOType *io)
    {
      // preprocess before enqueuing in the input queue
      FDS_PLOG(qda_log) << "Request " << io->io_req_id << " being enqueued at queue " << queue_id; 
    }

    // Assumes caller has the qda read lock
    virtual void ioProcessForDispatch(fds_uint32_t queue_id, FDS_IOType *io)
    {
      // do necessary processing before dispatching the io
      FDS_PLOG(qda_log) << "Dispatching " << io->io_req_id << " from queue " << queue_id; 
    }

    // Quiesce queued IOs on this queue & block any new IOs
    virtual void quiesceIOs(fds_uint32_t queue_id)  {
    Error err(ERR_OK);
      if (queue_map.count(queue_id) != 0) {	
	return;
      }
      FDS_VolumeQueue *que = queue_map[queue_id];
      que->quiesceIOs();

    }

    virtual void suspendQueue(fds_uint32_t queue_id) {
      if (queue_map.count(queue_id) != 0) {	
	return;
      }
      FDS_VolumeQueue *que = queue_map[queue_id];
      que->suspendIO();

    }

    virtual void resumeQueue(fds_uint32_t queue_id) {
      if (queue_map.count(queue_id) != 0) {	
        return;
      }
      FDS_VolumeQueue *que = queue_map[queue_id];
      que->resumeIO();

    } 

    virtual void setThrottleLevel(float throttle_level) {
      assert((throttle_level >= -10) && (throttle_level <= 10));
      current_throttle_level = throttle_level;
      FDS_PLOG(qda_log) << "Dispatcher adjusting current throttle level to " << throttle_level;
    }

    virtual Error enqueueIO(fds_uint32_t queue_id, FDS_IOType *io) {

      Error err(ERR_OK);

      qda_lock.read_lock();
      FDS_VolumeQueue *que = queue_map[queue_id];
      
      que->enqueueIO(io);
      
      ioProcessForEnqueue(queue_id, io);  
      
      // do while loop is just to limit the scope of the unique_lock, limiting the time we hold ios_pending_mtx
#if 0
      do {
	std::unique_lock<std::mutex> lck(ios_pending_mtx);
	num_pending_ios++;
	if (num_pending_ios == 1) {
	  // wakeup scheduler here.
	  ios_pending_cv.notify_all();
	}
      } while (0);
#endif
      fds_uint32_t n_pios;
      n_pios = atomic_fetch_add(&(num_pending_ios), (unsigned int)1);
      FDS_PLOG(qda_log) << "Dispatcher: enqueueIO: # of pending ios = " << n_pios+1;
      assert(n_pios >= 0);

      qda_lock.read_unlock();
      return err;

    }

    virtual Error dispatchIOs() {

      Error err(ERR_OK);
      pthread_t this_thread = pthread_self();
      struct sched_param params;
      int ret = 0;

      params.sched_priority = sched_get_priority_max(SCHED_FIFO);
      FDS_PLOG(qda_log) << "Dispatcher: trying to set dispatcher thread realtime prio = " << params.sched_priority;
 
      /* Attempt to set thread real-time priority to the SCHED_FIFO policy */
      ret = pthread_setschedparam(this_thread, SCHED_FIFO, &params);
      if (ret != 0) {
   	FDS_PLOG(qda_log) << "Dispatcher: Unsuccessful in setting scheduler thread realtime prio";
      }
      else {
      /* success - Now verify the change in thread priority */
	int policy = 0;
	ret = pthread_getschedparam(this_thread, &policy, &params);
	if (ret != 0) {
	  FDS_PLOG(qda_log) << "Dispatcher: Couldn't retrieve real-time scheduling parameters for dispatcher thread";
	}
 
	/* Check the correct policy was applied */
	if(policy != SCHED_FIFO) {
	  FDS_PLOG(qda_log) << "Dispatcher:Scheduling is NOT SCHED_FIFO!";
	} else {
	  FDS_PLOG(qda_log) << "Dispatcher: thread SCHED_FIFO OK";
	}
 
	/* Print thread scheduling priority */
	FDS_PLOG(qda_log) << "Dispatcher: Scheduler thread priority is " << params.sched_priority;
      }
      
      while(1) {

	parent_ctrlr->waitForWorkers();

#if 0
	do {
	  std::unique_lock<std::mutex> lck(ios_pending_mtx);
	  while (num_pending_ios == 0) {
	    ios_pending_cv.wait(lck);
	  }
	} while (0);
#endif
	fds_uint32_t n_pios = 0;
	while (1) {
	  n_pios = atomic_load(&num_pending_ios);
	  if (n_pios > 0) {
	    break;
	  }
	  boost::this_thread::sleep(boost::posix_time::microseconds(1000000/total_svc_rate));
	} 
	
	qda_lock.read_lock();

	fds_uint32_t queue_id = getNextQueueForDispatch();
	FDS_VolumeQueue *que = queue_map[queue_id];

	FDS_IOType *io = que->dequeueIO();
	assert(io != NULL);

	ioProcessForDispatch(queue_id, io);
#if 0
	do {
	  std::unique_lock<std::mutex> lck(ios_pending_mtx);
	  num_pending_ios --;
	} while (0);
#endif

	qda_lock.read_unlock();

	parent_ctrlr->processIO(io);

	n_pios = 0;
	n_pios = atomic_fetch_sub(&(num_pending_ios), (unsigned int)1);
	assert(n_pios >= 1);
	
      }

      return err;
    }

    virtual Error markIODone(FDS_IOType *io) {
      Error err(ERR_OK);
      FDS_PLOG(qda_log) << "IO Request " << io->io_req_id << " completed in " << io->io_service_time << " usecs.";
      return err;
    }
    
  };
}

#endif
