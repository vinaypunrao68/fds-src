/*
 * Copyright 2014-2015 Formation Data Systems, Inc.
 */

#ifndef SOURCE_ACCESS_MGR_STOR_HVISOR_STORHVQOSCTRL_H_
#define SOURCE_ACCESS_MGR_STOR_HVISOR_STORHVQOSCTRL_H_

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
  Error modifyVolumeQosParams(fds_volid_t vol_uuid,
                              fds_int64_t iops_assured,
                              fds_int64_t iops_throttle,
                              fds_uint32_t prio);
  Error   deregisterVolume(fds_volid_t vol_uuid);
  Error enqueueIO(fds_volid_t volUUID, FDS_IOType *io);
};
}

#endif  // SOURCE_ACCESS_MGR_STOR_HVISOR_STORHVQOSCTRL_H_
