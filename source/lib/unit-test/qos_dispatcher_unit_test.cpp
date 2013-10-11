#include <iostream>
#include <string>
#include <thread>
#include <atomic>
#include <random>
#include <util/Log.h>

#include "fds_err.h"
#include "fds_types.h"
#include "fds_qos.h"
#include "test_stat.h"
#include "QoSWFQDispatcher.h"

#define NUM_VOLUMES 100
#define TOTAL_RATE 10000
#define NUM_TESTIOS_PER_VOL 1000

using namespace std;
using namespace fds;

std::atomic<unsigned int> next_io_req_id;
std::atomic<unsigned int> num_ios_dispatched;
fds_log *ut_log;
StatIOPS *iops_stats;


class VolQueueDesc {

public:
  fds_uint32_t queue_id;
  fds_uint64_t iops_min;
  fds_uint64_t iops_max;
  int          queue_priority;

  fds_uint64_t in_rate;
  fds_uint32_t max_burst_size;
  
  VolQueueDesc(fds_uint32_t q_id, fds_uint64_t min_iops, fds_uint64_t max_iops,
	       int priority, fds_uint64_t ingress_rate, fds_uint32_t max_burst):
    queue_id(q_id), iops_min(min_iops), iops_max(max_iops),
    queue_priority(priority), in_rate(ingress_rate), max_burst_size(max_burst)
  {
    
  }

};

void start_queue_ios(VolQueueDesc *volQDesc, FDS_QoSControl *qctrl) {

  FDS_Volume fds_vol;

  fds_uint32_t avg_inter_arr_time = (fds_uint32_t)1000000/volQDesc->in_rate; // in usecs

  std::default_random_engine generator;
  std::poisson_distribution<unsigned int> int_arr_distribution(avg_inter_arr_time);

  cout << "Starting thread for volume " << volQDesc->queue_id << endl; 

  fds_vol.voldesc = new VolumeDesc(std::string("FdsVol") + std::to_string(volQDesc->queue_id), volQDesc->queue_id);
  fds_vol.voldesc->iops_min = volQDesc->iops_min;
  fds_vol.voldesc->iops_max = volQDesc->iops_max;
  fds_vol.voldesc->relativePrio = volQDesc->queue_priority;

  cout << "Registering volume " << volQDesc->queue_id << endl;
  FDS_VolumeQueue *volQ = new FDS_VolumeQueue(256, fds_vol.voldesc->iops_max, fds_vol.voldesc->iops_min, fds_vol.voldesc->relativePrio);

  qctrl->registerVolume(volQDesc->queue_id, volQ );

  cout << "Volume " << volQDesc->queue_id << " Registered"  << endl;

  for (int i = 0; i < NUM_TESTIOS_PER_VOL; i++) {
    unsigned int inter_arrival_time = int_arr_distribution(generator);
    usleep(inter_arrival_time); 
    FDS_IOType *io = new FDS_IOType;
    io->io_vol_id = volQDesc->queue_id;
    io ->io_req_id = atomic_fetch_add(&next_io_req_id, (unsigned int)1);
    FDS_PLOG(ut_log)  << "Enqueueing IO " << io->io_vol_id << ":" << io->io_req_id;
    qctrl->enqueueIO(volQDesc->queue_id, io);
  }

}  

void start_scheduler(FDS_QoSControl *qctrl) {
  cout << "Starting scheduler thread" << endl;
  qctrl->runScheduler();
}

int main(int argc, char *argv[]) {

  ut_log = new fds_log("qosd_unit_test", "logs");
  cout << "Created logger" << endl;

  iops_stats = NULL;

  FDS_QoSControl *qctrl = new FDS_QoSControl();
  qctrl->setQosDispatcher(FDS_QoSControl::FDS_DISPATCH_WFQ, NULL);
  std::thread sch_thr(start_scheduler, qctrl);

  cout << "Started scheduler thread" << endl;

  next_io_req_id = ATOMIC_VAR_INIT(0);
  num_ios_dispatched = ATOMIC_VAR_INIT(0);

  std::vector<std::thread *> vol_threads;
  std::vector<fds_uint32_t> qids; /* array of queue ids (for perf stats) */

  for (int i = 0; i < NUM_VOLUMES; i++) {
    VolQueueDesc *volQDesc = new VolQueueDesc(i+1, (fds_uint64_t)30+i, TOTAL_RATE, i%10, (fds_uint64_t)(30+i)*2, 50 - 5 * (10-i%10)); 
    std::thread *next_thread = new std::thread(start_queue_ios,volQDesc, qctrl);
    cout << "Started volume thread for " <<  i+1 << endl;
    vol_threads.push_back(next_thread);
    qids.push_back(i+1);
  }

  iops_stats = new StatIOPS("qosd_unit_test", qids);

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


  delete iops_stats;
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

    boost::posix_time::ptime now = boost::posix_time::microsec_clock::universal_time();
    if (iops_stats) iops_stats->handleIOCompletion(io->io_vol_id, now);
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


