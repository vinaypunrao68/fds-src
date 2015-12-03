/*
 * Copyright 2013-2014 Formation Data Systems, Inc.
 */
#include "AmVolumeTable.h"

#include "AmVolume.h"
#include "AmVolumeAccessToken.h"

#include "AmQoSCtrl.h"
#include "requests/AttachVolumeReq.h"
#include "requests/DetachVolumeReq.h"
#include "requests/StatVolumeReq.h"
#include "PerfTrace.h"
#include "fds_timer.h"

namespace fds {

/*
 * Atomic for request ids.
 */
std::atomic_uint nextIoReqId;


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
    template<typename Cb>
    void remove_if(std::string const& vol_name, Cb&& cb);
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

void WaitQueue::resume_if(std::string const& vol_name) {
    remove_if(vol_name,
              [this, vol_name] (AmRequest* req) mutable -> bool {
                  if (vol_name == req->volume_name) {
                      AmDataProvider::unknownTypeResume(req);
                      return true;
                  }
                  return false;
              });
}

void WaitQueue::cancel_if(std::string const& vol_name, Error const error) {
    remove_if(vol_name,
              [this, vol_name, error] (AmRequest* req) mutable -> bool {
                  if (vol_name == req->volume_name) {
                      _next_in_chain->unknownTypeCb(req, error);
                      return true;
                  }
                  return false;
              });
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
                             CommonModuleProviderIf *modProvider,
                             fds_log *parent_log) :
    AmDataProvider(prev, new AmQoSCtrl(this, max_thrds, modProvider, parent_log)),
    HasModuleProvider(modProvider),
    volume_map(),
    read_queue(new WaitQueue(this)),
    write_queue(new WaitQueue(this))
{
    token_timer = MODULEPROVIDER()->getTimer();
    if (parent_log) {
        SetLog(parent_log);
    }
}

AmVolumeTable::~AmVolumeTable() = default;

void AmVolumeTable::start() {
    FdsConfigAccessor conf(MODULEPROVIDER()->get_fds_config(), "fds.am.");
    vol_tok_renewal_freq = std::chrono::duration<fds_uint32_t>(conf.get<fds_uint32_t>("token_renewal_freq"));
    renewal_task = boost::shared_ptr<FdsTimerTask>(new RenewLeaseTask (this, *token_timer));
    token_timer->schedule(renewal_task, vol_tok_renewal_freq);

    AmDataProvider::start();
}

bool
AmVolumeTable::done() {
    if (!read_queue->empty() || !write_queue->empty()) {
        return false;
    }
    return AmDataProvider::done();
}

void
AmVolumeTable::stop() {
    {
        // Close all open Volumes
        WriteGuard wg(map_rwlock);
        token_timer->cancel(renewal_task);

        for (auto volIt = volume_map.begin(); volume_map.end() != volIt; ) {
            auto vol = volIt->second;
            // Close the volume in case it was open
            if (vol->access_token) {
                auto volReq = new DetachVolumeReq(vol->voldesc->volUUID, vol->voldesc->name, nullptr);
                volReq->token = vol->getToken();
                AmDataProvider::closeVolume(volReq);
            }
            AmDataProvider::removeVolume(*vol->voldesc);
            volIt = volume_map.erase(volIt);
        }
    }
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
        if (volume_map.cend() == it) {
            /** Internal bookkeeping */
            // Create the volume and add it to the known volume map
            auto new_vol = std::make_shared<AmVolume>(volDesc, nullptr);
            volume_map[vol_uuid] = std::move(new_vol);

            LOGNOTIFY << "AmVolumeTable - Register new volume " << volDesc.name
                      << " " << std::hex << vol_uuid << std::dec
                      << ", policy " << volDesc.volPolicyId
                      << " (iops_throttle=" << volDesc.iops_throttle
                      << ", iops_assured=" << volDesc.iops_assured
                      << ", prio=" << volDesc.relativePrio << ")"
                      << ", primary=" << volDesc.primary
                      << ", replica=" << volDesc.replica;
        }
    }
    AmDataProvider::registerVolume(volDesc);
}

