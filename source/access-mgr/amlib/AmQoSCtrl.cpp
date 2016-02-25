/*
 * Copyright 2013-2016 Formation Data Systems, Inc.
 */

#include "AmQoSCtrl.h"
#include "lib/StatsCollector.h"
#include "AmTxManager.h"
#include "AmRequest.h"

namespace fds {

AmQoSCtrl::AmQoSCtrl(AmDataProvider* prev, uint32_t max_thrds, fds_log *log)
    : FDS_QoSControl::FDS_QoSControl(max_thrds, fds::FDS_QoSControl::FDS_DISPATCH_HIER_TOKEN_BUCKET, log, "SH"),
      AmDataProvider(prev, new AmTxManager(this))
{
    total_rate = 200000;
    htb_dispatcher = new QoSHTBDispatcher(this, qos_log, total_rate);
    dispatcher = htb_dispatcher;
}

AmQoSCtrl::~AmQoSCtrl() {
    htb_dispatcher->stop();
    delete htb_dispatcher;
    if (dispatcherThread) {
        dispatcherThread->join();
    }
}

Error
AmQoSCtrl::updateQoS(int64_t const* rate, float const* throttle) {
    Error err{ERR_OK};
    if (rate) {
        err = htb_dispatcher->modifyTotalRate(*rate);
    }
    if (err.ok() && throttle) {
        htb_dispatcher->setThrottleLevel(*throttle);
    }
    return err;
}

Error AmQoSCtrl::processIO(FDS_IOType *io) {
    fds_verify(io->io_module == FDS_IOType::ACCESS_MGR_IO);
    auto amReq = static_cast<AmRequest*>(io);
    PerfTracer::tracePointEnd(amReq->qos_perf_ctx);
    LOGTRACE << "Scheduling request: 0x" << std::hex << amReq->io_req_id;
    auto vol_id = io->io_vol_id;
    threadPool->schedule([this] (FDS_IOType* io) mutable -> void
                         { unknownTypeResume(static_cast<AmRequest*>(io)); }, io);
    {
        ReadGuard rg(queue_lock);
        auto queue = getQueue(vol_id);
        // If this queue was quiesced and empty, remove it
        if (queue && (FDS_VOL_Q_SUSPENDED == queue->volQState)) {
            htb_dispatcher->deregisterQueue(vol_id.get());
            delete queue;
        }
    }

    return ERR_OK;
}


void startSHDispatcher(AmQoSCtrl *qosctl) {
    if (qosctl && qosctl->dispatcher) {
        qosctl->dispatcher->dispatchIOs();
    }
}

void AmQoSCtrl::completeRequest(AmRequest* amReq, Error const error) {
    // This prevents responding to the same request > 1
    if (0 == amReq->enqueue_ts || amReq->testAndSetComplete()) {
        return AmDataProvider::unknownTypeCb(amReq, error);
    }
    auto remaining = htb_dispatcher->markIODone(amReq);
    // If we were told to stop and have drained the queue, stop
    {
        ReadGuard rg(queue_lock);
        if (stopping
            && (0 == remaining)
            && (0 == htb_dispatcher->num_pending_ios)) {
            FDS_QoSControl::stop();
            AmDataProvider::stop();
        }
    }

    switch (amReq->io_type) {
    case fds::FDS_PUT_BLOB_ONCE:
    case fds::FDS_PUT_BLOB:
        StatsCollector::singleton()->recordEvent(amReq->io_vol_id,
                                                 amReq->io_done_ts,
                                                 STAT_AM_PUT_OBJ,
                                                 amReq->io_total_time);
        if (amReq->io_type == fds::FDS_PUT_BLOB_ONCE) {
            StatsCollector::singleton()->recordEvent(amReq->io_vol_id,
                                                     amReq->io_done_ts,
                                                     STAT_AM_PUT_BLOB_ONCE,
                                                     amReq->io_total_time);
        }
        break;
    case fds::FDS_GET_BLOB:
        StatsCollector::singleton()->recordEvent(amReq->io_vol_id,
                                                 amReq->io_done_ts,
                                                 STAT_AM_GET_BLOB,
                                                 amReq->io_total_time);
        break;
    case fds::FDS_STAT_BLOB:
        StatsCollector::singleton()->recordEvent(amReq->io_vol_id,
                                                 amReq->io_done_ts,
                                                 STAT_AM_GET_BMETA,
                                                 amReq->io_total_time);
        break;
    case fds::FDS_SET_BLOB_METADATA:
        StatsCollector::singleton()->recordEvent(amReq->io_vol_id,
                                                 amReq->io_done_ts,
                                                 STAT_AM_PUT_BMETA,
                                                 amReq->io_total_time);
        break;
    case fds::FDS_ATTACH_VOL:
        StatsCollector::singleton()->recordEvent(amReq->io_vol_id,
                                                 amReq->io_done_ts,
                                                 STAT_AM_ATTACH_VOL,
                                                 amReq->io_total_time);
        break;
    case fds::FDS_DETACH_VOL:
        StatsCollector::singleton()->recordEvent(amReq->io_vol_id,
                                                 amReq->io_done_ts,
                                                 STAT_AM_DETACH_VOL,
                                                 amReq->io_total_time);
        break;
    case fds::FDS_STAT_VOLUME:
        StatsCollector::singleton()->recordEvent(amReq->io_vol_id,
                                                 amReq->io_done_ts,
                                                 STAT_AM_VSTAT,
                                                 amReq->io_total_time);
        break;
    case fds::FDS_SET_VOLUME_METADATA:
        StatsCollector::singleton()->recordEvent(amReq->io_vol_id,
                                                 amReq->io_done_ts,
                                                 STAT_AM_PUT_VMETA,
                                                 amReq->io_total_time);
        break;
    case fds::FDS_GET_VOLUME_METADATA:
        StatsCollector::singleton()->recordEvent(amReq->io_vol_id,
                                                 amReq->io_done_ts,
                                                 STAT_AM_GET_VMETA,
                                                 amReq->io_total_time);
        break;
    case fds::FDS_VOLUME_CONTENTS:
        StatsCollector::singleton()->recordEvent(amReq->io_vol_id,
                                                 amReq->io_done_ts,
                                                 STAT_AM_GET_VCONTENTS,
                                                 amReq->io_total_time);
        break;
    case fds::FDS_START_BLOB_TX:
        StatsCollector::singleton()->recordEvent(amReq->io_vol_id,
                                                 amReq->io_done_ts,
                                                 STAT_AM_START_BLOB_TX,
                                                 amReq->io_total_time);
        break;
    case fds::FDS_COMMIT_BLOB_TX:
        StatsCollector::singleton()->recordEvent(amReq->io_vol_id,
                                                 amReq->io_done_ts,
                                                 STAT_AM_COMMIT_BLOB_TX,
                                                 amReq->io_total_time);
        /**
         * Currently (01/14/2016) transactions are only used for FDS_PUT_BLOB.
         */
        StatsCollector::singleton()->recordEvent(amReq->io_vol_id,
                                                 amReq->io_done_ts,
                                                 STAT_AM_PUT_BLOB,
                                                 amReq->io_total_time);
        break;
    case fds::FDS_ABORT_BLOB_TX:
        StatsCollector::singleton()->recordEvent(amReq->io_vol_id,
                                                 amReq->io_done_ts,
                                                 STAT_AM_ABORT_BLOB_TX,
                                                 amReq->io_total_time);
        /**
         * Currently (01/14/2016) transactions are only used for FDS_PUT_BLOB.
         */
        StatsCollector::singleton()->recordEvent(amReq->io_vol_id,
                                                 amReq->io_done_ts,
                                                 STAT_AM_PUT_BLOB,
                                                 amReq->io_total_time);
        break;
    case fds::FDS_DELETE_BLOB:
        StatsCollector::singleton()->recordEvent(amReq->io_vol_id,
                                                 amReq->io_done_ts,
                                                 STAT_AM_DELETE_BLOB,
                                                 amReq->io_total_time);
        break;
    case fds::FDS_RENAME_BLOB:
        StatsCollector::singleton()->recordEvent(amReq->io_vol_id,
                                                 amReq->io_done_ts,
                                                 STAT_AM_RENAME_BLOB,
                                                 amReq->io_total_time);
        break;
    default:
        ;;
    };
    PerfTracer::incr(PerfEventType::AM_QOS_QUEUE_SIZE, amReq->io_vol_id, remaining, 1); // Let this be a latency counter
    if (remaining > 0) {
        StatsCollector::singleton()->recordEvent(amReq->io_vol_id,
                                                 amReq->io_done_ts,
                                                 STAT_AM_QUEUE_BACKLOG,
                                                 remaining);
    }
    StatsCollector::singleton()->recordEvent(amReq->io_vol_id,
                                             amReq->io_done_ts,
                                             STAT_AM_QUEUE_WAIT,
                                             amReq->io_wait_time);

    return AmDataProvider::unknownTypeCb(amReq, error);
}

void AmQoSCtrl::start() {
    AmDataProvider::start();
    runScheduler();
}

bool
AmQoSCtrl::done() {
    if (!dispatcher->shuttingDown) {
       return false;
    }
    return AmDataProvider::done();
}

void
AmQoSCtrl::registerVolume(VolumeDesc const& volDesc) {
    // If we don't already have this volume, let's register it
    {
        WriteGuard wg(queue_lock);
        if (stopping || volDesc.isSnapshot()) return;
        auto queue = getQueue(volDesc.volUUID);
        if (!queue) {
            queue = new FDS_VolumeQueue(4096,
                                        volDesc.iops_throttle,
                                        volDesc.iops_assured,
                                        volDesc.relativePrio);
            if (ERR_OK != htb_dispatcher->registerQueue(volDesc.volUUID.get(), queue)) {
                delete queue;
                return;
            }
            queue->activate();
        } else {
            htb_dispatcher->modifyQueueQosParams(volDesc.volUUID.get(),
                                                 volDesc.iops_assured,
                                                 volDesc.iops_throttle,
                                                 volDesc.relativePrio);
        }
    }
    AmDataProvider::registerVolume(volDesc);
}

Error
AmQoSCtrl::modifyVolumePolicy(const VolumeDesc& vdesc) {
    htb_dispatcher->modifyQueueQosParams(vdesc.volUUID.get(),
                                         vdesc.iops_assured,
                                         vdesc.iops_throttle,
                                         vdesc.relativePrio);
    return AmDataProvider::modifyVolumePolicy(vdesc);
}

/*
 * Removes volume from the map, returns error if volume does not exist
 */
void
AmQoSCtrl::removeVolume(VolumeDesc const& volDesc) {
    {
        WriteGuard wg(queue_lock);
        auto queue = getQueue(volDesc.volUUID);
        if (0 != queue->count()) {
            queue->quiesceIOs();
        } else {
            htb_dispatcher->deregisterQueue(volDesc.volUUID.get());
            delete queue;
        }
    }
    AmDataProvider::removeVolume(volDesc);
}

void AmQoSCtrl::enqueueRequest(AmRequest *amReq) {
    PerfTracer::tracePointBegin(amReq->qos_perf_ctx);

    GLOGDEBUG << "Entering QoS request: 0x" << std::hex << amReq->io_req_id << std::dec;
    Error err {ERR_OK};
    {
        ReadGuard rg(queue_lock);
        err = htb_dispatcher->enqueueIO(amReq->io_vol_id.get(), amReq);
    }
    if (ERR_OK != err) {
        GLOGERROR << "Had an issue with queueing a request to volume: " << amReq->volume_name
                  << " error: " << err;
        PerfTracer::tracePointEnd(amReq->qos_perf_ctx);
        AmDataProvider::unknownTypeCb(amReq, err);
    }
}

void
AmQoSCtrl::stop() {
    {
        WriteGuard wg(queue_lock);
        stopping = true;
        htb_dispatcher->quiesceIOs();
        if ((0 == htb_dispatcher->num_pending_ios) && (0 == htb_dispatcher->num_outstanding_ios)) {
            FDS_QoSControl::stop();
            AmDataProvider::stop();
        }
    }
}

}  // namespace fds
