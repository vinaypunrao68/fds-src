#ifndef __FDS_QOS_CTRL_H__
#define __FDS_QOS_CTRL_H__
#include <Ice/Ice.h>
#include <IceUtil/IceUtil.h>
#include <boost/thread/thread.hpp>
#include <boost/lockfree/queue.hpp>
#include <iostream>
#include <boost/atomic.hpp>
#include <concurrency/ThreadPool.h>
#include "fds_err.h"
#include <fds_types.h>
#include <fds_err.h>
#include <fdsp/FDSP.h>

namespace fds { 

class FDS_IOType {
public:
 FDS_IOType() { };
~FDS_IOType() { };

 typedef enum {
  STOR_HV_IO,
  STOR_MGR_IO,
  DATA_MGR_IO
 } ioModule;

 ioModule io_module; // IO belongs to which module for Qos proc 
 FDSP_MsgHdrTypePtr msgHdr;
 FDSP_PutObjTypePtr putObj;
 FDSP_GetObjTypePtr getObj;

 fbd_request_t *fbd_req;
};


typedef enum {
FDS_VOL_Q_INVALID,
FDS_VOL_Q_SUSPENDED,
FDS_VOL_Q_QUIESCING,
FDS_VOL_Q_ACTIVE
} VolumeQState;

/* **********************************************
 *  FDS_VolumeQueue: VolumeQueue
 *
 **********************************************************/
class FDS_VolumeQueue {
public:
 FDS_VolumeQueue();
~FDS_VolumeQueue();
 boost::lockfree::queue<fbd_request_t*>  *volQueue;
 VolumeQState volumeState;

 void  quiesceIos(); // Quiesce queued IOs on this queue & block any new IOs
 void   suspendIO();
 void   resumeIO(); 
 void   enqueueIO();
 void   dequeueIO();
};

/* **********************************************
 *  FDS_QosDispatcher: Pluggable Dispatcher with dispatchIOs - main routine
 *
 **********************************************************/
class FDS_QosDispatcher {
public :
 FDS_QosDispatcher();
~FDS_QosDispatcher();
 void  dispatchIOs(); // Run in a dispatch thread and runs forever.

 
 void  quiesceIos(FDS_VolumeQueue* ); // Quiesce queued IOs on this queue & block any new IOs

 void   registerVolumeQueue(FDS_VolumeQueue *queue);
 void   deregisterVolumeQueue(FDS_VolumeQueue *queue);

 void   suspendVolumeQueue(FDS_VolumeQueue *queue);
 void   resumeVolumeQueue(FDS_VolumeQueue *queue); 

 void   enqueueIO(FDS_VolumeQueue *queue);
};


/* **********************************************
 *  FDS_QosControl: Qos Control Class with a shared threadpool
 *
 **********************************************************/
class FDS_QosControl { 
   public :

   typedef enum {
     FDS_DISPATCH_TOKEN_BUCKET,
     FDS_DISPATCH_HIER_TOKEN_BUCKET,
     FDS_DISPATCH_WFQ,
     FDS_DISPATCH_DEADLINE,
     FDS_DISPATCH_ROUND_ROBIN
   } dispatchAlgoType;

   dispatchAlgoType dispatchAlgo;
   FDS_QosDispatcher* dispatcher; // Dispatcher Class 
   fds_threadpool *threadPool; // This is the global threadpool
   
   
   FDS_QosControl();
   ~FDS_QosControl();
   FDS_QosControl(FDS_QosDispatcher *dispatcher);
   
   Error RegisterVolume(FDS_Volume &volume);
   Error deregisterVolume(FDS_Volume& Volume);
   
   void   setQosDispatcher(dispatchAlgoType algo_type, FDS_QosDispatcher *qosDispatcher);
   void   runScheduler(); // Calls in the QosDispatcher's main dispatch routine
   Error   processIO(FDS_IOType& io); // Schedule an IO on a thread from thrd pool
   int     waitForWorkers(); // Blocks until there is a threshold num of workers in threadpool
   Error enqueueIO();
};


}
#endif
