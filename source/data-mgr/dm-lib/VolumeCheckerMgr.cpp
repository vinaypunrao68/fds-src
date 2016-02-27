/*
 * Copyright 2016 Formation Data Systems, Inc.
 */

#include <DataMgr.h>
#include <VolumeCheckerMgr.h>

namespace fds {

VolumeCheckerMgr::VolumeCheckerMgr(DataMgr &_dataMgr)
    : dataMgr(_dataMgr)
{
    // Nothing for now
}

Error
VolumeCheckerMgr::registerRequest(DmRequest *req) {
    DmIoVolumeCheck *request = static_cast<DmIoVolumeCheck*>(req);
    Error err(ERR_OK);

    auto msgPtr = request->reqMessage;

    // Get things needed for identifying request
    fds_volid_t incomingVolId;
    incomingVolId.v = msgPtr->volume_id;
    fpi::SvcUuid checkerUuid(msgPtr->volCheckerNodeUuid);

    LOGDEBUG << "Received initial checker message for volume " << incomingVolId.v;

    // First find to see if it's an existing request
    auto it = vcWorkerMap.emplace(std::make_pair(incomingVolId,
                       vcWorkerPtr(new VolumeCheckerWorker(incomingVolId, dataMgr))));
    if (it.second) {
        it.first->second->scheduleToRun();
    } else {
        LOGERROR << "Volume checker for volume: " << incomingVolId.v << " already exists";
        err = ERR_DUPLICATE;
    }
    // QueueHelper in the handler will send async resp back w/ this err code
    return err;
}


/**
 * Unit test implementations
 */
size_t
VolumeCheckerMgr::testGetvcWorkerMapSize() {
    return vcWorkerMap.size();
}

} // namespace fds
