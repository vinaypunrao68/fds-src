/*
 * Copyright 2013-2014 Formation Data Systems, Inc.
 */
#include "AmVolumeTable.h"

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

#include "AmVolume.h"
#include "AmVolumeAccessToken.h"

#include "AmTxManager.h"
#include "requests/AttachVolumeReq.h"
#include "requests/StatVolumeReq.h"
#include "PerfTrace.h"

namespace fds {

/***** AmVolumeTable methods ******/

/* creates its own logger */
AmVolumeTable::AmVolumeTable(AmDataProvider* prev,
                             CommonModuleProviderIf *modProvider,
                             fds_log *parent_log) :
    volume_map(),
    AmDataProvider(prev, new AmTxManager(this, modProvider))
{
    if (parent_log) {
        SetLog(parent_log);
    }
}

AmVolumeTable::~AmVolumeTable() = default;

void AmVolumeTable::start() {
    FdsConfigAccessor conf(g_fdsprocess->get_fds_config(), "fds.am.");
    vol_tok_renewal_freq = std::chrono::duration<fds_uint32_t>(conf.get<fds_uint32_t>("token_renewal_freq"));

    AmDataProvider::start();
}

void
AmVolumeTable::stop() {
    // Stop all timers, we're not going to attach anymore
    token_timer.destroy();
    AmDataProvider::stop();
}

/*
 * Register the volume and create QoS structs 
 * Search the wait queue for an attach on this volume (by name),
 * if found process it, and only it.
 */
Error
AmVolumeTable::registerVolume(VolumeDesc const& volDesc)
{
    fds_volid_t vol_uuid = volDesc.GetID();
    map_rwlock.write_lock();
    auto const& it = volume_map.find(vol_uuid);
    if (volume_map.cend() == it) {
        /** Internal bookkeeping */
        // Create the volume and add it to the known volume map
        auto new_vol = std::make_shared<AmVolume>(volDesc, nullptr);
        volume_map[vol_uuid] = std::move(new_vol);
        // Create caches
        AmDataProvider::registerVolume(volDesc);
        map_rwlock.write_unlock();

        LOGNOTIFY << "AmVolumeTable - Register new volume " << volDesc.name
                  << " " << std::hex << vol_uuid << std::dec
                  << ", policy " << volDesc.volPolicyId
                  << " (iops_throttle=" << volDesc.iops_throttle
                  << ", iops_assured=" << volDesc.iops_assured
                  << ", prio=" << volDesc.relativePrio << ")"
                  << ", primary=" << volDesc.primary
                  << ", replica=" << volDesc.replica;
    } else {
        map_rwlock.write_unlock();
    }
    return ERR_OK;
}

Error AmVolumeTable::modifyVolumePolicy(fds_volid_t vol_uuid, const VolumeDesc& vdesc) {
    auto vol = getVolume(vol_uuid);
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
        return ERR_OK;
    }

    return ERR_VOL_NOT_FOUND;
}

/*
 * Removes volume from the map, returns error if volume does not exist
 */
Error
AmVolumeTable::removeVolume(VolumeDesc const& volDesc) {
    WriteGuard wg(map_rwlock);

    auto volIt = volume_map.find(volDesc.volUUID);
    if (volume_map.end() == volIt) {
        GLOGDEBUG << "Called for non-attached volume " << volDesc.volUUID;
        return ERR_VOL_NOT_FOUND;
    }
    auto vol = volIt->second;

    volume_map.erase(volIt);
    LOGNOTIFY << "AmVolumeTable - Removed volume " << volDesc.volUUID;

    // If we had a token for a volume, give it back to DM
    if (vol && vol->access_token) {
        // If we had a cache token for this volume, close it
        fds_int64_t token = vol->getToken();
        if (token_timer.cancel(boost::dynamic_pointer_cast<FdsTimerTask>(vol->access_token))) {
            GLOGDEBUG << "Canceled timer for token: 0x" << std::hex << token;
        } else {
            LOGWARN << "Failed to cancel timer, volume will re-attach: "
                    << volDesc.volUUID << " using: 0x" << std::hex << token;
        }
        AmDataProvider::closeVolume(volDesc.volUUID, token);
    }

    // Remove the volume from the caches (if there is one)
    return AmDataProvider::removeVolume(volDesc);
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
            << " is not attached";
    }
    return ret_vol;
}

