/*
 * Copyright 2013-2014 Formation Data Systems, Inc.
 */
#include <atomic>

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

#include "access-mgr/am-block.h"

#include "StorHvisorNet.h"
#include "StorHvVolumes.h"
#include "StorHvQosCtrl.h"

#include "PerfTrace.h"

extern StorHvCtrl *storHvisor;

namespace fds {

StorHvVolume::StorHvVolume(const VolumeDesc& vdesc, StorHvCtrl *sh_ctrl, fds_log *parent_log)
        : FDS_Volume(vdesc), parent_sh(sh_ctrl), volQueue(0)
{
    if (vdesc.isSnapshot()) {
        volQueue = parent_sh->qos_ctrl->getQueue(vdesc.qosQueueId);
    }

    if (!volQueue) {
        volQueue = new FDS_VolumeQueue(4096, vdesc.iops_max, vdesc.iops_min, vdesc.relativePrio);
    }
    volQueue->activate();

    is_valid = true;
}

StorHvVolume::~StorHvVolume() {
    parent_sh->qos_ctrl->deregisterVolume(voldesc->volUUID);
    delete volQueue; volQueue = nullptr;
}

/* safely destroys journal table and volume catalog cache
 * after this, object is not valid */
void StorHvVolume::destroy() {
    rwlock.write_lock();
    if (!is_valid) {
        rwlock.write_unlock();
        return;
    }

    /* destroy data */
    delete volQueue; volQueue = nullptr;
    is_valid = false;
    rwlock.write_unlock();
}

bool StorHvVolume::isValidLocked() const
{
    return is_valid;
}

void StorHvVolume::readLock()
{
    rwlock.read_lock();
}

void StorHvVolume::readUnlock()
{
    rwlock.read_unlock();
}

StorHvVolumeLock::StorHvVolumeLock(StorHvVolume *vol)
{
    shvol = vol;
    if (shvol) shvol->readLock();
}

StorHvVolumeLock::~StorHvVolumeLock()
{
    if (shvol) shvol->readUnlock();
}

/***** StorHvVolumeTable methods ******/

std::atomic<unsigned int> next_io_req_id;

const fds_volid_t StorHvVolumeTable::fds_default_vol_uuid;

/* creates its own logger */
StorHvVolumeTable::StorHvVolumeTable(StorHvCtrl *sh_ctrl, fds_log *parent_log)
        : parent_sh(sh_ctrl)
{
    if (parent_log) {
        SetLog(parent_log);
    }

    next_io_req_id = ATOMIC_VAR_INIT(0);

    /* setup 'admin' queue that will hold requests to OM
     * such as 'get bucket stats' and 'get bucket' */
    {
        VolumeDesc admin_vdesc("admin_vol", admin_vol_id);
        admin_vdesc.iops_min = 10;
        admin_vdesc.iops_max = 500;
        admin_vdesc.relativePrio = 9;
        admin_vdesc.capacity = 0; /* not really a volume, using volume struct to hold admin requests  */
        StorHvVolume *admin_vol = new StorHvVolume(admin_vdesc, parent_sh, GetLog());
        if (admin_vol) {
            volume_map[admin_vol_id] = admin_vol;
            sh_ctrl->qos_ctrl->registerVolume(admin_vol_id, admin_vol->volQueue);
            LOGNOTIFY << "StorHvVolumeTable -- constructor registered admin volume";
        } else {
            LOGERROR << "StorHvVolumeTable -- failed to allocate admin volume struct";
        }
    }

    if (sh_ctrl->GetRunTimeMode() == StorHvCtrl::TEST_BOTH) {
        VolumeDesc vdesc("default_vol", fds_default_vol_uuid);
        vdesc.iops_min = 200;
        vdesc.iops_max = 500;
        vdesc.relativePrio = 8;
        vdesc.capacity = 10*1024;
        StorHvVolume *vol =
                volume_map[fds_default_vol_uuid] = new StorHvVolume(vdesc, parent_sh, GetLog());
        sh_ctrl->qos_ctrl->registerVolume(vdesc.volUUID, vol->volQueue);
        LOGNORMAL << "StorHvVolumeTable - constructor registered volume 1";
    }
}

StorHvVolumeTable::StorHvVolumeTable(StorHvCtrl *sh_ctrl)
        : StorHvVolumeTable(sh_ctrl, NULL) {
}

StorHvVolumeTable::~StorHvVolumeTable()
{
    /*
     * Iterate volume_map and free the volume pointers
     */

    map_rwlock.write_lock();
    for (std::unordered_map<fds_volid_t, StorHvVolume*>::iterator it = volume_map.begin();
         it != volume_map.end();
         ++it)
    {
        StorHvVolume *vol = it->second;
        vol->destroy();
        delete vol;
    }
    volume_map.clear();
    map_rwlock.write_unlock();
}


/*
 * Creates volume if it has not been created yet
 * Does nothing if volume is already registered
 */
Error StorHvVolumeTable::registerVolume(const VolumeDesc& vdesc)
{
    Error err(ERR_OK);
    fds_volid_t vol_uuid = vdesc.GetID();

    map_rwlock.write_lock();
    if (volume_map.count(vol_uuid) == 0) {
        StorHvVolume *new_vol = new StorHvVolume(vdesc, parent_sh, GetLog());
        if (new_vol) {
            // Register this volume queue with the QOS ctrl
            err = parent_sh->qos_ctrl->registerVolume(vdesc.volUUID, new_vol->volQueue);
            if (err.ok()) {
                volume_map[vol_uuid] = new_vol;
            } else {
                new_vol->destroy();
                delete new_vol;
            }
        } else {
            err = ERR_INVALID_ARG; // TODO: need more error types
        }
    }
    map_rwlock.write_unlock();

    LOGNOTIFY << "StorHvVolumeTable - Register new volume " << vdesc.name << " "
              << std::hex << vol_uuid << std::dec << ", policy " << vdesc.volPolicyId
              << " (iops_min=" << vdesc.iops_min << ", iops_max="
              << vdesc.iops_max <<", prio=" << vdesc.relativePrio << ")"
              << " result: " << err.GetErrstr();

    /* check if any blobs are waiting for volume to be registered, and if so,
     * move them to appropriate qos queue  */
    if (err.ok()) {
        moveWaitBlobsToQosQueue(vol_uuid, vdesc.name, err);
        if (vdesc.volType == fpi::FDSP_VOL_BLKDEV_TYPE) {
            blk_vol_creat_t vreq;

            vreq.v_name  = vdesc.name.c_str();
            vreq.v_dev   = NULL;
            vreq.v_uuid  = vdesc.volUUID;
            vreq.v_blksz = 4 << 10; /* TODO(Vy): 4K for now. */

            /* The volume capacity is in MB. */
            vreq.v_blkdev[0] = '\0';
            vreq.v_vol_blksz =
                static_cast<fds_uint64_t>(vdesc.capacity / vreq.v_blksz) << 10;

            BlockMod *mod = BlockMod::blk_singleton();
            if (mod->blk_attach_vol(&vreq) == 0) {
                fds_assert(vreq.v_blkdev[0] != '\0');
            }
            LOGNOTIFY << "Create block vol " << vdesc.name << ", dev " << vreq.v_blkdev;
        }
    }
    return err;
}

Error StorHvVolumeTable::modifyVolumePolicy(fds_volid_t vol_uuid,
                                            const VolumeDesc& vdesc)
{
    Error err(ERR_OK);
    StorHvVolume *vol = getLockedVolume(vol_uuid);
    if (vol && vol->volQueue)
    {
        /* update volume descriptor */
        (vol->voldesc)->modifyPolicyInfo(vdesc.iops_min,
                                         vdesc.iops_max,
                                         vdesc.relativePrio);

        /* notify appropriate qos queue about the change in qos params*/
        err = storHvisor->qos_ctrl->modifyVolumeQosParams(vol_uuid,
                                                          vdesc.iops_min,
                                                          vdesc.iops_max,
                                                          vdesc.relativePrio);
    }
    else {
        err = Error(ERR_NOT_FOUND);
    }
    vol->readUnlock();

    LOGNOTIFY << "StorHvVolumeTable - modify policy info for volume "
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
Error StorHvVolumeTable::removeVolume(fds_volid_t vol_uuid)
{
    Error err(ERR_OK);
    StorHvVolume *vol = NULL;

    map_rwlock.write_lock();
    if (volume_map.count(vol_uuid) == 0) {
        LOGWARN << "StorHvVolumeTable - removeVolume called for non-existing volume "
                << vol_uuid;
        err = ERR_INVALID_ARG;
        map_rwlock.write_unlock();
        return err;
    }

    vol = volume_map[vol_uuid];
    volume_map.erase(vol_uuid);
    map_rwlock.write_unlock();

    vol->destroy();
    delete vol;

    LOGNOTIFY << "StorHvVolumeTable - Removed volume "
              << vol_uuid;

    return err;
}

/*
 * Returns SH volume object or NULL if the volume has not been created
 * Not thread-safe, so use StorHvVolumeLock to lock the volume and check
 * if volume object is still valid via StorHvVolume::isValidLocked()
 * before using the volume object
 */
StorHvVolume* StorHvVolumeTable::getVolume(fds_volid_t vol_uuid)
{
    StorHvVolume *ret_vol = NULL;

    map_rwlock.read_lock();
    if (volume_map.count(vol_uuid) > 0) {
        ret_vol = volume_map[vol_uuid];
    } else {
        LOGWARN << "StorHvVolumeTable::getVolume - Volume "
                << std::hex << vol_uuid << std::dec
                << " does not exist";
    }
    map_rwlock.read_unlock();

    return ret_vol;
}

/*
 * Returns the locked volume object. Guarantees that returned volume
 * object is valid (i.e., can safely access journal table and volume catalog cache)
 * Call StorHvVolume::readUnlock() when finished using volume object
 */
StorHvVolume* StorHvVolumeTable::getLockedVolume(fds_volid_t vol_uuid)
{
    StorHvVolume *ret_vol = getVolume(vol_uuid);

    if (ret_vol) {
        ret_vol->readLock();
        if (!ret_vol->isValidLocked())
        {
            ret_vol->readUnlock();
            LOGERROR << "StorHvVolumeTable::getLockedVolume - Volume " << vol_uuid
                     << "does not exist anymore, must have been destroyed";
            return NULL;
        }
    }

    return ret_vol;
}

/* returns true if volume is registered and is in volume_map, otherwise returns false */
fds_bool_t StorHvVolumeTable::volumeExists(const std::string& vol_name)
{
    return (getVolumeUUID(vol_name) != invalid_vol_id);
}

StorHvVolVec
StorHvVolumeTable::getVolumeIds() {
    StorHvVolVec volIds;
    map_rwlock.read_lock();
    for (std::unordered_map<fds_volid_t, StorHvVolume*>::const_iterator it = volume_map.cbegin();
         it != volume_map.cend();
         ++it) {
        volIds.push_back(it->first);
    }
    map_rwlock.read_unlock();

    return volIds;
}

fds_uint32_t
StorHvVolumeTable::getVolMaxObjSize(fds_volid_t volUuid) {
    map_rwlock.read_lock();
    fds_verify(volume_map.count(volUuid) > 0);
    fds_uint32_t maxObjSize = volume_map[volUuid]->voldesc->maxObjSizeInBytes;
    map_rwlock.read_unlock();
    return maxObjSize;
}

/**
 * returns volume UUID if found in volume map, otherwise returns invalid_vol_id
 */
fds_volid_t
StorHvVolumeTable::getVolumeUUID(const std::string& vol_name) {
    fds_volid_t ret_id = invalid_vol_id;
    map_rwlock.read_lock();
    /* we need to iterate to check names of volumes (or we could keep vol name -> uuid
     * map, but we would need to synchronize it with volume_map, etc, can revisit this later) */
    for (std::unordered_map<fds_volid_t, StorHvVolume*>::iterator it = volume_map.begin();
         it != volume_map.end();
         ++it)
    {
        if (vol_name.compare((it->second)->voldesc->name) == 0) {
            /* we found the volume, however if we found it second time, not good  */
            fds_verify(ret_id == invalid_vol_id);
            ret_id = it->first;
        }
    }
    map_rwlock.read_unlock();
    return ret_id;
}

fds_volid_t StorHvVolumeTable::getBaseVolumeId(fds_volid_t vol_uuid)
{
    read_synchronized(map_rwlock) {
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
StorHvVolumeTable::getVolumeName(fds_volid_t volId) {
    fds_verify(volId != invalid_vol_id);
    std::string foundVolName;

    map_rwlock.read_lock();
    // we need to iterate to check ids of volumes (or we could keep vol name -> uuid
    // map, but we would need to synchronize it with volume_map, etc, can revisit this later)
    for (std::unordered_map<fds_volid_t, StorHvVolume*>::iterator it = volume_map.begin();
         it != volume_map.end();
         ++it) {
        if ((it->second)->voldesc->volUUID == volId) {
            fds_verify((it->second)->voldesc->name.empty() == false);
            foundVolName = (it->second)->voldesc->name;
            break;
        }
    }
    map_rwlock.read_unlock();
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
        fds::StorHvVolume *shvol = getLockedVolume(vol_uuid);
        if (( shvol == NULL) || (shvol->volQueue == NULL)) {
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
        if (shvol) {
            shvol->readUnlock();
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
    map_rwlock.read_lock();
    for (std::unordered_map<fds_volid_t, StorHvVolume*>::iterator it = volume_map.begin();
         it != volume_map.end();
         ++it)
    {
        LOGNORMAL << "StorHvVolumeTable - existing volume"
                  << "UUID " << it->second->voldesc->volUUID
                  << ", name " << it->second->voldesc->name
                  << ", type " << it->second->voldesc->volType;
    }
    map_rwlock.read_unlock();
}

} // namespace fds
