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
    dmRequest->cb(err, dmRequest);
}

}  // namespace dm
}  // namespace fds
