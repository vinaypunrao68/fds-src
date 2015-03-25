/*
 * Copyright 2013-2015 Formation Data Systems, Inc.
 */

#include "AmVolume.h"
#include "StorHvQosCtrl.h"
#include "StorHvCtrl.h"

extern StorHvCtrl* storHvisor;

namespace fds
{

AmVolume::AmVolume(const VolumeDesc& vdesc, fds_log *parent_log)
    : FDS_Volume(vdesc),
      volQueue(nullptr)
{
    if (vdesc.isSnapshot()) {
        volQueue.reset(storHvisor->qos_ctrl->getQueue(vdesc.qosQueueId));
    }

    if (!volQueue) {
        volQueue.reset(new FDS_VolumeQueue(4096, vdesc.iops_max, vdesc.iops_min, vdesc.relativePrio));
    }
    volQueue->activate();
}

AmVolume::~AmVolume() {
    storHvisor->qos_ctrl->deregisterVolume(voldesc->volUUID);
}

}  // namespace fds
