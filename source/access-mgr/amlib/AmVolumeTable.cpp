/*
 * Copyright 2013-2016 Formation Data Systems, Inc.
 */
#include "AmVolumeTable.h"

#include "AmVolume.h"
#include "AmVolumeAccessToken.h"

#include "AmQoSCtrl.h"
#include "requests/AttachVolumeReq.h"
#include "requests/DetachVolumeReq.h"
#include "requests/StatVolumeReq.h"
#include "net/SvcRequestPool.h"
#include "net/SvcRequest.h"
#include "net/SvcMgr.h"
#include "PerfTrace.h"
#include "fds_timer.h"
#include "fdsp_utils.h"

namespace fds {

/*
 * Atomic for request ids.
 */
std::atomic_uint nextIoReqId {0};


/**
 * Timer task to renew our leases
 */
struct RenewLeaseTask : public FdsTimerTask {
    explicit RenewLeaseTask(AmVolumeTable* table, FdsTimer& timer) :
        FdsTimerTask(timer),
        _table(table)
    { }

    void runTimerTask() override { _table->renewTokens(); }
 private:
    AmVolumeTable* const _table;
};

/**
 * Wait queue for refuge requests
 */
class WaitQueue : public AmDataProvider {
    using volume_queue_type = std::deque<AmRequest*>;
    using wait_queue_type = std::unordered_map<std::string, volume_queue_type>;
    wait_queue_type queue;
    mutable std::mutex wait_lock;

 public:
    explicit WaitQueue(AmDataProvider* next) : AmDataProvider(nullptr, next) {}
    WaitQueue(WaitQueue const&) = delete;
    WaitQueue& operator=(WaitQueue const&) = delete;
    ~WaitQueue() override
    { _next_in_chain.release(); } // This is a hack so we don't double delete.

