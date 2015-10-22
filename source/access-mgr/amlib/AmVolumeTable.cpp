/*
 * Copyright 2013-2014 Formation Data Systems, Inc.
 */
#include <atomic>

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

#include "AmVolumeTable.h"
#include "AmVolume.h"

#include "AmRequest.h"
#include "requests/AttachVolumeReq.h"
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

/***** AmVolumeTable methods ******/

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
}

AmVolumeTable::~AmVolumeTable() = default;

void AmVolumeTable::registerCallback(processor_cb_type cb) {
    /** Create a closure for the QoS Controller to call when it wants to
     * process a request. I just find this cleaner than function pointers. */
    auto closure = [cb](AmRequest* amReq) mutable -> void {
        fds::PerfTracer::tracePointEnd(amReq->qos_perf_ctx);
        cb(amReq);
    };
    qos_ctrl->runScheduler(std::move(closure));
}

/*
 * Register the volume and create QoS structs 
 * Search the wait queue for an attach on this volume (by name),
 * if found process it, and only it.
 */
Error
AmVolumeTable::registerVolume(const VolumeDesc& volDesc)
{
    Error err(ERR_OK);
    fds_volid_t vol_uuid = volDesc.GetID();
    map_rwlock.write_lock();
    auto const& it = volume_map.find(vol_uuid);
    if (volume_map.cend() == it) {
        /** Create queue and register with QoS */
        FDS_VolumeQueue* queue {nullptr};
        if (volDesc.isSnapshot()) {
            LOGDEBUG << "Volume is a snapshot : [0x" << std::hex << vol_uuid << "]";
            queue = qos_ctrl->getQueue(volDesc.qosQueueId);
        }
        if (!queue) {
            queue = new FDS_VolumeQueue(4096,
                                        volDesc.iops_throttle,
                                        volDesc.iops_assured,
                                        volDesc.relativePrio);
            err = qos_ctrl->registerVolume(volDesc.volUUID, queue);
        }

        /** Internal bookkeeping */
        if (err.ok()) {
            // Create the volume and add it to the known volume map
            auto new_vol = std::make_shared<AmVolume>(volDesc, queue, nullptr);
            volume_map[vol_uuid] = std::move(new_vol);
            map_rwlock.write_unlock();

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
                                          this->enqueueRequest(amReq);
                                          found = true;
                                          return true;
                                      }
                                      return false;
                                  });

            LOGNOTIFY << "AmVolumeTable - Register new volume " << volDesc.name
                      << " " << std::hex << vol_uuid << std::dec
                      << ", policy " << volDesc.volPolicyId
                      << " (iops_throttle=" << volDesc.iops_throttle
                      << ", iops_assured=" << volDesc.iops_assured
                      << ", prio=" << volDesc.relativePrio << ")"
                      << ", primary=" << volDesc.primary
                      << ", replica=" << volDesc.replica
                      << " result: " << err.GetErrstr();
            return err;
        } else {
            LOGERROR << "Volume failed to register : [0x"
                     << std::hex << vol_uuid << "]"
                     << " because: " << err;
        }
    }
    map_rwlock.write_unlock();
    return err;
}

/*
 * Binds access token to volume and drains any pending IO
 */
Error
AmVolumeTable::processAttach(const VolumeDesc& volDesc,
                             boost::shared_ptr<AmVolumeAccessToken> access_token)
{
    auto vol = getVolume(volDesc.GetID());
    if (!vol) {
        return ERR_VOL_NOT_FOUND;
    }

    // Assign the volume the token we got from DM
    vol->access_token.swap(access_token);

    /** Drain wait queue into QoS */
    wait_queue->remove_if(volDesc.name,
                          [this, vol_uuid = volDesc.GetID()] (AmRequest* amReq) {
                              amReq->setVolId(vol_uuid);
                              this->enqueueRequest(amReq);
                              return true;
                          });
    return ERR_OK;
}

