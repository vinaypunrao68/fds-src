#ifndef __FDS_QOS_CTRL_H__
#define __FDS_QOS_CTRL_H__
#include <Ice/Ice.h>
#include <IceUtil/IceUtil.h>
#include <boost/thread/thread.hpp>
#include <boost/lockfree/queue.hpp>
#include <iostream>
#include <boost/atomic.hpp>
#include <util/Log.h>
#include <concurrency/ThreadPool.h>
#include "fds_err.h"
#include <fdsp/FDSP.h>
#include <fds_types.h>
#include <fds_err.h>
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
   fds_threadpool *threadPool; // This is the global threadpool
   fds_uint64_t   total_rate;
   

   fds_log *qos_log;
   
   FDS_QoSControl();
   ~FDS_QoSControl();

   FDS_QoSControl(uint32_t _max_thrds, dispatchAlgoType algo, fds_log *log);
   
   Error   registerVolume(fds_uint64_t voluuid, FDS_VolumeQueue *q);
   Error   deregisterVolume(fds_uint64_t voluuid);
   
   void    setQosDispatcher(dispatchAlgoType algo_type, FDS_QoSDispatcher *qosDispatcher);
   void    runScheduler(); // Calls in the QosDispatcher's main dispatch routine
   Error   processIO(FDS_IOType* io); // Schedule an IO on a thread from thrd pool
   fds_uint32_t     waitForWorkers(); // Blocks until there is a threshold num of workers in threadpool
   Error   enqueueIO(fds_volid_t volUUID, FDS_IOType *io);
};

}
#endif
