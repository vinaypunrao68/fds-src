#include "qos_ctrl.h"
#include "fds_qos.h"
#include "fds_err.h"
#include "StorHvisorCPP.h"

namespace fds {

FDS_QoSControl::FDS_QoSControl(uint32_t _max_threads, 
                               dispatchAlgoType algo, 
                               fds_log *log): qos_max_threads(_max_threads)  {
   threadPool = new fds_threadpool(qos_max_threads);
   dispatchAlgo = algo;
   switch(algo) { 
    case FDS_DISPATCH_HIER_TOKEN_BUCKET :
        //dispatcher = new htb_dispatcher();
        break;

    case FDS_DISPATCH_WFQ : 
        //dispatcher = new wfqDispatcher();
        break;

     default :
       break;
   }
   qos_log = log;
}


FDS_QoSControl::~FDS_QoSControl()  {
  //delete dispatcher;
  delete threadPool;
}


void FDS_QoSControl::runScheduler() { 
  
#if 0
  if (dispatcher) {
     dispatcher->dispatchIOs();
  }
#endif

}


Error   FDS_QoSControl::registerVolume(FDS_Volume &volume) {
Error err(ERR_OK);
   //dispatcher->registerQueue(volume.voldesc->volUUID, volume.volQueue);
   return err;
}

Error   FDS_QoSControl::deregisterVolume(FDS_Volume& Volume) {
Error err(ERR_OK);
    //dispatcher->deregisterQueue(volume.volUUID);
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
                FDS_PLOG(qos_log)  << " Invalid op ";
     }
    return err;
   };

}
