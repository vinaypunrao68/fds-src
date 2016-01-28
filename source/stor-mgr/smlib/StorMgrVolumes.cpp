/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#include <map>
#include <string>
#include <StorMgr.h>
#include <odb.h>
#include <StorMgrVolumes.h>
#include <fds_process.h>

namespace fds {

extern ObjectStorMgr *objStorMgr;

StorMgrVolume::StorMgrVolume(const VolumeDesc&  vdb,
                             fds_log           *parent_log)
        : FDS_Volume(vdb) {
    /*
     * Setup storage prefix.
     * All other values are default for now
     */
    const FdsRootDir *root = g_fdsprocess->proc_fdsroot();
    root->fds_mkdir(root->dir_sys_repo_volume().c_str());

    SetLog(parent_log);
    /*
     * Create volume queue with parameters based on
     * the volume descriptor.
     * TODO: The queue capacity is still hard coded. We
     * should calculate this somehow.
     */
    if (voldesc->isSnapshot()) {
        volQueue.reset(objStorMgr->getQueue(voldesc->qosQueueId));
    }

    if (!volQueue) {
        volQueue.reset(new SmVolQueue(voldesc->isSnapshot() ? voldesc->qosQueueId
                : voldesc->GetID(), 100, voldesc->getIopsThrottle(), voldesc->getIopsAssured(),
                voldesc->getPriority()));
    }

    averageObjectsRead = 0;
    dedupBytes_ = 0;
    domainDedupBytesFrac_ = 0;
}

StorMgrVolume::~StorMgrVolume() {
}

/***** StorMgrVolumeTable methods ******/

StorMgrVolumeTable::StorMgrVolumeTable() {
    created_log = false;
    parent_sm = NULL;
}

/* creates its own logger */
StorMgrVolumeTable::StorMgrVolumeTable(ObjectStorMgr *sm, fds_log *parent_log)
        : parent_sm(sm) {
    if (parent_log) {
        SetLog(parent_log);
        created_log = false;
    } else {
        SetLog(new fds_log("sm_voltab", "logs"));
        created_log = true;
    }
}

StorMgrVolumeTable::StorMgrVolumeTable(ObjectStorMgr *sm)
        : StorMgrVolumeTable(sm, NULL) {
}

StorMgrVolumeTable::~StorMgrVolumeTable() {
    /*
     * Iterate volume_map and free the volume pointers
     */
    map_rwlock.write_lock();
    for (std::unordered_map<fds_volid_t, StorMgrVolume*>::iterator it = volume_map.begin();
         it != volume_map.end();
         ++it) {
        delete it->second;
    }
    volume_map.clear();
    map_rwlock.write_unlock();

    /*
     * Delete log if we created one
     */
    if (created_log) {
        delete logptr;
    }
}

/*
 * Creates volume if it has not been created yet
 * Does nothing if volume is already registered
 */
Error
StorMgrVolumeTable::registerVolume(const VolumeDesc& vdb) {
    Error       err(ERR_OK);
    fds_volid_t volUuid = vdb.GetID();

    map_rwlock.write_lock();
    if (volume_map.count(volUuid) == 0) {
        LOGNORMAL << "Registering new " << (vdb.isSnapshot() ? "snapshot" : "volume")
                << ": " << volUuid;
        StorMgrVolume* vol = new StorMgrVolume(vdb,
                                               GetLog());
        fds_assert(vol != NULL);
        volume_map[volUuid] = vol;
    } else {
        /*
         * TODO: Should probably compare the known volume's details with
         * the one we're being told about.
         */
        LOGNOTIFY << "Register already known volume " << volUuid;
    }
    map_rwlock.write_unlock();

    return err;
}

/*
 * TODO: another thread may delete the volume since we release the read lock,
 * so we should revisit this when we implement volume delete
 */
StorMgrVolume* StorMgrVolumeTable::getVolume(fds_volid_t vol_uuid) {
    StorMgrVolume *ret_vol = NULL;

    map_rwlock.read_lock();
    if (volume_map.count(vol_uuid) > 0) {
        ret_vol = volume_map[vol_uuid];
    } else {
        LOGERROR << "StorMgrVolumeTable::getVolume - Volume " << vol_uuid
                 << " does not exist";
    }
    map_rwlock.read_unlock();

    return ret_vol;
}

Error
StorMgrVolumeTable::deregisterVolume(fds_volid_t vol_uuid) {
    Error err(ERR_OK);
    StorMgrVolume *vol = NULL;

    map_rwlock.write_lock();
    if (volume_map.count(vol_uuid) == 0) {
        LOGERROR << "StorMgrVolumeTable - deregistering volume called for non-existing volume "
                 << vol_uuid;
        err = ERR_INVALID_ARG;
        map_rwlock.write_unlock();
        return err;
    }

    vol = volume_map[vol_uuid];
    volume_map.erase(vol_uuid);
    map_rwlock.write_unlock();
    delete vol;

    LOGNORMAL << "StorMgrVolumeTable - Removed volume " << vol_uuid;

    return err;
}

fds_uint32_t StorMgrVolumeTable::getVolAccessStats(fds_volid_t vol_uuid) {
    StorMgrVolume *vol = NULL;
    fds_uint32_t  AveNumVolObj = -1;

    map_rwlock.read_lock();
    if (volume_map.count(vol_uuid) > 0) {
        vol = volume_map[vol_uuid];
    }  else {
        LOGERROR << "STATS-VOL stats  requested on - Volume " << vol_uuid
                 << " does not exist";
        map_rwlock.read_unlock();
        return AveNumVolObj;
    }

    map_rwlock.read_unlock();
    AveNumVolObj = vol->averageObjectsRead;
    return AveNumVolObj;
}

void StorMgrVolumeTable::updateDupObj(fds_volid_t volid,
                                      const ObjectID &objId,
                                      fds_uint32_t obj_size,
                                      fds_bool_t incr,
                                      std::map<fds_volid_t, fds_uint64_t>& vol_refcnt) {
    fds_uint64_t total_refcnt = 0;
    double dedup_bytes = 0;
    if (vol_refcnt.size() == 0) {
        // ignore this
        return;
    }
    if (parent_sm && !parent_sm->amIPrimary(objId)) return;

    if (incr && (vol_refcnt.count(volid) == 0)) {
      vol_refcnt[volid] = 0;
    }
    for (std::map<fds_volid_t, fds_uint64_t>::const_iterator cit = vol_refcnt.cbegin();
         cit != vol_refcnt.cend();
         ++cit) {
        total_refcnt += cit->second;
    }
    fds_uint64_t new_total_refcnt = total_refcnt;
    if (incr) {
        ++new_total_refcnt;
    } else {
        fds_verify(new_total_refcnt > 0);
        --new_total_refcnt;
    }

    // dedupe bytes = ObjSize * (refcnt[volid] - 1)
    // domain dedupe bytes fraction = ObjSize * (total_refcnt - 1) * refcnt[volid] / total_refcnt

    for (std::map<fds_volid_t, fds_uint64_t>::const_iterator cit = vol_refcnt.cbegin();
         cit != vol_refcnt.cend();
         ++cit) {
        fds_uint64_t my_refcnt = cit->second;
        double old_dedup_bytes = 0;
        double old_domain_dedup_bytes_frac = 0;

        if (my_refcnt > 0) {
            old_dedup_bytes = obj_size * (my_refcnt - 1);
        }

        if (total_refcnt > 0) {
            old_domain_dedup_bytes_frac = obj_size * my_refcnt * (total_refcnt - 1) / total_refcnt;
        }

        /**
         * Adjust refcnt for the given volume.
         */
        if (volid == cit->first) {
            if (incr) {
                ++my_refcnt;
            } else {
                fds_verify(my_refcnt > 0);
                --my_refcnt;
            }
        }
        double new_dedup_bytes = 0;
        double new_domain_dedup_bytes_frac = 0;
        if (my_refcnt > 0) {
            new_dedup_bytes = obj_size * (my_refcnt - 1);
        }

        if (new_total_refcnt > 0) {
            new_domain_dedup_bytes_frac = obj_size * my_refcnt * (new_total_refcnt - 1) / new_total_refcnt;
        }

        map_rwlock.write_lock();
        if (volume_map.count(cit->first) > 0) {
            StorMgrVolume *vol = volume_map[cit->first];
            double before_dedup_bytes = vol->getDedupBytes();
            vol->updateDedupBytes(new_dedup_bytes - old_dedup_bytes);
            LOGTRACE << "Dedup bytes for volume " << std::hex << cit->first << std::dec
                     << " old " << before_dedup_bytes << " new " << vol->getDedupBytes();

            before_dedup_bytes = vol->getDomainDedupBytesFrac();
            vol->updateDomainDedupBytesFrac(new_domain_dedup_bytes_frac - old_domain_dedup_bytes_frac);
            LOGTRACE << "Domain dedup byte fraction for volume " << std::hex << cit->first << std::dec
                     << " old " << before_dedup_bytes << " new " << vol->getDomainDedupBytesFrac();
        }
        map_rwlock.write_unlock();
    }
}

std::pair<double, double> StorMgrVolumeTable::getDedupBytes(fds_volid_t volid) {
    double dedup_bytes = 0;
    double domain_dedup_bytes_frac = 0;
    map_rwlock.read_lock();
    if (volume_map.count(volid) > 0) {
        StorMgrVolume *vol = volume_map[volid];
        dedup_bytes = vol->getDedupBytes();
        domain_dedup_bytes_frac = vol->getDomainDedupBytesFrac();
    }
    map_rwlock.read_unlock();
    return std::pair<double, double>(dedup_bytes, domain_dedup_bytes_frac);
}

Error StorMgrVolumeTable::updateVolStats(fds_volid_t vol_uuid) {
    Error err(ERR_OK);
    StorMgrVolume *vol = NULL;
    fds_bool_t  slotChange;

    map_rwlock.write_lock();
    if (volume_map.count(vol_uuid) > 0) {
        vol = volume_map[vol_uuid];
    }  else {
        LOGERROR << "STATS-VOL - update stats request for non-existing volume "
                 << vol_uuid;
        err = ERR_INVALID_ARG;
        map_rwlock.write_unlock();
        return err;
    }

    /*
     * update the stats
     */
    // TODO(Anna) -- remember why we need this, and when we do, uncomment or move
    // to appropriate place
    /*
    vol->lastAccessTimeR =  vol->objStats.last_access_ts;
    slotChange = vol->objStats.increment(objStorMgr->objStats->startTime, COUNTER_UPDATE_SLOT_TIME);
    if (slotChange == true) {
        vol->averageObjectsRead += vol->objStats.getWeightedCount(objStorMgr->objStats->startTime, \
                                                                  COUNTER_UPDATE_SLOT_TIME);
        LOGDEBUG << "STATS-VOL: Average Objects  per Vol slot : " << vol->averageObjectsRead;
    }
    */
    volume_map[vol_uuid] = vol;

    map_rwlock.write_unlock();
    return err;
}

/**
* @brief Returns true if all volumes in inVols are flash only.  Non-existent volumes are treated
* as NOT-flash only
*
* @param inVols
*
* @return
* NOTE: It's better to have a cached list of flash only volumes to avoid the lock.
*/
bool StorMgrVolumeTable::hasFlashOnlyVolumes(const std::vector<fds_volid_t>& inVols)
{
    map_rwlock.read_lock();
    for (auto &volId : inVols) {
       auto itr = volume_map.find(volId);
       if (itr == volume_map.end()) {
           return false;
       }
       if (itr->second->voldesc->mediaPolicy != FDSP_MEDIA_POLICY_SSD) {
           return false;
       }
    }
    map_rwlock.read_unlock();
    return true;
}
}  // namespace fds
