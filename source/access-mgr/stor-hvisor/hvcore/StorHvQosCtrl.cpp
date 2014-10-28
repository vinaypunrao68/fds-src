#include "StorHvQosCtrl.h"
#include <StorHvVolumes.h>
#include <lib/StatsCollector.h>
#include <StorHvisorNet.h>

namespace fds {

StorHvQosCtrl *storHvQosCtrl; // global pointer to track the singleton instance

StorHvQosCtrl::StorHvQosCtrl(uint32_t max_thrds, dispatchAlgoType algo, fds_log *log)
  : FDS_QoSControl::FDS_QoSControl(max_thrds, algo, log, "SH")
{
     total_rate = 200000;
     if ( dispatchAlgo == FDS_QoSControl::FDS_DISPATCH_HIER_TOKEN_BUCKET) {
        htb_dispatcher = new QoSHTBDispatcher(this, qos_log, total_rate);
     }
     storHvQosCtrl = this;
}

StorHvQosCtrl::~StorHvQosCtrl() {
  htb_dispatcher->stop();
  delete htb_dispatcher;
}

FDS_VolumeQueue* StorHvQosCtrl::getQueue(fds_volid_t queueId) {
    return htb_dispatcher->getQueue(queueId);
}

Error StorHvQosCtrl::processIO(FDS_IOType *io) {
  fds_verify(io->io_module == FDS_IOType::STOR_HV_IO);
  threadPool->schedule(processBlobReq, static_cast<AmRequest*>(io));
  return ERR_OK;
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
  assert(io->io_magic == FDS_SH_IO_MAGIC_IN_USE);
  io->io_magic = FDS_SH_IO_MAGIC_NOT_IN_USE;
  htb_dispatcher->markIODone(io);

  switch (io->io_type) {
      case fds::FDS_IO_WRITE:
      case fds::FDS_PUT_BLOB_ONCE:
      case fds::FDS_PUT_BLOB:
          StatsCollector::singleton()->recordEvent(io->io_vol_id,
                                                   io->io_done_ts,
                                                   STAT_AM_PUT_OBJ,
                                                   io->io_total_time);
          break;
      case fds::FDS_IO_READ:
      case fds::FDS_GET_BLOB:
          StatsCollector::singleton()->recordEvent(io->io_vol_id,
                                                   io->io_done_ts,
                                                   STAT_AM_GET_OBJ,
                                                   io->io_total_time);
          break;
      case fds::FDS_STAT_BLOB:
          StatsCollector::singleton()->recordEvent(io->io_vol_id,
                                                   io->io_done_ts,
                                                   STAT_AM_GET_BMETA,
                                                   io->io_total_time);
          break;
      case fds::FDS_SET_BLOB_METADATA:
          StatsCollector::singleton()->recordEvent(io->io_vol_id,
                                                   io->io_done_ts,
                                                   STAT_AM_PUT_BMETA,
                                                   io->io_total_time);
          break;
      default:
          ;;
  };
  fds_uint32_t queue_size = htb_dispatcher->count(io->io_vol_id);
  PerfTracer::incr(AM_QOS_QUEUE_SIZE, io->io_vol_id, queue_size, 1); // Let this be a latency counter
  if (queue_size > 0) {
      StatsCollector::singleton()->recordEvent(io->io_vol_id,
                                               io->io_done_ts,
                                               STAT_AM_QUEUE_FULL,
                                               queue_size);
  }
  StatsCollector::singleton()->recordEvent(io->io_vol_id,
                                           io->io_done_ts,
                                           STAT_AM_QUEUE_WAIT,
                                           io->io_wait_time);

  delete io;
  return err;
}

  void   StorHvQosCtrl::setQosDispatcher(dispatchAlgoType algo_type, FDS_QoSDispatcher *qosDispatcher) {
    dispatchAlgo = algo_type;
    if (qosDispatcher) {
      dispatcher = qosDispatcher;
    }
  }

Error   StorHvQosCtrl::registerVolume(fds_int64_t  vol_uuid, FDS_VolumeQueue *volq) {
Error err(ERR_OK);
   err = htb_dispatcher->registerQueue(vol_uuid, volq);
   return err;
}

Error StorHvQosCtrl::modifyVolumeQosParams(fds_volid_t vol_uuid,
					  fds_uint64_t iops_min,
					  fds_uint64_t iops_max,
					  fds_uint32_t prio)
{
  Error err(ERR_OK);
  err = htb_dispatcher->modifyQueueQosParams(vol_uuid, iops_min, iops_max, prio);
  return err;
}


fds_uint32_t StorHvQosCtrl::waitForWorkers() {
  return 1;
}

Error   StorHvQosCtrl::deregisterVolume(fds_int64_t vol_uuid) {
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
