#include <fds_types.h>
#include <fds_err.h>
#include <util/Log.h>
#include <concurrency/RwLock.h>
#include <unordered_map>
#include <mutex> // std::mutex, std::unique_lock
#include <condition_variable> 

#include "qos_ctrl.h"

#ifndef __FDS_FDS_QOS_H__
#define __FDS_FDS_QOS_H__

namespace fds {

  typedef std::unordered_map<fds_uint32_t,FDS_VolumeQueue *> queue_map_t;

  class FDS_QoSDispatcher {

  public:

    fds_log *qda_log;
    queue_map_t queue_map;
    FDS_QoSControl *parent_ctrlr;
    fds_rwlock qda_lock; // Protects queue_map (and any high level structures in derived class) 
                         // from events like volumes being inserted or removed during enqueue IO or dispatchIO.

    std::mutex ios_pending_mtx; // to protect the counter num_pending_ios
    std::condition_variable ios_pending_cv; // Condn variable to signal dispatcher when there is an IO available, from enqueueIO thread.
    fds_uint32_t num_pending_ios; // ios pending, aggregated across all queues, protected by ios_pending_mtx;

 
    virtual fds_uint32_t getNextQueueForDispatch() = 0;

    FDS_QoSDispatcher();
    ~FDS_QoSDispatcher();
 
    FDS_QoSDispatcher(FDS_QoSControl *ctrlr) {
      parent_ctrlr = ctrlr;
      qda_log = new fds_log("qda", "logs");
      num_pending_ios = 0;
    }

    Error registerQueueWithLockHeld(fds_uint32_t queue_id, FDS_VolumeQueue *queue, fds_uint64_t queue_rate, fds_uint32_t queue_priority) {

      Error err(ERR_OK);
      
      if (queue_map.count(queue_id) != 0) {	
	err = ERR_DUPLICATE;
	return err;
      }
      queue_map[queue_id] = queue;

      return err;
    }

    virtual Error registerQueue(fds_uint32_t queue_id, FDS_VolumeQueue *queue, fds_uint64_t queue_rate, fds_uint32_t queue_priority) {
      Error err(ERR_OK);
      qda_lock.write_lock();
      err = registerQueueWithLockHeld(queue_id, queue, queue_rate, queue_priority);
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

    }

    virtual void suspendQueue(fds_uint32_t queue_id) {

    }

    virtual void resumeQueue(fds_uint32_t queue_id) {

    } 

    virtual Error enqueueIO(fds_uint32_t queue_id, FDS_IOType *io) {

      Error err(ERR_OK);

      qda_lock.read_lock();
      FDS_VolumeQueue *que = queue_map[queue_id];
      
      que->enqueueIO(io);
      
      ioProcessForEnqueue(queue_id, io);  
      
      // do while loop is just to limit the scope of the unique_lock, limiting the time we hold ios_pending_mtx
      do {
	std::unique_lock<std::mutex> lck(ios_pending_mtx);
	num_pending_ios++;
	if (num_pending_ios == 1) {
	  // wakeup scheduler here.
	  ios_pending_cv.notify_all();
	}
      } while (0);
      qda_lock.read_unlock();
      return err;

    }

    virtual Error dispatchIOs() {

      Error err(ERR_OK);
      
      while(1) {

	parent_ctrlr->waitForWorkers();

	qda_lock.read_lock();

	do {
	  std::unique_lock<std::mutex> lck(ios_pending_mtx);
	  while (num_pending_ios == 0) {
	    ios_pending_cv.wait(lck);
	  }
	} while (0);

	fds_uint32_t queue_id = getNextQueueForDispatch();
	FDS_VolumeQueue *que = queue_map[queue_id];

	FDS_IOType *io = que->dequeueIO();
	assert(io != NULL);

	ioProcessForDispatch(queue_id, io);
	do {
	  std::unique_lock<std::mutex> lck(ios_pending_mtx);
	  num_pending_ios --;
	} while (0);
	qda_lock.read_unlock();

	parent_ctrlr->processIO(io);
	
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
