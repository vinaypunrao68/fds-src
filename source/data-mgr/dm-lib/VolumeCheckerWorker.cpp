/*
 * Copyright 2016 Formation Data Systems, Inc.
 */

#include <DataMgr.h>
#include <VolumeCheckerWorker.h>
namespace fds {

VolumeCheckerWorker::VolumeCheckerWorker(fds_volid_t _volId,
                                         DataMgr &_dm)
    : volId(_volId),
      qosHelper(_dm)
{
    // Nothing
}

std::string
VolumeCheckerWorker::logString() {
    std::string ret;
    ret = "VolumeCheckerWorker for Node: " +
            std::to_string(MODULEPROVIDER()->getSvcMgr()->getSelfSvcUuid().svc_uuid) +
            " volume: " +
            std::to_string(volId.v) + " ";
    return ret;
}

void
VolumeCheckerWorker::scheduleToRun() {
    auto req = new DmFunctor(FdsDmSysTaskId,
            std::bind(&VolumeCheckerWorker::markVolumeOffline, this));
    qosHelper.addToQueue(req);
    LOGDEBUG << logString() << " has been scheduled.";
}

void
VolumeCheckerWorker::markVolumeOffline() {
    LOGDEBUG << logString() << " to mark the volume offline.";
    // TODO
}

} // namespace fds
