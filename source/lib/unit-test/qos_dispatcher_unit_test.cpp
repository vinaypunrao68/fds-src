#include <iostream>
#include <string>
#include <thread>
#include <atomic>
#include <random>
#include <util/Log.h>

#include <pthread.h>
#include <sched.h>

#include "fds_err.h"
#include "fds_types.h"
#include "fds_qos.h"
#include "test_stat.h"
#include "QoSWFQDispatcher.h"

unsigned long num_volumes = 10;
unsigned long tot_rate = 1000;
unsigned long time_to_run = 50;

#define NUM_VOLUMES num_volumes
#define TOTAL_RATE tot_rate
#define IOPS_TO_ALLOCATE ((unsigned int) 3*TOTAL_RATE/4)
#define AVG_IOPS_TO_ALLOCATE_PER_VOLUME ((unsigned int) IOPS_TO_ALLOCATE/NUM_VOLUMES)
#define VOL_IOPS(i) ((unsigned int) AVG_IOPS_TO_ALLOCATE_PER_VOLUME + (i - 4.5) * TOTAL_RATE/100)

using namespace std;
using namespace fds;

std::atomic<unsigned int> next_io_req_id;
std::atomic<unsigned int> num_ios_dispatched;
fds_log *ut_log;
StatIOPS *iops_stats;

// boost::posix_time::ptime last_work_time;

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

  std::string ToString() {
    char tmp_string[128];
    sprintf(tmp_string, "(%d : (%u, %u, %d, %u, %u))", 
	    (int)queue_id, (unsigned int)iops_min, (unsigned int)iops_max, (int)queue_priority, (unsigned int)in_rate, (unsigned int)max_burst_size); 
    return std::string(tmp_string);
  }

};

void create_volume(VolQueueDesc *volQDesc, FDS_QoSControl *qctrl) {

  FDS_Volume fds_vol;

  fds_vol.voldesc = new VolumeDesc(std::string("FdsVol") + std::to_string(volQDesc->queue_id), volQDesc->queue_id);
  fds_vol.voldesc->iops_min = volQDesc->iops_min;
  fds_vol.voldesc->iops_max = volQDesc->iops_max;
  fds_vol.voldesc->relativePrio = volQDesc->queue_priority;

  cout << "Registering volume " << volQDesc->queue_id << endl;
  FDS_VolumeQueue *volQ = new FDS_VolumeQueue(1024, fds_vol.voldesc->iops_max, fds_vol.voldesc->iops_min, fds_vol.voldesc->relativePrio);

  qctrl->registerVolume(volQDesc->queue_id, volQ );

  FDS_PLOG(ut_log) << "Volume " << volQDesc->queue_id << " Registered"  << endl;

}

void start_queue_ios(VolQueueDesc *volQDesc, FDS_QoSControl *qctrl) {

  fds_uint32_t n_ios_issues = 0;

  fds_uint32_t avg_burst_size = (1 + volQDesc->max_burst_size)/2;
  //fds_uint32_t avg_burst_size = 1;
  fds_uint32_t avg_inter_arr_time = (fds_uint32_t)1000000 * avg_burst_size/volQDesc->in_rate; // in usecs

  boost::posix_time::ptime start_time = boost::posix_time::microsec_clock::universal_time();
  boost::posix_time::ptime last_io_time  = start_time; 

  std::default_random_engine rgen1(2*volQDesc->queue_id);
  std::default_random_engine rgen2(2*volQDesc->queue_id+1);
  std::poisson_distribution<unsigned int> int_arr_distribution(avg_inter_arr_time);
  std::uniform_int_distribution<unsigned int> burst_size_distribution(1, volQDesc->max_burst_size);

  FDS_PLOG(ut_log) << "Starting thread for volume " << volQDesc->ToString() << endl; 

  while (1) {
    unsigned int inter_arrival_time = int_arr_distribution(rgen1);
    //unsigned int inter_arrival_time = avg_inter_arr_time;
    boost::posix_time::ptime now = boost::posix_time::microsec_clock::universal_time();
    boost::posix_time::time_duration time_since_start = now - start_time;
    if (time_since_start > boost::posix_time::seconds(time_to_run)) {
      break;
    }
    boost::posix_time::time_duration delta = now - last_io_time;
    last_io_time = now;
    if (delta < boost::posix_time::microseconds(inter_arrival_time)) {
      boost::this_thread::sleep(boost::posix_time::microseconds(inter_arrival_time) - delta);
    }
    
    unsigned int burst_size = 1;
    burst_size = burst_size_distribution(rgen2);
    if (burst_size > volQDesc->max_burst_size) {
      burst_size = 1 + burst_size % volQDesc->max_burst_size;
    }
    FDS_PLOG(ut_log)  << "After " << inter_arrival_time << " usecs, bursting " << burst_size << " IOs for volume " << volQDesc->queue_id;
    for (int n_ios = 0; n_ios < burst_size; n_ios++) {
      FDS_IOType *io = new FDS_IOType;
      io->io_vol_id = volQDesc->queue_id;
      io ->io_req_id = atomic_fetch_add(&next_io_req_id, (unsigned int)1);
      FDS_PLOG(ut_log)  << "Enqueueing IO " << io->io_vol_id << ":" << io->io_req_id;
      qctrl->enqueueIO(volQDesc->queue_id, io);
    }
  }

}  


