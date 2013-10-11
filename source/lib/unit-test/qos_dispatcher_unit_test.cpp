#include <iostream>
#include <string>
#include <thread>
#include <atomic>
#include <util/Log.h>

#include "fds_err.h"
#include "fds_types.h"
#include "fds_qos.h"
#include "QoSWFQDispatcher.h"

#define NUM_VOLUMES 100
#define TOTAL_RATE 10000
#define NUM_TESTIOS_PER_VOL 1000

using namespace std;
using namespace fds;

std::atomic<unsigned int> next_io_req_id;
std::atomic<unsigned int> num_ios_dispatched;
fds_log *ut_log;

void start_queue_ios(fds_uint32_t queue_id, fds_uint64_t queue_rate, int queue_priority, FDS_QoSControl *qctrl) {

  FDS_Volume fds_vol;
  cout << "Starting thread for volume " << queue_id << endl; 

  fds_vol.voldesc = new VolumeDesc(std::string("FdsVol") + std::to_string(queue_id), queue_id);
  fds_vol.voldesc->iops_min = queue_rate;
  fds_vol.voldesc->iops_max = TOTAL_RATE;
  fds_vol.voldesc->relativePrio = queue_priority;

  cout << "Registering volume " << queue_id << endl;
  FDS_VolumeQueue *volQ = new FDS_VolumeQueue(256, fds_vol.voldesc->iops_max, fds_vol.voldesc->iops_min, fds_vol.voldesc->relativePrio);

  qctrl->registerVolume(queue_id, volQ );

  cout << "Volume " << queue_id << " Registered"  << endl;

  for (int i = 0; i < NUM_TESTIOS_PER_VOL; i++) {
    usleep(500000/queue_rate); // twice the rate per sec
    FDS_IOType *io = new FDS_IOType;
    io->io_vol_id = queue_id;
    io ->io_req_id = atomic_fetch_add(&next_io_req_id, (unsigned int)1);
    FDS_PLOG(ut_log)  << "Enqueueing IO " << io->io_vol_id << ":" << io->io_req_id;
    qctrl->enqueueIO(queue_id, io);
  }

}  

void start_scheduler(FDS_QoSControl *qctrl) {
  cout << "Starting scheduler thread" << endl;
  qctrl->runScheduler();
}

int main(int argc, char *argv[]) {

  ut_log = new fds_log("qosd_unit_test", "logs");

  cout << "Created logger" << endl;

  FDS_QoSControl *qctrl = new FDS_QoSControl();
  qctrl->setQosDispatcher(FDS_QoSControl::FDS_DISPATCH_WFQ, NULL);
  std::thread sch_thr(start_scheduler, qctrl);

  cout << "Started scheduler thread" << endl;

  next_io_req_id = ATOMIC_VAR_INIT(0);
  num_ios_dispatched = ATOMIC_VAR_INIT(0);

  std::vector<std::thread *> vol_threads;

  for (int i = 0; i < NUM_VOLUMES; i++) {
    std::thread *next_thread = new std::thread(start_queue_ios, i+1, (fds_uint64_t)30+i, i%10, qctrl);
    cout << "Started volume thread for " <<  i+1 << endl;
    vol_threads.push_back(next_thread);
  }

  for (int i = 0; i < NUM_VOLUMES; i++) {
    vol_threads[i]->join();
  }
  // Check here if we have processed all IOs before exiting.
  fds_uint32_t n_ios_enqueued, n_ios_dispatched;
    
  do {
    usleep(1000);
    n_ios_enqueued = atomic_load(&next_io_req_id);
    n_ios_dispatched = atomic_load(&num_ios_dispatched);
  } while (n_ios_enqueued != n_ios_dispatched);

  exit(0);

}

namespace fds {
  
#if 0 

  FDS_VolumeQueue::FDS_VolumeQueue() {
    volQueue = new boost::lockfree::queue<FDS_IOType *> (512);
  }

  FDS_VolumeQueue::~FDS_VolumeQueue() {

  }

  void  FDS_VolumeQueue::quiesceIos() {
    // Quiesce queued IOs on this queue & block any new IOs
  }

  void  FDS_VolumeQueue::suspendIO() {

  }
  void  FDS_VolumeQueue::resumeIO() {

  } 
  void  FDS_VolumeQueue::enqueueIO(FDS_IOType *io) {
    volQueue->push(io);
  }

  FDS_IOType *FDS_VolumeQueue::dequeueIO() {
    FDS_IOType *io;
    volQueue->pop(io);
    return (io);
  }

#endif

  FDS_QoSControl::FDS_QoSControl() {
    dispatcher = NULL;
  }
  FDS_QoSControl::~FDS_QoSControl() {

  }

  Error FDS_QoSControl::registerVolume(fds_uint64_t voluuid, FDS_VolumeQueue *volQ ) {
    Error err(ERR_OK);
    dispatcher->registerQueue(voluuid, volQ);
    volQ->activate();
    return err;
  }

  Error FDS_QoSControl::deregisterVolume(fds_uint64_t voluuid) {
    Error err(ERR_OK);
    dispatcher->deregisterQueue(voluuid);
    return err;
  }
   
  void   FDS_QoSControl::setQosDispatcher(dispatchAlgoType algo_type, FDS_QoSDispatcher *qosDispatcher) {
    dispatchAlgo = algo_type;
    if (qosDispatcher) {
      dispatcher = qosDispatcher;
    } else {
      switch (algo_type) {
      case FDS_QoSControl::FDS_DISPATCH_WFQ:
	dispatcher = new QoSWFQDispatcher(this, TOTAL_RATE, ut_log);
	break;
      default:
	dispatcher = NULL;
      }
    }
  }

  void  FDS_QoSControl::runScheduler() {
    dispatcher->dispatchIOs();
  }

  Error FDS_QoSControl::processIO(FDS_IOType* io) {
    Error err(ERR_OK);
    fds_uint32_t n_ios_dispatched;
    n_ios_dispatched = atomic_fetch_add(&num_ios_dispatched, (unsigned int)1);
    FDS_PLOG(ut_log) << "Dispatching IO " << io->io_vol_id << ":" << io->io_req_id << ". " << n_ios_dispatched+1 << " ios dispatched so far.";
    return err;
  }

  fds_uint32_t FDS_QoSControl::waitForWorkers() {
    usleep(1000000/TOTAL_RATE);
    return (0);
  }


  Error FDS_QoSControl::enqueueIO(fds_volid_t volUUID, FDS_IOType *io) {
    Error err(ERR_OK);
    dispatcher->enqueueIO(volUUID, io);
    return err;
  }

}


