/*
 * Copyright 2013-2014 Formation Data Systems, Inc.
 */
#include <atomic>

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

#include "StorHvVolumes.h"

#include "AmRequest.h"
#include "PerfTrace.h"
#include "StorHvQosCtrl.h"
#include "StorHvCtrl.h"

namespace fds {

StorHvVolume::StorHvVolume(const VolumeDesc& vdesc, StorHvCtrl *sh_ctrl, fds_log *parent_log)
    : FDS_Volume(vdesc),
      parent_sh(sh_ctrl),
      volQueue(nullptr)
{
    if (vdesc.isSnapshot()) {
        volQueue.reset(parent_sh->qos_ctrl->getQueue(vdesc.qosQueueId));
    }

    if (!volQueue) {
        volQueue.reset(new FDS_VolumeQueue(4096,
                                           vdesc.iops_throttle,
                                           vdesc.iops_assured,
                                           vdesc.relativePrio));
    }
    volQueue->activate();
}

StorHvVolume::~StorHvVolume() {
    parent_sh->qos_ctrl->deregisterVolume(voldesc->volUUID);
}

/***** StorHvVolumeTable methods ******/

constexpr fds_volid_t StorHvVolumeTable::fds_default_vol_uuid;

/* creates its own logger */
StorHvVolumeTable::StorHvVolumeTable(StorHvCtrl *sh_ctrl, fds_log *parent_log)
: parent_sh(sh_ctrl)
{
    if (parent_log) {
        SetLog(parent_log);
    }

    /* setup 'admin' queue that will hold requests to OM
     * such as 'get bucket stats' and 'get bucket' */
    {
        VolumeDesc admin_vdesc("admin_vol", admin_vol_id);
        admin_vdesc.iops_assured = 10;
        admin_vdesc.iops_throttle = 500;
        admin_vdesc.relativePrio = 9;
        admin_vdesc.capacity = 0; /* not really a volume, using volume struct to hold admin requests  */
        auto admin_vol = std::make_shared<StorHvVolume>(admin_vdesc, parent_sh, GetLog());
        if (admin_vol) {
            sh_ctrl->qos_ctrl->registerVolume(admin_vol_id, admin_vol->volQueue.get());
            volume_map[admin_vol_id] = std::move(admin_vol);
            LOGNOTIFY << "StorHvVolumeTable -- constructor registered admin volume";
        } else {
            LOGERROR << "StorHvVolumeTable -- failed to allocate admin volume struct";
        }
    }

    if (sh_ctrl->GetRunTimeMode() == StorHvCtrl::TEST_BOTH) {
        VolumeDesc vdesc("default_vol", fds_default_vol_uuid);
        vdesc.iops_assured = 200;
        vdesc.iops_throttle = 500;
        vdesc.relativePrio = 8;
        vdesc.capacity = 10*1024;
        auto vol = std::make_shared<StorHvVolume>(vdesc, parent_sh, GetLog());
        sh_ctrl->qos_ctrl->registerVolume(vdesc.volUUID, vol->volQueue.get());
        volume_map[fds_default_vol_uuid] = std::move(vol);
        LOGNORMAL << "StorHvVolumeTable - constructor registered volume 1";
    }
}

/*
 * Creates volume if it has not been created yet
 * Does nothing if volume is already registered
 */
Error StorHvVolumeTable::registerVolume(const VolumeDesc& vdesc)
{
    Error err(ERR_OK);
    fds_volid_t vol_uuid = vdesc.GetID();

    {
        WriteGuard wg(map_rwlock);
        if (volume_map.count(vol_uuid) == 0) {
            auto new_vol = std::make_shared<StorHvVolume>(vdesc, parent_sh, GetLog());
            if (new_vol) {
                // Register this volume queue with the QOS ctrl
                err = parent_sh->qos_ctrl->registerVolume(vdesc.volUUID, new_vol->volQueue.get());
                if (err.ok()) {
                    volume_map[vol_uuid] = new_vol;
                } else {
                    // Discard the volume.
                    new_vol.reset();
                }
            } else {
                err = ERR_INVALID_ARG; // TODO: need more error types
            }
        }
    }

    LOGNOTIFY << "StorHvVolumeTable - Register new volume " << vdesc.name << " "
              << std::hex << vol_uuid << std::dec << ", policy " << vdesc.volPolicyId
              << " (iops_assured=" << vdesc.iops_assured << ", iops_throttle="
              << vdesc.iops_throttle <<", prio=" << vdesc.relativePrio << ")"
              << " result: " << err.GetErrstr();

    /* check if any blobs are waiting for volume to be registered, and if so,
     * move them to appropriate qos queue  */
    if (err.ok()) {
        moveWaitBlobsToQosQueue(vol_uuid, vdesc.name, err);
    }
    return err;
}

Error StorHvVolumeTable::modifyVolumePolicy(fds_volid_t vol_uuid,
                                            const VolumeDesc& vdesc)
{
    Error err(ERR_OK);
    auto vol = getVolume(vol_uuid);
    if (vol && vol->volQueue.get())
    {
        /* update volume descriptor */
        (vol->voldesc)->modifyPolicyInfo(vdesc.iops_assured,
                                         vdesc.iops_throttle,
                                         vdesc.relativePrio);

        /* notify appropriate qos queue about the change in qos params*/
        err = storHvisor->qos_ctrl->modifyVolumeQosParams(vol_uuid,
                                                          vdesc.iops_assured,
                                                          vdesc.iops_throttle,
                                                          vdesc.relativePrio);
    }
    else {
        err = Error(ERR_NOT_FOUND);
    }

    LOGNOTIFY << "StorHvVolumeTable - modify policy info for volume "
              << vdesc.name
              << " (iops_assured=" << vdesc.iops_assured
              << ", iops_throttle=" << vdesc.iops_throttle
              << ", prio=" << vdesc.relativePrio << ")"
              << " RESULT " << err.GetErrstr();

    return err;
}

/*
 * Removes volume from the map, returns error if volume does not exist
 */
Error StorHvVolumeTable::removeVolume(fds_volid_t vol_uuid)
{
    WriteGuard wg(map_rwlock);
    if (0 == volume_map.erase(vol_uuid)) {
        LOGWARN << "Called for non-existing volume " << vol_uuid;
        return ERR_INVALID_ARG;
    }
    LOGNOTIFY << "StorHvVolumeTable - Removed volume " << vol_uuid;
    return ERR_OK;
}

/*
 * Returns SH volume object or NULL if the volume has not been created
 */
StorHvVolumeTable::volume_ptr_type StorHvVolumeTable::getVolume(fds_volid_t vol_uuid)
{
    ReadGuard rg(map_rwlock);
    volume_ptr_type ret_vol;
    auto map = volume_map.find(vol_uuid);
    if (volume_map.end() != map) {
        ret_vol = map->second;
    } else {
        LOGWARN << "StorHvVolumeTable::getVolume - Volume "
            << std::hex << vol_uuid << std::dec
            << " does not exist";
    }
    return ret_vol;
}

/* returns true if volume is registered and is in volume_map, otherwise returns false */
fds_bool_t StorHvVolumeTable::volumeExists(const std::string& vol_name) const
{
    return (getVolumeUUID(vol_name) != invalid_vol_id);
}

StorHvVolVec StorHvVolumeTable::getVolumeIds() const {
    ReadGuard rg(map_rwlock);
    StorHvVolVec volIds;
    for (auto& it : volume_map) {
        volIds.push_back(it.first);
    }
    return volIds;
}

fds_uint32_t
StorHvVolumeTable::getVolMaxObjSize(fds_volid_t volUuid) const {
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
StorHvVolumeTable::getVolumeUUID(const std::string& vol_name) const {
    ReadGuard rg(map_rwlock);
    fds_volid_t ret_id = invalid_vol_id;
    /* we need to iterate to check names of volumes (or we could keep vol name -> uuid
     * map, but we would need to synchronize it with volume_map, etc, can revisit this later) */
    for (auto& it: volume_map) {
        if (vol_name.compare((it.second)->voldesc->name) == 0) {
            /* we found the volume, however if we found it second time, not good  */
            fds_verify(ret_id == invalid_vol_id);
            ret_id = it.first;
        }
    }
    return ret_id;
}

fds_volid_t StorHvVolumeTable::getBaseVolumeId(fds_volid_t vol_uuid) const
{
    {
        ReadGuard rg(map_rwlock);
        const auto iter = volume_map.find(vol_uuid);
        if (iter != volume_map.end()) {
            VolumeDesc& volDesc = *(iter->second->voldesc);
            if (volDesc.isSnapshot() || volDesc.isClone()) {
                return volDesc.getSrcVolumeId();
            }
        }
    }
    return vol_uuid;
}


/**
 * returns volume name if found in volume map, otherwise returns empy string
 */
std::string
StorHvVolumeTable::getVolumeName(fds_volid_t volId) const {
    ReadGuard rg(map_rwlock);

    fds_verify(volId != invalid_vol_id);
    std::string foundVolName;
    // we need to iterate to check ids of volumes (or we could keep vol name -> uuid
    // map, but we would need to synchronize it with volume_map, etc, can revisit this later)
    for (auto& vol: volume_map) {
        if (vol.second->voldesc->volUUID == volId) {
            fds_verify(vol.second->voldesc->name.empty() == false);
            foundVolName = vol.second->voldesc->name;
            break;
        }
    }
    return foundVolName;
}

/*
 * Add blob request to wait queue because a blob is waiting for OM
 * to attach bucjets to AM; once vol table receives vol attach event,
 * it will move all requests waiting in the queue for that bucket
 * to appropriate qos queue
 */
void StorHvVolumeTable::addBlobToWaitQueue(const std::string& bucket_name,
                                           AmRequest* blob_req)
{
    fds_verify(blob_req->magicInUse() == true);
    LOGDEBUG << "VolumeTable -- adding blob to wait queue, waiting for "
        << "bucket " << bucket_name;

    /*
     * Pack to qos req
     * TODO: We're hard coding 885 as the request ID to
     * pass to QoS. SHCtrl has a request counter but
     * we can't see if from here...Let's fix that eventually.
     */
    blob_req->io_req_id = 885;

    wait_rwlock.write_lock();
    wait_blobs_it_t it = wait_blobs.find(bucket_name);
    if (it != wait_blobs.end()) {
        /* we already have vector of waiting blobs for this bucket name */
        (it->second).push_back(blob_req);
    } else {
        /* we need to start a new vector */
        wait_blobs[bucket_name].push_back(blob_req);
    }
    wait_rwlock.write_unlock();

    // If the volume got attached in the meantime, we need
    // to move the requests ourselves.
    fds_volid_t volid = getVolumeUUID(bucket_name);
    if (volid != invalid_vol_id) {
        LOGDEBUG << " volid " << std::hex << volid << std::dec
            << " bucket " << bucket_name << " got attached";
        moveWaitBlobsToQosQueue(volid,
                                bucket_name,
                                ERR_OK);
    }
}

void StorHvVolumeTable::moveWaitBlobsToQosQueue(fds_volid_t vol_uuid,
                                                const std::string& vol_name,
                                                Error error)
{
    Error err = error;
    bucket_wait_vec_t blobs;

    wait_rwlock.read_lock();
    wait_blobs_it_t it = wait_blobs.find(vol_name);
    if (it != wait_blobs.end()) {
        /* we have a wait queue of blobs for this volume name */
        blobs.swap(it->second);
        wait_blobs.erase(it);
    }
    wait_rwlock.read_unlock();

    if (blobs.size() == 0) {
        LOGDEBUG << "VolumeTable::moveWaitBlobsToQueue -- "
            << "no blobs waiting for bucket " << vol_name;
        return; // no blobs waiting
    }

    if ( err.ok() )
    {
        auto shVol = getVolume(vol_uuid);
        if (!shVol) {
            LOGERROR <<  "VolumeTable::moveWaitBlobsToQosQueue -- "
                << "Volume and  Queueus are NOT setup :" << vol_uuid;
            err = ERR_INVALID_ARG;
        }
        if (err.ok()) {
            for (uint i = 0; i < blobs.size(); ++i) {
                LOGDEBUG << "VolumeTable - moving blob to qos queue of vol  " << std::hex
                    << vol_uuid << std::dec << " vol_name " << vol_name;
                // since we did not know the volume id before, volid for this io is set to 0
                // so set its volume id now to actual volume id
                AmRequest* req = blobs[i];
                fds_verify(req != NULL);
                req->io_vol_id = vol_uuid;

                fds::PerfTracer::tracePointBegin(req->qos_perf_ctx);
                storHvisor->qos_ctrl->enqueueIO(vol_uuid, req);
            }
            blobs.clear();
        }
    }

    if ( !err.ok()) {
        /* we haven't pushed requests to qos queue either because we already
         * got 'error' parameter with !err.ok() or we couldn't find volume
         * so complete blobs in 'blobs' vector with error (if there are any) */
        for (uint i = 0; i < blobs.size(); ++i) {
            AmRequest* blobReq = blobs[i];
            blobs[i] = nullptr;
            // Hard coding a result!
            LOGERROR << "some issue : " << err;
            LOGWARN << "Calling back with error since request is waiting"
                << " for volume " << blobReq->io_vol_id
                << " that doesn't exist";
            blobReq->cb->call(FDSN_StatusEntityDoesNotExist);
            delete blobReq;
        }
        blobs.clear();
    }
}


/* print detailed info into log */
void StorHvVolumeTable::dump()
{
    ReadGuard rg(map_rwlock);
    for (auto& it: volume_map) {
        LOGNORMAL << "StorHvVolumeTable - existing volume"
            << "UUID " << it.second->voldesc->volUUID
            << ", name " << it.second->voldesc->name
            << ", type " << it.second->voldesc->volType;
    }
}

} // namespace fds