bool
AmVolumeTable::ensureReadable(AmRequest* amReq) const {
    auto vol = getVolume(amReq->io_vol_id);
    if (vol) {
        amReq->object_size = vol->voldesc->maxObjSizeInBytes;
        // Set the "can use cache" flag
        if (!vol->getMode().second){
            amReq->forced_unit_access = true;
            amReq->page_out_cache = false;
        }
        return true;
    }
    return false;
}

bool
AmVolumeTable::ensureWritable(AmRequest* amReq) const {
    auto vol = getVolume(amReq->io_vol_id);
    if (vol && !vol->voldesc->isSnapshot() && vol->getMode().first) {
        amReq->object_size = vol->voldesc->maxObjSizeInBytes;
        return true;
    }
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
AmVolumeTable::openVolume(AmRequest *amReq) {
    // Check if we already are attached so we can have a current token
    auto volReq = static_cast<AttachVolumeReq*>(amReq);
    auto vol = getVolume(amReq->io_vol_id);
    if (!vol) {
        AmDataProvider::openVolumeCb(amReq, ERR_VOLUME_ACCESS_DENIED);
    }

    volReq->volDesc = boost::make_shared<VolumeDesc>(*vol->voldesc);
    if (vol->access_token) {
        auto cb = SHARED_DYN_CAST(AttachCallback, amReq->cb);
        cb->mode = boost::make_shared<fpi::VolumeAccessMode>(vol->access_token->getMode());
        cb->volDesc = volReq->volDesc;
        AmDataProvider::openVolumeCb(amReq, ERR_OK);
    }

    /**
     * FEATURE TOGGLE: Single AM Enforcement
     * Wed 01 Apr 2015 01:52:55 PM PDT
     */
    GLOGDEBUG << "Dispatching open volume with mode: cache(" << volReq->mode.can_cache
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
        AmDataProvider::openVolumeCb(amReq, ERR_VOLUME_ACCESS_DENIED);
    }

    if (error.ok()) {
        GLOGDEBUG << "For volume: " << amReq->io_vol_id
                  << ", received access token: 0x" << std::hex << volReq->token;
    } else {
        LOGNOTIFY << "Failed to open volume with mode: cache(" << volReq->mode.can_cache
                  << ") write(" << volReq->mode.can_write
                  << ") error(" << error << ") access is R/O.";
        volReq->mode.can_cache = false;
        volReq->mode.can_write = false;
        volReq->token = invalid_vol_token;
    }

    auto access_token = boost::make_shared<AmVolumeAccessToken>(
                            token_timer,
                            volReq->mode,
                            volReq->token,
                            [this, vol_id = amReq->io_vol_id] () mutable -> void {
                                this->renewToken(vol_id);
                        });

    // Assign the volume the token we got from DM
    vol->access_token = access_token;

    // Renew this token at a regular interval
    auto timer_task = boost::dynamic_pointer_cast<FdsTimerTask>(access_token);
    if (!token_timer.schedule(timer_task, vol_tok_renewal_freq))
        { LOGWARN << "Failed to schedule token renewal timer!"; }

    // If this is a real request, set the return data (could be implicit// from QoS)
    if (amReq->cb) {
        auto cb = SHARED_DYN_CAST(AttachCallback, amReq->cb);
        cb->volDesc = boost::make_shared<VolumeDesc>(*vol->voldesc);
        cb->mode = boost::make_shared<fpi::VolumeAccessMode>(volReq->mode);
    }
    AmDataProvider::openVolumeCb(amReq, error);
}

void
AmVolumeTable::renewToken(const fds_volid_t vol_id) {
    // Get the current volume and token
    auto vol = getVolume(vol_id);
    if (!vol) {
        GLOGDEBUG << "Ignoring token renewal for unknown (detached?) volume: " << vol_id;
        return;
    }

    token_timer.cancel(boost::dynamic_pointer_cast<FdsTimerTask>(vol->access_token));

    fpi::VolumeAccessMode rw;
    auto volReq = new AttachVolumeReq(vol_id, "", rw, nullptr);
    volReq->token = vol->getToken();
    volReq->renewal = true;
    // Dispatch for a renewal to DM, update the token on success. Remove the
    // volume otherwise.
    AmDataProvider::openVolume(volReq);
}

