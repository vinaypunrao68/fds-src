#include <fds_types.h>
#include <fds_err.h>
#include <util/Log.h>
#include <concurrency/Mutex.h>

namespace fds {

  typedef std::unordered_map<fds_uint32_t,fds_queue_t> queue_map_t;

  class QoSDispatchAlgorithm {

    fds_log *qda_log;
    queue_map_t queue_map;
    QosCtrl *parent_ctrlr;
    fds_mutex_t *qda_mutex;
    fds_uint32_t num_pending_ios;

  private:
    virtual fds_io_t *getNextQueueForDispatch() = 0;
    virtual void ioProcessForEnqueue(fds_uint32_t queue_id, fds_io_t *io)
    {
      // preprocess before enqueuing in the input queue
      FDS_PLOG(qda_log) << "Request " << io->io_req_id << " being enqueued at queue " << queue_id; 
    }
    virtual void ioProcessForDispatch(fds_uint32_t queue_id, fds_io_t *io)
    {
      // do necessary processing before dispatching the io
      FDS_PLOG(qda_log) << "Dispatching " << io->io_req_id << " from queue " << queue_id; 
    }
    wakeUpDispatcher() {
      // Signal dispatcher thread and unblock him
    }
    waitForIo() {
      // Sleep until woken up, using a condition variable, releasing qda_mutex atomically while going to sleep.
    }

  public:
    QoSDispatchAlgorithm(QoSCtrl *ctrlr) {
      parent_ctrlr = ctrlr;
      qda_log = new fds_log("qda", "logs");
      qda_mutex = new fds_mutex("QoSDispatchAlgorithm");
      num_pending_ios = 0;
    }

    virtual Error registerQueue(fds_uint32_t queue_id, fds_queue_t *queue, fds_uint32_t queue_rate, fds_uint32_t queue_priority) {
      qda_mutex->lock();
      if (queue_map.count(queue_id) != 0) {
	qda_mutex->unlock();
	return ERR_DUPLICATE;
      }
      queue_map[queue_id] = queue;
      qda_mutex->unlock();
      return ERR_OK;
    }

    virtual Error deregisterQueue(fds_uint32_t queue_id) {
      qda_mutex->lock();
      if  (queue_map.count(queue_id) == 0) {
	qda_mutex->unlock();
	return ERR_INVALID_ARG;
      }
      fds_queue_t *que = queue_map[queue_id];
      if (!que->empty) {
	qda_mutex->unlock();
	return ERR_INVALID_ARG; // Should be really ERR_BUSY
      }
      queue_map.erase(queue_id);
      qda_mutex->unlock();
      return ERR_OK;
    }

    virtual Error submitIo(fds_uint32_t queue_id, fds_io_t *io) {

      qda_mutex->lock();
      fds_queue_t que = queue_map[queue_id];
      qda_mutex->unlock();

      err = que->enqueue(io);

      qda_mutex->lock();
      ioProcessForEnqueue(queue_id, io);  
      
      num_pending_ios++;
      if (num_pending_ios == 1) {
	// wakeup scheduler here.
	wakeUpDispatcher();
      }
      qda_mutex->unlock();
      return ERR_OK;

    }

    virtual Error dispatchIos() {
      
      while(1) {

	parent_ctrlr->waitForWorkers();
	qda_mutex->lock();
	if (num_pending_ios == 0) {
	  waitForIo();
	} 
	fds_io_t queue_id = getNextQueueForDispatch();
	fds_queue_t que = queue_map[queue_id];
	io = que->deque();
	assert(io != NULL);
	ioProcessForDispatch(queue_id, io);
	num_pending_ios --;
	qda_mutex->unlock();
	parent_ctrlr->processIO(io);
	
      }
      
    }
    
  }
