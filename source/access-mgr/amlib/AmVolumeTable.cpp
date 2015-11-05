/*
 * Copyright 2013-2014 Formation Data Systems, Inc.
 */
#include <atomic>

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

#include "AmVolumeTable.h"
#include "AmVolume.h"
#include "AmVolumeAccessToken.h"

#include "AmTxManager.h"
#include "AmRequest.h"
#include "requests/AttachVolumeReq.h"
#include "PerfTrace.h"
#include "AmQoSCtrl.h"

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

/***** AmVolumeTable methods ******/

/* creates its own logger */
AmVolumeTable::AmVolumeTable(CommonModuleProviderIf *modProvider, fds_log *parent_log) :
    volume_map(),
    wait_queue(new WaitQueue()),
    qos_ctrl(nullptr),
    txMgr(new AmTxManager(modProvider))
{
    if (parent_log) {
        SetLog(parent_log);
    }
}

AmVolumeTable::~AmVolumeTable() = default;

void AmVolumeTable::init(processor_en_type enqueue_cb,
                         processor_cb_type complete_cb) {
    /** Create a closure for the QoS Controller to call when it wants to
     * process a request. I just find this cleaner than function pointers. */
    auto process_cb = [enqueue_cb](AmRequest* amReq) mutable -> void {
        fds::PerfTracer::tracePointEnd(amReq->qos_perf_ctx);
        enqueue_cb(amReq);
    };

    processor_cb = [this, complete_cb](AmRequest* amReq, Error const& error) mutable -> void {
        GLOGTRACE << "Completing request: 0x" << std::hex << amReq->io_req_id;
        // We only tell QoS about commands it actually saw
        if (0 != amReq->enqueue_ts && qos_ctrl->markIODone(amReq)) {
            complete_cb(amReq, error);
        }
    };

    FdsConfigAccessor conf(g_fdsprocess->get_fds_config(), "fds.am.");
    auto qos_threads = conf.get<int>("qos_threads");

    FdsConfigAccessor features(g_fdsprocess->get_fds_config(), "fds.feature_toggle.");
    vol_tok_renewal_freq = std::chrono::duration<fds_uint32_t>(conf.get<fds_uint32_t>("token_renewal_freq"));
    auto safe_atomic_write = features.get<bool>("am.safe_atomic_write", false);
    volume_open_support = features.get<bool>("common.volume_open_support", volume_open_support);

    LOGNORMAL << "Features: safe_atomic_write(" << safe_atomic_write
              << ") volume_open_support("       << volume_open_support << ")";

    qos_ctrl.reset(new AmQoSCtrl(qos_threads,
                                 fds::FDS_QoSControl::FDS_DISPATCH_HIER_TOKEN_BUCKET,
                                 GetLog()));
    qos_ctrl->runScheduler(std::move(process_cb));

    txMgr->init(safe_atomic_write, processor_cb);
}

void
AmVolumeTable::stop() {
    // Stop all timers, we're not going to attach anymore
    token_timer.destroy();
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
            GLOGDEBUG << "Volume is a snapshot : [0x" << std::hex << vol_uuid << "]";
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
                                          openVolume(amReq);
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
Error
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
        GLOGDEBUG << "Called for non-attached volume " << volId;
        return ERR_VOL_NOT_FOUND;
    }
    auto vol = volIt->second;
    volume_map.erase(volIt);
    LOGNOTIFY << "AmVolumeTable - Removed volume " << volId;
    qos_ctrl->deregisterVolume(volId);

    // If we had a token for a volume, give it back to DM
    if (vol && vol->access_token) {
        // If we had a cache token for this volume, close it
        fds_int64_t token = vol->getToken();
        if (token_timer.cancel(boost::dynamic_pointer_cast<FdsTimerTask>(vol->access_token))) {
            GLOGDEBUG << "Canceled timer for token: 0x" << std::hex << token;
        } else {
            LOGWARN << "Failed to cancel timer, volume will re-attach: "
                    << volName << " using: 0x" << std::hex << token;
        }
        txMgr->closeVolume(volId, token);
    }

    // Remove the volume from the caches (if there is one)
    txMgr->removeVolume(volId);

    return ERR_OK;
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
        GLOGDEBUG << "AmVolumeTable::getVolume - Volume "
            << std::hex << vol_uuid << std::dec
            << " does not exist";
    }
    return ret_vol;
}