Error AmVolumeTable::modifyVolumePolicy(const VolumeDesc& vdesc) {
    auto vol = getVolume(vdesc.volUUID);
    if (vol)
    {
        /* update volume descriptor */
        (vol->voldesc)->modifyPolicyInfo(vdesc.iops_assured,
                                         vdesc.iops_throttle,
                                         vdesc.relativePrio);

        LOGNOTIFY << "AmVolumeTable - modify policy info for volume "
            << vdesc.name
            << " (iops_assured=" << vdesc.iops_assured
            << ", iops_throttle=" << vdesc.iops_throttle
            << ", prio=" << vdesc.relativePrio << ")";
        return AmDataProvider::modifyVolumePolicy(vdesc);
    }

    return ERR_VOL_NOT_FOUND;
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
            GLOGDEBUG << "Called for non-attached volume " << volDesc.volUUID;
            return;
        }
        auto vol = volIt->second;
        // Close the volume in case it was open
        if (vol->access_token) {
            auto volReq = new DetachVolumeReq(volDesc.volUUID, volDesc.name, nullptr);
            volReq->token = vol->getToken();
            AmDataProvider::closeVolume(volReq);
        }
        volume_map.erase(volIt);
    }
    LOGNOTIFY << "AmVolumeTable - Removed volume " << volDesc.volUUID;

    // Remove the volume from the caches (if there is one)
    AmDataProvider::removeVolume(volDesc);
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
        GLOGTRACE << "AmVolumeTable::getVolume - Volume "
                  << std::hex << vol_uuid << std::dec
                  << " is not attached";
    }
    return ret_vol;
}

AmVolumeTable::volume_ptr_type
AmVolumeTable::ensureReadable(AmRequest* amReq,
                              bool const otherwise_queue) {
    auto vol = getVolume(amReq->volume_name);
    if (vol) {
        amReq->io_req_id = nextIoReqId.fetch_add(1, std::memory_order_relaxed);
        amReq->setVolId(vol->voldesc->GetID());
        amReq->object_size = vol->voldesc->maxObjSizeInBytes;
        // Set the "can use cache" flag
        if (!vol->getMode().second){
            amReq->forced_unit_access = true;
            amReq->page_out_cache = false;
        }
        return vol;
    }
    // We do not know about this volume, delay it and try and look up the
    // required VolDesc to continue the operation.
    if (otherwise_queue &&
        ERR_VOL_NOT_FOUND == read_queue->delay(amReq)) {
        GLOGTRACE << "Looking up volume: " << amReq->volume_name;
        AmDataProvider::lookupVolume(amReq->volume_name);
    }
    return nullptr;
}

AmVolumeTable::volume_ptr_type
AmVolumeTable::ensureWritable(AmRequest* amReq,
                              bool const otherwise_queue) {
    static fpi::VolumeAccessMode const default_access_mode;
    auto vol = ensureReadable(amReq, otherwise_queue);
    if (vol) {
       if (vol->getMode().first) {
           return vol;
       }

       // Implicitly attach and delay till open response
       if (otherwise_queue &&
           ERR_VOL_NOT_FOUND == write_queue->delay(amReq)) {
           GLOGTRACE << "Trying to update an unleased volume, implicit open.";
           auto volReq = new AttachVolumeReq(amReq->io_vol_id,
                                             amReq->volume_name,
                                             default_access_mode,
                                             nullptr);
           AmDataProvider::openVolume(volReq);
       }
    }
    return nullptr;
}

/**
 * returns volume UUID if found in volume map, otherwise returns invalid_vol_id
 */
AmVolumeTable::volume_ptr_type
AmVolumeTable::getVolume(const std::string& vol_name) const {
    ReadGuard rg(map_rwlock);
    /* we need to iterate to check names of volumes (or we could keep vol name -> uuid
     * map, but we would need to synchronize it with volume_map, etc, can revisit this later) */
    for (auto& it: volume_map) {
        if (vol_name.compare((it.second)->voldesc->name) == 0) {
            return it.second;
        }
    }
    return nullptr;
}

void
AmVolumeTable::getVolumes(std::vector<VolumeDesc>& volumes) {
    ReadGuard rg(map_rwlock);
    volumes.clear();
    for (auto const& it: volume_map) {
        volumes.emplace_back(*it.second->voldesc);
    }
}