    void cancel_if(std::string const&, Error const error);
    void resume_if(std::string const&);
    Error delay(AmRequest*);
    bool empty() const;
 private:
    void remove_if(std::string const& vol_name, volume_queue_type& replays);
};

void WaitQueue::remove_if(std::string const& vol_name, volume_queue_type& replays) {
    std::lock_guard<std::mutex> l(wait_lock);
    auto wait_it = queue.find(vol_name);
    if (queue.end() != wait_it) {
        replays.swap(wait_it->second);
        queue.erase(wait_it);
    }
}

void WaitQueue::resume_if(std::string const& vol_name) {
    volume_queue_type replays;
    remove_if(vol_name, replays);
    for (auto request : replays) {
        AmDataProvider::unknownTypeResume(request);
    }
}

void WaitQueue::cancel_if(std::string const& vol_name, Error const error) {
    volume_queue_type replays;
    remove_if(vol_name, replays);
    for (auto request : replays) {
        _next_in_chain->unknownTypeCb(request, error);
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
AmVolumeTable::AmVolumeTable(AmDataProvider* prev,
                             size_t const max_thrds,
                             fds_log *parent_log) :
    AmDataProvider(prev, new AmQoSCtrl(this, max_thrds, parent_log)),
    volume_map(),
    lookup_queue(new WaitQueue(this)),
    open_queue(new WaitQueue(this))
{
    if (parent_log) {
        SetLog(parent_log);
    }
}

AmVolumeTable::~AmVolumeTable() = default;

void AmVolumeTable::start() {
    /**
     * FEATURE TOGGLE: "VolumeGroup" support in Dispatcher
     * Fri Jan 15 10:24:22 2016
     */
    FdsConfigAccessor ft_conf(g_fdsprocess->get_fds_config(), "fds.feature_toggle.");
    volume_grouping_support = ft_conf.get<bool>("common.enable_volumegrouping", volume_grouping_support);

    // We don't renew tokens when using VGS
    if (!volume_grouping_support) {
        FdsConfigAccessor conf(MODULEPROVIDER()->get_fds_config(), "fds.am.");
        vol_tok_renewal_freq = std::chrono::duration<fds_uint32_t>(conf.get<fds_uint32_t>("token_renewal_freq"));
        token_timer = MODULEPROVIDER()->getTimer();
        renewal_task = boost::shared_ptr<FdsTimerTask>(new RenewLeaseTask (this, *token_timer));
        token_timer->schedule(renewal_task, vol_tok_renewal_freq);
    }


    AmDataProvider::start();
}

bool
AmVolumeTable::done() {
    if (!lookup_queue->empty() || !open_queue->empty()) {
        return false;
    }
    return AmDataProvider::done();
}

void
AmVolumeTable::stop() {
    {
        // Close all open Volumes
        WriteGuard wg(map_rwlock);
        LOGDEBUG << "VolumeTable shutdown, stopping renewals";
        if (!volume_grouping_support) {
            token_timer->cancel(renewal_task);
        }
        LOGNOTIFY << "VolumeTable shutdown, closing open volumes";
        for (auto const& vol_pair : volume_map) {
            auto const& vol = vol_pair.second;
            // Close the volume in case it was open
            if (vol->isWritable()) {
                auto volReq = new DetachVolumeReq(vol->voldesc->volUUID, vol->voldesc->name, nullptr);
                volReq->token = vol->clearToken();
                continueRequest(volReq, vol, &AmDataProvider::closeVolume);
            }
        }
        volume_map.clear();
    }
    LOGNOTIFY << "VolumeTable shutdown, stopping lower layers";
    AmDataProvider::stop();
}

/*
 * Register the volume and create QoS structs 
 * Search the wait queue for an attach on this volume (by name),
 * if found process it, and only it.
 */
void
AmVolumeTable::registerVolume(VolumeDesc const& volDesc)
{
    fds_volid_t vol_uuid = volDesc.GetID();
    {
        WriteGuard wg(map_rwlock);
        auto const& it = volume_map.find(vol_uuid);
        if (volume_map.cend() != it) {
            return;
        }

        AmDataProvider::registerVolume(volDesc);
        /** Internal bookkeeping */
        // Create the volume and add it to the known volume map
        auto new_vol = std::make_shared<AmVolume>(volDesc);
        volume_map[vol_uuid] = std::move(new_vol);

        LOGNOTIFY << "vol:" << volDesc.name
                  << " volid:" << vol_uuid
                  << " policy:" << volDesc.volPolicyId
                  << " iops_throttle:" << volDesc.iops_throttle
                  << " iops_assured:" << volDesc.iops_assured
                  << " prio:" << volDesc.relativePrio
                  << " primary:" << volDesc.primary
                  << " replica:" << volDesc.replica
                  << " register new volume";
    }
}

Error AmVolumeTable::modifyVolumePolicy(const VolumeDesc& vdesc) {
    Error err {ERR_VOL_NOT_FOUND};
    WriteGuard wg(map_rwlock);
    auto vol = getVolume(vdesc.volUUID);
    if (vol)
    {
        err = AmDataProvider::modifyVolumePolicy(vdesc);
        /* update volume descriptor if everyone is happy with it */
        if (ERR_OK == err) {
            (vol->voldesc)->modifyPolicyInfo(vdesc.iops_assured,
                                             vdesc.iops_throttle,
                                             vdesc.relativePrio);
        } else if (vol->isWritable()) {
            // Uh-oh, let's close it so any new IO will require a new open
            LOGNOTIFY << "vol:" << vdesc.name << " closing due to lease transfer";
            auto volReq = new DetachVolumeReq(vdesc.volUUID, vdesc.name, nullptr);
            volReq->token = vol->clearToken();
            continueRequest(volReq, vol, &AmDataProvider::closeVolume);
        }
    }

    return err;
}

/*
 * Removes volume from the map, returns error if volume does not exist
 */
void
AmVolumeTable::removeVolume(VolumeDesc const& volDesc) {
    {
        WriteGuard wg(map_rwlock);

        auto volIt = volume_map.find(volDesc.volUUID);
        if (volume_map.end() == volIt) {
            LOGDEBUG << "volid:" << volDesc.volUUID << " non-attached volume";
            return;
        }
        auto vol = volIt->second;
        // Close the volume in case it was open
        if (vol->isWritable()) {
            auto volReq = new DetachVolumeReq(volDesc.volUUID, volDesc.name, nullptr);
            volReq->token = vol->clearToken();
            LOGNOTIFY << "vol:" << volDesc.name << " releasing lease";
            continueRequest(volReq, vol, &AmDataProvider::closeVolume);
        }
        // Remove the volume from the caches (if there is one)
        AmDataProvider::removeVolume(volDesc);
        volume_map.erase(volIt);
    }
    LOGNOTIFY << "vol:" << volDesc.volUUID << " removed volume";
}

void
AmVolumeTable::continueRequest(AmRequest* amReq,
                               volume_ptr_type const vol,
                               void (AmDataProvider::*func)(AmRequest*)) {
    amReq->setVolId(vol->voldesc->GetID());
    amReq->object_size = vol->voldesc->maxObjSizeInBytes;
    amReq->page_out_cache = vol->isCacheable();
    amReq->forced_unit_access = !vol->isCacheable();
    amReq->io_req_id = nextIoReqId.fetch_add(1, std::memory_order_relaxed);
    forward_request(func, amReq);
}

void
AmVolumeTable::read(AmRequest* amReq, void (AmDataProvider::*func)(AmRequest*)) {
    ReadGuard rg(map_rwlock);
    auto vol = getVolume(amReq->volume_name);
    if (vol) {
        // We need to open the volume in R/O mode if VG is enabled
        if (volume_grouping_support && !vol->isOpen()) {
            if (ERR_VOL_NOT_FOUND == open_queue->delay(amReq) && vol->startOpen()) {
                // Otherwise implicitly attach and delay till open response
                GLOGDEBUG << "Trying to read from unopened volume, implicit open.";
                fpi::VolumeAccessMode ro;
                ro.can_write = false; ro.can_cache = false;
                auto volReq = new AttachVolumeReq(amReq->io_vol_id,
                                                  amReq->volume_name,
                                                  ro,
                                                  nullptr);
                continueRequest(volReq, vol, &AmDataProvider::openVolume);
            }
        } else {
            continueRequest(amReq, vol, func);
        }
    } else {
        // We do not know about this volume, delay it and try and look up the
        // required VolDesc to continue the operation.
        if (ERR_VOL_NOT_FOUND == lookup_queue->delay(amReq)) {
            lookupVolume(amReq->volume_name);
        }
    }
}

void
AmVolumeTable::write(AmRequest* amReq, void (AmDataProvider::*func)(AmRequest*)) {
    // Check we're writable
    ReadGuard rg(map_rwlock);
    auto vol = getVolume(amReq->volume_name);
    if (vol) {
        if (!vol->isWritable()) {
            if (ERR_VOL_NOT_FOUND == open_queue->delay(amReq) && vol->startOpen()) {
                // Otherwise implicitly attach and delay till open response
                LOGTRACE << "vol:" << amReq->volume_name << " update un-writable volume, implicit open";
                fpi::VolumeAccessMode default_access_mode;
                auto volReq = new AttachVolumeReq(amReq->io_vol_id,
                                                  amReq->volume_name,
                                                  default_access_mode,
                                                  nullptr);
                continueRequest(volReq, vol, &AmDataProvider::openVolume);
            }
        } else {
            continueRequest(amReq, vol, func);
        }
    } else {
        // If we didn't even know about the volume, we may need to look it up still
        if (ERR_VOL_NOT_FOUND == lookup_queue->delay(amReq)) {
            lookupVolume(amReq->volume_name);
        }
    }
}

void
AmVolumeTable::openVolume(AmRequest *amReq) {
    // Check we're writable
    ReadGuard wg(map_rwlock);
    auto vol = getVolume(amReq->volume_name);
    if (vol) {
        auto volReq = static_cast<AttachVolumeReq*>(amReq);
        auto wants_write = volReq->mode.can_write;
        auto wants_cache = volReq->mode.can_cache;
        // If we already have the appropriate mode or are currently opening the
        // volume, just return OK
        if (((!wants_write || vol->isWritable()) && (!wants_cache || vol->isCacheable())) || !vol->startOpen()) {
            auto cb = std::dynamic_pointer_cast<AttachCallback>(volReq->cb);
            cb->mode = boost::make_shared<fpi::VolumeAccessMode>(volReq->mode);
            cb->volDesc = boost::make_shared<VolumeDesc>(*vol->voldesc);
            return AmDataProvider::openVolumeCb(amReq, ERR_OK);
        }

        LOGTRACE << "vol:" << amReq->volume_name
                 << " cache:" << volReq->mode.can_cache
                 << " write:" << volReq->mode.can_write
                 << " open volume";
        continueRequest(amReq, vol, &AmDataProvider::openVolume);
    } else if (ERR_VOL_NOT_FOUND == lookup_queue->delay(amReq)) {
        lookupVolume(amReq->volume_name);
    }
}

void
AmVolumeTable::openVolumeCb(AmRequest *amReq, const Error error) {
    auto volReq = static_cast<AttachVolumeReq*>(amReq);
    auto err = error;
    {
        WriteGuard wg(map_rwlock);
        auto vol = getVolume(amReq->io_vol_id);
        if (!vol) {
            LOGDEBUG << "vol:" << amReq->volume_name
                     << " volume removed, dropping open";
            open_queue->cancel_if(amReq->volume_name, ERR_VOLUME_ACCESS_DENIED);
            return AmDataProvider::openVolumeCb(amReq, ERR_VOLUME_ACCESS_DENIED);
        }

        if (err.ok()) {
            LOGDEBUG << "vol:" << amReq->volume_name
                     << " opened";
            // Assign the volume the token we got from DM
            vol->setToken(AmVolumeAccessToken(volReq->mode, volReq->token));
            if (amReq->cb) {
                auto cb = std::dynamic_pointer_cast<AttachCallback>(amReq->cb);
                cb->volDesc = boost::make_shared<VolumeDesc>(*vol->voldesc);
                cb->mode = boost::make_shared<fpi::VolumeAccessMode>(volReq->mode);
            }
        } else if (ERR_DM_VOL_NOT_ACTIVATED == err && !volReq->mode.can_write) {
            // No coordinator exists on this volume yet, retry the open with
            // write access:
            volReq->mode.can_write = true;
            volReq->mode.can_cache = true;
            continueRequest(volReq, vol, &AmDataProvider::openVolume);
            return;
        } else {
            LOGNOTIFY << "vol:" << amReq->volume_name
                      << " err:" << err
                      << " failed to open volume";
            vol->clearToken();
        }
        vol->stopOpen();
    }

    if (err.ok()) {
        open_queue->resume_if(amReq->volume_name);
    } else {
        open_queue->cancel_if(amReq->volume_name, ERR_VOLUME_ACCESS_DENIED);
    }

    // If this is a real request, set the return data (could be implicit// from QoS)
    AmDataProvider::openVolumeCb(amReq, err);
}

void
AmVolumeTable::closeVolume(AmRequest *amReq) {
    // Check we're writable
    WriteGuard wg(map_rwlock);
    auto vol = getVolume(amReq->volume_name);
    if (!vol || !vol->isWritable()) {
        return AmDataProvider::closeVolumeCb(amReq, ERR_OK);
    }

    auto volReq = static_cast<DetachVolumeReq*>(amReq);
    volReq->token = vol->clearToken();
    continueRequest(amReq, vol, &AmDataProvider::closeVolume);
}

void
AmVolumeTable::renewTokens() {
    // For each of our volumes that we currently have a lease on, let's renew
    // it so we can ensure mutually exclusivity
    ReadGuard wg(map_rwlock);
    for (auto& vol_pair : volume_map) {
        auto& vol = vol_pair.second;
        if (vol->isWritable() && vol->startOpen()) {
            fpi::VolumeAccessMode rw;
            auto volReq = new AttachVolumeReq(vol_pair.first, vol->voldesc->name, rw, nullptr);
            volReq->token = vol->getToken();
            continueRequest(volReq, vol, &AmDataProvider::openVolume);
        }
    }
    token_timer->schedule(renewal_task, vol_tok_renewal_freq);
}

void
AmVolumeTable::statVolumeCb(AmRequest* amReq, Error const error) {
    if (error.ok()) {
        auto cb = std::dynamic_pointer_cast<StatVolumeCallback>(amReq->cb);
        auto volReq = static_cast<StatVolumeReq *>(amReq);
        cb->current_usage_bytes = volReq->size;
        cb->blob_count = volReq->blob_count;
    }
    checkFailureResponse(amReq, error);
}

void
AmVolumeTable::lookupVolume(std::string const volume_name) {
    static auto const lookup_timeout = 2500ull;
    try {
        auto req = gSvcRequestPool->newEPSvcRequest(MODULEPROVIDER()->getSvcMgr()->getOmSvcUuid());
        fpi::GetVolumeDescriptorPtr msg(new fpi::GetVolumeDescriptor());
        msg->volume_name = volume_name;
        req->setPayload(FDSP_MSG_TYPEID(fpi::GetVolumeDescriptor), msg);
        req->onResponseCb(RESPONSE_MSG_HANDLER(AmVolumeTable::lookupVolumeCb, volume_name));
        req->setTimeoutMs(lookup_timeout);
        req->invoke();
        LOGDEBUG << "vol:" << volume_name
                  << " retrieving volume descriptor from OM";
    } catch(...) {
        LOGERROR << "vol:" << volume_name
                 << " unable to request volume descriptor from OM";
        VolumeDesc dummy(volume_name, invalid_vol_id);
        lookupVolumeCb(volume_name, nullptr, ERR_NOT_READY, nullptr);
    }
}

void
AmVolumeTable::lookupVolumeCb(std::string const volume_name,
                              EPSvcRequest* svcReq,
                              const Error& error,
                              boost::shared_ptr<std::string> payload)  {
    // Deserialize if all OK
    auto err = error;
    if (err.ok()) {
        // using the same structure for input and output
        auto response = deserializeFdspMsg<fpi::GetVolumeDescriptorResp>(err, payload);
        if (err.ok()) {
            LOGDEBUG << "vol:" << volume_name
                     << " found volume, dispatching queued IO";
            registerVolume(response->vol_desc);
            lookup_queue->resume_if(volume_name);
            return;
        }
    }
    // Drain any wait queue into as any Error
    LOGERROR << "vol:" << volume_name
             << " err:" << err
             << " could not find volume";
    lookup_queue->cancel_if(volume_name, err);
}

/*
 * Returns SH volume object or NULL if the volume has not been created
 */
AmVolumeTable::volume_ptr_type
AmVolumeTable::getVolume(fds_volid_t const vol_uuid) const
{
    volume_ptr_type ret_vol;
    auto map = volume_map.find(vol_uuid);
    if (volume_map.end() != map) {
        ret_vol = map->second;
    } else {
        LOGTRACE << "volid:" << vol_uuid
                 << " volume not attached";
    }
    return ret_vol;
}

/**
 * returns volume ptr if found in volume map, otherwise returns nullptr
 */
AmVolumeTable::volume_ptr_type
AmVolumeTable::getVolume(const std::string& vol_name) const {
    /* we need to iterate to check names of volumes (or we could keep vol name -> uuid
     * map, but we would need to synchronize it with volume_map, etc, can revisit this later) */
    for (auto& it: volume_map) {
        if (vol_name.compare((it.second)->voldesc->name) == 0) {
            return it.second;
        }
    }
    return nullptr;
}

/***
 * We check the error code for Volume state changes, we may need to re-open
 * the volume in some cases in order to re-establish an IO path; e.g. switchover
 * or multi-DM failures have occurred and a resync need to happen.
 */
void
AmVolumeTable::checkFailureResponse(AmRequest * amReq, Error const error) {
    if (ERR_INVALID_COORDINATOR == error ||
        ERR_VOLUMEGROUP_INVALID == error ||
        ERR_DM_VOL_NOT_ACTIVATED == error) {
        WriteGuard wg(map_rwlock);
        auto vol = getVolume(amReq->io_vol_id);
        if (nullptr != vol) {
            LOGNORMAL << "vol:" << amReq->volume_name << " closing due to:" << error;
            auto volReq = new DetachVolumeReq(amReq->io_vol_id, amReq->volume_name, nullptr);
            volReq->token = vol->clearToken();
            continueRequest(volReq, vol, &AmDataProvider::closeVolume);
        }
    }
    AmDataProvider::unknownTypeCb(amReq, error);
}

}  // namespace fds
