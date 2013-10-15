/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#include "qos_ctrl.h"
#include "fds_qos.h"
#include "fds_err.h"
#include "qos_htb.h"

namespace fds {

FDS_QoSControl::FDS_QoSControl(uint32_t _max_threads, 
                               dispatchAlgoType algo, 
                               fds_log *log) :
    qos_max_threads(_max_threads)  {
   threadPool = new fds_threadpool(qos_max_threads);
   dispatchAlgo = algo;
   qos_log = log;
   total_rate = 20000; //IOPS
   switch(algo) { 
    case FDS_DISPATCH_HIER_TOKEN_BUCKET:
        dispatcher = (FDS_QoSDispatcher *)new QoSHTBDispatcher(this, qos_log, total_rate);
        break;

    case FDS_DISPATCH_WFQ : 
        //dispatcher =(FDS_QoSDispatcher *) new QoSWFQDispatcher(this, total_rate);
        break;

     default :
       break;
   }
}


FDS_QoSControl::~FDS_QoSControl()  {
  //delete dispatcher;
  delete threadPool;
}

fds_uint32_t FDS_QoSControl::waitForWorkers() {

  return 10;
}


void FDS_QoSControl::runScheduler() { 
  
  if (dispatcher) {
    // threadPool->schedule(dispatcher->dispatchIOs, this);
  }
}


Error   FDS_QoSControl::registerVolume(fds_volid_t vol_uuid, FDS_VolumeQueue *volq) {
Error err(ERR_OK);
   err = dispatcher->registerQueue(vol_uuid, volq);
   return err;
}


Error   FDS_QoSControl::deregisterVolume(fds_volid_t vol_uuid) {
Error err(ERR_OK);
    err = dispatcher->deregisterQueue(vol_uuid);
   return err;
}

Error FDS_QoSControl::processIO(FDS_IOType *io) { 
    
  Error err(ERR_OK);
  if ( io->io_module == FDS_IOType::STOR_HV_IO) { 
    if(io->io_type == FDS_IO_READ) {
      //threadPool->schedule(StorHvisorProcIoRd, io);
    } else if(io->io_type == FDS_IO_WRITE) {
      //threadPool->schedule(StorHvisorProcIoWr, io);
    } else
      FDS_PLOG(qos_log)  << "Invalid op";
  }
  return err;
};

Error FDS_QoSControl::enqueueIO(fds_volid_t volUUID, FDS_IOType *io) {
  Error err(ERR_OK);

  FDS_PLOG(qos_log)  << "Enqueuing IO";
  err = dispatcher->enqueueIO(volUUID, io);
  return err;
}

}  // namespace fds
