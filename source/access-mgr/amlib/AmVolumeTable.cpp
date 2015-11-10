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
#include "PerfTrace.h"

namespace fds {

/***** AmVolumeTable methods ******/

/* creates its own logger */
AmVolumeTable::AmVolumeTable(CommonModuleProviderIf *modProvider, fds_log *parent_log) :
    volume_map(),
    txMgr(new AmTxManager(modProvider))
{
    if (parent_log) {
        SetLog(parent_log);
    }
}

AmVolumeTable::~AmVolumeTable() = default;

void AmVolumeTable::init(processor_cb_type const& complete_cb) {
    processor_cb = [this, complete_cb](AmRequest* amReq, Error const& error) mutable -> void {
        GLOGTRACE << "Completing request: 0x" << std::hex << amReq->io_req_id;
        complete_cb(amReq, error);
    };

    FdsConfigAccessor conf(g_fdsprocess->get_fds_config(), "fds.am.");
    vol_tok_renewal_freq = std::chrono::duration<fds_uint32_t>(conf.get<fds_uint32_t>("token_renewal_freq"));

    FdsConfigAccessor features(g_fdsprocess->get_fds_config(), "fds.feature_toggle.");
    auto safe_atomic_write = features.get<bool>("am.safe_atomic_write", false);
    volume_open_support = features.get<bool>("common.volume_open_support", volume_open_support);

    LOGNORMAL << "Features: safe_atomic_write(" << safe_atomic_write
              << ") volume_open_support("       << volume_open_support << ")";

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
AmVolumeTable::registerVolume(VolumeDesc const& volDesc, FDS_VolumeQueue *volq)
{
    fds_volid_t vol_uuid = volDesc.GetID();
    map_rwlock.write_lock();
    auto const& it = volume_map.find(vol_uuid);
    if (volume_map.cend() == it) {
        /** Internal bookkeeping */
        // Create the volume and add it to the known volume map
        auto new_vol = std::make_shared<AmVolume>(volDesc, volq, nullptr);
        volume_map[vol_uuid] = std::move(new_vol);
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
    if (vol && vol->volQueue)
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
AmVolumeTable::removeVolume(fds_volid_t const volId) {
    WriteGuard wg(map_rwlock);
    auto volIt = volume_map.find(volId);
    if (volume_map.end() == volIt) {
        GLOGDEBUG << "Called for non-attached volume " << volId;
        return ERR_VOL_NOT_FOUND;
    }
    auto vol = volIt->second;
    volume_map.erase(volIt);
    LOGNOTIFY << "AmVolumeTable - Removed volume " << volId;

    // If we had a token for a volume, give it back to DM
    if (vol && vol->access_token) {
        // If we had a cache token for this volume, close it
        fds_int64_t token = vol->getToken();
        if (token_timer.cancel(boost::dynamic_pointer_cast<FdsTimerTask>(vol->access_token))) {
            GLOGDEBUG << "Canceled timer for token: 0x" << std::hex << token;
        } else {
            LOGWARN << "Failed to cancel timer, volume will re-attach: "
                    << volId << " using: 0x" << std::hex << token;
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
            << " is not attached";
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
    processor_cb(amReq, ERR_VOLUME_ACCESS_DENIED);
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

Error
AmVolumeTable::attachVolume(std::string const& volume_name) {
    return txMgr->attachVolume(volume_name);
}

void
AmVolumeTable::openVolume(AmRequest *amReq) {
    // Check if we already are attached so we can have a current token
    auto volReq = static_cast<AttachVolumeReq*>(amReq);
    auto vol = getVolume(amReq->io_vol_id);
    if (!vol) {
        return processor_cb(amReq, ERR_VOL_NOT_FOUND);
    }

    if (vol->access_token)
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
    openVolume(amReq);
}

void
AmVolumeTable::attachVolumeCb(AmRequest *amReq, const Error& error) {
    auto volReq = static_cast<AttachVolumeReq*>(amReq);

    auto vol = getVolume(amReq->io_vol_id);
    if (!vol) {
        GLOGDEBUG << "Volume has been removed, dropping lease to: " << amReq->volume_name;
        return processor_cb(amReq, ERR_VOL_NOT_FOUND);
    }

    auto& vol_desc = *vol->voldesc;
    if (error.ok()) {
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

            // Create caches if we have a token
            txMgr->registerVolume(vol_desc, volReq->mode.can_cache);
        } else {
            access_token->setMode(volReq->mode);
            access_token->setToken(volReq->token);
        }

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
        volReq->volDesc = boost::make_shared<VolumeDesc>(vol_desc);
    } else {
        LOGNOTIFY << "Failed to open volume with mode: cache(" << volReq->mode.can_cache
                  << ") write(" << volReq->mode.can_write
                  << ") error(" << error << ")";
        // Flush the volume's wait queue and return errors for pending requests
        removeVolume(vol_desc.volUUID);
    }
    processor_cb(amReq, error);
}

void
AmVolumeTable::statVolume(AmRequest *amReq) {
    return txMgr->statVolume(amReq);
}

void
AmVolumeTable::setVolumeMetadata(AmRequest *amReq) {
    if (ensureWritable(amReq)) {
        return txMgr->setVolumeMetadata(amReq);
    }
}

void
AmVolumeTable::getVolumeMetadata(AmRequest *amReq) {
    return txMgr->getVolumeMetadata(amReq);
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
AmVolumeTable::putBlob(AmRequest *amReq) {
    if (ensureWritable(amReq)) {
        return txMgr->putBlob(amReq);
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
