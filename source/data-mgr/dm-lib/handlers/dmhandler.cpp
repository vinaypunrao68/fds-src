/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <DataMgr.h>
#include <dmhandler.h>
#include <util/Log.h>

namespace fds { namespace dm {

RequestHelper::RequestHelper(DataMgr& dataManager, DmRequest *dmRequest)
    : dmRequest(dmRequest),
      _dataManager(dataManager)
{}

RequestHelper::~RequestHelper() {
    if (_dataManager.features.isQosEnabled()) {
        Error err = _dataManager.qosCtrl->enqueueIO(dmRequest->getVolId(), dmRequest);
        if (err != ERR_OK) {
            LOGWARN << "Unable to enqueue request for volid:" << dmRequest->getVolId();
            dmRequest->cb(err, dmRequest);
        }
    }
}

QueueHelper::QueueHelper(DataMgr& dataManager, DmRequest *dmRequest)
        : dmRequest(dmRequest),
          ioIsMarkedAsDone(false),
          cancelled(false),
          skipImplicitCb(false),
          dataManager_(dataManager)
{}

QueueHelper::~QueueHelper() {
    if (!cancelled) {
        markIoDone();
        /*
         * TODO(umesh): ignore this for now, uncomment it later
        PerfTracer::tracePointEnd(dmRequest->opLatencyCtx);
        PerfTracer::tracePointEnd(dmRequest->opReqLatencyCtx);
        if (!err.ok()) {
            PerfTracer::incr(dmRequest->opReqFailedPerfEventType, dmRequest->getVolId());
        }
         */
        if (!skipImplicitCb) {
            LOGDEBUG << "calling cb for volid: " << dmRequest->volId;
            dmRequest->cb(err, dmRequest);
        }
    }
}

void QueueHelper::markIoDone() {
    if (!ioIsMarkedAsDone) {
        if (dataManager_.features.isQosEnabled()) dataManager_.qosCtrl->markIODone(*dmRequest);
        ioIsMarkedAsDone = true;
    }
}

void QueueHelper::cancel() {
    cancelled = true;
}

Handler::Handler(DataMgr& dataManager)
    : dataManager(dataManager)
{}

void Handler::handleQueueItem(DmRequest *dmRequest) {
}

void Handler::addToQueue(DmRequest *dmRequest) {
    if (!dataManager.features.isQosEnabled()) {
        LOGWARN << "qos disabled .. not queuing";
        return;
    }
    const VolumeDesc * voldesc = dataManager.getVolumeDesc(dmRequest->getVolId());
    Error err = dataManager.qosCtrl->enqueueIO(voldesc && voldesc->isSnapshot()
                                               ? voldesc->qosQueueId
                                               : dmRequest->getVolId(),
                                               dmRequest);
    if (err != ERR_OK) {
        LOGWARN << "Unable to enqueue request for volid:" << dmRequest->getVolId();
        /*
         * TODO(umesh): ignore this for now, uncomment it later
        PerfTracer::tracePointEnd(dmRequest->opLatencyCtx);
        if (!err.ok()) {
            PerfTracer::incr(dmRequest->opReqFailedPerfEventType, dmRequest->getVolId());
        }
         */
        dmRequest->cb(err, dmRequest);
    } else {
        LOGTRACE << "dmrequest " << dmRequest << " added to queue successfully";
    }
}

Handler::~Handler() {
}

}  // namespace dm
}  // namespace fds