bool
AmVolumeTable::ensureReadable(AmRequest* amReq) const {
    auto vol = getVolume(amReq->io_vol_id);
    if (vol) {
        amReq->object_size = vol->voldesc->maxObjSizeInBytes;
        /**
         * FEATURE TOGGLE: Single AM Enforcement
         * Wed 01 Apr 2015 01:52:55 PM PDT
         */
        if (!volume_open_support || (invalid_vol_token != vol->getToken())) {
            if (vol->getMode().second) {
                return true;
            }
        }
    }
    return false;
}

bool
AmVolumeTable::ensureWritable(AmRequest* amReq) const {
    auto vol = getVolume(amReq->io_vol_id);
    if (vol && !vol->voldesc->isSnapshot()) {
        amReq->object_size = vol->voldesc->maxObjSizeInBytes;
        /**
         * FEATURE TOGGLE: Single AM Enforcement
         * Wed 01 Apr 2015 01:52:55 PM PDT
         */
        if (!volume_open_support || (invalid_vol_token != vol->getToken())) {
            if (vol->getMode().first) {
                return true;
            }
        } else if (txMgr->getNoNetwork()) {
            // This is for testing purposes only
            return true;
        }
    }
    processor_cb(amReq, ERR_VOLUME_ACCESS_DENIED);
    return false;
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
    static fpi::VolumeAccessMode const default_access_mode;

    amReq->io_req_id = nextIoReqId.fetch_add(1, std::memory_order_relaxed);

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

    /** Queue and dispatch an attachment if we didn't find a volume */
    Error err {ERR_OK};
    if (!volume) {
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
            err = txMgr->attachVolume(amReq->volume_name);
            if (ERR_NOT_READY == err) {
                // We don't have domain tables yet...just reject.
                removeVolume(amReq->volume_name, invalid_vol_id);
            }
        }
    }

    GLOGTRACE << "Queuing request: 0x" << std::hex << amReq->io_req_id << std::dec;
    err = qos_ctrl->enqueueIO(amReq);

    return err;
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

Error
AmVolumeTable::attachVolume(std::string const& volume_name) {
    return txMgr->attachVolume(volume_name);
}

void
AmVolumeTable::openVolume(AmRequest *amReq) {
    // Check if we already are attached so we can have a current token
    auto volReq = static_cast<AttachVolumeReq*>(amReq);
    auto vol = getVolume(amReq->io_vol_id);
    if (vol && vol->access_token)
    {
        token_timer.cancel(boost::dynamic_pointer_cast<FdsTimerTask>(vol->access_token));
        volReq->token = vol->getToken();
    }
    amReq->proc_cb = [this, amReq] (Error const& error) mutable -> void {
        attachVolumeCb(amReq, error);
    };

    /**
     * FEATURE TOGGLE: Single AM Enforcement
     * Wed 01 Apr 2015 01:52:55 PM PDT
     */
    GLOGDEBUG << "Dispatching open volume with mode: cache(" << volReq->mode.can_cache
              << ") write(" << volReq->mode.can_write << ")";
    return txMgr->openVolume(amReq);
}

void
AmVolumeTable::renewToken(const fds_volid_t vol_id) {
    // Get the current volume and token
    auto vol = getVolume(vol_id);
    if (!vol) {
        GLOGDEBUG << "Ignoring token renewal for unknown (detached?) volume: " << vol_id;
        return;
    }

    // Dispatch for a renewal to DM, update the token on success. Remove the
    // volume otherwise.
    auto amReq = new AttachVolumeReq(vol_id, "", vol->access_token->getMode(), nullptr);
    amReq->token = vol->getToken();
    openVolume(amReq);
}

