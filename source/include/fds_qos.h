#include <fds_types.h>
#include <fds_err.h>
#include <util/Log.h>
#include <concurrency/Mutex.h>
#include <unordered_map>

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
    fds_mutex *qda_mutex;
    fds_uint32_t num_pending_ios;

 
    virtual fds_uint32_t getNextQueueForDispatch() = 0;

    void wakeUpDispatcher() {
      // Signal dispatcher thread and unblock him
    }
    void waitForIo() {
      // Sleep until woken up, using a condition variable, releasing qda_mutex atomically while going to sleep.
    }

  
    FDS_QoSDispatcher();
    ~FDS_QoSDispatcher();
 
    FDS_QoSDispatcher(FDS_QoSControl *ctrlr) {
      parent_ctrlr = ctrlr;
      qda_log = new fds_log("qda", "logs");
      qda_mutex = new fds_mutex("QoSDispatchAlgorithm");
      num_pending_ios = 0;
    }

    virtual Error registerQueue(fds_uint32_t queue_id, FDS_VolumeQueue *queue, fds_uint32_t queue_rate, fds_uint32_t queue_priority) {
      Error err(ERR_OK);
      qda_mutex->lock();
      if (queue_map.count(queue_id) != 0) {
	qda_mutex->unlock();
	err = ERR_DUPLICATE;
	return err;
      }
      queue_map[queue_id] = queue;
      qda_mutex->unlock();
      return err;
    }

    virtual Error deregisterQueue(fds_uint32_t queue_id) {
      Error err(ERR_OK);
      qda_mutex->lock();
      if  (queue_map.count(queue_id) == 0) {
	qda_mutex->unlock();
	err = ERR_INVALID_ARG;
	return err;
      }
      FDS_VolumeQueue *que = queue_map[queue_id];
      // Assert here that queue is empty, when one such API is available.
      queue_map.erase(queue_id);
      qda_mutex->unlock();
      return err;
    }

    virtual void ioProcessForEnqueue(fds_uint32_t queue_id, FDS_IOType *io)
    {
      // preprocess before enqueuing in the input queue
      FDS_PLOG(qda_log) << "Request " << io->io_req_id << " being enqueued at queue " << queue_id; 
    }
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

      qda_mutex->lock();
      FDS_VolumeQueue *que = queue_map[queue_id];
      qda_mutex->unlock();

      que->enqueueIO(io);
      
      qda_mutex->lock();
      ioProcessForEnqueue(queue_id, io);  
      
      num_pending_ios++;
      if (num_pending_ios == 1) {
	// wakeup scheduler here.
	wakeUpDispatcher();
      }
      qda_mutex->unlock();
      return err;

    }

    virtual Error dispatchIOs() {

      Error err(ERR_OK);
      
      while(1) {

	parent_ctrlr->waitForWorkers();

	qda_mutex->lock();
	if (num_pending_ios == 0) {
	  waitForIo();
	} 
	fds_uint32_t queue_id = getNextQueueForDispatch();
	FDS_VolumeQueue *que = queue_map[queue_id];
	qda_mutex->unlock();

	FDS_IOType *io = que->dequeueIO();
	assert(io != NULL);

	qda_mutex->lock();
	ioProcessForDispatch(queue_id, io);
	num_pending_ios --;
	qda_mutex->unlock();

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
