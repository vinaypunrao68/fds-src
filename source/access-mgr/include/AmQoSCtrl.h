/*
 * Copyright 2014-2015 Formation Data Systems, Inc.
 */

#ifndef SOURCE_ACCESS_MGR_INCLUDE_AMQOSCTRL_H_
#define SOURCE_ACCESS_MGR_INCLUDE_AMQOSCTRL_H_

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
struct AmRequest;

class AmQoSCtrl : public FDS_QoSControl {
    using vol_callback_type = std::function<void(AmRequest*)>;
    vol_callback_type vol_callback;
 public:
    QoSHTBDispatcher *htb_dispatcher;

    AmQoSCtrl(uint32_t max_thrds, dispatchAlgoType algo, fds_log *log);
    virtual ~AmQoSCtrl();
    virtual FDS_VolumeQueue* getQueue(fds_volid_t queueId);
    Error processIO(FDS_IOType *io);
    void runScheduler(vol_callback_type&& cb);
    Error markIODone(AmRequest *io);
    fds_uint32_t waitForWorkers();
    void   setQosDispatcher(dispatchAlgoType algo_type, FDS_QoSDispatcher *qosDispatcher);
    Error   registerVolume(fds_volid_t vol_uuid, FDS_VolumeQueue *volq);
    Error modifyVolumeQosParams(fds_volid_t vol_uuid, fds_uint64_t iops_min, fds_uint64_t iops_max, fds_uint32_t prio);
    Error   deregisterVolume(fds_volid_t vol_uuid);
    Error enqueueIO(AmRequest *io);
};

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_AMQOSCTRL_H_
