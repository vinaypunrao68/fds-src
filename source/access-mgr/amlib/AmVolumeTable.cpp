/*
 * Copyright 2013-2014 Formation Data Systems, Inc.
 */
#include <atomic>

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

#include "AmVolumeTable.h"
#include "AmVolume.h"

#include "AmRequest.h"
#include "PerfTrace.h"
#include "AmQoSCtrl.h"

namespace fds {

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
    void drain(std::string const&, Cb&&);
    Error delay(AmRequest*);
    bool empty() const;
    void pop(AmRequest*);
};

template<typename Cb>
void WaitQueue::drain(std::string const& vol_name, Cb&& cb) {
    auto wait_it = queue.find(vol_name);
    if (queue.end() != wait_it) {
        for (auto& req: wait_it->second) {
            cb(req);
        }
        queue.erase(wait_it);
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

void WaitQueue::pop(AmRequest* amReq) {
    std::lock_guard<std::mutex> l(wait_lock);
    auto wait_it = queue.find(amReq->volume_name);
    if (queue.end() != wait_it) {
        auto& queue = wait_it->second;
        auto queue_it = std::find(queue.begin(), queue.end(), amReq);
        if (queue.end() != queue_it)
        { queue.erase(queue_it); }
    }
}

/***** AmVolumeTable methods ******/

constexpr fds_volid_t AmVolumeTable::fds_default_vol_uuid;

/* creates its own logger */
AmVolumeTable::AmVolumeTable(size_t const qos_threads, fds_log *parent_log) :
    volume_map(),
    wait_queue(new WaitQueue()),
    qos_ctrl(new AmQoSCtrl(qos_threads,
                           fds::FDS_QoSControl::FDS_DISPATCH_HIER_TOKEN_BUCKET,
                           GetLog()))
{
    if (parent_log) {
        SetLog(parent_log);
    }

    /* setup 'admin' queue that will hold requests to OM
     * such as 'get bucket stats' and 'get bucket' */
    {
        VolumeDesc admin_vdesc("admin_vol", admin_vol_id);
        admin_vdesc.iops_min = 10;
        admin_vdesc.iops_max = 500;
        admin_vdesc.relativePrio = 9;
        admin_vdesc.capacity = 0; /* not really a volume, using volume struct to hold admin requests  */
        if (ERR_OK == registerVolume(admin_vdesc)) {
            LOGNOTIFY << "AmVolumeTable -- constructor registered admin volume";
        } else {
            LOGERROR << "AmVolumeTable -- failed to allocate admin volume struct";
        }
    }
}

AmVolumeTable::~AmVolumeTable() {
    map_rwlock.write_lock();
    volume_map.clear();
}

void AmVolumeTable::registerCallback(tx_callback_type cb) {
    /** Create a closure for the QoS Controller to call when it wants to
     * process a request. I just find this cleaner than function pointers. */
    auto closure = [cb](AmRequest* amReq) mutable -> void { cb(amReq); };
    qos_ctrl->runScheduler(std::move(closure));
}

/*
 * Creates volume if it has not been created yet
 * Does nothing if volume is already registered
 */
Error
AmVolumeTable::registerVolume(const VolumeDesc& vdesc, fds_int64_t token)
{
    Error err(ERR_OK);
    fds_volid_t vol_uuid = vdesc.GetID();

    {
        WriteGuard wg(map_rwlock);
        if (volume_map.count(vol_uuid) == 0) {
            /** Create queue and register with QoS */
            FDS_VolumeQueue* queue {nullptr};
            if (vdesc.isSnapshot()) {
                LOGDEBUG << "Volume is a snapshot : [0x" << std::hex << vol_uuid << "]";
                queue = qos_ctrl->getQueue(vdesc.qosQueueId);
            }
            if (!queue) {
                queue = new FDS_VolumeQueue(4096, vdesc.iops_max, vdesc.iops_min, vdesc.relativePrio);
                err = qos_ctrl->registerVolume(vdesc.volUUID, queue);
            }

            /** Internal bookkeeping */
            if (err.ok()) {
                auto new_vol = std::make_shared<AmVolume>(vdesc, queue, token);
                /** Drain wait queue into QoS */
                wait_queue->drain(vdesc.name,
                                  [this, vol_uuid] (AmRequest* amReq) mutable -> void {
                                      amReq->io_vol_id = vol_uuid;
                                      this->enqueueRequest(amReq);
                                  });
                volume_map[vol_uuid] = std::move(new_vol);

                LOGNOTIFY << "AmVolumeTable - Register new volume " << vdesc.name << " "
                          << std::hex << vol_uuid << std::dec << ", policy " << vdesc.volPolicyId
                          << " (iops_min=" << vdesc.iops_min << ", iops_max="
                          << vdesc.iops_max <<", prio=" << vdesc.relativePrio << ")"
                          << " result: " << err.GetErrstr();
            } else {
                LOGERROR << "Volume failed to register : [0x"
                         << std::hex << vol_uuid << "]"
                         << " because: " << err;
            }
        } else {
            LOGNOTIFY << "Volume already registered: [0x" << std::hex << vol_uuid << "]";
        }
    }

    return err;
}

Error AmVolumeTable::modifyVolumePolicy(fds_volid_t vol_uuid,
                                            const VolumeDesc& vdesc)
{
    Error err(ERR_OK);
    auto vol = getVolume(vol_uuid);
    if (vol && vol->volQueue)
    {
        /* update volume descriptor */
        (vol->voldesc)->modifyPolicyInfo(vdesc.iops_min,
                                         vdesc.iops_max,
                                         vdesc.relativePrio);

        /* notify appropriate qos queue about the change in qos params*/
        err = qos_ctrl->modifyVolumeQosParams(vol_uuid,
                                                          vdesc.iops_min,
                                                          vdesc.iops_max,
                                                          vdesc.relativePrio);
    } else {
        err = Error(ERR_NOT_FOUND);
    }

    LOGNOTIFY << "AmVolumeTable - modify policy info for volume "
        << vdesc.name
        << " (iops_min=" << vdesc.iops_min
        << ", iops_max=" << vdesc.iops_max
        << ", prio=" << vdesc.relativePrio << ")"
        << " RESULT " << err.GetErrstr();

    return err;
}

/*
 * Removes volume from the map, returns error if volume does not exist
 */
Error AmVolumeTable::removeVolume(const VolumeDesc& volDesc)
{
    WriteGuard wg(map_rwlock);
    /** Drain any wait queue into as any Error */
    wait_queue->drain(volDesc.name,
                      [] (AmRequest* amReq) {
                          amReq->cb->call(FDSN_StatusEntityDoesNotExist);
                          delete amReq;
                      });
    if (0 == volume_map.erase(volDesc.volUUID)) {
        LOGWARN << "Called for non-existing volume " << volDesc.volUUID;
        return ERR_INVALID_ARG;
    }
    LOGNOTIFY << "AmVolumeTable - Removed volume " << volDesc.volUUID;
    return qos_ctrl->deregisterVolume(volDesc.volUUID);
}

/*
 * Returns SH volume object or NULL if the volume has not been created
 */
AmVolumeTable::volume_ptr_type AmVolumeTable::getVolume(fds_volid_t vol_uuid) const
{
    ReadGuard rg(map_rwlock);
    volume_ptr_type ret_vol;
    auto map = volume_map.find(vol_uuid);
    if (volume_map.end() != map) {
        ret_vol = map->second;
    } else {
        LOGWARN << "AmVolumeTable::getVolume - Volume "
            << std::hex << vol_uuid << std::dec
            << " does not exist";
    }
    return ret_vol;
}

fds_uint32_t
AmVolumeTable::getVolMaxObjSize(fds_volid_t volUuid) const {
    ReadGuard rg(map_rwlock);
    auto it = volume_map.find(volUuid);
    fds_uint32_t maxObjSize = 0;
    if (volume_map.end() != it) {
        maxObjSize = it->second->voldesc->maxObjSizeInBytes;
    } else {
        LOGERROR << "Do not have volume: " << volUuid;
    }
    return maxObjSize;
}

/**
 * returns volume UUID if found in volume map, otherwise returns invalid_vol_id
 */
fds_volid_t
AmVolumeTable::getVolumeUUID(const std::string& vol_name) const {
    ReadGuard rg(map_rwlock);
    fds_volid_t ret_id = invalid_vol_id;
    /* we need to iterate to check names of volumes (or we could keep vol name -> uuid
     * map, but we would need to synchronize it with volume_map, etc, can revisit this later) */
    for (auto& it: volume_map) {
        if (vol_name.compare((it.second)->voldesc->name) == 0) {
            ret_id = it.first;
            break;
        }
    }
    return ret_id;
}

Error
AmVolumeTable::enqueueRequest(AmRequest* amReq) {
    /**
     * If the volume id is invalid, we haven't attached to it yet just queue
     * the request, hopefully we'll get an attach.
     * TODO(bszmyd):
     * Time these out if we don't get the attach
     */
    amReq->io_vol_id = (invalid_vol_id == amReq->io_vol_id) ?
        getVolumeUUID(amReq->volume_name) : amReq->io_vol_id;

    if (invalid_vol_id == amReq->io_vol_id) {
        ReadGuard rg(map_rwlock);
        auto it = volume_map.find(amReq->io_vol_id);
        if (volume_map.end() != it) {
            amReq->io_vol_id = it->first;
        } else {
            GLOGDEBUG << "Delaying request: " << amReq;
            return wait_queue->delay(amReq);
        }
    }
    PerfTracer::tracePointBegin(amReq->qos_perf_ctx);
    return qos_ctrl->enqueueIO(amReq->io_vol_id, amReq);
}

Error
AmVolumeTable::markIODone(AmRequest* amReq) {
    if (invalid_vol_id == amReq->io_vol_id) {
        /**
         * Must have failed to dispatch attach, expect to find this request in
         * the wait queue.
         */
        wait_queue->pop(amReq);
        return ERR_OK;
    }
    return qos_ctrl->markIODone(amReq);
}

bool
AmVolumeTable::drained() {
    return (qos_ctrl->htb_dispatcher->num_outstanding_ios == 0) && wait_queue->empty();
}

Error
AmVolumeTable::updateQoS(long int const* rate,
                         float const* throttle) {
    Error err{ERR_OK};
    if (rate) {
        err = qos_ctrl->htb_dispatcher->modifyTotalRate(*rate);
    }
    if (err.ok() && throttle) {
        qos_ctrl->htb_dispatcher->setThrottleLevel(*throttle);
    }
    return err;
}

}  // namespace fds
