/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#include <string>
#include "qos_ctrl.h"
#include "fds_qos.h"
#include "fds_error.h"
#include "qos_htb.h"

namespace fds {

FDS_QoSControl::FDS_QoSControl() {
}

FDS_QoSControl::FDS_QoSControl(fds_uint32_t _max_threads, dispatchAlgoType algo, fds_log *log,
                                       const std::string& prefix) :qos_max_threads(_max_threads)
{
    threadPool = new fds_threadpool("QosThreadpool", qos_max_threads);
    dispatchAlgo = algo;
    qos_log = log;
    total_rate = 20000;  // IOPS
}

FDS_QoSControl::~FDS_QoSControl()  {
    delete threadPool;
}

fds_uint32_t FDS_QoSControl::waitForWorkers() {
    return 10;
}

static void startDispatcher(FDS_QoSControl *qosctl) {
    if (qosctl && qosctl->dispatcher) {
        try {
            qosctl->dispatcher->dispatchIOs();
        } catch (const std::exception &e) {
            fds_assert(!"Exception in qos dispatcher");
            GLOGERROR << "Qos dispatcher exited with exception." << e.what();
        } catch (...) {
            fds_assert(!"Exception in qos dispatcher");
            GLOGERROR << "Qos dispatcher exited with unknown exception.";
        }
    }
}

void FDS_QoSControl::runScheduler() {
    dispatcherThread.reset(new std::thread(startDispatcher, this));
}

void   FDS_QoSControl::setQosDispatcher(dispatchAlgoType algo_type,
                                    FDS_QoSDispatcher *qosDispatcher) {
}

Error   FDS_QoSControl::registerVolume(fds_volid_t vol_uuid, FDS_VolumeQueue *volq) {
    Error err(ERR_OK);
    err = dispatcher->registerQueue(vol_uuid.get(), volq);
    return err;
}

Error FDS_QoSControl::modifyVolumeQosParams(fds_volid_t vol_uuid,
                                            fds_int64_t iops_assured,
                                            fds_int64_t iops_throttle,
                                            fds_uint32_t prio)
{
    Error err(ERR_OK);
    err = dispatcher->modifyQueueQosParams(vol_uuid.get(), iops_assured, iops_throttle, prio);
    return err;
}


Error   FDS_QoSControl::deregisterVolume(fds_volid_t vol_uuid) {
    Error err(ERR_OK);
    err = dispatcher->deregisterQueue(vol_uuid.get());
    return err;
}

Error FDS_QoSControl::enqueueIO(fds_volid_t volUUID, FDS_IOType *io) {
    Error err(ERR_OK);
    err = dispatcher->enqueueIO(volUUID.get(), io);
    return err;
}

fds_uint32_t FDS_QoSControl::queueSize(fds_volid_t volId) {
    return dispatcher->count(volId.get());
}

FDS_VolumeQueue* FDS_QoSControl::getQueue(fds_volid_t queueId) {
    return dispatcher->getQueue(queueId.get());
}

void FDS_QoSControl::quieseceIOs(fds_volid_t volUUID) {
    dispatcher->quiesceIOs(volUUID.get());
}

void FDS_QoSControl::resumeIOs(fds_volid_t volUUID) {
    dispatcher->resumeQueue(volUUID.get());
}

void FDS_QoSControl::stopDequeue(fds_volid_t volUUID) {
    dispatcher->stopDequeue(volUUID.get());
}

Error FDS_QoSControl::processIO(FDS_IOType *io_type) {
    Error err(ERR_OK);

    return err;
}

}  // namespace fds
