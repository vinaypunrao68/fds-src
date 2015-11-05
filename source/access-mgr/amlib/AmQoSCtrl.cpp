/*
 * Copyright 2013-2015 Formation Data Systems, Inc.
 */

#include "AmQoSCtrl.h"
#include "lib/StatsCollector.h"
#include "AmRequest.h"
#include "AmVolumeTable.h"
#include "AmVolume.h"
#include "requests/AttachVolumeReq.h"
#include <fiu-control.h>
#include <util/fiu_util.h>

namespace fds {

/*
 * Atomic for request ids.
 */
std::atomic_uint nextIoReqId;

/**
 * Wait queue for refuge requests
 */
class WaitQueue {
    using volume_queue_type = std::deque<AmRequest*>;
    using wait_queue_type = std::unordered_map<std::string, volume_queue_type>;
    wait_queue_type queue;
    mutable std::mutex wait_lock;

 public:
    WaitQueue() = default;
    WaitQueue(WaitQueue const&) = delete;
    WaitQueue& operator=(WaitQueue const&) = delete;
    ~WaitQueue() = default;

    template<typename Cb>
    void remove_if(std::string const&, Cb&&);
    Error delay(AmRequest*);
    bool empty() const;
};

template<typename Cb>
void WaitQueue::remove_if(std::string const& vol_name, Cb&& cb) {
    std::lock_guard<std::mutex> l(wait_lock);
    auto wait_it = queue.find(vol_name);
    if (queue.end() != wait_it) {
        auto& vol_queue = wait_it->second;
        auto new_end = std::remove_if(vol_queue.begin(),
                                      vol_queue.end(),
                                      std::forward<Cb>(cb));
        vol_queue.erase(new_end, vol_queue.end());
        if (wait_it->second.empty()) {
            queue.erase(wait_it);
        }
    }
}

Error WaitQueue::delay(AmRequest* amReq) {
    std::lock_guard<std::mutex> l(wait_lock);
    auto wait_it = queue.find(amReq->volume_name);
    if (queue.end() != wait_it) {
        wait_it->second.push_back(amReq);
        return ERR_OK;
    } else {
        queue[amReq->volume_name].push_back(amReq);
        return ERR_VOL_NOT_FOUND;
    }
}

bool WaitQueue::empty() const {
    std::lock_guard<std::mutex> l(wait_lock);
    return queue.empty();
}


AmQoSCtrl::AmQoSCtrl(uint32_t max_thrds, dispatchAlgoType algo, CommonModuleProviderIf* provider, fds_log *log)
    : FDS_QoSControl::FDS_QoSControl(max_thrds, algo, log, "SH"),
      volTable(new AmVolumeTable(provider, GetLog())),
      wait_queue(new WaitQueue())
{
    total_rate = 200000;
    if (FDS_QoSControl::FDS_DISPATCH_HIER_TOKEN_BUCKET == dispatchAlgo) {
        htb_dispatcher = new QoSHTBDispatcher(this, qos_log, total_rate);
    }
}

AmQoSCtrl::~AmQoSCtrl() {
    htb_dispatcher->stop();
    delete htb_dispatcher;
    if (dispatcherThread) {
        dispatcherThread->join();
    }
}

FDS_VolumeQueue* AmQoSCtrl::getQueue(fds_volid_t queueId) {
    return htb_dispatcher->getQueue(queueId.get());
}

Error
AmQoSCtrl::updateQoS(long int const* rate, float const* throttle) {
    Error err{ERR_OK};
    if (rate) {
        err = htb_dispatcher->modifyTotalRate(*rate);
    }
    if (err.ok() && throttle) {
        htb_dispatcher->setThrottleLevel(*throttle);
    }
    return err;
}

void
AmQoSCtrl::execRequest(FDS_IOType* io) {
    auto amReq = static_cast<AmRequest*>(io);
    switch (amReq->io_type) {
        /* === Volume operations === */
        case fds::FDS_ATTACH_VOL:
            volTable->openVolume(amReq);
            break;

        case fds::FDS_DETACH_VOL:
            detachVolume(amReq);
            break;

        case fds::FDS_GET_VOLUME_METADATA:
            fiu_do_on("am.uturn.processor.getVolMetadata",
                      completeRequest(amReq, ERR_OK); \
                      return;);

            volTable->getVolumeMetadata(amReq);
            break;

        case fds::FDS_SET_VOLUME_METADATA:
            fiu_do_on("am.uturn.processor.setVolMetadata",
                      completeRequest(amReq, ERR_OK); \
                      return;);
            volTable->setVolumeMetadata(amReq);
            break;

        case fds::FDS_STAT_VOLUME:
            fiu_do_on("am.uturn.processor.statVol",
                      completeRequest(amReq, ERR_OK); \
                      return;);
            volTable->statVolume(amReq);
            break;

        case fds::FDS_VOLUME_CONTENTS:
            volTable->volumeContents(amReq);
            break;

        /* == Tx based operations == */
        case fds::FDS_ABORT_BLOB_TX:
            volTable->abortBlobTx(amReq);
            break;

        case fds::FDS_COMMIT_BLOB_TX:
            volTable->commitBlobTx(amReq);
            break;

        case fds::FDS_DELETE_BLOB:
            volTable->deleteBlob(amReq);
            break;

        case fds::FDS_SET_BLOB_METADATA:
            volTable->setBlobMetadata(amReq);
            break;

        case fds::FDS_START_BLOB_TX:
            fiu_do_on("am.uturn.processor.startBlobTx",
                      completeRequest(amReq, ERR_OK); \
                      return;);

            volTable->startBlobTx(amReq);
            break;


        /* ==== Read operations ==== */
        case fds::FDS_GET_BLOB:
            fiu_do_on("am.uturn.processor.getBlob",
                      completeRequest(amReq, ERR_OK); \
                      return;);

            volTable->getBlob(amReq);
            break;

        case fds::FDS_STAT_BLOB:
            volTable->statBlob(amReq);
            break;

        case fds::FDS_PUT_BLOB:
        case fds::FDS_PUT_BLOB_ONCE:
            // We piggy back this operation with some runtime conditions
            fiu_do_on("am.uturn.processor.putBlob",
                      completeRequest(amReq, ERR_OK); \
                      return;);

            volTable->updateCatalog(amReq);
            break;

        case fds::FDS_RENAME_BLOB:
            volTable->renameBlob(amReq);
            break;

        default :
            LOGCRITICAL << "unimplemented request: " << amReq->io_type;
            completeRequest(amReq, ERR_NOT_IMPLEMENTED);
            break;
    }
}

Error AmQoSCtrl::processIO(FDS_IOType *io) {
    fds_verify(io->io_module == FDS_IOType::ACCESS_MGR_IO);
    threadPool->schedule([this] (FDS_IOType* io) mutable -> void { execRequest(io); }, io);
    return ERR_OK;
}


void startSHDispatcher(AmQoSCtrl *qosctl) {
    if (qosctl && qosctl->htb_dispatcher) {
        qosctl->htb_dispatcher->dispatchIOs();
    }
}

void AmQoSCtrl::completeRequest(AmRequest* amReq, Error const& error) {
    if (FDS_ATTACH_VOL == amReq->io_type && error.ok()) {
        auto volReq = static_cast<AttachVolumeReq*>(amReq);
        auto& vol_desc = *volReq->volDesc;
        /** Drain wait queue into QoS */
        wait_queue->remove_if(vol_desc.name,
                              [this, vol_uuid = vol_desc.GetID()] (AmRequest* amReq) {
                                  amReq->setVolId(vol_uuid);
                                  GLOGTRACE << "Resuming request: 0x" << std::hex << amReq->io_req_id;
                                  htb_dispatcher->enqueueIO(amReq->io_vol_id.get(), amReq);
                                  return true;
                              });
    }
    if (0 != amReq->enqueue_ts && markIODone(amReq)) {
        processor_cb(amReq, error);
    }
}


void AmQoSCtrl::init(processor_cb_type const& cb) {
    processor_cb = cb;
    volTable->init([this] (AmRequest* amReq, Error const& error) mutable -> void { completeRequest(amReq, error); });
    dispatcherThread.reset(new std::thread(startSHDispatcher, this));
}


Error AmQoSCtrl::markIODone(AmRequest *io) {
    Error err(ERR_OK);

    // This prevents responding to the same request > 1
    if (io->testAndSetComplete()) {
        return ERR_DUPLICATE;
    }
    auto remaining = htb_dispatcher->markIODone(io);

    switch (io->io_type) {
    case fds::FDS_IO_WRITE:
    case fds::FDS_PUT_BLOB_ONCE:
    case fds::FDS_PUT_BLOB:
        StatsCollector::singleton()->recordEvent(io->io_vol_id,
                                                 io->io_done_ts,
                                                 STAT_AM_PUT_OBJ,
                                                 io->io_total_time);
        break;
    case fds::FDS_IO_READ:
    case fds::FDS_GET_BLOB:
        StatsCollector::singleton()->recordEvent(io->io_vol_id,
                                                 io->io_done_ts,
                                                 STAT_AM_GET_OBJ,
                                                 io->io_total_time);
        break;
    case fds::FDS_STAT_BLOB:
        StatsCollector::singleton()->recordEvent(io->io_vol_id,
                                                 io->io_done_ts,
                                                 STAT_AM_GET_BMETA,
                                                 io->io_total_time);
        break;
    case fds::FDS_SET_BLOB_METADATA:
        StatsCollector::singleton()->recordEvent(io->io_vol_id,
                                                 io->io_done_ts,
                                                 STAT_AM_PUT_BMETA,
                                                 io->io_total_time);
        break;
    default:
        ;;
    };
    PerfTracer::incr(PerfEventType::AM_QOS_QUEUE_SIZE, io->io_vol_id, remaining, 1); // Let this be a latency counter
    if (remaining > 0) {
        StatsCollector::singleton()->recordEvent(io->io_vol_id,
                                                 io->io_done_ts,
                                                 STAT_AM_QUEUE_FULL,
                                                 remaining);
    }
    StatsCollector::singleton()->recordEvent(io->io_vol_id,
                                             io->io_done_ts,
                                             STAT_AM_QUEUE_WAIT,
                                             io->io_wait_time);

    return err;
}

void   AmQoSCtrl::setQosDispatcher(dispatchAlgoType algo_type, FDS_QoSDispatcher *qosDispatcher) {
    dispatchAlgo = algo_type;
    if (qosDispatcher) {
        dispatcher = qosDispatcher;
    }
}

Error AmQoSCtrl::registerVolume(VolumeDesc const& volDesc) {
    Error err(ERR_OK);
    auto vol = volTable->getVolume(volDesc.volUUID);
    // If we don't already have this volume, let's register it
    if (!vol) {
        /** Create queue and register with QoS */
        FDS_VolumeQueue* queue {nullptr};
        if (volDesc.isSnapshot()) {
            GLOGDEBUG << "Volume is a snapshot : [0x" << std::hex << volDesc.volUUID << "]";
            queue = getQueue(volDesc.qosQueueId);
        }
        if (!queue) {
            queue = new FDS_VolumeQueue(4096,
                                        volDesc.iops_throttle,
                                        volDesc.iops_assured,
                                        volDesc.relativePrio);
            err = htb_dispatcher->registerQueue(volDesc.volUUID.get(), queue);
            if (ERR_OK != err) {
                LOGERROR << "Volume failed to register : [0x"
                    << std::hex << volDesc.volUUID << "]"
                    << " because: " << err;
            } else {
                volTable->registerVolume(volDesc, queue);
            }
        }
    }

    // Search the queue for the first attach we find.
    // The wait queue will iterate the queue for the given volume and
    // apply the callback to each request. For the first attach request
    // we find we process it (to cause volumeOpen to start), and remove
    // it from the queue...but only the first.
    wait_queue->remove_if(volDesc.name,
                          [this, &vol_desc = volDesc, found = false]
                          (AmRequest* amReq) mutable -> bool {
                              if (!found && FDS_ATTACH_VOL == amReq->io_type) {
                                  auto volReq = static_cast<AttachVolumeReq*>(amReq);
                                  volReq->setVolId(vol_desc.volUUID);
                                  volTable->openVolume(amReq);
                                  found = true;
                                  return true;
                              }
                              return false;
                          });

    return err;
}

void
AmQoSCtrl::detachVolume(AmRequest *amReq) {
    // This really can not fail, we have to be attached to be here
    auto vol = volTable->getVolume(amReq->io_vol_id);
    if (!vol) {
        LOGCRITICAL << "unable to get volume info for vol: " << amReq->io_vol_id;
        return completeRequest(amReq, ERR_NOT_READY);
    }

    removeVolume(vol->voldesc->name, vol->voldesc->volUUID);
    completeRequest(amReq, ERR_OK);
}

Error
AmQoSCtrl::modifyVolumePolicy(fds_volid_t vol_uuid, const VolumeDesc& vdesc) {
    Error err(ERR_OK);

    err = volTable->modifyVolumePolicy(vol_uuid, vdesc);
    if (ERR_OK == err) {
        err = htb_dispatcher->modifyQueueQosParams(vol_uuid.get(),
                                                   vdesc.iops_assured,
                                                   vdesc.iops_throttle,
                                                   vdesc.relativePrio);
    }
    return err;
}


fds_uint32_t AmQoSCtrl::waitForWorkers() {
    return 1;
}

/*
 * Removes volume from the map, returns error if volume does not exist
 */
Error
AmQoSCtrl::removeVolume(std::string const& volName, fds_volid_t const volId) {
    htb_dispatcher->deregisterQueue(volId.get());

    /** Drain any wait queue into as any Error */
    wait_queue->remove_if(volName,
                          [] (AmRequest* amReq) {
                              if (amReq->cb)
                                  amReq->cb->call(fpi::MISSING_RESOURCE);
                              delete amReq;
                              return true;
                          });

    // Remove the volume from the caches (if there is one)
    volTable->removeVolume(volId);

    return ERR_OK;
}

Error AmQoSCtrl::enqueueRequest(AmRequest *amReq) {
    static fpi::VolumeAccessMode const default_access_mode;
    Error err(ERR_OK);
    PerfTracer::tracePointBegin(amReq->qos_perf_ctx);

    if (invalid_vol_id == amReq->io_vol_id) {
        auto vol = volTable->getVolume(amReq->volume_name);
        if (vol) {
            amReq->setVolId(vol->voldesc->volUUID);
        }
    }
    amReq->io_req_id = nextIoReqId.fetch_add(1, std::memory_order_relaxed);

    if (invalid_vol_id == amReq->io_vol_id) {
        /**
         * If the volume id is invalid, we haven't attached to it yet just queue
         * the request, hopefully we'll get an attach.
         * TODO(bszmyd):
         * Time these out if we don't get the attach
         */
        GLOGTRACE << "Delaying request: 0x" << std::hex << amReq->io_req_id << std::dec;
        err = wait_queue->delay(amReq);

        // TODO(bszmyd): Wed 27 May 2015 09:01:43 PM MDT
        // This code is here to support the fact that not all the connectors
        // send an AttachVolume currently. For now ensure one is enqueued in
        // the wait list by queuing a no-op attach request ourselves, this
        // will cause the attach to use the default mode.
        if (ERR_VOL_NOT_FOUND == err) {
            if (FDS_ATTACH_VOL != amReq->io_type) {
                // We've delayed the real request, replace it with an attach
                amReq = new AttachVolumeReq(invalid_vol_id,
                                            amReq->volume_name,
                                            default_access_mode,
                                            nullptr);
                amReq->io_req_id = nextIoReqId.fetch_add(1, std::memory_order_relaxed);
                wait_queue->delay(amReq);
            }
            err = volTable->attachVolume(amReq->volume_name);
            if (ERR_NOT_READY == err) {
                // We don't have domain tables yet...just reject.
                removeVolume(amReq->volume_name, invalid_vol_id);
            }
        }
    } else {
        err = htb_dispatcher->enqueueIO(amReq->io_vol_id.get(), amReq);
    }
    return err;
}

bool
AmQoSCtrl::drained() {
    return (htb_dispatcher->num_outstanding_ios == 0) && wait_queue->empty();
}

Error
AmQoSCtrl::updateDlt(bool dlt_type, std::string& dlt_data, FDS_Table::callback_type const& cb) {
    return volTable->updateDlt(dlt_type, dlt_data, cb);
}

Error
AmQoSCtrl::updateDmt(bool dmt_type, std::string& dmt_data, FDS_Table::callback_type const& cb) {
    return volTable->updateDmt(dmt_type, dmt_data, cb);
}

Error
AmQoSCtrl::getDMT() {
    return volTable->getDMT();
}

Error
AmQoSCtrl::getDLT() {
    return volTable->getDLT();
}

std::vector<std::shared_ptr<AmVolume>>
AmQoSCtrl::getVolumes() const {
    return volTable->getVolumes();
}

}  // namespace fds
