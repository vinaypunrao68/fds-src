/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#include <string>
#include <list>
#include <vector>
#include <net/net_utils.h>
#include <fds_timestamp.h>
#include <lib/StatsCollector.h>
#include <DataMgr.h>
#include "fdsp/sm_api_types.h"
#include <dmhandler.h>
#include <util/stringutils.h>
#include <util/path.h>
namespace {
Error sendReloadVolumeRequest(const NodeUuid & nodeId, const fds_volid_t & volId) {
    auto asyncReq = gSvcRequestPool->newEPSvcRequest(nodeId.toSvcUuid());

    boost::shared_ptr<fpi::ReloadVolumeMsg> msg = boost::make_shared<fpi::ReloadVolumeMsg>();
    msg->volume_id = volId.get();
    asyncReq->setPayload(FDSP_MSG_TYPEID(fpi::ReloadVolumeMsg), msg);

    SvcRequestCbTask<EPSvcRequest, fpi::ReloadVolumeRspMsg> waiter;
    asyncReq->onResponseCb(waiter.cb);

    asyncReq->invoke();
    waiter.await();
    return waiter.error;
}
} // namespace

namespace fds {

/**
 * Receiver DM processing of volume sync state.
 * @param[in] fdw_complete false if rsync is completed = start processing
 * forwarded updates (activate shadow queue); true if forwarding updates
 * is completed (activate volume queue)
 */
Error
DataMgr::processVolSyncState(fds_volid_t volume_id, fds_bool_t fwd_complete) {
    Error err(ERR_OK);
    LOGMIGRATE << "DM received volume sync state for volume " << std::hex
               << volume_id << std::dec << " forward complete? " << fwd_complete;

    // open catalogs so we can start processing updates
    // TODO(Andrew): Actually do something if this fails. It doesn't really matter
    // now because our caller never replies to other other DM anyways...
    fds_verify(ERR_OK == timeVolCat_->activateVolume(volume_id));

    if (!fwd_complete) {
        err = timeVolCat_->activateVolume(volume_id);
        if (err.ok()) {
            // start processing forwarded updates
            if (features.isCatSyncEnabled()) {
                catSyncRecv->startProcessFwdUpdates(volume_id);
            } else {
                LOGWARN << "catalog sync feature - NOT enabled";
            }
        }
    } else {
        LOGDEBUG << "Will queue DmIoMetaRecvd message "
                 << std::hex << volume_id << std::dec;
        if (features.isCatSyncEnabled()) {
            DmIoMetaRecvd* metaRecvdIo = new(std::nothrow) DmIoMetaRecvd(volume_id);
            err = enqueueMsg(catSyncRecv->shadowVolUuid(volume_id), metaRecvdIo);
            fds_verify(err.ok());
        } else {
            LOGWARN << "catalog sync feature - NOT enabled";
        }
    }

    if (!err.ok()) {
        LOGERROR << "DM failed to process vol sync state for volume " << std::hex
                 << volume_id << " fwd complete? " << fwd_complete << " " << err;
    }

    return err;
}

//
// receiving DM notified that forward is complete and ok to open
// qos queue. Some updates may still be forwarded, but they are ok to
// go in parallel with updates from AM
//
void DataMgr::handleForwardComplete(dmCatReq *io) {
    LOGNORMAL << "Will open up QoS queue for volume " << std::hex
              << io->volId << std::dec;
    if (features.isCatSyncEnabled()) {
        catSyncRecv->handleFwdDone(io->volId);
    } else {
        LOGWARN << "catalog sync feature - NOT enabled";
    }

    vol_map_mtx->lock();
    auto volIt = vol_meta_map.find(io->volId);
    fds_verify(vol_meta_map.end() != volIt);
    VolumeMeta *vol_meta = volIt->second;
    vol_meta->dmVolQueue->activate();
    vol_map_mtx->unlock();

    if (features.isQosEnabled()) qosCtrl->markIODone(*io);
    delete io;
}

//
// Called by stats collector to sample DM specific stats
//
void DataMgr::sampleDMStats(fds_uint64_t timestamp) {
    LOGTRACE << "Sampling DM stats";
    Error err(ERR_OK);
    std::unordered_set<fds_volid_t> prim_vols;
    std::unordered_set<fds_volid_t>::const_iterator cit;
    fds_uint64_t total_bytes = 0;
    fds_uint64_t total_blobs = 0;
    fds_uint64_t total_objects = 0;

    // find all volumes for which this DM is primary (we only need
    // to collect capacity stats from DMs that are primary).
    vol_map_mtx->lock();
    for (auto vol_it = vol_meta_map.begin();
         vol_it != vol_meta_map.end();
         ++vol_it) {
        if (vol_it->second->vol_desc->fSnapshot) {
            continue;
        }
        fds_volid_t volId (vol_it->first);
        if (amIPrimary(volId)) {
            prim_vols.insert(volId);
        }
    }
    vol_map_mtx->unlock();

    // collect capacity stats for volumes for which this DM is primary
    for (cit = prim_vols.cbegin(); cit != prim_vols.cend(); ++cit) {
        err = timeVolCat_->queryIface()->statVolume(*cit,
                                                    &total_bytes,
                                                    &total_blobs,
                                                    &total_objects);
        if (!err.ok()) {
            LOGERROR << "Failed to get volume meta for vol " << std::hex
                     << *cit << std::dec << " " << err;
            continue;  // try other volumes
        }
        LOGDEBUG << "volume " << std::hex << *cit << std::dec
                 << " bytes " << total_bytes << " blobs "
                 << total_blobs << " objects " << total_objects;
        StatsCollector::singleton()->recordEvent(*cit,
                                                 timestamp,
                                                 STAT_DM_CUR_TOTAL_BYTES,
                                                 total_bytes);
        StatsCollector::singleton()->recordEvent(*cit,
                                                 timestamp,
                                                 STAT_DM_CUR_TOTAL_BLOBS,
                                                 total_blobs);
        StatsCollector::singleton()->recordEvent(*cit,
                                                 timestamp,
                                                 STAT_DM_CUR_TOTAL_OBJECTS,
                                                 total_objects);
    }
}

void DataMgr::handleLocalStatStream(fds_uint64_t start_timestamp,
                                    fds_volid_t volume_id,
                                    const std::vector<StatSlot>& slots) {
    LOGTRACE << "Streaming local stats to stats aggregator";
    statStreamAggr_->handleModuleStatStream(start_timestamp,
                                            volume_id,
                                            slots);
}

/**
 * Is called on timer to finish forwarding for all volumes that
 * are still forwarding, send DMT close ack, and remove vcat/tcat
 * of volumes that do not longer this DM responsibility
 */
void
DataMgr::finishCloseDMT() {
    std::unordered_set<fds_volid_t> done_vols;
    fds_bool_t all_finished = false;

    LOGNOTIFY << "Finish handling DMT close and send ack if not sent yet";

    // move all volumes that are in 'finish forwarding' state
    // to not forwarding state, and make a set of these volumes
    // the set is used so that we call catSyncMgr to cleanup/and
    // and send push meta done msg outside of vol_map_mtx lock
    vol_map_mtx->lock();
    for (auto vol_it = vol_meta_map.begin();
         vol_it != vol_meta_map.end();
         ++vol_it) {
        fds_volid_t volid (vol_it->first);
        VolumeMeta *vol_meta = vol_it->second;
        if (vol_meta->isForwarding()) {
            vol_meta->setForwardFinish();
            done_vols.insert(volid);
        }
        // we expect that volume is not forwarding
        fds_verify(!vol_meta->isForwarding());
    }
    vol_map_mtx->unlock();

    for (std::unordered_set<fds_volid_t>::const_iterator it = done_vols.cbegin();
         it != done_vols.cend();
         ++it) {
        // remove the volume from sync_volume list
        LOGNORMAL << "CleanUP: remove Volume " << std::hex << *it << std::dec;
        if (features.isCatSyncEnabled()) {
            if (catSyncMgr->finishedForwardVolmeta(*it)) {
                all_finished = true;
            }
        } else {
            LOGWARN << "catalog sync feature - NOT enabled";
            all_finished = true;
        }
    }

    // at this point we expect that all volumes are done
    fds_verify(all_finished || !catSyncMgr->isSyncInProgress());

    // Note that finishedForwardVolmeta sends ack to close DMT when it returns true
    // either in this method or when we called finishedForwardVolume for the last
    // volume to finish forwarding

    LOGDEBUG << "Will cleanup catalogs for volumes DM is not responsible anymore";
    // TODO(Anna) remove volume catalog for volumes we finished forwarding
}

Error
DataMgr::deleteSnapshot(const fds_uint64_t snapshotId) {
    // TODO(umesh): implement this
    return ERR_OK;
}

std::string DataMgr::getSnapDirBase() const
{
    const FdsRootDir *root = g_fdsprocess->proc_fdsroot();
    return root->dir_user_repo_dm();
}

std::string DataMgr::getSysVolumeName(const fds_volid_t &volId) const
{
    std::stringstream stream;
    stream << "SYSTEM_VOLUME_" << getVolumeDesc(volId)->tennantId;
    return stream.str();
}

std::string DataMgr::getSnapDirName(const fds_volid_t &volId,
                                    const fds_volid_t snapId) const
{
    std::stringstream stream;
    stream << getSnapDirBase() << "/" << volId << "/" << snapId;
    return stream.str();
}

//
// handle finish forward for volume 'volid'
//
void DataMgr::finishForwarding(fds_volid_t volid) {
    // FIXME(DAC): This local is only ever assigned.
    fds_bool_t all_finished = false;

    // set the state
    vol_map_mtx->lock();
    auto volIt = vol_meta_map.find(volid);
    fds_assert(vol_meta_map.end() != volIt);
    VolumeMeta *vol_meta = volIt->second;
    // Skip this verify because it's actually set later?
    // TODO(Andrew): If the above comment is correct, we
    // remove most of what this function actually do
    // fds_verify(vol_meta->isForwarding());
    vol_meta->setForwardFinish();
    vol_map_mtx->unlock();

    // notify cat sync mgr
    if (catSyncMgr->finishedForwardVolmeta(volid)) {
        all_finished = true;
    }
}

/**
 * Note that volId may not necessarily match volume id in ioReq
 * Example when it will not match: if we enqueue request for volumeA
 * to shadow queue, we will pass shadow queue's volId (queue id)
 * but ioReq will contain actual volume id; so maybe we should change
 * this method to pass queue id as first param not volid
 */
Error DataMgr::enqueueMsg(fds_volid_t volId,
                          dmCatReq* ioReq) {
    Error err(ERR_OK);

    switch (ioReq->io_type) {
        case FDS_DM_PURGE_COMMIT_LOG:
        case FDS_DM_SNAP_VOLCAT:
        case FDS_DM_SNAPDELTA_VOLCAT:
        case FDS_DM_FWD_CAT_UPD:
        case FDS_DM_PUSH_META_DONE:
        case FDS_DM_META_RECVD:
            err = qosCtrl->enqueueIO(volId, static_cast<FDS_IOType*>(ioReq));
            break;
        default:
            fds_assert(!"Unknown message");
    };

    if (!err.ok()) {
        LOGERROR << "Failed to enqueue message " << ioReq->log_string()
                 << " to queue "<< std::hex << volId << std::dec << " " << err;
    }
    return err;
}

/*
 * Adds the volume if it doesn't exist already.
 * Note this does NOT return error if the volume exists.
 */
Error DataMgr::_add_if_no_vol(const std::string& vol_name,
                              fds_volid_t vol_uuid, VolumeDesc *desc) {
    Error err(ERR_OK);

    /*
     * Check if we already know about this volume
     */
    vol_map_mtx->lock();
    if (volExistsLocked(vol_uuid) == true) {
        LOGNORMAL << "Received add request for existing vol uuid "
                  << vol_uuid << ", so ignoring.";
        vol_map_mtx->unlock();
        return err;
    }

    vol_map_mtx->unlock();

    err = _add_vol_locked(vol_name, vol_uuid, desc, true);


    return err;
}

VolumeMeta*  DataMgr::getVolumeMeta(fds_volid_t volId, bool fMapAlreadyLocked) {
    if (!fMapAlreadyLocked) {
        FDSGUARD(*vol_map_mtx);
        auto iter = vol_meta_map.find(volId);
        if (iter != vol_meta_map.end()) {
            return iter->second;
        }
    } else {
        auto iter = vol_meta_map.find(volId);
        if (iter != vol_meta_map.end()) {
            return iter->second;
        }
    }
    return NULL;
}

/*
 * Meant to be called holding the vol_map_mtx.
 * @param vol_will_sync true if this volume's meta will be synced from
 * other DM first
 */
Error DataMgr::_add_vol_locked(const std::string& vol_name,
                               fds_volid_t vol_uuid,
                               VolumeDesc *vdesc,
                               fds_bool_t vol_will_sync) {
    Error err(ERR_OK);
    bool fActivated = false;
    // create vol catalogs, etc first

    bool fPrimary = false;
    bool fOldVolume = (!vdesc->isSnapshot() && vdesc->isStateCreated());

    LOGDEBUG << "vol:" << vol_name
             << " snap:" << vdesc->isSnapshot()
             << " state:" << vdesc->getState()
             << " created:" << vdesc->isStateCreated()
             << " old:" << fOldVolume;

    if (vdesc->isSnapshot() || vdesc->isClone()) {
        fPrimary = amIPrimary(vdesc->srcVolumeId);
    } else {
        fPrimary = amIPrimary(vdesc->volUUID);
    }

    if (vdesc->isSnapshot() && !fPrimary) {
        LOGWARN << "not primary - nothing to do for snapshot "
                << "for vol:" << vdesc->srcVolumeId;
        return err;
    }

    // do this processing only in the case of primary ..
    if ((vdesc->isSnapshot() || vdesc->isClone()) && fPrimary) {
        VolumeMeta * volmeta = getVolumeMeta(vdesc->srcVolumeId);
        if (!volmeta) {
            GLOGWARN << "Volume '" << std::hex << vdesc->srcVolumeId << std::dec <<
                    "' not found!";
            return ERR_NOT_FOUND;
        }

        LOGDEBUG << "Creating a " << (vdesc->isClone()?"clone":"snapshot")
                 << " name:" << vdesc->name << " vol:" << vdesc->volUUID
                 << " srcVolume:"<< vdesc->srcVolumeId;

        // create commit log entry and enable buffering in case of snapshot
        if (vdesc->isSnapshot()) {
            // snapshot
            util::TimeStamp createTime = util::getTimeStampMicros();
            err = timeVolCat_->copyVolume(*vdesc);
            if (err.ok()) {
                // add it to timeline
                if (features.isTimelineEnabled()) {
                    timeline->addSnapshot(vdesc->srcVolumeId, vdesc->volUUID, createTime);
                }
            }
        } else if (features.isTimelineEnabled()) {
            // clone
            // find the closest snapshot to clone the base from
            fds_volid_t srcVolumeId = vdesc->srcVolumeId;

            LOGDEBUG << "Timeline time is: '" << vdesc->timelineTime << "' for clone: '"
                    << vdesc->volUUID << "'";
            // timelineTime is in seconds
            util::TimeStamp createTime = vdesc->timelineTime * (1000*1000);

            if (createTime == 0) {
                LOGDEBUG << "clone vol:" << vdesc->volUUID
                         << " of srcvol:" << vdesc->srcVolumeId
                         << " will be a clone at current time";
                err = timeVolCat_->copyVolume(*vdesc);
            } else {
                fds_volid_t latestSnapshotId = invalid_vol_id;
                timeline->getLatestSnapshotAt(srcVolumeId, createTime, latestSnapshotId);
                util::TimeStamp snapshotTime = 0;
                if (latestSnapshotId > invalid_vol_id) {
                    LOGDEBUG << "clone vol:" << vdesc->volUUID
                             << " of srcvol:" << vdesc->srcVolumeId
                             << " will be based of snapshot:" << latestSnapshotId;
                    // set the src volume to be the snapshot
                    vdesc->srcVolumeId = latestSnapshotId;
                    timeline->getSnapshotTime(srcVolumeId, latestSnapshotId, snapshotTime);
                    LOGDEBUG << "about to copy :" << vdesc->srcVolumeId;
                    err = timeVolCat_->copyVolume(*vdesc, srcVolumeId);
                    // recopy the original srcVolumeId
                    vdesc->srcVolumeId = srcVolumeId;
                } else {
                    LOGDEBUG << "clone vol:" << vdesc->volUUID
                             << " of srcvol:" << vdesc->srcVolumeId
                             << " will be created from scratch as no nearest snapshot found";
                    // vol create time is in millis
                    snapshotTime = volmeta->vol_desc->createTime * 1000;
                    err = timeVolCat_->addVolume(*vdesc);
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
                    err = timeVolCat_->activateVolume(vdesc->volUUID);
                    if (err.ok()) fActivated = true;
                }

                err = timeVolCat_->replayTransactions(srcVolumeId, vdesc->volUUID,
                                                      snapshotTime, createTime);
            }
        }
    } else {
        LOGDEBUG << "Adding volume" << " name:" << vdesc->name << " vol:" << vdesc->volUUID;
        err = timeVolCat_->addVolume(*vdesc);
    }

    if (!err.ok()) {
        LOGERROR << "Failed to " << (vdesc->isSnapshot() ? "create snapshot"
                                     : (vdesc->isClone() ? "create clone" : "add volume")) << " "
                 << std::hex << vol_uuid << std::dec;
        return err;
    }

    if (err.ok() && !vol_will_sync && !fActivated) {
        // not going to sync this volume, activate volume
        // so that we can do get/put/del cat ops to this volume
        err = timeVolCat_->activateVolume(vol_uuid);
        if (err.ok()) fActivated = true;
    }

    if (err.ok() && vdesc->isClone() && fPrimary) {
        // all actions were successful now rsync it to other DMs
        DmtColumnPtr nodes = MODULEPROVIDER()->getSvcMgr()->getDMTNodesForVolume(vdesc->srcVolumeId);
        for (uint i = 1; i < nodes->getLength(); i++) {
            LOGDEBUG << "rsyncing vol:" << vdesc->volUUID
                     << "to node:" << nodes->get(i);
            auto volDir = timeVolCat_->queryIface();
            err = volDir->syncCatalog(vdesc->volUUID, nodes->get(i));
            if (!err.ok()) {
                LOGWARN << "catalog sync failed on clone, vol:" << vdesc->volUUID;
            } else {
                // send message to reload volume
                err = sendReloadVolumeRequest((*nodes)[i], vdesc->volUUID);
                if (!err.ok()) {
                    LOGWARN << "catalog reload failed on clone, vol:" << vdesc->volUUID;
                }
            }
        }
    }

    VolumeMeta *volmeta = new(std::nothrow) VolumeMeta(vol_name,
                                                       vol_uuid,
                                                       GetLog(),
                                                       vdesc);
    if (!volmeta) {
        LOGERROR << "Failed to allocate VolumeMeta for volume "
                 << std::hex << vol_uuid << std::dec;
        return ERR_OUT_OF_MEMORY;
    }

    if (vdesc->isSnapshot()) {
        volmeta->dmVolQueue.reset(qosCtrl->getQueue(vdesc->qosQueueId));
    }

    bool needReg = false;
    if (!volmeta->dmVolQueue.get()) {
        // FIXME: Why are we doubling the assured rate here?
        volmeta->dmVolQueue.reset(new FDS_VolumeQueue(4096,
                                                      vdesc->iops_throttle,
                                                      2*vdesc->iops_assured,
                                                      vdesc->relativePrio));
        volmeta->dmVolQueue->activate();
        needReg = true;
    }


    LOGDEBUG << "Added vol meta for vol uuid and per Volume queue " << std::hex
             << vol_uuid << std::dec << ", created catalogs? " << !vol_will_sync;

    vol_map_mtx->lock();
    if (needReg) {
        err = qosCtrl->registerVolume(vdesc->isSnapshot() ? vdesc->qosQueueId : vol_uuid,
                                      static_cast<FDS_VolumeQueue*>(volmeta->dmVolQueue.get()));
    }
    if (!err.ok()) {
        delete volmeta;
        vol_map_mtx->unlock();
        return err;
    }

    // if we will sync vol meta, block processing IO requests from this volume's
    // qos queue and create a shadow queue for receiving volume meta from another DM
    if (vol_will_sync) {
        // we will use the priority of the volume, but no min iops (otherwise we will
        // need to implement temp deregister of vol queue, so we have enough total iops
        // to admit iops of shadow queue)
        FDS_VolumeQueue* shadowVolQueue =
                new(std::nothrow) FDS_VolumeQueue(4096, 10000, 0, vdesc->relativePrio);
        if (shadowVolQueue) {
            fds_volid_t shadow_volid = catSyncRecv->shadowVolUuid(vol_uuid);
            // block volume's qos queue
            volmeta->dmVolQueue->stopDequeue();
            // register shadow queue
            err = qosCtrl->registerVolume(shadow_volid, shadowVolQueue);
            if (err.ok()) {
                LOGERROR << "Registered shadow volume queue for volume 0x"
                         << std::hex << vol_uuid << " shadow id " << shadow_volid << std::dec;
                // pass ownership of shadow volume queue to volume meta receiver
                catSyncRecv->startRecvVolmeta(vol_uuid, shadowVolQueue);
            } else {
                LOGERROR << "Failed to register shadow volume queue for volume 0x"
                         << std::hex << vol_uuid << " shadow id " << shadow_volid
                         << std::dec << " " << err;
                // cleanup, we will revert volume registration and creation
                delete shadowVolQueue;
                shadowVolQueue = NULL;
            }
        } else {
            LOGERROR << "Failed to allocate shadow volume queue for volume "
                     << std::hex << vol_uuid << std::dec;
            err = ERR_OUT_OF_MEMORY;
        }
    }

    if (err.ok()) {
        // For now, volumes only land in the map if it is already active.
        if (fActivated) {
            volmeta->vol_desc->setState(fpi::Active);
        } else {
            LOGWARN << "vol:" << vol_uuid << " not activated";
            volmeta->vol_desc->setState(fpi::InError);
        }

        // we registered queue and shadow queue if needed
        vol_meta_map[vol_uuid] = volmeta;
    }
    vol_map_mtx->unlock();

    if (!err.ok()) {
        // cleanup volmeta and deregister queue
        LOGERROR << "Cleaning up volume queue and vol meta because of error "
                 << " volid 0x" << std::hex << vol_uuid << std::dec;
        qosCtrl->deregisterVolume(vdesc->isSnapshot() ? vdesc->qosQueueId : vol_uuid);
        volmeta->dmVolQueue.reset();
        delete volmeta;
    }

    if (vdesc->isSnapshot()) {
        return err;
    }

    // Primary is responsible for persisting the latest seq number.
    // latest seq_number is provided to AM on volume open.
    if (fPrimary) {
        err = timeVolCat_->queryIface()->getVolumeSequenceId(vol_uuid,
                                                             volmeta->sequence_id);

        if (!err.ok()) {
            return err;
        }
    }

    /*
     * XXX: The logic below will use source volume id to collect stats for clone,
     *      but it will now stream the stats to AM because amIPrimary() check is
     *      done there. Effective we are going to collect stats for clone but not
     *      stream it.
     * TODO(xxx): Migrate clone and then stream stats
     */
    if (err.ok() && amIPrimary(vdesc->isClone() ? vdesc->srcVolumeId : vol_uuid)) {
        // will aggregate stats for this volume and log them
        statStreamAggr_->attachVolume(vol_uuid);
        // create volume stat  directory.
        const FdsRootDir *root = g_fdsprocess->proc_fdsroot();
        const std::string stat_dir = root->dir_sys_repo_stats() + std::to_string(vol_uuid.get());
        auto sret = std::system((const char *)("mkdir -p "+stat_dir+" ").c_str());
    }

    // now load all the snapshots for the volume
    if (fOldVolume) {
        // get the list of snapshots.
        std::string snapDir = dmutil::getSnapshotDir(vol_uuid);
        std::vector<std::string> vecDirs;
        util::getSubDirectories(snapDir, vecDirs);
        fds_volid_t snapId;
        LOGDEBUG << "will load [" << vecDirs.size() << "]"
                 << " snapshots for vol:" << vol_uuid
                 << " from:" << snapDir;
        for (const auto& snap : vecDirs) {
            snapId = std::atoll(snap.c_str());
            //now add the snap
            VolumeDesc *desc = new VolumeDesc(*vdesc);
            desc->fSnapshot = true;
            desc->srcVolumeId = vol_uuid;
            desc->lookupVolumeId = vol_uuid;
            desc->qosQueueId = vol_uuid;
            desc->volUUID = snapId;
            desc->contCommitlogRetention = 0;
            desc->timelineTime = 0;
            desc->setState(fpi::Active);
            desc->name = util::strformat("snaphot_%ld_%ld",
                                         vol_uuid.get(), snapId.get());
            VolumeMeta *meta = new(std::nothrow) VolumeMeta(desc->name,
                                                            snapId,
                                                            GetLog(),
                                                            desc);
            vol_meta_map[snapId] = meta;
            LOGDEBUG << "Adding snapshot" << " name:" << desc->name << " vol:" << desc->volUUID;
            Error err1 = timeVolCat_->addVolume(*desc);
            if (!err1.ok()) {
                LOGERROR << "unable to load snapshot [" << snapId << "] for vol:"<< vol_uuid
                         << " - " << err1;
            }
        }
    }

    return err;
}

Error DataMgr::_process_add_vol(const std::string& vol_name,
                                fds_volid_t vol_uuid,
                                VolumeDesc *desc,
                                fds_bool_t vol_will_sync) {
    Error err(ERR_OK);

    vol_map_mtx->lock();
    if (volExistsLocked(vol_uuid) == true) {
        err = Error(ERR_DUPLICATE);
        vol_map_mtx->unlock();
        LOGDEBUG << "Received add request for existing vol uuid "
                 << std::hex << vol_uuid << std::dec;
        return err;
    }
    vol_map_mtx->unlock();

    err = _add_vol_locked(vol_name, vol_uuid, desc, vol_will_sync);
    return err;
}

Error DataMgr::_process_mod_vol(fds_volid_t vol_uuid, const VolumeDesc& voldesc)
{
    Error err(ERR_OK);

    vol_map_mtx->lock();
    /* make sure volume exists */
    if (volExistsLocked(vol_uuid) == false) {
        LOGERROR << "Received modify policy request for "
                 << "non-existant volume [" << vol_uuid
                 << ", " << voldesc.name << "]";
        err = Error(ERR_NOT_FOUND);
        vol_map_mtx->unlock();
        return err;
    }
    VolumeMeta *vm = vol_meta_map[vol_uuid];
    // FIXME: Why are we doubling assured?
    vm->vol_desc->modifyPolicyInfo(2 * voldesc.iops_assured,
                                   voldesc.iops_throttle,
                                   voldesc.relativePrio);
    err = qosCtrl->modifyVolumeQosParams(vol_uuid,
                                         2 * voldesc.iops_assured,
                                         voldesc.iops_throttle,
                                         voldesc.relativePrio);
    vol_map_mtx->unlock();

    LOGNOTIFY << "Modify policy for volume "
              << voldesc.name << " RESULT: " << err.GetErrstr();

    return err;
}

Error DataMgr::process_rm_vol(fds_volid_t vol_uuid, fds_bool_t check_only) {
    Error err(ERR_OK);

    // mark volume as deleted if it's not empty
    if (check_only) {
        // check if not empty and mark volume as deleted to
        // prevent further updates to the volume as we are going
        // to remove it
        err = timeVolCat_->markVolumeDeleted(vol_uuid);
        if (!err.ok()) {
            LOGERROR << "Failed to mark volume as deleted " << err;
            return err;
        }
        LOGNORMAL << "Notify volume rm check only, did not "
                  << " remove vol meta for vol " << std::hex
                  << vol_uuid << std::dec;
        // if notify delete asked to only check if deleting volume
        // was ok; so we return here; DM will get
        // another notify volume delete with check_only == false to
        // actually cleanup all other data structures for this volume
        return err;
    }

    // we we are here, check_only == false
    err = timeVolCat_->deleteEmptyVolume(vol_uuid);
    if (err.ok()) {
        VolumeMeta* vol_meta = NULL;
        qosCtrl->deregisterVolume(vol_uuid);
        vol_map_mtx->lock();
        if (vol_meta_map.count(vol_uuid) > 0) {
            vol_meta = vol_meta_map[vol_uuid];
            vol_meta_map.erase(vol_uuid);
        }
        vol_map_mtx->unlock();
        if (vol_meta) {
            vol_meta->dmVolQueue.reset();
            delete vol_meta;
        }
        statStreamAggr_->detachVolume(vol_uuid);
        LOGNORMAL << "Removed vol meta for vol uuid "
                  << std::hex << vol_uuid << std::dec;
    } else {
        LOGERROR << "Failed to remove volume " << std::hex
                 << vol_uuid << std::dec << " " << err;
    }

    return err;
}

Error DataMgr::deleteVolumeContents(fds_volid_t volId) {
    Error err(ERR_OK);
    // get list of blobs for volume
    fpi::BlobDescriptorListType blobList;
    VolumeCatalogQueryIface::ptr volCatIf = timeVolCat_->queryIface();
    blob_version_t version = 0;
    // FIXME(DAC): This needs to happen within the context of a transaction.
    err = volCatIf->listBlobs(volId, &blobList);
    LOGWARN << "deleting all [" << blobList.size() << "]"
            << " blobs from vol:" << volId;
    fpi::FDSP_MetaDataList metaList;
    fds_uint64_t blobSize;
    for (const auto& blob : blobList) {
        metaList.clear();
        version = 0;
        // FIXME(DAC): Error is ignored, and only the last error from deleteBlob will be returned
        //             to the caller.
        err = volCatIf->getBlobMeta(volId,
                                    blob.name,
                                    &version, &blobSize, &metaList);
        err = volCatIf->deleteBlob(volId, blob.name, version);
    }

    return err;
}

/**
 * For all volumes that are in forwarding state, move them to
 * finish forwarding state.
 */
Error DataMgr::notifyDMTClose() {
    Error err(ERR_OK);
    DmIoPushMetaDone* pushMetaDoneIo = NULL;

    if (features.isCatSyncEnabled()) {
        if (!catSyncMgr->isSyncInProgress()) {
            err = ERR_CATSYNC_NOT_PROGRESS;
            return err;
        }
        // set DMT close time in catalog sync mgr
        // TODO(Andrew): We're doing this later in the process,
        // so can remove this when that works for sure.
        // catSyncMgr->setDmtCloseNow();
    } else {
        LOGWARN << "catalog sync feature - NOT enabled";
    }

    // set every volume's state to 'finish forwarding
    // and enqueue DMT close marker to qos queues of volumes
    // that are in 'finish forwarding' state
    vol_map_mtx->lock();
    for (auto vol_it = vol_meta_map.begin();
         vol_it != vol_meta_map.end();
         ++vol_it) {
        VolumeMeta *vol_meta = vol_it->second;
        vol_meta->finishForwarding();
        pushMetaDoneIo = new DmIoPushMetaDone(vol_it->first);
        handleDMTClose(pushMetaDoneIo);
    }
    vol_map_mtx->unlock();

    // TODO(Andrew): Um, no where to we have a useful error statue
    // to even return.
    sendDmtCloseCb(err);
    LOGMIGRATE << "Sent DMT close message to OM";

    return err;
}

//
// Handle DMT close for the volume in io req -- sends
// push meta done to destination DM
//
void DataMgr::handleDMTClose(dmCatReq *io) {
    DmIoPushMetaDone *pushMetaDoneReq = static_cast<DmIoPushMetaDone*>(io);
    LOGMIGRATE << "Processed all commits that arrived before DMT close "
               << "will now notify dst DM to open up volume queues: vol "
               << std::hex << pushMetaDoneReq->volId << std::dec;

    // Notify any other DMs we're migrating to that it's OK to dispatch
    // from their blocked QoS queues.
    // If we commit any currently open or in process transactions they
    // will still get forwarded when they complete.
    fds_verify(ERR_OK == catSyncMgr->issueServiceVolumeMsg(pushMetaDoneReq->volId));
    LOGMIGRATE << "Sent message to migration destination to open blocked queues.";
    delete pushMetaDoneReq;
}

/*
 * Returns the maxObjSize in the volume.
 * TODO(Andrew): This should be refactored into a
 * common library since everyone needs it, not just DM
 * TODO(Andrew): Also this shouldn't be hard coded obviously...
 */
Error DataMgr::getVolObjSize(fds_volid_t volId,
                             fds_uint32_t *maxObjSize) {
    Error err(ERR_OK);

    /*
     * Get a local reference to the vol meta.
     */
    vol_map_mtx->lock();
    VolumeMeta *vol_meta = vol_meta_map[volId];
    vol_map_mtx->unlock();

    fds_verify(vol_meta != NULL);
    *maxObjSize = vol_meta->vol_desc->maxObjSizeInBytes;
    return err;
}

DataMgr::DataMgr(CommonModuleProviderIf *modProvider)
        : Module("dm"),
          modProvider_(modProvider)
{
    // NOTE: Don't put much stuff in the constructor.  Move any construction
    // into mod_init()
}

int DataMgr::mod_init(SysParams const *const param)
{
    Error err(ERR_OK);

    standalone = false;
    numTestVols = 10;
    scheduleRate = 10000;
    shuttingDown = false;

    catSyncRecv = boost::make_shared<CatSyncReceiver>(this);
    closedmt_timer = boost::make_shared<FdsTimer>();
    closedmt_timer_task = boost::make_shared<CloseDMTTimerTask>(*closedmt_timer,
                                                                std::bind(&DataMgr::finishCloseDMT,
                                                                          this));

    // qos threadpool config
    qosThreadCount = modProvider_->get_fds_config()->\
            get<fds_uint32_t>("fds.dm.qos.default_qos_threads", 10);
    qosOutstandingTasks = modProvider_->get_fds_config()->\
            get<fds_uint32_t>("fds.dm.qos.default_outstanding_io", 20);

    // If we're in test mode, don't daemonize.
    // TODO(Andrew): We probably want another config field and
    // not to override test_mode

    // Set testing related members
    testUturnAll = modProvider_->get_fds_config()->\
            get<bool>("fds.dm.testing.uturn_all", false);
    testUturnStartTx = modProvider_->get_fds_config()->\
            get<bool>("fds.dm.testing.uturn_starttx", false);
    testUturnUpdateCat = modProvider_->get_fds_config()->\
            get<bool>("fds.dm.testing.uturn_updatecat", false);
    testUturnSetMeta   = modProvider_->get_fds_config()->\
            get<bool>("fds.dm.testing.uturn_setmeta", false);

    // timeline feature toggle
    features.setTimelineEnabled(modProvider_->get_fds_config()->get<bool>(
            "fds.dm.enable_timeline", true));
    if (features.isTimelineEnabled()) {
        timeline.reset(new TimelineDB());
    }
    /**
     * FEATURE TOGGLE: Volume Open Support
     * Thu 02 Apr 2015 12:39:27 PM PDT
     */
    features.setVolumeTokensEnabled(modProvider_->get_fds_config()->get<bool>(
            "fds.feature_toggle.common.volume_open_support", false));

    vol_map_mtx = new fds_mutex("Volume map mutex");

    /*
     * Retrieves the number of primary DMs from the config file.
     */
    int primary_check = MODULEPROVIDER()->get_fds_config()->
				  get<int>("fds.dm.number_of_primary");
    fds_verify(primary_check > 0);
    setNumOfPrimary((unsigned)primary_check);

    return 0;
}

void DataMgr::initHandlers() {
    // TODO: Inject these.
    handlers[FDS_LIST_BLOB] = new dm::GetBucketHandler(*this);
    handlers[FDS_DELETE_BLOB] = new dm::DeleteBlobHandler(*this);
    handlers[FDS_DM_SYS_STATS] = new dm::DmSysStatsHandler(*this);
    handlers[FDS_CAT_UPD] = new dm::UpdateCatalogHandler(*this);
    handlers[FDS_GET_BLOB_METADATA] = new dm::GetBlobMetaDataHandler(*this);
    handlers[FDS_CAT_QRY] = new dm::QueryCatalogHandler(*this);
    handlers[FDS_START_BLOB_TX] = new dm::StartBlobTxHandler(*this);
    handlers[FDS_DM_STAT_STREAM] = new dm::StatStreamHandler(*this);
    handlers[FDS_COMMIT_BLOB_TX] = new dm::CommitBlobTxHandler(*this);
    handlers[FDS_CAT_UPD_ONCE] = new dm::UpdateCatalogOnceHandler(*this);
    handlers[FDS_SET_BLOB_METADATA] = new dm::SetBlobMetaDataHandler(*this);
    handlers[FDS_ABORT_BLOB_TX] = new dm::AbortBlobTxHandler(*this);
    handlers[FDS_DM_FWD_CAT_UPD] = new dm::ForwardCatalogUpdateHandler(*this);
    handlers[FDS_STAT_VOLUME] = new dm::StatVolumeHandler(*this);
    handlers[FDS_SET_VOLUME_METADATA] = new dm::SetVolumeMetadataHandler(*this);
    handlers[FDS_GET_VOLUME_METADATA] = new dm::GetVolumeMetadataHandler(*this);
    handlers[FDS_OPEN_VOLUME] = new dm::VolumeOpenHandler(*this);
    handlers[FDS_CLOSE_VOLUME] = new dm::VolumeCloseHandler(*this);
    handlers[FDS_DM_RELOAD_VOLUME] = new dm::ReloadVolumeHandler(*this);
    handlers[FDS_DM_MIGRATION] = new dm::DmMigrationHandler(*this);
}

DataMgr::~DataMgr()
{
    // shutdown all data manager modules
    LOGDEBUG << "Received shutdown message DM ... shutdown mnodules..";
    mod_shutdown();
}

int DataMgr::run()
{
    // TODO(Rao): Move this into module init
    initHandlers();

    _shutdownGate.waitUntilOpened();

    return 0;
}

void DataMgr::mod_startup()
{
    Error err;
    fds::DmDiskInfo     *info;
    fds::DmDiskQuery     in;
    fds::DmDiskQueryOut  out;
    fds_bool_t      useTestMode = false;

    runMode = NORMAL_MODE;

    useTestMode = modProvider_->get_fds_config()->get<bool>("fds.dm.testing.test_mode", false);
    if (useTestMode == true) {
        runMode = TEST_MODE;
    }
    LOGNORMAL << "Data Manager using control port "
              << MODULEPROVIDER()->getSvcMgr()->getSvcPort();

    setup_metasync_service();

    LOGNORMAL;
}

void DataMgr::mod_enable_service() {
    Error err(ERR_OK);
    const FdsRootDir *root = g_fdsprocess->proc_fdsroot();
    auto svcmgr = MODULEPROVIDER()->getSvcMgr();
    fds_uint32_t diskIOPsMin = features.isTestMode() ? 60*1000 :
            svcmgr->getSvcProperty<fds_uint32_t>(svcmgr->getMappedSelfPlatformUuid(),
                                                 "node_iops_min");

    // note that qos dispatcher in SM/DM uses total rate just to assign
    // guaranteed slots, it still will dispatch more IOs if there is more
    // perf capacity available (based on how fast IOs return). So setting
    // totalRate to node_iops_min does not actually restrict the SM from
    // servicing more IO if there is more capacity (eg.. because we have
    // cache and SSDs)
    scheduleRate = 2 * diskIOPsMin;
    LOGNOTIFY << "Will set totalRate to " << scheduleRate;

    /*
     *  init Data Manager  QOS class.
     */
    qosCtrl = new dmQosCtrl(this, qosThreadCount, FDS_QoSControl::FDS_DISPATCH_WFQ, GetLog());
    qosCtrl->runScheduler();

    // Create a queue for system (background) tasks
    sysTaskQueue = new FDS_VolumeQueue(1024, 10000, 20, FdsDmSysTaskPrio);
    sysTaskQueue->activate();
    err = qosCtrl->registerVolume(FdsDmSysTaskId, sysTaskQueue);
    fds_verify(err.ok());
    LOGNORMAL << "Registered System Task Queue";


    // create time volume catalog
    timeVolCat_ = DmTimeVolCatalog::ptr(new DmTimeVolCatalog("DM Time Volume Catalog",
                                                             *qosCtrl->threadPool,
                                                             *this));


    // create stats aggregator that aggregates stats for vols for which
    // this DM is primary
    statStreamAggr_ =
            StatStreamAggregator::ptr(new StatStreamAggregator("DM Stat Stream Aggregator",
                                                               modProvider_->get_fds_config(),
                                                               *this));

    // enable collection of local stats in DM
    StatsCollector::singleton()->setSvcMgr(MODULEPROVIDER()->getSvcMgr());
    if (!features.isTestMode()) {
        // since aggregator is in the same module, for stats that need to go to
        // local aggregator, we just directly stream to aggregator (not over network)
        StatsCollector::singleton()->startStreaming(
            std::bind(&DataMgr::sampleDMStats, this, std::placeholders::_1),
            std::bind(&DataMgr::handleLocalStatStream, this,
                      std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
        statStreamAggr_->mod_startup();

        // get DMT from OM if DMT already exist
        MODULEPROVIDER()->getSvcMgr()->getDMT();
    }
    // finish setting up time volume catalog
    timeVolCat_->mod_startup();

    // register expunge callback
    // TODO(Andrew): Move the expunge work down to the volume
    // catalog so this callback isn't needed
    timeVolCat_->queryIface()->registerExpungeObjectsCb(std::bind(
        &DataMgr::expungeObjectsIfPrimary, this,
        std::placeholders::_1, std::placeholders::_2));
    root->fds_mkdir(root->dir_sys_repo_dm().c_str());
    if (features.isTimelineEnabled()) {
        timeline->open();
    }

    // Register the DLT manager with service layer so that
    // outbound requests have the correct dlt_version.
    if (!features.isTestMode()) {
        gSvcRequestPool->setDltManager(MODULEPROVIDER()->getSvcMgr()->getDltManager());
    }
}

void DataMgr::shutdown()
{
    flushIO();
    mod_shutdown();
}

//   Block new IO's  and flush queued IO's
void DataMgr::flushIO()
{
    shuttingDown = true;
    for (auto it = vol_meta_map.begin();
         it != vol_meta_map.end();
         ++it) {
        qosCtrl->quieseceIOs(it->first);
    }

}

void DataMgr::mod_shutdown()
{
    // Don't double-free.
    {
        // The expected goofiness is due to c_e_s() setting "expected" to the current value if it
        // is not equal to expected. Which makes "expected" a misnomer as it's an in/out variable.
        // We don't care (or rather, we already know) what the current value is if it's not equal
        // to expected though, so we ignore it.
        bool expected = false;
        if (!shuttingDown.compare_exchange_strong(expected, true))
        {
            return;
        }
    }

    LOGNORMAL;
    statStreamAggr_->mod_shutdown();
    timeVolCat_->mod_shutdown();
    if (features.isCatSyncEnabled()) {
        catSyncMgr->mod_shutdown();
    } else {
        LOGWARN << "catalog sync feature - NOT enabled";
    }

    LOGNORMAL << "Destructing the Data Manager";
    closedmt_timer->destroy();

    for (auto it = vol_meta_map.begin();
         it != vol_meta_map.end();
         it++) {
        //  qosCtrl->quieseceIOs(it->first);
        qosCtrl->deregisterVolume(it->first);
        delete it->second;
    }
    vol_meta_map.clear();

    qosCtrl->deregisterVolume(FdsDmSysTaskId);
    delete sysTaskQueue;
    delete vol_map_mtx;
    delete qosCtrl;

    _shutdownGate.open();
}

void DataMgr::setup_metasync_service()
{
    catSyncMgr.reset(new CatalogSyncMgr(1, this, *this));
    // TODO(xxx) should we start catalog sync manager when no OM?
    catSyncMgr->mod_startup();
}

void DataMgr::swapMgrId(const FDS_ProtocolInterface::
                        FDSP_MsgHdrTypePtr& fdsp_msg) {
    FDS_ProtocolInterface::FDSP_MgrIdType temp_id;
    temp_id = fdsp_msg->dst_id;
    fdsp_msg->dst_id = fdsp_msg->src_id;
    fdsp_msg->src_id = temp_id;

    fds_uint64_t tmp_addr;
    tmp_addr = fdsp_msg->dst_ip_hi_addr;
    fdsp_msg->dst_ip_hi_addr = fdsp_msg->src_ip_hi_addr;
    fdsp_msg->src_ip_hi_addr = tmp_addr;

    tmp_addr = fdsp_msg->dst_ip_lo_addr;
    fdsp_msg->dst_ip_lo_addr = fdsp_msg->src_ip_lo_addr;
    fdsp_msg->src_ip_lo_addr = tmp_addr;

    fds_uint32_t tmp_port;
    tmp_port = fdsp_msg->dst_port;
    fdsp_msg->dst_port = fdsp_msg->src_port;
    fdsp_msg->src_port = tmp_port;
}

std::string DataMgr::getPrefix() const {
    return stor_prefix;
}

/*
 * Intended to be called while holding the vol_map_mtx.
 */
fds_bool_t DataMgr::volExistsLocked(fds_volid_t vol_uuid) const {
    if (vol_meta_map.count(vol_uuid) != 0) {
        return true;
    }
    return false;
}

fds_bool_t DataMgr::volExists(fds_volid_t vol_uuid) const {
    fds_bool_t result;
    vol_map_mtx->lock();
    result = volExistsLocked(vol_uuid);
    vol_map_mtx->unlock();

    return result;
}

/**
 * Checks the current DMT to determine if this DM primary
 * or not for a given volume.
 * topPrimary - If specified to be true, will check to see if
 * this current DM is the top element of the DMT column.
 */
fds_bool_t
DataMgr::_amIPrimaryImpl(fds_volid_t &volUuid, bool topPrimary) {
	if (MODULEPROVIDER()->getSvcMgr()->hasCommittedDMT()) {
		const DmtColumnPtr nodes = MODULEPROVIDER()->getSvcMgr()->getDMTNodesForVolume(volUuid);
		fds_verify(nodes->getLength() > 0);
		const fpi::SvcUuid myUuid (MODULEPROVIDER()->getSvcMgr()->getSelfSvcUuid());

        if (topPrimary) {
        	// Only the 0th element is considered top Primary
        	return (myUuid == nodes->get(0).toSvcUuid());
        } else {
        	// Anything else within number_of_primary is within primary group
        	const int numberOfPrimaryDMs = getNumOfPrimary();
        	fds_verify(numberOfPrimaryDMs > 0);
        	for (int i = 0; i < numberOfPrimaryDMs; i++) {
        		if (nodes->get(i).toSvcUuid() == myUuid) {
        			return true;
        		}
        	}
        }
    }
    return false;
}

fds_bool_t
DataMgr::amIPrimary(fds_volid_t volUuid) {
	return (_amIPrimaryImpl(volUuid, true));
}

fds_bool_t
DataMgr::amIPrimaryGroup(fds_volid_t volUuid) {
	return (_amIPrimaryImpl(volUuid, false));
}

/**
 * Make snapshot of volume catalog for sync and notify
 * CatalogSync.
 */
void
DataMgr::snapVolCat(dmCatReq *io) {
    Error err(ERR_OK);
    fds_verify(io != NULL);

    DmIoSnapVolCat *snapReq = static_cast<DmIoSnapVolCat*>(io);
    fds_verify(snapReq != NULL);

    if (io->io_type == FDS_DM_SNAPDELTA_VOLCAT) {
        // we are doing second rsync, set volume state to forward
        // We are doing this before we do catalog rsync, so some
        // updates that will make it to the persistent volume catalog
        // will also be sent to the destination node; this is ok
        // because we are serializing the updates to the same blob.
        // so they will also make it in order (assuming that sending
        // update A before update B will also cause receiving DM to receive
        // update A before update B -- otherwise we will have a race
        // which will also happen with other forwarded updates).
        vol_map_mtx->lock();
        fds_verify(vol_meta_map.count(snapReq->volId) > 0);
        VolumeMeta *vol_meta = vol_meta_map[snapReq->volId];
        // TODO(Andrew): We're setting the forwarding state separately from
        // taking the snapshot, which means that we'll start forwarding
        // data that will also be in the delta snapshot that we rsync, which
        // is incorrect. These two steps should be made atomic.
        vol_meta->setForwardInProgress();
        vol_map_mtx->unlock();
        LOGMIGRATE << "Starting 2nd (delta) rsync for volume " << std::hex
                   << snapReq->volId << " to node "
                   << (snapReq->node_uuid).uuid_get_val() << std::dec;
    } else {
        fds_verify(FDS_DM_SNAP_VOLCAT == io->io_type);
        LOGMIGRATE << "Starting 1st rsync for volume " << std::hex
                   << snapReq->volId << " to node "
                   << (snapReq->node_uuid).uuid_get_val() << std::dec;
    }

    // sync the catalog
    err = timeVolCat_->queryIface()->syncCatalog(snapReq->volId,
                                                 snapReq->node_uuid);

    // notify sync mgr that we did rsync
    snapReq->dmio_snap_vcat_cb(snapReq->volId, err);

    // mark this request as complete
    if (features.isQosEnabled()) qosCtrl->markIODone(*snapReq);
    delete snapReq;
}

/**
 * Populates an fdsp message header with stock fields.
 *
 * @param[in] Ptr to msg header to modify
 */
void
DataMgr::initSmMsgHdr(fpi::FDSP_MsgHdrTypePtr msgHdr) {
    msgHdr->minor_ver = 0;
    msgHdr->msg_id    = 1;

    msgHdr->major_ver = 0xa5;
    msgHdr->minor_ver = 0x5a;

    msgHdr->num_objects = 1;
    msgHdr->frag_len    = 0;
    msgHdr->frag_num    = 0;

    msgHdr->tennant_id      = 0;
    msgHdr->local_domain_id = 0;

    msgHdr->src_id = fpi::FDSP_DATA_MGR;
    msgHdr->dst_id = fpi::FDSP_STOR_MGR;

    auto svcmgr = MODULEPROVIDER()->getSvcMgr();
    msgHdr->src_node_name = svcmgr->getSelfSvcName();

    msgHdr->origin_timestamp = fds::get_fds_timestamp_ms();

    msgHdr->err_code = ERR_OK;
    msgHdr->result   = fpi::FDSP_ERR_OK;
}

/**
 * Issues delete calls for a set of objects in 'oids' list
 * if DM is primary for volume 'volid'
 */
Error
DataMgr::expungeObjectsIfPrimary(fds_volid_t volid,
                                 const std::vector<ObjectID>& oids) {
    Error err(ERR_OK);
    if (features.isTimelineEnabled()) return err; // No immediate deletes
    if (runMode == TEST_MODE) return err;  // no SMs, no one to notify
    if (amIPrimary(volid) == false) return err;  // not primary

    for (std::vector<ObjectID>::const_iterator cit = oids.cbegin();
         cit != oids.cend();
         ++cit) {
        err = expungeObject(volid, *cit);
        fds_verify(err == ERR_OK);
    }
    return err;
}

/**
 * Issues delete calls for an object when it is dereferenced
 * by a blob. Objects should only be expunged whenever a
 * blob's reference to a object is permanently removed.
 *
 * @param[in] The volume in which the obj is being deleted
 * @param[in] The object to expunge
 * return The result of the expunge
 */
Error
DataMgr::expungeObject(fds_volid_t volId, const ObjectID &objId) {
    Error err(ERR_OK);

    // Create message
    fpi::DeleteObjectMsgPtr expReq(new fpi::DeleteObjectMsg());

    // Set message parameters
    expReq->volId = volId.get();
    fds::assign(expReq->objId, objId);

    // Make RPC call
    DLTManagerPtr dltMgr = MODULEPROVIDER()->getSvcMgr()->getDltManager();
    // get DLT and increment refcount so that DM will respond to
    // DLT commit of the next DMT only after all deletes with this DLT complete
    const DLT* dlt = dltMgr->getAndLockCurrentDLT();

    auto asyncExpReq = gSvcRequestPool->newQuorumSvcRequest(
        boost::make_shared<DltObjectIdEpProvider>(dlt->getNodes(objId)));
    asyncExpReq->setPayload(FDSP_MSG_TYPEID(fpi::DeleteObjectMsg), expReq);
    asyncExpReq->setTimeoutMs(5000);
    asyncExpReq->onResponseCb(RESPONSE_MSG_HANDLER(DataMgr::expungeObjectCb,
                                                   dlt->getVersion()));
    asyncExpReq->invoke();
    return err;
}

/**
 * Callback for expungeObject call
 */
void
DataMgr::expungeObjectCb(fds_uint64_t dltVersion,
                         QuorumSvcRequest* svcReq,
                         const Error& error,
                         boost::shared_ptr<std::string> payload) {
    DBG(GLOGDEBUG << "Expunge cb called");
    DLTManagerPtr dltMgr = MODULEPROVIDER()->getSvcMgr()->getDltManager();
    dltMgr->decDLTRefcnt(dltVersion);
}

void DataMgr::InitMsgHdr(const fpi::FDSP_MsgHdrTypePtr& msg_hdr)
{
    msg_hdr->minor_ver = 0;
    msg_hdr->msg_code = fpi::FDSP_MSG_PUT_OBJ_REQ;
    msg_hdr->msg_id =  1;

    msg_hdr->major_ver = 0xa5;
    msg_hdr->minor_ver = 0x5a;

    msg_hdr->num_objects = 1;
    msg_hdr->frag_len = 0;
    msg_hdr->frag_num = 0;

    msg_hdr->tennant_id = 0;
    msg_hdr->local_domain_id = 0;

    msg_hdr->src_id = fpi::FDSP_DATA_MGR;
    msg_hdr->dst_id = fpi::FDSP_STOR_MGR;

    msg_hdr->src_node_name = "";

    msg_hdr->err_code = ERR_OK;
    msg_hdr->result = fpi::FDSP_ERR_OK;
}

void CloseDMTTimerTask::runTimerTask() {
    timeout_cb();
}

void DataMgr::setResponseError(fpi::FDSP_MsgHdrTypePtr& msg_hdr, const Error& err) {
    if (err.ok()) {
        msg_hdr->result  = fpi::FDSP_ERR_OK;
        msg_hdr->err_msg = "OK";
    } else {
        msg_hdr->result   = fpi::FDSP_ERR_FAILED;
        msg_hdr->err_msg  = "FDSP_ERR_FAILED";
        msg_hdr->err_code = err.GetErrno();
    }
}

namespace dmutil {

std::string getVolumeDir(fds_volid_t volId, fds_volid_t snapId) {
    const FdsRootDir* root = g_fdsprocess->proc_fdsroot();
    if (invalid_vol_id < snapId) {
        return util::strformat("%s/%ld/snapshot/%ld_vcat.ldb",
                               root->dir_user_repo_dm().c_str(),
                               volId.get(), snapId.get());
    } else {
        return util::strformat("%s/%ld",
                               root->dir_sys_repo_dm().c_str(), volId.get());
    }
}

// location of all snapshots for a volume
std::string getSnapshotDir(fds_volid_t volId) {
    const FdsRootDir* root = g_fdsprocess->proc_fdsroot();
    return util::strformat("%s/%ld/snapshot",
                           root->dir_user_repo_dm().c_str(), volId.get());
}

std::string getLevelDBFile(fds_volid_t volId, fds_volid_t snapId) {
    const FdsRootDir* root = g_fdsprocess->proc_fdsroot();
    if (invalid_vol_id < snapId) {
        return util::strformat("%s/%ld/snapshot/%ld_vcat.ldb",
                                 root->dir_user_repo_dm().c_str(), volId.get(), snapId);
    } else {
        return util::strformat("%s/%ld/%ld_vcat.ldb",
                                 root->dir_sys_repo_dm().c_str(), volId, volId);
    }
}
}  // namespace dmutil



}  // namespace fds