void
AmVolumeTable::renewTokenCb(AmRequest *amReq, const Error& error) {
    auto volReq = static_cast<AttachVolumeReq*>(amReq);

    auto vol = getVolume(volReq->io_vol_id);
    if (vol) {
        auto access_token = vol->access_token;
        if (error.ok()) {
            GLOGDEBUG << "For volume: " << volReq->io_vol_id
                      << ", received renew for token: 0x" << std::hex << volReq->token;
            access_token->setMode(volReq->mode);
            access_token->setToken(volReq->token);
        } else {
            LOGWARN << "Failed to renew volume lease with mode: cache(" << volReq->mode.can_cache
                    << ") write(" << volReq->mode.can_write
                    << ") error(" << error << ")";
            // Fallback to no cache and read/only access until we have a good
            // token again...keep renewal thread going.
            fpi::VolumeAccessMode ro;
            ro.can_cache = false;
            ro.can_write = false;
            access_token->setMode(ro);
            access_token->setToken(invalid_vol_token);
        }

        // Renew this token at a regular interval
        auto timer_task = boost::dynamic_pointer_cast<FdsTimerTask>(access_token);
        if (!token_timer.schedule(timer_task, vol_tok_renewal_freq))
            { LOGWARN << "Failed to schedule token renewal timer!"; }
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
AmVolumeTable::setVolumeMetadata(AmRequest *amReq) {
    if (!ensureWritable(amReq)) {
        return AmDataProvider::setVolumeMetadataCb(amReq, ERR_VOLUME_ACCESS_DENIED);
    }
    AmDataProvider::setVolumeMetadata(amReq);
}

void
AmVolumeTable::volumeContents(AmRequest *amReq) {
    if (!ensureReadable(amReq)) {
        return AmDataProvider::volumeContentsCb(amReq, ERR_VOLUME_ACCESS_DENIED);
    }
    AmDataProvider::volumeContents(amReq);
}

void
AmVolumeTable::startBlobTx(AmRequest *amReq) {
    if (!ensureWritable(amReq)) {
        return AmDataProvider::startBlobTxCb(amReq, ERR_VOLUME_ACCESS_DENIED);
    }
    AmDataProvider::startBlobTx(amReq);
}

void
AmVolumeTable::commitBlobTx(AmRequest *amReq) {
    if (!ensureWritable(amReq)) {
        return AmDataProvider::commitBlobTxCb(amReq, ERR_VOLUME_ACCESS_DENIED);
    }
    AmDataProvider::commitBlobTx(amReq);
}

void
AmVolumeTable::statBlob(AmRequest *amReq) {
    if (!ensureReadable(amReq)) {
        return AmDataProvider::statBlobCb(amReq, ERR_VOLUME_ACCESS_DENIED);
    }
    AmDataProvider::statBlob(amReq);
}

void
AmVolumeTable::setBlobMetadata(AmRequest *amReq) {
    if (!ensureWritable(amReq)) {
        return AmDataProvider::setBlobMetadataCb(amReq, ERR_VOLUME_ACCESS_DENIED);
    }
    AmDataProvider::setBlobMetadata(amReq);
}

void
AmVolumeTable::deleteBlob(AmRequest *amReq) {
    if (!ensureWritable(amReq)) {
        return AmDataProvider::deleteBlobCb(amReq, ERR_VOLUME_ACCESS_DENIED);
    }
    AmDataProvider::deleteBlob(amReq);
}

void
AmVolumeTable::renameBlob(AmRequest *amReq) {
    if (!ensureWritable(amReq)) {
        return AmDataProvider::renameBlobCb(amReq, ERR_VOLUME_ACCESS_DENIED);
    }
    AmDataProvider::renameBlob(amReq);
}

void
AmVolumeTable::getBlob(AmRequest *amReq) {
    if (!ensureReadable(amReq)) {
        return AmDataProvider::getBlobCb(amReq, ERR_VOLUME_ACCESS_DENIED);
    }
    AmDataProvider::getBlob(amReq);
}

void
AmVolumeTable::putBlob(AmRequest *amReq) {
    if (!ensureWritable(amReq)) {
        return AmDataProvider::putBlobCb(amReq, ERR_VOLUME_ACCESS_DENIED);
    }
    AmDataProvider::putBlob(amReq);
}

void
AmVolumeTable::putBlobOnce(AmRequest *amReq) {
    if (!ensureWritable(amReq)) {
        return AmDataProvider::putBlobOnceCb(amReq, ERR_VOLUME_ACCESS_DENIED);
    }
    AmDataProvider::putBlobOnce(amReq);
}

}  // namespace fds
