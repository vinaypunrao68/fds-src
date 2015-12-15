/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

#include <DataMgr.h>
#include <util/path.h>
#include <util/stringutils.h>
#include <unistd.h> // for unlink

namespace fds { namespace timeline {
#define TIMELINE_FEATURE_CHECK(...) if (!dm->features.isTimelineEnabled()) { return ERR_FEATURE_DISABLED ;}
TimelineManager::TimelineManager(fds::DataMgr* dm): dm(dm) {
    if (!dm->features.isTimelineEnabled()) {
        LOGWARN << "timeline feature is disabled";
        return;
    }
    timelineDB.reset(new TimelineDB());
    Error err = timelineDB->open();
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
        vecSnaps.reserve(blooms[volId].size());

        for (auto const& item : blooms[volId]) {
            vecSnaps.push_back(item.first);
        }
        LOGDEBUG << "will delete [" << vecSnaps.size() << "] snaps of vol:" << volId;
    } else {
        vecSnaps.push_back(snapshotId);
    }

    for (const auto& snapshotId : vecSnaps) {
        LOGDEBUG << "deleting snap:" << snapshotId
                 << " of vol:" << volId;
        // remove the bloom filter from the map
        // we need to do this first because we should not check this bf
        // during expunge check.
        blooms[volId].erase(snapshotId);

        // node remove the contents of the snapshot
        err = dm->deleteVolumeContents(snapshotId);
        err = dm->process_rm_vol(snapshotId, true);

        // remove info about this snapshot from timelinedb
        timelineDB->removeSnapshot(snapshotId);

        // remove the bloomfilter file
        unlink(getSnapshotBloomFile(snapshotId).c_str());
    }
    return err;
}

Error TimelineManager::loadSnapshot(fds_volid_t volid, fds_volid_t snapshotid) {
    TIMELINE_FEATURE_CHECK();
    // get the list of snapshots.
    std::string snapDir;
    std::vector<std::string> vecDirs;
    Error err;

    if (invalid_vol_id == snapshotid) {
        // load all snapshots
        snapDir  = dmutil::getSnapshotDir(volid);
        util::getSubDirectories(snapDir, vecDirs);
    } else {
        // load only a particluar snapshot
        snapDir = dmutil::getVolumeDir(volid, snapshotid);
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
            LOGWARN << "snapshot:" << snapId << " already loaded";
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
        desc->name = util::strformat("snaphot_%ld_%ld",
                                     volid.get(), snapId.get());
        VolumeMeta *meta = new(std::nothrow) VolumeMeta(desc->name,
                                                        snapId,
                                                        GetLog(),
                                                        desc);
        {
            FDSGUARD(dm->vol_map_mtx);
            if (dm->vol_meta_map.find(snapId) != dm->vol_meta_map.end()) {
                LOGWARN << "snapshot:" << snapId << " already loaded";
                continue;
            }
            dm->vol_meta_map[snapId] = meta;
        }
        LOGDEBUG << "Adding snapshot" << " name:" << desc->name << " vol:" << desc->volUUID;
        err = dm->timeVolCat_->addVolume(*desc);
        if (!err.ok()) {
            LOGERROR << "unable to load snapshot [" << snapId << "] for vol:"<< volid
                     << " - " << err;
        }

        // load bloom
        markObjectsInSnapshot(volid, snapId);
    }
    return err;
}

Error TimelineManager::unloadSnapshot(fds_volid_t volid, fds_volid_t snapshotid){
    TIMELINE_FEATURE_CHECK();
    Error err;
    return err;
}

Error TimelineManager::createSnapshot(VolumeDesc *vdesc) {
    TIMELINE_FEATURE_CHECK();
    util::TimeStamp createTime = util::getTimeStampMicros();
    Error err = dm->timeVolCat_->copyVolume(*vdesc);
    if (err.ok()) {
        // add it to timeline
        if (dm->features.isTimelineEnabled()) {
            timelineDB->addSnapshot(vdesc->srcVolumeId, vdesc->volUUID, createTime);
        }

        // activate it
        err = dm->timeVolCat_->activateVolume(vdesc->volUUID);

        if (err.ok()) {
            // mark the objects present in this snapshot
            markObjectsInSnapshot(vdesc->srcVolumeId, vdesc->volUUID);
        } else {
            LOGWARN << "snapshot activation failed. vol:" << vdesc->srcVolumeId
                    << " snap:" << vdesc->volUUID;
        }
    }
    return err;
}

Error TimelineManager::markObjectsInSnapshot(fds_volid_t volId, fds_volid_t snapshotId) {
    TIMELINE_FEATURE_CHECK();
    LOGDEBUG << "will mark objects in vol:" << volId << " snap:" << snapshotId;
    if (blooms.find(volId) == blooms.end()) {
        blooms[volId].find(snapshotId);
    }
    bool fNew = false, fLoaded=false;
    std::string bfFileName = getSnapshotBloomFile(snapshotId);
    LOGDEBUG << "object bf for snap:" << bfFileName;

    auto& bloom = blooms[volId][snapshotId];
    if (bloom.get() == NULL) {
        bloom.reset(new util::BloomFilter());
        fNew = true;
    }

    if (fNew) {
        // try to load the
        if (util::fileExists(bfFileName)) {
            LOGDEBUG << "will load object-bf: " << bfFileName;
            serialize::Deserializer* d=serialize::getFileDeserializer(bfFileName);
            if (bloom->read(d) > 0) {
                fLoaded = true;
            }
            delete d;
        }
    }

    if (fLoaded) {
        LOGWARN << "successfully loaded bloom for snap:" << snapshotId;
        return ERR_OK;
    }

    std::function<void (const ObjectID&)> func = [&bloom](const ObjectID& objId) {
        bloom->add(objId);
        // LOGDEBUG << "adding to bf:" << objId;
    };

    dm->timeVolCat_->queryIface()->forEachObject(snapshotId, func);

    // save the bloom
    serialize::Serializer* s=serialize::getFileSerializer(bfFileName);
    if (bloom->write(s) <= 0) {
        LOGWARN << "unable to store bloom for snap:" << snapshotId;
    }
    delete s;
    return ERR_OK;
}

Error TimelineManager::removeVolume(fds_volid_t volId) {
    TIMELINE_FEATURE_CHECK();
    return timelineDB->removeVolume(volId);    
}

Error TimelineManager::createClone(VolumeDesc *vdesc) {
    TIMELINE_FEATURE_CHECK();
    Error err;

    VolumeMeta * volmeta = dm->getVolumeMeta(vdesc->srcVolumeId);
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

    fds_volid_t latestSnapshotId = invalid_vol_id;
    timelineDB->getLatestSnapshotAt(srcVolumeId, createTime, latestSnapshotId);
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
            const FdsRootDir *root = g_fdsprocess->proc_fdsroot();
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

bool TimelineManager::isObjectInSnapshot(const ObjectID& objId, fds_volid_t volId) {
    auto snapMapIter = blooms.find(volId);
    if (snapMapIter == blooms.end()) {
        // LOGDEBUG << "no snaps found for vol:" << volId;
        return false;
    }

    for (const auto& item : snapMapIter->second) {
        if (item.second->lookup(objId)) return true;
    }

    return false;
}

std::string TimelineManager::getSnapshotBloomFile(fds_volid_t snapshotId) {
    std::string metaDir=dmutil::getVolumeMetaDir(snapshotId);
    FdsRootDir::fds_mkdir(metaDir.c_str());

    return util::strformat("%s/objectset.bf",metaDir.c_str());
}

}  // namespace timeline
}  // namespace fds
