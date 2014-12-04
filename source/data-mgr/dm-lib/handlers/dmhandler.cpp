/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <DataMgr.h>
#include <dmhandler.h>
#include <util/Log.h>

namespace fds { namespace dm {

RequestHelper::RequestHelper(dmCatReq *dmRequest) : dmRequest(dmRequest) {
}

RequestHelper::~RequestHelper() {
    if (dataMgr->feature.isQosEnabled()) {
        Error err = dataMgr->qosCtrl->enqueueIO(dmRequest->getVolId(), dmRequest);
        if (err != ERR_OK) {
            LOGWARN << "Unable to enqueue request for volid:" << dmRequest->getVolId();
            dmRequest->cb(err, dmRequest);
        }
    }
}

QueueHelper::QueueHelper(dmCatReq *dmRequest)
    : dmRequest(dmRequest), ioIsMarkedAsDone(false), cancelled(false) {
}

QueueHelper::~QueueHelper() {
    if (!cancelled) {
        markIoDone();
        /*
         * TODO(umesh): ignore this for now, uncomment it later
        PerfTracer::tracePointEnd(dmRequest->opLatencyCtx);
        PerfTracer::tracePointEnd(dmRequest->opReqLatencyCtx);
        if (!err.ok()) {
            PerfTracer::incr(dmRequest->opReqFailedPerfEventType, dmRequest->getVolId(),
                    dmRequest->perfNameStr);
        }
         */
        if (dmRequest->cb != NULL) {
            LOGDEBUG << "calling cb for volid: " << dmRequest->volId;
            dmRequest->cb(err, dmRequest);
        }
    }
}

void QueueHelper::markIoDone() {
    if (!ioIsMarkedAsDone) {
        if (dataMgr->feature.isQosEnabled()) dataMgr->qosCtrl->markIODone(*dmRequest);
        ioIsMarkedAsDone = true;
    }
}

void QueueHelper::cancel() {
    cancelled = true;
}

void Handler::handleQueueItem(dmCatReq *dmRequest) {
}

void Handler::addToQueue(dmCatReq *dmRequest) {
    if (!dataMgr->feature.isQosEnabled()) {
        LOGWARN << "qos disabled .. not queuing";
        return;
    }
    const VolumeDesc * voldesc = dataMgr->getVolumeDesc(dmRequest->getVolId());
    Error err = dataMgr->qosCtrl->enqueueIO(voldesc && voldesc->isSnapshot() ?
            voldesc->qosQueueId : dmRequest->getVolId(), dmRequest);
    if (err != ERR_OK) {
        LOGWARN << "Unable to enqueue request for volid:" << dmRequest->getVolId();
        /*
         * TODO(umesh): ignore this for now, uncomment it later
        PerfTracer::tracePointEnd(dmRequest->opLatencyCtx);
        if (!err.ok()) {
            PerfTracer::incr(dmRequest->opReqFailedPerfEventType, dmRequest->getVolId(),
                    dmRequest->perfNameStr);
        }
         */
        dmRequest->cb(err, dmRequest);
    } else {
        LOGTRACE << "dmrequest added to queue successfully";
    }
}

Handler::~Handler() {
}

}  // namespace dm
}  // namespace fds
