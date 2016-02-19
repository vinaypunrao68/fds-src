#ifndef __FDS_QOS_CTRL_H__
#define __FDS_QOS_CTRL_H__
#include <boost/thread/thread.hpp>
#include <boost/lockfree/queue.hpp>
#include <iostream>
#include <boost/atomic.hpp>
#include <util/Log.h>
#include <concurrency/ThreadPool.h>
#include "fds_error.h"
#include <fdsp/FDSP_types.h>
#include <fds_types.h>
#include <fds_error.h>
#include <fds_volume.h>

namespace fds { 

/* **********************************************
 *  FDS_QoSDispatcher: Pluggable Dispatcher with dispatchIOs - main routine
 *
 **********************************************************/
 class FDS_QoSDispatcher;

/* **********************************************
 *  FDS_QosControl: Qos Control Class with a shared threadpool
 *
 **********************************************************/
class FDS_QoSControl { 
   public :

   typedef enum {
     FDS_DISPATCH_TOKEN_BUCKET,
     FDS_DISPATCH_HIER_TOKEN_BUCKET,
     FDS_DISPATCH_WFQ,
     FDS_DISPATCH_DEADLINE,
     FDS_DISPATCH_ROUND_ROBIN
   } dispatchAlgoType;

   dispatchAlgoType dispatchAlgo;
   FDS_QoSDispatcher* dispatcher; // Dispatcher Class 

   fds_uint32_t  qos_max_threads; // Max number of threads in the pool
   std::unique_ptr<std::thread> dispatcherThread;
   fds_threadpool *threadPool; // This is the global threadpool
   fds_uint64_t   total_rate;
   

   fds_log *qos_log;
   
   FDS_QoSControl();
   ~FDS_QoSControl();
   void stop();

   FDS_QoSControl(fds_uint32_t _max_thrds, dispatchAlgoType algo, fds_log *log, const std::string& prefix);
   
   virtual FDS_VolumeQueue* getQueue(fds_volid_t queueId);
   Error   registerVolume(fds_volid_t voluuid, FDS_VolumeQueue *q);
   Error   deregisterVolume(fds_volid_t voluuid);
   Error modifyVolumeQosParams(fds_volid_t vol_uuid, 
			       fds_int64_t iops_assured,
			       fds_int64_t iops_throttle,
			       fds_uint32_t prio);
   
   void    setQosDispatcher(dispatchAlgoType algo_type, FDS_QoSDispatcher *qosDispatcher);
   void    runScheduler(); // Calls in the QosDispatcher's main dispatch routine
   virtual Error   processIO(FDS_IOType* io); // Schedule an IO on a thread from thrd pool
   virtual Error markIODone(FDS_IOType* io);
   fds_uint32_t     waitForWorkers(); // Blocks until there is a threshold num of workers in threadpool
   virtual Error   enqueueIO(fds_volid_t volUUID, FDS_IOType *io);
   void quieseceIOs(fds_volid_t volUUID);
   void quieseceIOs();
   void stopDequeue(fds_volid_t volUUID);
   void resumeIOs(fds_volid_t volUUID);

   virtual fds_uint32_t queueSize(fds_volid_t volId);
};

}
#endif