void
AmVolumeTable::openVolume(AmRequest *amReq) {
    auto vol = ensureReadable(amReq);
    if (!vol) {
        return;
    }

    // Check if we already are attached so we can have a current token
    auto volReq = static_cast<AttachVolumeReq*>(amReq);

    volReq->volDesc = boost::make_shared<VolumeDesc>(*vol->voldesc);
    if (vol->access_token) {
        auto cb = SHARED_DYN_CAST(AttachCallback, amReq->cb);
        cb->mode = boost::make_shared<fpi::VolumeAccessMode>(vol->access_token->getMode());
        cb->volDesc = volReq->volDesc;
        return AmDataProvider::openVolumeCb(amReq, ERR_OK);
    }

    /**
     * FEATURE TOGGLE: Single AM Enforcement
     * Wed 01 Apr 2015 01:52:55 PM PDT
     */
    GLOGTRACE << "Dispatching open volume with mode: cache(" << volReq->mode.can_cache
              << ") write(" << volReq->mode.can_write << ")";
    AmDataProvider::openVolume(amReq);
}

void
AmVolumeTable::openVolumeCb(AmRequest *amReq, const Error error) {
    auto volReq = static_cast<AttachVolumeReq*>(amReq);
    if (volReq->renewal) {
        return renewTokenCb(amReq, error);
    }

    auto vol = getVolume(amReq->io_vol_id);
    if (!vol) {
        GLOGDEBUG << "Volume has been removed, dropping lease to: " << amReq->volume_name;
        return AmDataProvider::openVolumeCb(amReq, ERR_VOLUME_ACCESS_DENIED);
    }

    auto err = error;
    if (err.ok()) {
        LOGDEBUG << "Leased volume: " << amReq->volume_name;
    } else {
        LOGNOTIFY << "Failed to lease volume: " << amReq->volume_name
                  << " error(" << err << ") access is R/O.";
        err = ERR_OK;
        volReq->mode.can_cache = false;
        volReq->mode.can_write = false;
        volReq->token = invalid_vol_token;
    }

    auto access_token = boost::make_shared<AmVolumeAccessToken>(volReq->mode, volReq->token);

    // Assign the volume the token we got from DM
    vol->access_token = access_token;

    if (volReq->mode.can_write) {
        write_queue->resume_if(amReq->volume_name);
    } else {
        write_queue->cancel_if(amReq->volume_name, ERR_VOLUME_ACCESS_DENIED);
    }

    // If this is a real request, set the return data (could be implicit// from QoS)
    if (amReq->cb) {
        auto cb = SHARED_DYN_CAST(AttachCallback, amReq->cb);
        cb->volDesc = boost::make_shared<VolumeDesc>(*vol->voldesc);
        cb->mode = boost::make_shared<fpi::VolumeAccessMode>(volReq->mode);
    }
    AmDataProvider::openVolumeCb(amReq, err);
}

void
AmVolumeTable::closeVolume(AmRequest *amReq) {
    auto vol = ensureReadable(amReq, false);

    // If we didn't have the volume open, fine...
    if (!vol || !vol->access_token) {
        return AmDataProvider::closeVolumeCb(amReq, ERR_OK);
    }

    auto volReq = static_cast<DetachVolumeReq*>(amReq);
    {
        WriteGuard wg(map_rwlock);
        volReq->token = vol->getToken();
        vol->access_token.reset();
    }

    // No need to dispatch if we did not have a WRITE token
    if (invalid_vol_token == volReq->token) {
        return AmDataProvider::closeVolumeCb(amReq, ERR_OK);
    }
    AmDataProvider::closeVolume(volReq);
}

void
AmVolumeTable::renewTokens() {
    // For each of our volumes that we currently have a lease on, let's renew
    // it so we can ensure mutually exclusivity
    WriteGuard wg(map_rwlock);
    for (auto& vol_pair : volume_map) {
        if (vol_pair.second->access_token) {
            fpi::VolumeAccessMode rw;
            auto volReq = new AttachVolumeReq(vol_pair.first, "", rw, nullptr);
            volReq->token = vol_pair.second->getToken();
            volReq->renewal = true;
            AmDataProvider::openVolume(volReq);
        }
    }
    token_timer->schedule(renewal_task, vol_tok_renewal_freq);
}

