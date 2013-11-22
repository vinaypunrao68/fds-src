#include "StorHvQosCtrl.h" 
#include "StorHvisorCPP.h"

namespace fds {

StorHvQosCtrl *storHvQosCtrl; // global pointer to track the singleton instance

void StorHvQosCtrl::throttleCmdHandler(const float throttle_level) {
  storHvQosCtrl->htb_dispatcher->setThrottleLevel(throttle_level);
}

StorHvQosCtrl::StorHvQosCtrl(uint32_t max_thrds, dispatchAlgoType algo, fds_log *log) 
  : FDS_QoSControl::FDS_QoSControl(max_thrds, algo, log, "SH") 
{
     total_rate = 3000;
     if ( dispatchAlgo == FDS_QoSControl::FDS_DISPATCH_HIER_TOKEN_BUCKET) {
        htb_dispatcher = new QoSHTBDispatcher(this, qos_log, total_rate);
     }
     storHvQosCtrl = this;

     /* base class created stats, but it is disabled by default */
     stats->enable();
}

StorHvQosCtrl::~StorHvQosCtrl() {
  htb_dispatcher->stop();
  delete htb_dispatcher;
  if (stats)
    stats->disable();
}


Error StorHvQosCtrl::processIO(FDS_IOType *io) {
  Error err(ERR_OK);
  fds_verify(io->io_module == FDS_IOType::STOR_HV_IO);
  
  // TODO: New call to eventually be uncommented
  threadPool->schedule(processBlobReq, static_cast<AmQosReq*>(io));

  // TODO: Old code to eventually remove
  /*
  if(io->io_type == FDS_IO_READ) {
    threadPool->schedule(StorHvisorProcIoRd, io);
  } else {
    fds_verify(io->io_type == FDS_IO_WRITE);
    threadPool->schedule(StorHvisorProcIoWr, io);
  }
  */

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
  Error err(ERR_OK);
  /* TODO: also record latency (now passign 0) */
  assert(io->io_magic == FDS_SH_IO_MAGIC_IN_USE);
  io->io_magic = FDS_SH_IO_MAGIC_NOT_IN_USE;
  stats->recordIO(io->io_vol_id, 0);
  htb_dispatcher->markIODone(io);
  delete io;
  return err;
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
