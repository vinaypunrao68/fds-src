#ifndef __FDS_FDS_QOS_H__
#define __FDS_FDS_QOS_H__
#include <fds_types.h>
#include <fds_err.h>
#include <util/Log.h>
#include <concurrency/RwLock.h>
#include <concurrency/Mutex.h>
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
    fds_uint32_t max_outstanding_ios;
    fds_rwlock qda_lock; // Protects queue_map (and any high level structures in derived class) 
                         // from events like volumes being inserted or removed during enqueue IO or dispatchIO.

    std::atomic<unsigned int> num_pending_ios;
    std::atomic<unsigned int> num_outstanding_ios;

    fds_uint32_t max_svc_time = 0;
    fds_uint32_t min_svc_time = 0;
    fds_uint64_t total_svc_time = 0;

    fds_uint32_t max_wait_time = 0;
    fds_uint32_t min_wait_time = 0;
    fds_uint64_t total_wait_time = 0;

    fds_mutex *stats_mutex;
    fds_uint64_t num_ios_completed = 0;
    fds_uint64_t num_ios_dispatched = 0;

    std::atomic_bool shuttingDown;

    virtual fds_uint32_t getNextQueueForDispatch() = 0;
    

  FDS_QoSDispatcher() :
    shuttingDown(false) {
    }
  FDS_QoSDispatcher(FDS_QoSControl *ctrlr,
                      fds_log *log,
                      fds_uint64_t total_server_rate) :
    FDS_QoSDispatcher() {
      parent_ctrlr = ctrlr;
      qda_log = log;
      total_svc_rate = total_server_rate;
      max_outstanding_ios = 0;
      num_pending_ios = ATOMIC_VAR_INIT(0);
      num_outstanding_ios = ATOMIC_VAR_INIT(0);
      stats_mutex = new fds_mutex("QoSDispatcherMutex");
    }
    ~FDS_QoSDispatcher() {
      shuttingDown = true;
    }

    void stop() {
      shuttingDown = true;
    }

    Error registerQueueWithLockHeld(fds_uint32_t queue_id, FDS_VolumeQueue *queue) {

      Error err(ERR_OK);
      
      if (queue_map.count(queue_id) != 0) {	
	err = ERR_DUPLICATE;
	return err;
      }
      queue_map[queue_id] = queue;

	FDS_PLOG(qda_log) << "Dispatcher: registering queue with min - "
			<< queue->iops_min << ", max - " << queue->iops_max
			<< ", priority - " << queue->priority
			<< ", total server rate = " << total_svc_rate; 

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

      io->enqueue_time = boost::posix_time::microsec_clock::universal_time();

      qda_lock.read_lock();
      FDS_VolumeQueue *que = queue_map[queue_id];
      
      que->enqueueIO(io);
      
      ioProcessForEnqueue(queue_id, io);  
      
      fds_uint32_t n_pios;
      n_pios = atomic_fetch_add(&(num_pending_ios), (unsigned int)1);
      FDS_PLOG(qda_log) << "Dispatcher: enqueueIO at queue - " << queue_id
			<<  " : # of pending ios = " << n_pios+1;
      assert(n_pios >= 0);

      qda_lock.read_unlock();
      return err;

    }

    void setSchedThreadPriority() {

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

    }

    virtual Error dispatchIOs() {

      Error err(ERR_OK);
       
      setSchedThreadPriority();

      while(1) {

        if (shuttingDown == true) {
          break;
        }

	parent_ctrlr->waitForWorkers();

	fds_uint32_t n_pios = 0;
	while (1) {
	  n_pios = atomic_load(&num_pending_ios);
          if (shuttingDown == true) {
            return err;
          } else if (n_pios > 0) {
	    break;
	  }
	  boost::this_thread::sleep(boost::posix_time::microseconds(100));
	}

	fds_uint32_t n_oios = 0;
	if (max_outstanding_ios > 0) {
	  while (1) {
	    n_oios = atomic_load(&num_outstanding_ios);
	    if (n_oios < max_outstanding_ios) {
	      break;
	    }
	    boost::this_thread::sleep(boost::posix_time::microseconds(100));
	  }
	}
	
	
	qda_lock.read_lock();

	fds_uint32_t queue_id = getNextQueueForDispatch();
	FDS_VolumeQueue *que = queue_map[queue_id];

	FDS_IOType *io = que->dequeueIO();
	assert(io != NULL);

	ioProcessForDispatch(queue_id, io);

	qda_lock.read_unlock();

	io->dispatch_time = boost::posix_time::microsec_clock::universal_time();

	n_pios = 0;
	n_pios = atomic_fetch_sub(&(num_pending_ios), (unsigned int)1);
	// assert(n_pios >= 1);

	n_oios = 0;
	n_oios = atomic_fetch_add(&(num_outstanding_ios), (unsigned int)1);
	FDS_PLOG(qda_log) << "Dispatcher: dispatchIO from queue " << queue_id
			<< " : # of outstanding ios = " << n_oios+1
			<< " : # of pending ios = " << n_pios-1;
	// assert(n_oios >= 0);

	parent_ctrlr->processIO(io);
	
      }

      return err;
    }

    void updateIoStats(FDS_IOType *io) {

	stats_mutex->lock();
	num_ios_completed ++;
	if (io->io_service_time > max_svc_time) {
		max_svc_time = io->io_service_time;
	}

	if ((io->io_service_time < min_svc_time) || (min_svc_time == 0)){
		min_svc_time = io->io_service_time;
	}

	total_svc_time += io->io_service_time;

	if (io->io_wait_time > max_wait_time) {
		max_wait_time = io->io_wait_time;
	}

	if ((io->io_wait_time < min_wait_time) || (min_wait_time == 0)){
		min_wait_time = io->io_wait_time;
	}

	total_wait_time += io->io_wait_time;

	if (num_ios_completed % 50 == 0) {
	  FDS_PLOG(qda_log) << "Dispatcher: IO svc time stats: (min-"
			    << min_svc_time << ", max-" << max_svc_time
			    << ", avg-" << (total_svc_time/50)
			    << "): IO wait time stats: (min-"
			    << min_wait_time << ", max-" << max_wait_time
			    << ", avg-" << (total_wait_time/50) << ")";
	  min_svc_time = max_svc_time = 0;
	  total_svc_time = 0;
	  min_wait_time = max_wait_time = 0;
	  total_wait_time = 0;

	}

	stats_mutex->unlock();

    }


    virtual Error markIODone(FDS_IOType *io) {
      Error err(ERR_OK);

      fds_uint32_t n_oios = 0;
      n_oios = atomic_fetch_sub(&(num_outstanding_ios), (unsigned int)1);

      io->io_done_time = boost::posix_time::microsec_clock::universal_time();

      boost::posix_time::time_duration wait_duration = io->dispatch_time - io->enqueue_time;
      boost::posix_time::time_duration service_duration = io->io_done_time - io->dispatch_time;

      io->io_wait_time = wait_duration.total_microseconds();
      io->io_service_time = service_duration.total_microseconds();

      FDS_PLOG(qda_log) << "Dispatcher: IO Request " << io->io_req_id 
			<< " for vol id " << io->io_vol_id
			<< " completed in " << io->io_service_time
			<< " usecs with a wait time of " << io->io_wait_time
			<< " usecs. # of outstanding ios = " << n_oios-1;

      updateIoStats(io);

      return err;
    }
    
  };
}

#endif
