#ifndef __STOR_HV_QOS_CTRL_H__
#define  __STOR_HV_QOS_CTRL_H__
#include "qos_ctrl.h"
#include "fds_qos.h"
#include "../lib/qos_htb.h"
#include <util/Log.h>
#include <concurrency/ThreadPool.h>
#include "fds_error.h"
#include <fds_types.h>
#include <fds_error.h>
#include <fds_volume.h>

namespace fds {
class StorHvQosCtrl : public FDS_QoSControl { 
  public:
  QoSHTBDispatcher *htb_dispatcher;

  StorHvQosCtrl(uint32_t max_thrds, dispatchAlgoType algo, fds_log *log);
  virtual ~StorHvQosCtrl();
  virtual FDS_VolumeQueue* getQueue(fds_volid_t queueId);
  Error processIO(FDS_IOType *io) ;
  void runScheduler();
  Error markIODone(FDS_IOType *io);
  fds_uint32_t waitForWorkers();
  void   setQosDispatcher(dispatchAlgoType algo_type, FDS_QoSDispatcher *qosDispatcher);
  Error   registerVolume(fds_volid_t vol_uuid, FDS_VolumeQueue *volq);
  Error modifyVolumeQosParams(fds_volid_t vol_uuid, fds_uint64_t iops_min, fds_uint64_t iops_max, fds_uint32_t prio);
  Error   deregisterVolume(fds_volid_t vol_uuid);
  Error enqueueIO(fds_volid_t volUUID, FDS_IOType *io);

  static void throttleCmdHandler(const float throttle_level);
  static Error qosCtrlCmdHandler(fds_uint64_t tot_rate);

};
}

#endif