void
AmVolumeTable::renewTokenCb(AmRequest *amReq, const Error& error) {
    auto volReq = static_cast<AttachVolumeReq*>(amReq);

    auto vol = getVolume(volReq->io_vol_id);
    if (vol) {
        auto& access_token = vol->access_token;
        if (error.ok()) {
            LOGTRACE << "Lease for volume: " << vol->voldesc->name << ", was renewed.";
            access_token->setMode(volReq->mode);
            access_token->setToken(volReq->token);
        } else {
            LOGWARN << "Failed to renew volume lease for: " << vol->voldesc->name;
            // Fallback to no cache and read/only access until we have a good
            // token again...keep renewal thread going.
            fpi::VolumeAccessMode ro;
            ro.can_cache = false;
            ro.can_write = false;
            access_token->setMode(ro);
            access_token->setToken(invalid_vol_token);
        }
    }

    delete volReq;
}

void
AmVolumeTable::statVolumeCb(AmRequest* amReq, Error const error) {
    if (error.ok()) {
        StatVolumeCallback::ptr cb =
            SHARED_DYN_CAST(StatVolumeCallback, amReq->cb);
        auto volReq = static_cast<StatVolumeReq *>(amReq);
        cb->current_usage_bytes = volReq->size;
        cb->blob_count = volReq->blob_count;
    }
    AmDataProvider::statVolumeCb(amReq, error);
}

void
AmVolumeTable::statVolume(AmRequest *amReq) {
    if (ensureReadable(amReq)) {
        AmDataProvider::statVolume(amReq);
    }
}

void
AmVolumeTable::getVolumeMetadata(AmRequest *amReq) {
    if (ensureReadable(amReq)) {
        AmDataProvider::getVolumeMetadata(amReq);
    }
}

void
AmVolumeTable::setVolumeMetadata(AmRequest *amReq) {
    if (ensureWritable(amReq)) {
        AmDataProvider::setVolumeMetadata(amReq);
    }
}

void
AmVolumeTable::volumeContents(AmRequest *amReq) {
    if (ensureReadable(amReq)) {
        AmDataProvider::volumeContents(amReq);
    }
}

void
AmVolumeTable::startBlobTx(AmRequest *amReq) {
    if (ensureWritable(amReq)) {
        AmDataProvider::startBlobTx(amReq);
    }
}

void
AmVolumeTable::commitBlobTx(AmRequest *amReq) {
    if (ensureWritable(amReq)) {
        AmDataProvider::commitBlobTx(amReq);
    }
}

void
AmVolumeTable::statBlob(AmRequest *amReq) {
    if (ensureReadable(amReq)) {
        AmDataProvider::statBlob(amReq);
    }
}

void
AmVolumeTable::setBlobMetadata(AmRequest *amReq) {
    if (ensureWritable(amReq)) {
        AmDataProvider::setBlobMetadata(amReq);
    }
}

void
AmVolumeTable::deleteBlob(AmRequest *amReq) {
    if (ensureWritable(amReq)) {
        AmDataProvider::deleteBlob(amReq);
    }
}

void
AmVolumeTable::renameBlob(AmRequest *amReq) {
    if (ensureWritable(amReq)) {
        AmDataProvider::renameBlob(amReq);
    }
}

void
AmVolumeTable::getBlob(AmRequest *amReq) {
    if (ensureReadable(amReq)) {
        AmDataProvider::getBlob(amReq);
    }
}

void
AmVolumeTable::putBlob(AmRequest *amReq) {
    if (ensureWritable(amReq)) {
        AmDataProvider::putBlob(amReq);
    }
}

void
AmVolumeTable::putBlobOnce(AmRequest *amReq) {
    if (ensureWritable(amReq)) {
        AmDataProvider::putBlobOnce(amReq);
    }
}

void
AmVolumeTable::lookupVolume(std::string const volume_name) {
    auto vol = getVolume(volume_name);
    if (vol) {
        return AmDataProvider::lookupVolumeCb(*vol->voldesc, ERR_OK);
    }
    AmDataProvider::lookupVolume(volume_name);
}

void
AmVolumeTable::lookupVolumeCb(VolumeDesc const vol_desc, Error const error) {
    if (ERR_OK == error) {
        registerVolume(vol_desc);
        read_queue->resume_if(vol_desc.name);
    } else {
        // Drain any wait queue into as any Error
        read_queue->cancel_if(vol_desc.name, ERR_VOL_NOT_FOUND);
    }
}


}  // namespace fds
