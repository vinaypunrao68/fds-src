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
#include "StorHvQosCtrl.h"
#include "StorHvCtrl.h"

namespace fds {

/***** AmVolumeTable methods ******/

constexpr fds_volid_t AmVolumeTable::fds_default_vol_uuid;

/* creates its own logger */
AmVolumeTable::AmVolumeTable(fds_log *parent_log) {
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
        auto admin_vol = std::make_shared<AmVolume>(admin_vdesc, GetLog());
        if (admin_vol) {
            storHvisor->qos_ctrl->registerVolume(admin_vol_id, admin_vol->volQueue.get());
            volume_map[admin_vol_id] = std::move(admin_vol);
            LOGNOTIFY << "AmVolumeTable -- constructor registered admin volume";
        } else {
            LOGERROR << "AmVolumeTable -- failed to allocate admin volume struct";
        }
    }

    if (storHvisor->GetRunTimeMode() == StorHvCtrl::TEST_BOTH) {
        VolumeDesc vdesc("default_vol", fds_default_vol_uuid);
        vdesc.iops_min = 200;
        vdesc.iops_max = 500;
        vdesc.relativePrio = 8;
        vdesc.capacity = 10*1024;
        auto vol = std::make_shared<AmVolume>(vdesc, GetLog());
        storHvisor->qos_ctrl->registerVolume(vdesc.volUUID, vol->volQueue.get());
        volume_map[fds_default_vol_uuid] = std::move(vol);
        LOGNORMAL << "AmVolumeTable - constructor registered volume 1";
    }
}

/*
 * Creates volume if it has not been created yet
 * Does nothing if volume is already registered
 */
Error AmVolumeTable::registerVolume(const VolumeDesc& vdesc)
{
    Error err(ERR_OK);
    fds_volid_t vol_uuid = vdesc.GetID();

    {
        WriteGuard wg(map_rwlock);
        if (volume_map.count(vol_uuid) == 0) {
            auto new_vol = std::make_shared<AmVolume>(vdesc, GetLog());
            if (new_vol) {
                // Register this volume queue with the QOS ctrl
                err = storHvisor->qos_ctrl->registerVolume(vdesc.volUUID, new_vol->volQueue.get());
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

    LOGNOTIFY << "AmVolumeTable - Register new volume " << vdesc.name << " "
        << std::hex << vol_uuid << std::dec << ", policy " << vdesc.volPolicyId
        << " (iops_min=" << vdesc.iops_min << ", iops_max="
        << vdesc.iops_max <<", prio=" << vdesc.relativePrio << ")"
        << " result: " << err.GetErrstr();

    /* check if any blobs are waiting for volume to be registered, and if so,
     * move them to appropriate qos queue  */
    if (err.ok()) {
        storHvisor->moveWaitBlobsToQosQueue(vol_uuid, vdesc.name, err);
    }
    return err;
}

Error AmVolumeTable::modifyVolumePolicy(fds_volid_t vol_uuid,
                                            const VolumeDesc& vdesc)
{
    Error err(ERR_OK);
    auto vol = getVolume(vol_uuid);
    if (vol && vol->volQueue.get())
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
Error AmVolumeTable::removeVolume(fds_volid_t vol_uuid)
{
    WriteGuard wg(map_rwlock);
    if (0 == volume_map.erase(vol_uuid)) {
        LOGWARN << "Called for non-existing volume " << vol_uuid;
        return ERR_INVALID_ARG;
    }
    LOGNOTIFY << "AmVolumeTable - Removed volume " << vol_uuid;
    return ERR_OK;
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
            /* we found the volume, however if we found it second time, not good  */
            fds_verify(ret_id == invalid_vol_id);
            ret_id = it.first;
        }
    }
    return ret_id;
}

}  // namespace fds
