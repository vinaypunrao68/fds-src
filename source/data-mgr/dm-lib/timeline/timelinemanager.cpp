/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

#include <DataMgr.h>
#include <util/path.h>
#include <util/stringutils.h>
#include <unistd.h> // for unlink
#include <util/disk_utils.h>
#include <boost/filesystem.hpp>

namespace fds { namespace timeline {
#define TIMELINE_FEATURE_CHECK(...) if (!dm->features.isTimelineEnabled()) { return ERR_FEATURE_DISABLED ;}
TimelineManager::TimelineManager(fds::DataMgr* dm): dm(dm) {
    if (!dm->features.isTimelineEnabled()) {
        LOGWARN << "timeline feature is disabled";
        return;
    }
    timelineDB.reset(new TimelineDB());
    Error err = timelineDB->open(dm->getModuleProvider()->proc_fdsroot());
    if (!err.ok()) {
        LOGERROR << "unable to open timeline db";
    }
    journalMgr.reset(new JournalManager(dm));
}

boost::shared_ptr<TimelineDB> TimelineManager::getDB() {
    return timelineDB;
}

Error TimelineManager::deleteSnapshot(fds_volid_t volId, fds_volid_t snapshotId) {
    TIMELINE_FEATURE_CHECK();
    Error err = loadSnapshot(volId, snapshotId);

    //get the list of snapIds
    std::vector<fds_volid_t> vecSnaps;

    if (invalid_vol_id == snapshotId) {
        timelineDB->getSnapshotsForVolume(volId, vecSnaps);
        LOGDEBUG << "path:voldelete will delete [" << vecSnaps.size() << "] snaps of vol:" << volId;
    } else {
        vecSnaps.push_back(snapshotId);
    }

    for (const auto& snapshotId : vecSnaps) {
        LOGNOTIFY << "path:voldelete snap:" << snapshotId << " vol:" << volId << " deleting";

        // mark as deleted
        err = dm->removeVolume(snapshotId, true);
        // remove the snapshot
        err = dm->removeVolume(snapshotId, false);

        // remove info about this snapshot from timelinedb
        timelineDB->removeSnapshot(snapshotId);
    }
    return err;
}

Error TimelineManager::getSnapshotsForVolume(fds_volid_t volId, std::vector<fds_volid_t>& vecVolIds) {
    TIMELINE_FEATURE_CHECK();
    return timelineDB->getSnapshotsForVolume(volId, vecVolIds);
}

Error TimelineManager::loadSnapshot(fds_volid_t volid, fds_volid_t snapshotid) {
    TIMELINE_FEATURE_CHECK();
    // get the list of snapshots.
    std::string snapDir;
    std::vector<std::string> vecDirs;
    Error err;
    const auto root = dm->getModuleProvider()->proc_fdsroot();
    if (invalid_vol_id == snapshotid) {
        // load all snapshots
        snapDir  = dmutil::getSnapshotDir(root, volid);
        util::getSubDirectories(snapDir, vecDirs);
    } else {
        // load only a particular snapshot
        snapDir = dmutil::getVolumeDir(root, volid, snapshotid);
        if (!util::dirExists(snapDir)) {
            LOGERROR << "unable to locate snapshot [" << snapshotid << "]"
                     << " for vol:" << volid
                     << " @" << snapDir;
            return ERR_NOT_FOUND;
        }
    }

    fds_volid_t snapId;
    LOGDEBUG << "will load [" << vecDirs.size() << "]"
             << " snapshots for vol:" << volid
             << " from:" << snapDir;
    for (const auto& snap : vecDirs) {
        snapId = std::atoll(snap.c_str());
        //now add the snap
        if (dm->getVolumeDesc(snapId) != NULL) {
            LOGDEBUG << "snapshot:" << snapId << " already loaded";
            continue;
        }
        VolumeDesc *desc = new VolumeDesc(*(dm->getVolumeDesc(volid)));
        desc->fSnapshot = true;
        desc->srcVolumeId = volid;
        desc->lookupVolumeId = volid;
        desc->qosQueueId = volid;
        desc->volUUID = snapId;
        desc->contCommitlogRetention = 0;
        desc->timelineTime = 0;
        desc->setState(fpi::Active);
        desc->name = util::strformat("snaphot_%ld_%ld", volid.get(), snapId.get());

        err = dm->addVolume(desc->name, snapId, desc);
        if (!err.ok()) {
            LOGERROR << "unable to load snapshot [" << snapId << "] for vol:"<< volid
                     << " - " << err;
        }
    }
    return err;
}

Error TimelineManager::unloadSnapshot(fds_volid_t volid, fds_volid_t snapshotid){
    TIMELINE_FEATURE_CHECK();
    Error err;
    return err;
}

Error TimelineManager::createSnapshot(VolumeDesc *vdesc) {
    Error err;
    TIMELINE_FEATURE_CHECK();

    float_t dm_user_repo_pct_used = dmutil::getUsedCapacityOfUserRepo(MODULEPROVIDER()->proc_fdsroot());
    if (dm_user_repo_pct_used >= dm->dmFullnessThreshold) {
           err = ERR_DM_DISK_CAPACITY_ERROR_THRESHOLD;
            LOGERROR << "ERROR: DM user-repo already used " << dm_user_repo_pct_used
                    << "% of available storage space!"
                     <<" Not creating a new snapshot for vol: ." << vdesc->volUUID
                    << err;
            return err;
    }


    util::TimeStamp createTime = util::getTimeStampMicros();
    err = dm->timeVolCat_->copyVolume(*vdesc);
    if (err.ok()) {
        // add it to timeline
        if (dm->features.isTimelineEnabled()) {
            timelineDB->addSnapshot(vdesc->srcVolumeId, vdesc->volUUID, createTime);
        }

        // activate it
        err = dm->timeVolCat_->activateVolume(vdesc->volUUID);

        if (!err.ok()) {
            LOGWARN << "snapshot activation failed. vol:" << vdesc->srcVolumeId
                    << " snap:" << vdesc->volUUID;
        }
    }
    return err;
}

Error TimelineManager::removeVolume(fds_volid_t volId) {
    TIMELINE_FEATURE_CHECK();
    LOGNOTIFY << "path:voldelete vol:"<< volId << " removing from timeline db";
    Error err =  timelineDB->removeVolume(volId);

    const auto root = dm->getModuleProvider()->proc_fdsroot();
    std::string timelineDir = dmutil::getTimelineDir(root, volId);
    LOGNOTIFY << "path:voldelete vol:" << volId << " removing timeline journal archives @ " << timelineDir;
    boost::filesystem::remove_all(timelineDir);

    LOGNORMAL << "path:voldelete vol:" << volId << " will delete all snapshots";
    deleteSnapshot(volId);

    std::string snapshotDir = dmutil::getSnapshotDir(root, volId);
    LOGNOTIFY << "path:voldelete vol:" << volId << " removing snapshot dir @ " << snapshotDir;
    boost::filesystem::remove_all(snapshotDir);
    return err;
}

Error TimelineManager::createClone(VolumeDesc *vdesc) {
    TIMELINE_FEATURE_CHECK();
    Error err;

    auto volmeta = dm->getVolumeMeta(vdesc->srcVolumeId);
    if (!volmeta) {
        LOGERROR << "vol:" << vdesc->srcVolumeId
                 << " not found";
        return ERR_NOT_FOUND;
    }

    // find the closest snapshot to clone the base from
    fds_volid_t srcVolumeId = vdesc->srcVolumeId;

    LOGDEBUG << "Timeline time is: " << vdesc->timelineTime
             << " for clone: [" << vdesc->volUUID << "]";
    // timelineTime is in seconds
    util::TimeStamp createTime = vdesc->timelineTime * (1000*1000);

    if (createTime == 0) {
        LOGDEBUG << "clone vol:" << vdesc->volUUID
                 << " of srcvol:" << vdesc->srcVolumeId
                 << " will be a clone at current time";
        err = dm->timeVolCat_->copyVolume(*vdesc);
        LOGDEBUG << "will attempt to activate vol:" << vdesc->volUUID;
        err = dm->timeVolCat_->activateVolume(vdesc->volUUID);
        return err;
    }

    // get the correct snapshot with the exact time of creation based of OM's vol creation time
    fds_volid_t latestSnapshotId = getLatestSnapshotFromOMTime(srcVolumeId, vdesc->timelineTime);
    // if no matching snap found, get the snapshot based on DM's creation time
    if (invalid_vol_id == latestSnapshotId) {
        LOGWARN << ATTR_TIME(vdesc->timelineTime) << "no snap matched at exact creation time";
        timelineDB->getLatestSnapshotAt(srcVolumeId, createTime, latestSnapshotId);
    } else {
        LOGNORMAL << ATTR_SNAP(latestSnapshotId) << ATTR_TIME(vdesc->timelineTime) << "snap found at exact createtime";
    }

    util::TimeStamp snapshotTime = 0;
    if (latestSnapshotId > invalid_vol_id) {
        LOGDEBUG << "clone vol:" << vdesc->volUUID
                 << " of srcvol:" << vdesc->srcVolumeId
                 << " will be based of snapshot:" << latestSnapshotId;
        // set the src volume to be the snapshot
        vdesc->srcVolumeId = latestSnapshotId;
        timelineDB->getSnapshotTime(srcVolumeId, latestSnapshotId, snapshotTime);
        LOGDEBUG << "about to copy :" << vdesc->srcVolumeId;
        err = dm->timeVolCat_->copyVolume(*vdesc, srcVolumeId);
        // recopy the original srcVolumeId
        vdesc->srcVolumeId = srcVolumeId;
    } else {
        // no matching snapshot

        util::TimeStamp timeDiff = abs(createTime - util::getTimeStampMicros())/(1000*1000);
        // if there is no matching snapshot & the timeline time is in the last 1 minute
        // then create from current time
        if (timeDiff <= 60) {
            snapshotTime = util::getTimeStampMicros();
            LOGDEBUG << "clone vol:" << vdesc->volUUID
                     << " of srcvol:" << vdesc->srcVolumeId
                     << " will be created from current time scratch as no nearest snapshot found(< 1m)";
            err = dm->timeVolCat_->copyVolume(*vdesc);
        } else {
            LOGDEBUG << "clone vol:" << vdesc->volUUID
                     << " of srcvol:" << vdesc->srcVolumeId
                     << " will be created from scratch as no nearest snapshot found";
            // vol create time is in millis
            snapshotTime = volmeta->vol_desc->createTime * 1000;
            err = dm->timeVolCat_->addVolume(*vdesc);
            if (!err.ok()) {
                LOGWARN << "Add volume returned error: '" << err << "'";
            }
            // XXX: Here we are creating and activating clone without copying src
            // volume, directory needs to exist for activation
            const FdsRootDir *root = dm->getModuleProvider()->proc_fdsroot();
            const std::string dirPath = root->dir_sys_repo_dm() +
                    std::to_string(vdesc->volUUID.get());
            root->fds_mkdir(dirPath.c_str());
        }
    }

    // now replay necessary commit logs as needed
    // TODO(dm-team): check for validity of TxnLogs
    bool fHasValidTxnLogs = (util::getTimeStampMicros() - snapshotTime) <=
            volmeta->vol_desc->contCommitlogRetention * 1000*1000;
    if (!fHasValidTxnLogs) {
        LOGWARN << "time diff does not fall within logretention time "
                << " srcvol:" << srcVolumeId << " onto vol:" << vdesc->volUUID
                << " logretention time:" << volmeta->vol_desc->contCommitlogRetention;
    }


    if (err.ok()) {
        LOGDEBUG << "will attempt to activate vol:" << vdesc->volUUID;
        err = dm->timeVolCat_->activateVolume(vdesc->volUUID);
        Error replayErr = journalMgr->replayTransactions(srcVolumeId, vdesc->volUUID,
                                                         snapshotTime, createTime);

        if (!replayErr.ok()) {
            LOGERROR << "replay error of src:" << srcVolumeId
                     << " onto:" << vdesc->volUUID
                     << " err:" << replayErr;
        }
    }

    return err;
}

fds_volid_t TimelineManager::getLatestSnapshotFromOMTime(fds_volid_t srcVolId, TimeStamp omTime) {
    FDSGUARD(dm->vol_map_mtx);
    for (auto const& element : dm->vol_meta_map) {
        if (element.second) {
            auto descriptor = element.second->vol_desc;
            LOGTRACE  << ATTR_VOL(descriptor->volUUID)
                      << ATTR_SRC(descriptor->srcVolumeId)
                      << ATTR_SNAP(descriptor->isSnapshot())
                      << ATTR("createtime", descriptor->createTime)
                      << ATTR("omtime", omTime);

            if (descriptor->isSnapshot() &&
                descriptor->srcVolumeId == srcVolId &&
                descriptor->createTime == omTime) {
                return descriptor->volUUID;
            }
        }
    }
    return invalid_vol_id;
}

const std::string TimelineManager::getCurrentJournalFile(fds_volid_t volId) {
    return dmutil::getVolumeDir(dm->getModuleProvider()->proc_fdsroot(), volId) + "/catalog.journal";
}

}  // namespace timeline
}  // namespace fds