Error AmVolumeTable::modifyVolumePolicy(fds_volid_t vol_uuid,
                                        const VolumeDesc& vdesc)
{
    Error err(ERR_OK);

    auto vol = getVolume(vol_uuid);
    if (vol && vol->volQueue)
    {
        /* update volume descriptor */
        (vol->voldesc)->modifyPolicyInfo(vdesc.iops_assured,
                                         vdesc.iops_throttle,
                                         vdesc.relativePrio);

        /* notify appropriate qos queue about the change in qos params*/
        err = qos_ctrl->modifyVolumeQosParams(vol_uuid,
                                              vdesc.iops_assured,
                                              vdesc.iops_throttle,
                                              vdesc.relativePrio);
        LOGNOTIFY << "AmVolumeTable - modify policy info for volume "
            << vdesc.name
            << " (iops_assured=" << vdesc.iops_assured
            << ", iops_throttle=" << vdesc.iops_throttle
            << ", prio=" << vdesc.relativePrio << ")"
            << " RESULT " << err.GetErrstr();
    }

    // NOTE(bszmyd): Thu 14 May 2015 06:53:13 AM MDT
    // Return ERR_OK even if we weren't using the volume. We may
    // want to re-investigate all AMs getting Mod messages for all
    // volumes, though I think that event will be relatively rare.
    return err;
}

/*
 * Removes volume from the map, returns error if volume does not exist
 */
AmVolumeTable::volume_ptr_type
AmVolumeTable::removeVolume(std::string const& volName, fds_volid_t const volId)
{
    WriteGuard wg(map_rwlock);
    /** Drain any wait queue into as any Error */
    wait_queue->remove_if(volName,
                          [] (AmRequest* amReq) {
                              if (amReq->cb)
                                  amReq->cb->call(fpi::MISSING_RESOURCE);
                              delete amReq;
                              return true;
                          });
    auto volIt = volume_map.find(volId);
    if (volume_map.end() == volIt) {
        LOGDEBUG << "Called for non-attached volume " << volId;
        return nullptr;
    }
    auto vol = volIt->second;
    volume_map.erase(volIt);
    LOGNOTIFY << "AmVolumeTable - Removed volume " << volId;
    qos_ctrl->deregisterVolume(volId);
    return vol;
}

/*
 * Returns SH volume object or NULL if the volume has not been created
 */
AmVolumeTable::volume_ptr_type AmVolumeTable::getVolume(fds_volid_t const vol_uuid) const
{
    ReadGuard rg(map_rwlock);
    volume_ptr_type ret_vol;
    auto map = volume_map.find(vol_uuid);
    if (volume_map.end() != map) {
        ret_vol = map->second;
    } else {
        LOGDEBUG << "AmVolumeTable::getVolume - Volume "
            << std::hex << vol_uuid << std::dec
            << " does not exist";
    }
    return ret_vol;
}

std::vector<AmVolumeTable::volume_ptr_type>
AmVolumeTable::getVolumes() const
{
    std::vector<volume_ptr_type> volumes;
    ReadGuard rg(map_rwlock);
    volumes.reserve(volume_map.size());

    // Create a vector of volume pointers from the values in our map
    for (auto const& kv : volume_map) {
      volumes.push_back(kv.second);
    }
    return volumes;
}

fds_uint32_t
AmVolumeTable::getVolMaxObjSize(fds_volid_t const volUuid) const {
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
    // Lookup volume by name if need be
    auto vol_id = amReq->io_vol_id;
    if (invalid_vol_id == vol_id) {
        vol_id = getVolumeUUID(amReq->volume_name);
        if (invalid_vol_id != vol_id) {
            // Set the volume and initialize some of the perf contexts
            amReq->setVolId(vol_id);
        }
    }
    auto volume = getVolume(vol_id);

    // If we don't have the volume attached, we'll need to delay the request
    // until it is
    if (!volume) {
        /**
         * If the volume id is invalid, we haven't attached to it yet just queue
         * the request, hopefully we'll get an attach.
         * TODO(bszmyd):
         * Time these out if we don't get the attach
         */
        GLOGDEBUG << "Delaying request: " << amReq;
        return wait_queue->delay(amReq);
    }

    PerfTracer::tracePointBegin(amReq->qos_perf_ctx);
    return qos_ctrl->enqueueIO(amReq->io_vol_id, amReq);
}

Error
AmVolumeTable::markIODone(AmRequest* amReq) {
  // If this request didn't go through QoS, then just return
  return (0 == amReq->dispatch_ts) ? ERR_OK : qos_ctrl->markIODone(amReq);
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
