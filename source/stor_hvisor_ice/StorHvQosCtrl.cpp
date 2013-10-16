#include "StorHvQosCtrl.h" 
#include "StorHvisorCPP.h"

namespace fds {

StorHvQosCtrl::StorHvQosCtrl(uint32_t max_thrds, dispatchAlgoType algo, fds_log *log) : FDS_QoSControl::FDS_QoSControl(max_thrds, algo, log) {
     if ( dispatchAlgo == FDS_QoSControl::FDS_DISPATCH_HIER_TOKEN_BUCKET) {
        htb_dispatcher = new QoSHTBDispatcher(this, qos_log, total_rate);
     }
}

StorHvQosCtrl::~StorHvQosCtrl() {
  delete htb_dispatcher;
}


Error StorHvQosCtrl::processIO(FDS_IOType *io) {
   Error err(ERR_OK);
    if ( io->io_module == FDS_IOType::STOR_HV_IO) {
        if(io->io_type == FDS_IO_READ) {
             threadPool->schedule(StorHvisorProcIoRd, io);
        } else if(io->io_type == FDS_IO_WRITE) {
             threadPool->schedule(StorHvisorProcIoWr, io);
        } else
                FDS_PLOG(qos_log)  << " Invalid op ";
    }
    else {
      FDS_PLOG(qos_log) << "StorHvQosCtrl: unexpected FDS_IOType: " << io->io_module
			<< "; expecting STOR_HV_IO";
      err = ERR_MAX;    
    }
    return err;
}


void startSHDispatcher(StorHvQosCtrl *qosctl) {
   if (qosctl && qosctl->htb_dispatcher) {
      qosctl->htb_dispatcher->dispatchIOs();
  }

}
void StorHvQosCtrl::runScheduler() {
  threadPool->schedule(startSHDispatcher, this);
}


Error StorHvQosCtrl::markIODone(FDS_IOType *io) {
    htb_dispatcher->markIODone(io);
}

  void   StorHvQosCtrl::setQosDispatcher(dispatchAlgoType algo_type, FDS_QoSDispatcher *qosDispatcher) {
    dispatchAlgo = algo_type;
    if (qosDispatcher) {
      dispatcher = qosDispatcher;
    } 
  }

Error   StorHvQosCtrl::registerVolume(fds_uint64_t  vol_uuid, FDS_VolumeQueue *volq) {
Error err(ERR_OK);
   err = htb_dispatcher->registerQueue(vol_uuid, volq);
   return err;
}

fds_uint32_t StorHvQosCtrl::waitForWorkers() {
  return 1;
}

Error   StorHvQosCtrl::deregisterVolume(fds_uint64_t vol_uuid) {
Error err(ERR_OK);
    err = htb_dispatcher->deregisterQueue(vol_uuid);
   return err;
}


Error StorHvQosCtrl::enqueueIO(fds_volid_t volUUID, FDS_IOType *io) {
    Error err(ERR_OK);
    htb_dispatcher->enqueueIO(volUUID, io);
    return err;
  }
}
