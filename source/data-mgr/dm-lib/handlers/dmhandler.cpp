/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <DataMgr.h>
#include <dmhandler.h>

namespace fds { namespace dm {

RequestHelper::RequestHelper(dmCatReq *dmRequest) : dmRequest(dmRequest) {
}

RequestHelper::~RequestHelper() {
    Error err = dataMgr->qosCtrl->enqueueIO(dmRequest->getVolId(), dmRequest);
    if (err != ERR_OK) {
        LOGWARN << "Unable to enqueue request for volid:" << dmRequest->getVolId();
        dmRequest->cb(err, dmRequest);
    }
}

QueueHelper::QueueHelper(dmCatReq *dmRequest) : dmRequest(dmRequest) {
}

QueueHelper::~QueueHelper() {
    dataMgr->qosCtrl->markIODone(*dmRequest);
    LOGDEBUG << "calling cb for volid: " << dmRequest->volId;
    dmRequest->cb(err, dmRequest);
}

void Handler::handleQueueItem(dmCatReq *dmRequest) {
}

void Handler::addToQueue(dmCatReq *dmRequest) {
    const VolumeDesc * voldesc = dataMgr->getVolumeDesc(dmRequest->getVolId());
    Error err = dataMgr->qosCtrl->enqueueIO(voldesc && voldesc->isSnapshot() ?
            voldesc->qosQueueId : dmRequest->getVolId(), dmRequest);
    if (err != ERR_OK) {
        LOGWARN << "Unable to enqueue request for volid:" << dmRequest->getVolId();
        dmRequest->cb(err, dmRequest);
    } else {
        LOGTRACE << "dmrequest added to queue successfully";
    }
}

Handler::~Handler() {
}

}  // namespace dm
}  // namespace fds