void start_scheduler(FDS_QoSControl *qctrl) {
  cout << "Starting scheduler thread" << endl;

  int ret;
  pthread_t this_thread = pthread_self();
  struct sched_param params;

  params.sched_priority = sched_get_priority_max(SCHED_FIFO);
  std::cout << "Trying to set schedule thread realtime prio = " << params.sched_priority << std::endl;
 
  // Attempt to set thread real-time priority to the SCHED_FIFO policy
  ret = pthread_setschedparam(this_thread, SCHED_FIFO, &params);
  if (ret != 0) {
    // Print the error
    std::cout << "Unsuccessful in setting scheduler thread realtime prio" << std::endl;
  }

  // Now verify the change in thread priority
  int policy = 0;
  ret = pthread_getschedparam(this_thread, &policy, &params);
  if (ret != 0) {
    std::cout << "Couldn't retrieve real-time scheduling parameters for scheduler thread" << std::endl;
    return;
  }
 
  // Check the correct policy was applied
  if(policy != SCHED_FIFO) {
    std::cout << "Scheduling is NOT SCHED_FIFO!" << std::endl;
  } else {
    std::cout << "Scheudler thread SCHED_FIFO OK" << std::endl;
  }
 
  // Print thread scheduling priority
  std::cout << "Scheduler thread priority is " << params.sched_priority << std::endl;

  qctrl->runScheduler();
}

int main(int argc, char *argv[]) {

  ut_log = new fds_log("qosd_unit_test", "logs");
  FDS_PLOG(ut_log) << "Created logger" << endl;
  num_volumes = 10;
  tot_rate = 1000;
  time_to_run = 50;

  for (int i = 1; i < argc; i++) {
    if (strncmp(argv[i], "--num_vols=",11) == 0) {
      num_volumes = strtoul(argv[i] + 11, NULL, 0);
    } else  if (strncmp(argv[i], "--total_rate=",13) == 0) {
      tot_rate = strtoul(argv[i] + 13, NULL, 0);
    } else if (strncmp(argv[i], "--test_time=", 12) == 0) {
      time_to_run = strtoul(argv[i] + 12, NULL, 0);
    }
  }

  iops_stats = NULL;

  FDS_QoSControl *qctrl = new FDS_QoSControl();
  qctrl->setQosDispatcher(FDS_QoSControl::FDS_DISPATCH_WFQ, NULL);

  std::thread sch_thr(start_scheduler, qctrl);
  FDS_PLOG(ut_log) << "Started scheduler thread" << endl;

  next_io_req_id = ATOMIC_VAR_INIT(0);
  num_ios_dispatched = ATOMIC_VAR_INIT(0);

  std::vector<VolQueueDesc *> volQDescs;
  std::vector<std::thread *> vol_threads;
  std::vector<fds_uint32_t> qids; /* array of queue ids (for perf stats) */

  for (int i = 0; i < NUM_VOLUMES; i++) {
    VolQueueDesc *volQDesc = new VolQueueDesc(i+1, // queue_id
					      (fds_uint64_t)VOL_IOPS(i), //min_iops
					      TOTAL_RATE, //max_iops
					      i%10, //priority
					      (fds_uint64_t)VOL_IOPS(i)*1.5, //ingress_rate
					      5 + 5 * (i%10)); // max_burst_size
    create_volume(volQDesc, qctrl);
    volQDescs.push_back(volQDesc);
    qids.push_back(i+1);
  }

  FDS_PLOG(ut_log) << "All volumes created" << endl;

  iops_stats = new StatIOPS("qosd_unit_test", qids);

  for (int i = 0; i < NUM_VOLUMES; i++) {
    
    std::thread *next_thread = new std::thread(start_queue_ios,volQDescs[i], qctrl);
    FDS_PLOG(ut_log) << "Started volume thread for " <<  i+1 << endl;
    vol_threads.push_back(next_thread);
  }

  FDS_PLOG(ut_log) << "All threads created" ;

  for (int i = 0; i < NUM_VOLUMES; i++) {
    vol_threads[i]->join();
  }

  fds_uint32_t n_ios_enqueued, n_ios_dispatched;

  n_ios_enqueued = atomic_load(&next_io_req_id);

  FDS_PLOG(ut_log) << "All volumes done submitting IOs. " << n_ios_enqueued << " IOs enqueued in total";

#if 0
  // Check here if we have processed all IOs before exiting.
    
  do {
    usleep(1000);
    n_ios_dispatched = atomic_load(&num_ios_dispatched);
    FDS_PLOG(ut_log) << n_ios_dispatched << " IOs done processing.";
  } while (n_ios_enqueued != n_ios_dispatched);

#endif
  StatIOPS *tmp_iops_stats = iops_stats;
  iops_stats = NULL;
  delete tmp_iops_stats;

  exit(0);

}