void
AmVolumeTable::attachVolumeCb(AmRequest *amReq, const Error& error) {
    auto volReq = static_cast<AttachVolumeReq*>(amReq);
    Error err {error};

    auto vol = getVolume(amReq->io_vol_id);

    auto& vol_desc = *vol->voldesc;
    if (err.ok()) {
        GLOGDEBUG << "For volume: " << vol_desc.volUUID
                  << ", received access token: 0x" << std::hex << volReq->token;


        // If this is a new token, create a access token for the volume
        auto access_token = vol->access_token;
        if (!access_token) {
            access_token = boost::make_shared<AmVolumeAccessToken>(
                token_timer,
                volReq->mode,
                volReq->token,
                [this, vol_id = vol_desc.volUUID] () mutable -> void {
                    this->renewToken(vol_id);
                });

            // Assign the volume the token we got from DM
            vol->access_token = access_token;

            /** Drain wait queue into QoS */
            wait_queue->remove_if(vol_desc.name,
                                  [this, vol_uuid = vol_desc.GetID()] (AmRequest* amReq) {
                                      amReq->setVolId(vol_uuid);
                                      GLOGTRACE << "Resuming request: 0x" << std::hex << amReq->io_req_id;
                                      auto err = qos_ctrl->enqueueIO(amReq);
                                      if (ERR_OK != err) {
                                          processor_cb(amReq, err);
                                      }
                                      return true;
                                  });

            // Create caches if we have a token
            txMgr->registerVolume(vol_desc, volReq->mode.can_cache);
        } else {
            access_token->setMode(volReq->mode);
            access_token->setToken(volReq->token);
        }

        if (err.ok()) {
            // Renew this token at a regular interval
            auto timer_task = boost::dynamic_pointer_cast<FdsTimerTask>(access_token);
            if (!token_timer.schedule(timer_task, vol_tok_renewal_freq))
                { LOGWARN << "Failed to schedule token renewal timer!"; }

            // If this is a real request, set the return data
            if (amReq->cb) {
                auto cb = SHARED_DYN_CAST(AttachCallback, amReq->cb);
                cb->volDesc = boost::make_shared<VolumeDesc>(vol_desc);
                cb->mode = boost::make_shared<fpi::VolumeAccessMode>(volReq->mode);
            }
        }
    }

    if (ERR_OK != err) {
        LOGNOTIFY << "Failed to open volume with mode: cache(" << volReq->mode.can_cache
                  << ") write(" << volReq->mode.can_write
                  << ") error(" << err << ")";
        // Flush the volume's wait queue and return errors for pending requests
        removeVolume(vol_desc);
    }
    processor_cb(amReq, err);
}

void
AmVolumeTable::statVolume(AmRequest *amReq) {
    if (ensureReadable(amReq)) {
        return txMgr->statVolume(amReq);
    }
}

void
AmVolumeTable::setVolumeMetadata(AmRequest *amReq) {
    if (ensureWritable(amReq)) {
        return txMgr->setVolumeMetadata(amReq);
    }
}

void
AmVolumeTable::getVolumeMetadata(AmRequest *amReq) {
    if (ensureReadable(amReq)) {
        return txMgr->getVolumeMetadata(amReq);
    }
}

void
AmVolumeTable::volumeContents(AmRequest *amReq) {
    if (ensureReadable(amReq)) {
        return txMgr->volumeContents(amReq);
    }
}

void
AmVolumeTable::startBlobTx(AmRequest *amReq) {
    if (ensureWritable(amReq)) {
        return txMgr->startBlobTx(amReq);
    }
}

void
AmVolumeTable::commitBlobTx(AmRequest *amReq) {
    if (ensureWritable(amReq)) {
        return txMgr->commitBlobTx(amReq);
    }
}

void
AmVolumeTable::abortBlobTx(AmRequest *amReq) {
    return txMgr->abortBlobTx(amReq);
}

void
AmVolumeTable::statBlob(AmRequest *amReq) {
    if (ensureReadable(amReq)) {
        return txMgr->statBlob(amReq);
    }
}

void
AmVolumeTable::setBlobMetadata(AmRequest *amReq) {
    if (ensureWritable(amReq)) {
        return txMgr->setBlobMetadata(amReq);
    }
}

void
AmVolumeTable::deleteBlob(AmRequest *amReq) {
    if (ensureWritable(amReq)) {
        return txMgr->deleteBlob(amReq);
    }
}

void
AmVolumeTable::renameBlob(AmRequest *amReq) {
    if (ensureWritable(amReq)) {
        return txMgr->renameBlob(amReq);
    }
}

void
AmVolumeTable::getBlob(AmRequest *amReq) {
    if (ensureReadable(amReq)) {
        return txMgr->getBlob(amReq);
    }
}

void
AmVolumeTable::updateCatalog(AmRequest *amReq) {
    if (ensureWritable(amReq)) {
        try {
        return txMgr->updateCatalog(amReq);
        } catch (std::exception& e) {
            fds_panic("Here");
        }
    }
}

Error 
AmVolumeTable::updateDlt(bool dlt_type, std::string& dlt_data, FDS_Table::callback_type const& cb) {
    return txMgr->updateDlt(dlt_type, dlt_data, cb);
}

Error 
AmVolumeTable::updateDmt(bool dmt_type, std::string& dmt_data, FDS_Table::callback_type const& cb) {
    return txMgr->updateDmt(dmt_type, dmt_data, cb);
}

Error 
AmVolumeTable::getDMT() {
    return txMgr->getDMT();
}

Error 
AmVolumeTable::getDLT() {
    return txMgr->getDLT();
}


}  // namespace fds