namespace fds {

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
    // boost::posix_time::ptime last_work_time = boost::posix_time::microsec_clock::universal_time();
    dispatcher->dispatchIOs();
  }

  Error FDS_QoSControl::processIO(FDS_IOType* io) {

    static boost::posix_time::time_duration deficit = boost::posix_time::microseconds(0);
    static int first_io = 1;
    static boost::posix_time::ptime last_work_time;
    static long inter_svc_time = 1000000/TOTAL_RATE;

    Error err(ERR_OK);
    fds_uint32_t n_ios_dispatched;
    n_ios_dispatched = atomic_fetch_add(&num_ios_dispatched, (unsigned int)1);
    FDS_PLOG(ut_log) << "Dispatching IO " << io->io_vol_id << ":" << io->io_req_id << ". " << n_ios_dispatched+1 << " ios dispatched so far.";

    boost::posix_time::ptime enter_time = boost::posix_time::microsec_clock::universal_time();
    if (first_io) {
      last_work_time = enter_time;
      first_io = 0;
    }

    if (iops_stats) iops_stats->handleIOCompletion(io->io_vol_id, enter_time);
 
    boost::posix_time::time_duration delta = enter_time - last_work_time;
    boost::posix_time::time_duration time_to_sleep = boost::posix_time::microseconds(inter_svc_time) - delta - deficit;
    if (time_to_sleep > boost::posix_time::microseconds(0)) {
      boost::this_thread::sleep(time_to_sleep);
      boost::posix_time::ptime exit_time = boost::posix_time::microsec_clock::universal_time();
      boost::posix_time::time_duration time_slept = exit_time - enter_time;
      deficit = time_slept - time_to_sleep; 
    } else {
      if (delta < boost::posix_time::microseconds(inter_svc_time)) {
	deficit = boost::posix_time::microseconds(0) - time_to_sleep;
      } else {
	deficit = boost::posix_time::microseconds(0);
      }
    }
    last_work_time = enter_time;
    
    return err;
  }

  fds_uint32_t FDS_QoSControl::waitForWorkers() {
#if 0     
    fds_uint32_t inter_svc_time = 1000000/TOTAL_RATE;
    static boost::posix_time::time_duration deficit = 0;

    boost::posix_time::ptime enter_time = boost::posix_time::microsec_clock::universal_time();
    boost::posix_time::time_duration delta = enter_time - last_work_time;
    boost::posix_time::time_duration time_to_sleep = inter_svc_time-delta;

    if (delta < boost::posix_time::microseconds(inter_svc_time) - deficit) {
      boost::this_thread::sleep(time_to_sleep);
      exit_time = boost::posix_time::microsec_clock::universal_time();
      boot::posix_time::time_slept = exit_time - enter_time;
      deficit = time_slept - time_to_sleep; // next time, we can sleep deficit usecs less because we overslept this time by that much.
    } else {
      deficit = (delta - boost::posix_time::microseconds(inter_svc_time)) + deficit;
    }

    while (delta < boost::posix_time::microseconds(inter_svc_time - 10)) { 
      boost::this_thread::sleep(boost::posix_time::microseconds(inter_svc_time) - delta);
      now = boost::posix_time::microsec_clock::universal_time();
      delta = now - last_work_time;
    }

    last_work_time = now;
#endif
    return (0);

  }


  Error FDS_QoSControl::enqueueIO(fds_volid_t volUUID, FDS_IOType *io) {
    Error err(ERR_OK);
    dispatcher->enqueueIO(volUUID, io);
    return err;
  }

}


