/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#include <string>
#include <list>
#include <vector>
#include <net/net_utils.h>
#include <fds_timestamp.h>
#include <fdsp_utils.h>
#include <lib/StatsCollector.h>
#include <DataMgr.h>
#include <net/net-service.h>
#include <net/net-service-tmpl.hpp>

namespace fds {

Error DataMgr::vol_handler(fds_volid_t vol_uuid,
                           VolumeDesc *desc,
                           fds_vol_notify_t vol_action,
                           fpi::FDSP_NotifyVolFlag vol_flag,
                           fpi::FDSP_ResultType result) {
    Error err(ERR_OK);
    GLOGNORMAL << "Received vol notif from OM for "
               << desc->getName() << ":"
               << std::hex << vol_uuid << std::dec;

    if (vol_action == fds_notify_vol_add) {
        /*
         * TODO: Actually take a volume string name, not
         * just the volume number.
         */
        err = dataMgr->_process_add_vol(dataMgr->getPrefix() +
                                        std::to_string(vol_uuid),
                                        vol_uuid, desc,
                                        (vol_flag == fpi::FDSP_NOTIFY_VOL_WILL_SYNC));
    } else if (vol_action == fds_notify_vol_rm) {
        err = dataMgr->process_rm_vol(vol_uuid, vol_flag == fpi::FDSP_NOTIFY_VOL_CHECK_ONLY);
    } else if (vol_action == fds_notify_vol_mod) {
        err = dataMgr->_process_mod_vol(vol_uuid, *desc);
    } else {
        assert(0);
    }
    return err;
}

void DataMgr::node_handler(fds_int32_t  node_id,
                           fds_uint32_t node_ip,
                           fds_int32_t  node_st,
                           fds_uint32_t node_port,
                           FDS_ProtocolInterface::FDSP_MgrIdType node_type) {
}

Error
DataMgr::volcat_evt_handler(fds_catalog_action_t catalog_action,
                            const FDS_ProtocolInterface::FDSP_PushMetaPtr& push_meta,
                            const std::string& session_uuid) {
    Error err(ERR_OK);
    OMgrClient* om_client = dataMgr->omClient;
    GLOGNORMAL << "Received Volume Catalog request";
    if (catalog_action == fds_catalog_push_meta) {
        err = dataMgr->catSyncMgr->startCatalogSync(push_meta->metaVol, om_client, session_uuid);
    } else if (catalog_action == fds_catalog_dmt_commit) {
        // thsi will ignore this msg if catalog sync is not in progress
        err = dataMgr->catSyncMgr->startCatalogSyncDelta(session_uuid);
    } else if (catalog_action == fds_catalog_dmt_close) {
        // will finish forwarding when all queued updates are processed
        GLOGNORMAL << "Received DMT Close";
        err = dataMgr->notifyDMTClose();
    } else {
        fds_assert(!"Unknown catalog command");
    }
    return err;
}

/**
 * Receiver DM processing of volume sync state.
 * @param[in] fdw_complete false if rsync is completed = start processing
 * forwarded updates (activate shadow queue); true if forwarding updates
 * is completed (activate volume queue)
 */
Error
DataMgr::processVolSyncState(fds_volid_t volume_id, fds_bool_t fwd_complete) {
    Error err(ERR_OK);
    LOGNORMAL << "DM received volume sync state for volume " << std::hex
              << volume_id << std::dec << " forward complete? " << fwd_complete;

    if (!fwd_complete) {
        // open catalogs so we can start processing updates
        err = timeVolCat_->activateVolume(volume_id);
        if (err.ok()) {
            // start processing forwarded updates
            catSyncRecv->startProcessFwdUpdates(volume_id);
        }
    } else {
        LOGDEBUG << "Will queue DmIoMetaRecvd message "
                 << std::hex << volume_id << std::dec;
        DmIoMetaRecvd* metaRecvdIo = new(std::nothrow) DmIoMetaRecvd(volume_id);
        err = enqueueMsg(catSyncRecv->shadowVolUuid(volume_id), metaRecvdIo);
        fds_verify(err.ok());
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
    catSyncRecv->handleFwdDone(io->volId);

    vol_map_mtx->lock();
    fds_verify(vol_meta_map.count(io->volId) > 0);
    VolumeMeta *vol_meta = vol_meta_map[io->volId];
    vol_meta->dmVolQueue->activate();
    vol_map_mtx->unlock();

    qosCtrl->markIODone(*io);
    delete io;
}

void DataMgr::handleStatStream(dmCatReq *io) {
    DmIoStatStream *statStreamReq = static_cast<DmIoStatStream*>(io);

    Error err = statStreamAggr_->handleModuleStatStream(statStreamReq->statStreamMsg);

    qosCtrl->markIODone(*io);
    statStreamReq->dmio_statstream_resp_cb(err, statStreamReq);
}

//
// Called by stats collector to sample DM specific stats
//
void DataMgr::sampleDMStats(fds_uint64_t timestamp) {
    LOGTRACE << "Sampling DM stats";
    Error err(ERR_OK);
    std::unordered_set<fds_volid_t> prim_vols;
    std::unordered_set<fds_volid_t>::const_iterator cit;
    std::unordered_map<fds_uint64_t, VolumeMeta*>::iterator vol_it;
    fds_uint64_t total_bytes = 0;
    fds_uint64_t total_blobs = 0;
    fds_uint64_t total_objects = 0;
    const NodeUuid *mySvcUuid = modProvider_->get_plf_manager()->plf_get_my_svc_uuid();

    // find all volumes for which this DM is primary (we only need
    // to collect capacity stats from DMs that are primary).
    vol_map_mtx->lock();
    for (vol_it = vol_meta_map.begin();
         vol_it != vol_meta_map.end();
         ++vol_it) {
        DmtColumnPtr nodes = omClient->getDMTNodesForVolume(vol_it->first);
        fds_verify(nodes->getLength() > 0);
        if (*mySvcUuid == nodes->get(0)) {
            prim_vols.insert(vol_it->first);
        }
    }
    vol_map_mtx->unlock();

    // collect capacity stats for volumes for which this DM is primary
    for (cit = prim_vols.cbegin(); cit != prim_vols.cend(); ++cit) {
        err = timeVolCat_->queryIface()->getVolumeMeta(*cit,
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
    std::unordered_map<fds_uint64_t, VolumeMeta*>::iterator vol_it;
    std::unordered_set<fds_volid_t> done_vols;
    fds_bool_t all_finished = false;

    LOGNOTIFY << "Finish handling DMT close and send ack if not sent yet";

    // move all volumes that are in 'finish forwarding' state
    // to not forwarding state, and make a set of these volumes
    // the set is used so that we call catSyncMgr to cleanup/and
    // and send push meta done msg outside of vol_map_mtx lock
    vol_map_mtx->lock();
    for (vol_it = vol_meta_map.begin();
         vol_it != vol_meta_map.end();
         ++vol_it) {
        fds_volid_t volid = vol_it->first;
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
        if (catSyncMgr->finishedForwardVolmeta(*it)) {
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
DataMgr::createSnapshot(const fpi::Snapshot & snapDetails) {
    VolumeMeta * volmeta = 0;
    VolumeMeta * snapmeta = 0;
    {
        FDSGUARD(*vol_map_mtx);
        volmeta = vol_meta_map[snapDetails.volumeId];
        snapmeta = vol_meta_map[snapDetails.snapshotId];

        if (!volmeta) {
            GLOGWARN << "Volume '" << std::hex << snapDetails.volumeId << std::dec <<
                    "' not found!";
            return ERR_NOT_FOUND;
        }

        if (snapmeta) {
            GLOGWARN << "Snapshot '" << std::hex << snapDetails.snapshotId << std::dec <<
                    "' already exists!";
            return FDSN_StatusErrorBucketAlreadyExists;
        }
    }

    VolumeDesc snapdesc(*volmeta->vol_desc);
    snapdesc.volUUID = snapDetails.snapshotId;
    snapdesc.name = snapDetails.snapshotName;
    snapdesc.parentVolumeId = snapDetails.volumeId;
    snapdesc.ctime = boost::posix_time::from_time_t(snapDetails.creationTimestamp);
    snapdesc.fSnapshot = true;

    Error rc = timeVolCat_->createSnapshot(*volmeta->vol_desc, snapdesc);
    if (!rc.ok()) {
        GLOGERROR << "Failed to create a snapshot '" << snapDetails.snapshotName << "'";
        return rc;
    }
    rc = timeVolCat_->activateVolume(snapdesc.volUUID);

    snapmeta = new VolumeMeta(snapdesc.name, snapdesc.volUUID, GetLog(), &snapdesc);

    snapmeta->dmVolQueue = new FDS_VolumeQueue(4096, snapdesc.iops_max, 2 * snapdesc.iops_min,
            snapdesc.relativePrio);
    snapmeta->dmVolQueue->activate();

    GLOGDEBUG << "Added vol meta for vol uuid and per Volume queue '" << std::hex <<
            snapdesc.volUUID << std::dec << "'";

    FDSGUARD(*vol_map_mtx);

    rc = dataMgr->qosCtrl->registerVolume(snapdesc.volUUID, static_cast<FDS_VolumeQueue*>(
            snapmeta->dmVolQueue));
    if (!rc.ok()) {
        delete snapmeta;
        return rc;
    }
    vol_meta_map[snapdesc.volUUID] = snapmeta;

    return rc;
}

Error
DataMgr::deleteSnapshot(const fds_uint64_t snapshotId) {
    // TODO(umesh): implement this
    return ERR_OK;
}

//
// handle finish forward for volume 'volid'
//
void DataMgr::finishForwarding(fds_volid_t volid) {
    fds_bool_t all_finished = false;

    // set the state
    vol_map_mtx->lock();
    VolumeMeta *vol_meta = vol_meta_map[volid];
    fds_verify(vol_meta->isForwarding());
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

    // create vol catalogs, etc first
    err = timeVolCat_->addVolume(*vdesc);
    if (err.ok() && !vol_will_sync) {
        // not going to sync this volume, activate volume
        // so that we can do get/put/del cat ops to this volume
        err = timeVolCat_->activateVolume(vol_uuid);
    }
    if (!err.ok()) {
        LOGERROR << "Failed to add volume " << std::hex << vol_uuid << std::dec;
        return err;
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
    volmeta->dmVolQueue = new(std::nothrow) FDS_VolumeQueue(4096,
                                                            vdesc->iops_max,
                                                            2*vdesc->iops_min,
                                                            vdesc->relativePrio);
    if (!volmeta->dmVolQueue) {
        LOGERROR << "Failed to allocate Qos queue for volume "
                 << std::hex << vol_uuid << std::dec;
        delete volmeta;
        return ERR_OUT_OF_MEMORY;
    }
    volmeta->dmVolQueue->activate();

    LOGDEBUG << "Added vol meta for vol uuid and per Volume queue" << std::hex
              << vol_uuid << std::dec << ", created catalogs? " << !vol_will_sync;

    vol_map_mtx->lock();
    err = dataMgr->qosCtrl->registerVolume(vol_uuid,
                                           static_cast<FDS_VolumeQueue*>(
                                               volmeta->dmVolQueue));
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
            err = dataMgr->qosCtrl->registerVolume(shadow_volid, shadowVolQueue);
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
        // we registered queue and shadow queue if needed
        vol_meta_map[vol_uuid] = volmeta;
    }
    vol_map_mtx->unlock();

    if (!err.ok()) {
        // cleanup volmeta and deregister queue
        LOGERROR << "Cleaning up volume queue and vol meta because of error "
                 << " volid 0x" << std::hex << vol_uuid << std::dec;
        dataMgr->qosCtrl->deregisterVolume(vol_uuid);
        delete volmeta->dmVolQueue;
        delete volmeta;
    }

    if (err.ok() && amIPrimary(vol_uuid)) {
        // will aggregate stats for this volume and log them
        statStreamAggr_->attachVolume(vol_uuid);
        // create volume stat  directory.
        const FdsRootDir *root = g_fdsprocess->proc_fdsroot();
        const std::string stat_dir = root->dir_user_repo_stats() + std::to_string(vol_uuid);
        std::system((const char *)("mkdir -p "+stat_dir+" ").c_str());
    }

    return err;
}

Error DataMgr::_process_add_vol(const std::string& vol_name,
                                fds_volid_t vol_uuid,
                                VolumeDesc *desc,
                                fds_bool_t vol_will_sync) {
    Error err(ERR_OK);

    /*
     * Verify that we don't already know about this volume
     */
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
    vm->vol_desc->modifyPolicyInfo(2*voldesc.iops_min, voldesc.iops_max, voldesc.relativePrio);
    err = qosCtrl->modifyVolumeQosParams(vol_uuid,
                                         2*voldesc.iops_min,
                                         voldesc.iops_max,
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
        // prevent futher updates to the volume as we are going
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
        // another notify volume delete with check_only ==false to
        // actually cleanup all other datastructures for this volume
        return err;
    }

    // we we are here, check_only == false
    err = timeVolCat_->deleteEmptyVolume(vol_uuid);
    if (err.ok()) {
        VolumeMeta* vol_meta = NULL;
        dataMgr->qosCtrl->deregisterVolume(vol_uuid);
        vol_map_mtx->lock();
        if (vol_meta_map.count(vol_uuid) > 0) {
            vol_meta = vol_meta_map[vol_uuid];
            vol_meta_map.erase(vol_uuid);
        }
        vol_map_mtx->unlock();
        if (vol_meta) {
            delete vol_meta->dmVolQueue;
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

/**
 * For all volumes that are in forwarding state, move them to
 * finish forwarding state.
 */
Error DataMgr::notifyDMTClose() {
    std::unordered_map<fds_uint64_t, VolumeMeta*>::iterator vol_it;
    Error err(ERR_OK);
    DmIoPushMetaDone* pushMetaDoneIo = NULL;

    if (!catSyncMgr->isSyncInProgress()) {
        err = ERR_CATSYNC_NOT_PROGRESS;
        return err;
    }

    // set DMT close time in catalog sync mgr
    catSyncMgr->setDmtCloseNow();

    // set every volume's state to 'finish forwarding
    // and enqueue DMT close marker to qos queues of volumes
    // that are in 'finish forwarding' state
    vol_map_mtx->lock();
    for (vol_it = vol_meta_map.begin();
         vol_it != vol_meta_map.end();
         ++vol_it) {
        VolumeMeta *vol_meta = vol_it->second;
        vol_meta->finishForwarding();
        pushMetaDoneIo = new DmIoPushMetaDone(vol_it->first);
        err = enqueueMsg(vol_it->first, pushMetaDoneIo);
        fds_verify(err.ok());
    }
    vol_map_mtx->unlock();

    // we will stop forwarding for each volume when all transactions
    // open before time we received dmt close get committed or aborted
    // but we timeout to ensure we send DMT close ack
    if (!closedmt_timer->schedule(closedmt_timer_task, std::chrono::seconds(60))) {
        // TODO(xxx) how do we handle this?
        // fds_panic("Failed to schedule closedmt timer!");
        LOGWARN << "Failed to schedule close dmt timer task";
    }
    return err;
}

//
// Handle DMT close for the volume in io req -- sends
// push meta done to destination DM
//
void DataMgr::handleDMTClose(dmCatReq *io) {
    DmIoPushMetaDone *pushMetaDoneReq = static_cast<DmIoPushMetaDone*>(io);

    LOGDEBUG << "Processed all commits that arrived before DMT close "
             << "will now notify dst DM to open up volume queues: vol "
             << std::hex << pushMetaDoneReq->volId << std::dec;

    Error err = catSyncMgr->issueServiceVolumeMsg(pushMetaDoneReq->volId);
    // TODO(Anna) if err, send DMT close ack with error???
    fds_verify(err.ok());

    // If there are no open transactions that started before we got
    // DMT close, we can finish forwarding right now
    if (!timeVolCat_->isPendingTx(pushMetaDoneReq->volId, catSyncMgr->dmtCloseTs())) {
        finishForwarding(pushMetaDoneReq->volId);
    }

    qosCtrl->markIODone(*pushMetaDoneReq);
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
    // NOTE: Don't put much stuff in the constuctor.  Move any construction
    // into mod_init()
}

int DataMgr::mod_init(SysParams const *const param)
{
    Error err(ERR_OK);

    omConfigPort = 0;
    use_om = true;
    numTestVols = 10;
    scheduleRate = 10000;

    counters_.reset(new FdsCounters("DM", modProvider_->get_cntrs_mgr().get()));

    catSyncRecv = boost::make_shared<CatSyncReceiver>(this);
    closedmt_timer = boost::make_shared<FdsTimer>();
    closedmt_timer_task = boost::make_shared<CloseDMTTimerTask>(*closedmt_timer,
                                                                std::bind(&DataMgr::finishCloseDMT,
                                                                          this));
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

    vol_map_mtx = new fds_mutex("Volume map mutex");

    /*
     * Comm with OM will be setup during run()
     */
    omClient = NULL;

    LOGNORMAL << "Completed";
    return 0;
}

void DataMgr::initHandlers() {
    handlers[FDS_LIST_BLOB]   = new dm::GetBucketHandler();
    handlers[FDS_DELETE_BLOB] = new dm::DeleteBlobHandler();
}

DataMgr::~DataMgr()
{
    // TODO(Rao): Move this code into mod_shutdown
    LOGNORMAL << "Destructing the Data Manager";

    closedmt_timer->destroy();

    for (std::unordered_map<fds_uint64_t, VolumeMeta*>::iterator
                 it = vol_meta_map.begin();
         it != vol_meta_map.end();
         it++) {
        delete it->second;
    }
    vol_meta_map.clear();

    qosCtrl->deregisterVolume(FdsDmSysTaskId);
    delete sysTaskQueue;

    delete omClient;
    delete vol_map_mtx;
    delete qosCtrl;
}

int DataMgr::run()
{
    // TODO(Rao): Move this into module init
    initHandlers();
    try {
        nstable->listenServer(metadatapath_session);
    }
    catch(...){
        std::cout << "starting server threw an exception" << std::endl;
    }
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

    // Get config values from that platform lib.
    //
    omConfigPort = modProvider_->get_plf_manager()->plf_get_om_ctrl_port();
    omIpStr      = *modProvider_->get_plf_manager()->plf_get_om_ip();

    use_om = !(modProvider_->get_fds_config()->get<bool>("fds.dm.no_om", false));
    useTestMode = modProvider_->get_fds_config()->get<bool>("fds.dm.testing.test_mode", false);
    if (useTestMode == true) {
        runMode = TEST_MODE;
    }
    LOGNORMAL << "Data Manager using control port "
        << modProvider_->get_plf_manager()->plf_get_my_ctrl_port();

    /* Set up FDSP RPC endpoints */
    nstable = boost::shared_ptr<netSessionTbl>(new netSessionTbl(FDSP_DATA_MGR));
    myIp = net::get_local_ip(modProvider_->get_fds_config()->get<std::string>("fds.nic_if"));
    assert(myIp.empty() == false);
    std::string node_name = "_DM_" + myIp;

    LOGNORMAL << "Data Manager using IP:"
        << myIp << " and node name " << node_name;

    setup_metadatapath_server(myIp);

    if (use_om) {
        LOGDEBUG << " Initialising the OM client ";
        /*
         * Setup communication with OM.
         */
        omClient = new OMgrClient(FDSP_DATA_MGR,
                                  omIpStr,
                                  omConfigPort,
                                  node_name,
                                  GetLog(),
                                  nstable,
                                  modProvider_->get_plf_manager());
        omClient->initialize();
        omClient->registerEventHandlerForNodeEvents(node_handler);
        omClient->registerEventHandlerForVolEvents(vol_handler);
        omClient->registerCatalogEventHandler(volcat_evt_handler);
        /*
         * Brings up the control path interface.
         * This does not require OM to be running and can
         * be used for testing DM by itself.
         */
        omClient->startAcceptingControlMessages();

        /*
         * Registers the DM with the OM. Uses OM for bootstrapping
         * on start. Requires the OM to be up and running prior.
         */
        omClient->registerNodeWithOM(modProvider_->get_plf_manager());
    }

    setup_metasync_service();

    LOGNORMAL;
}

void DataMgr::mod_enable_service() {
    Error err(ERR_OK);
    const FdsRootDir *root = g_fdsprocess->proc_fdsroot();
    const NodeUuid *mySvcUuid = Platform::plf_get_my_svc_uuid();
    NodeAgent::pointer my_agent = Platform::plf_dm_nodes()->agent_info(*mySvcUuid);
    fpi::StorCapMsg stor_cap;
    my_agent->init_stor_cap_msg(&stor_cap);
    LOGNOTIFY << "Will set totalRate to " << stor_cap.disk_iops_min;

    // note that qos dispatcher in SM/DM uses total rate just to assign
    // guaranteed slots, it still will dispatch more IOs if there is more
    // perf capacity available (based on how fast IOs return). So setting
    // totalRate to disk_iops_min does not actually restrict the SM from
    // servicing more IO if there is more capacity (eg.. because we have
    // cache and SSDs)
    scheduleRate = 2*stor_cap.disk_iops_min;

    /*
     *  init Data Manager  QOS class.
     */
    qosCtrl = new dmQosCtrl(this, 50, FDS_QoSControl::FDS_DISPATCH_WFQ, GetLog());
    qosCtrl->runScheduler();

    // Create a queue for system (background) tasks
    sysTaskQueue = new FDS_VolumeQueue(1024, 10000, 20, FdsDmSysTaskPrio);
    sysTaskQueue->activate();
    err = qosCtrl->registerVolume(FdsDmSysTaskId, sysTaskQueue);
    fds_verify(err.ok());
    LOGNORMAL << "Registered System Task Queue";


    // create time volume catalog
    timeVolCat_ = DmTimeVolCatalog::ptr(new
                                        DmTimeVolCatalog("DM Time Volume Catalog",
                                                         *qosCtrl->threadPool));

    // create stats aggregator that aggregates stats for vols for which
    // this DM is primary
    statStreamAggr_ = StatStreamAggregator::ptr(
        new StatStreamAggregator("DM Stat Stream Aggregator", modProvider_->get_fds_config()));

    // enable collection of local stats in DM
    StatsCollector::singleton()->registerOmClient(omClient);
    // since aggregator is in the same module, for stats that need to go to
    // local aggregator, we just directly stream to aggregator (not over network)
    StatsCollector::singleton()->startStreaming(
        std::bind(&DataMgr::sampleDMStats, this, std::placeholders::_1),
        std::bind(&DataMgr::handleLocalStatStream, this,
                  std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

    if (runMode == TEST_MODE) {
        // Create test volumes.
        std::string testVolName;
        VolumeDesc*  testVdb;
        for (fds_uint32_t testVolId = 1; testVolId < numTestVols + 1; testVolId++) {
            testVolName = "testVol" + std::to_string(testVolId);
            // We're using the ID as the min/max/priority
            // for the volume QoS.
            testVdb = new VolumeDesc(testVolName,
                                     testVolId,
                                     testVolId,
                                     testVolId * 2,
                                     testVolId);
            fds_assert(testVdb != NULL);
            vol_handler(testVolId,
                        testVdb,
                        fds_notify_vol_add,
                        fpi::FDSP_NOTIFY_VOL_NO_FLAG,
                        FDS_ProtocolInterface::FDSP_ERR_OK);
            delete testVdb;
        }
    }

    // finish setting up time volume catalog
    timeVolCat_->mod_startup();

    // register expunge callback
    // TODO(Andrew): Move the expunge work down to the volume
    // catalog so this callback isn't needed
    timeVolCat_->queryIface()->registerExpungeObjectsCb(std::bind(
            &DataMgr::expungeObjectsIfPrimary, this,
            std::placeholders::_1, std::placeholders::_2));

    statStreamAggr_->mod_startup();
    root->fds_mkdir(root->dir_user_repo_dm().c_str());
}

void DataMgr::mod_shutdown()
{
    LOGNORMAL;
    catSyncMgr->mod_shutdown();
    timeVolCat_->mod_shutdown();
    statStreamAggr_->mod_shutdown();
}

void DataMgr::setup_metadatapath_server(const std::string &ip)
{
    metadatapath_handler.reset(new ReqHandler());

    int myIpInt = netSession::ipString2Addr(ip);
    std::string node_name = "_DM_" + ip;
    // TODO(Andrew): Ideally createServerSession should take a shared pointer
    // for datapath_handler.  Make sure that happens.  Otherwise you
    // end up with a pointer leak.
    // TODO(Andrew): Figure out who cleans up datapath_session_
    metadatapath_session = nstable->\
            createServerSession<netMetaDataPathServerSession>(
                myIpInt,
                modProvider_->get_plf_manager()->plf_get_my_data_port(),
                node_name,
                FDSP_STOR_HVISOR,
                metadatapath_handler);
}

void DataMgr::setup_metasync_service()
{
    catSyncMgr.reset(new CatalogSyncMgr(1, this));
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

DataMgr::ReqHandler::ReqHandler() {
}

DataMgr::ReqHandler::~ReqHandler() {
}

void DataMgr::ReqHandler::GetVolumeBlobList(FDSP_MsgHdrTypePtr& msg_hdr,
                                            FDSP_GetVolumeBlobListReqTypePtr& blobListReq) {
    Error err(ERR_OK);
    fds_panic("must not get here");
}

/**
 * Checks the current DMT to determine if this DM primary
 * or not for a given volume.
 */
fds_bool_t
DataMgr::amIPrimary(fds_volid_t volUuid) {
    DmtColumnPtr nodes = omClient->getDMTNodesForVolume(volUuid);
    fds_verify(nodes->getLength() > 0);

    const NodeUuid *mySvcUuid = modProvider_->get_plf_manager()->plf_get_my_svc_uuid();
    return (*mySvcUuid == nodes->get(0));
}

void DataMgr::startBlobTx(dmCatReq *io)
{
    Error err(ERR_OK);
    DmIoStartBlobTx *startBlobReq= static_cast<DmIoStartBlobTx*>(io);

    LOGTRACE << "Will start transaction " << *startBlobReq;

    // TODO(Anna) If this DM is not forwarding for this io's volume anymore
    // we reject start TX on DMT mismatch
    vol_map_mtx->lock();
    if (vol_meta_map.count(startBlobReq->volId) > 0) {
        VolumeMeta* vol_meta = vol_meta_map[startBlobReq->volId];
        if ((!vol_meta->isForwarding() || vol_meta->isForwardFinishing()) &&
            (startBlobReq->dmt_version != omClient->getDMTVersion())) {
            PerfTracer::incr(startBlobReq->opReqFailedPerfEventType, startBlobReq->getVolId(),
                    startBlobReq->perfNameStr);
            err = ERR_IO_DMT_MISMATCH;
        }
    } else {
        PerfTracer::incr(startBlobReq->opReqFailedPerfEventType, startBlobReq->getVolId(),
                startBlobReq->perfNameStr);
        err = ERR_VOL_NOT_FOUND;
    }
    vol_map_mtx->unlock();

    if (err.ok()) {
        err = timeVolCat_->startBlobTx(startBlobReq->volId,
                                       startBlobReq->blob_name,
                                       startBlobReq->blob_mode,
                                       startBlobReq->ioBlobTxDesc);
        if (!err.ok()) {
            PerfTracer::incr(startBlobReq->opReqFailedPerfEventType, startBlobReq->getVolId(),
                    startBlobReq->perfNameStr);
        }
    }
    qosCtrl->markIODone(*startBlobReq);
    PerfTracer::tracePointEnd(startBlobReq->opLatencyCtx);
    PerfTracer::tracePointEnd(startBlobReq->opReqLatencyCtx);
    startBlobReq->dmio_start_blob_tx_resp_cb(err, startBlobReq);
}

void DataMgr::updateCatalog(dmCatReq *io)
{
    DmIoUpdateCat *updCatReq= static_cast<DmIoUpdateCat*>(io);
    Error err = timeVolCat_->updateBlobTx(updCatReq->volId,
                                    updCatReq->ioBlobTxDesc,
                                    updCatReq->obj_list);
    if (!err.ok()) {
        PerfTracer::incr(updCatReq->opReqFailedPerfEventType, updCatReq->getVolId(),
                updCatReq->perfNameStr);
    }
    qosCtrl->markIODone(*updCatReq);
    PerfTracer::tracePointEnd(updCatReq->opLatencyCtx);
    PerfTracer::tracePointEnd(updCatReq->opReqLatencyCtx);
    updCatReq->dmio_updatecat_resp_cb(err, updCatReq);
}

void
DataMgr::updateCatalogOnce(dmCatReq *io) {
    DmIoUpdateCatOnce *updCatReq= static_cast<DmIoUpdateCatOnce*>(io);
    // Start the transaction
    Error err = timeVolCat_->startBlobTx(updCatReq->volId,
                                         updCatReq->blob_name,
                                         updCatReq->updcatMsg->blob_mode,
                                         updCatReq->ioBlobTxDesc);
    if (err != ERR_OK) {
        qosCtrl->markIODone(*updCatReq);
        PerfTracer::incr(updCatReq->opReqFailedPerfEventType, updCatReq->getVolId(),
                         updCatReq->perfNameStr);
        PerfTracer::tracePointEnd(updCatReq->opLatencyCtx);
        PerfTracer::tracePointEnd(updCatReq->opReqLatencyCtx);
        updCatReq->dmio_updatecat_resp_cb(err, updCatReq);
        return;
    }

    // Apply the offset updates
    err = timeVolCat_->updateBlobTx(updCatReq->volId,
                                    updCatReq->ioBlobTxDesc,
                                    updCatReq->updcatMsg->obj_list);
    if (err != ERR_OK) {
        qosCtrl->markIODone(*updCatReq);
        PerfTracer::incr(updCatReq->opReqFailedPerfEventType, updCatReq->getVolId(),
                         updCatReq->perfNameStr);
        PerfTracer::tracePointEnd(updCatReq->opLatencyCtx);
        PerfTracer::tracePointEnd(updCatReq->opReqLatencyCtx);
        updCatReq->dmio_updatecat_resp_cb(err, updCatReq);
        return;
    }

    // Apply the metadata updates
    err = timeVolCat_->updateBlobTx(updCatReq->volId,
                                    updCatReq->ioBlobTxDesc,
                                    updCatReq->updcatMsg->meta_list);
    if (err != ERR_OK) {
        qosCtrl->markIODone(*updCatReq);
        PerfTracer::incr(updCatReq->opReqFailedPerfEventType, updCatReq->getVolId(),
                         updCatReq->perfNameStr);
        PerfTracer::tracePointEnd(updCatReq->opLatencyCtx);
        PerfTracer::tracePointEnd(updCatReq->opReqLatencyCtx);
        updCatReq->dmio_updatecat_resp_cb(err, updCatReq);
        return;
    }

    // Commit the metadata updates
    // The commit callback we pass in will actually call the
    // final service callback
    PerfTracer::tracePointBegin(updCatReq->commitBlobReq->opLatencyCtx);
    err = timeVolCat_->commitBlobTx(updCatReq->volId,
                                    updCatReq->blob_name,
                                    updCatReq->ioBlobTxDesc,
                                    std::bind(&DataMgr::commitBlobTxCb, this,
                                              std::placeholders::_1, std::placeholders::_2,
                                              std::placeholders::_3, std::placeholders::_4,
                                              updCatReq->commitBlobReq));
    if (err != ERR_OK) {
        qosCtrl->markIODone(*updCatReq);
        PerfTracer::incr(updCatReq->opReqFailedPerfEventType, updCatReq->getVolId(),
                         updCatReq->perfNameStr);
        PerfTracer::tracePointEnd(updCatReq->opLatencyCtx);
        PerfTracer::tracePointEnd(updCatReq->opReqLatencyCtx);
        updCatReq->dmio_updatecat_resp_cb(err, updCatReq);
        return;
    }
}

void DataMgr::commitBlobTx(dmCatReq *io)
{
    Error err;
    DmIoCommitBlobTx *commitBlobReq = static_cast<DmIoCommitBlobTx*>(io);

    LOGTRACE << "Will commit blob " << commitBlobReq->blob_name << " to tvc";
    err = timeVolCat_->commitBlobTx(commitBlobReq->volId,
                                    commitBlobReq->blob_name,
                                    commitBlobReq->ioBlobTxDesc,
                                    // TODO(Rao): We should use a static commit callback
                                    std::bind(&DataMgr::commitBlobTxCb, this,
                                              std::placeholders::_1, std::placeholders::_2,
                                              std::placeholders::_3, std::placeholders::_4,
                                              commitBlobReq));
    if (err != ERR_OK) {
        PerfTracer::incr(commitBlobReq->opReqFailedPerfEventType, commitBlobReq->getVolId(),
                commitBlobReq->perfNameStr);
        qosCtrl->markIODone(*commitBlobReq);
        PerfTracer::tracePointEnd(io->opLatencyCtx);
        PerfTracer::tracePointEnd(io->opReqLatencyCtx);
        commitBlobReq->dmio_commit_blob_tx_resp_cb(err, commitBlobReq);
    }
}

/**
 * Callback from volume catalog when transaction is commited
 * @param[in] version of blob that was committed or in case of deletion,
 * a version that was deleted
 * @param[in] blob_obj_list list of offset to object mapping that
 * was committed or NULL
 * @param[in] meta_list list of metadata k-v pairs that was committed or NULL
 */
void DataMgr::commitBlobTxCb(const Error &err,
                             blob_version_t blob_version,
                             const BlobObjList::const_ptr& blob_obj_list,
                             const MetaDataList::const_ptr& meta_list,
                             DmIoCommitBlobTx *commitBlobReq)
{
    Error error(err);
    fds_bool_t forwarded_commit = false;
    LOGDEBUG << "Dmt version: " << commitBlobReq->dmt_version << " blob "
             << commitBlobReq->blob_name << " vol " << std::hex
             << commitBlobReq->volId << std::dec << " ; current DMT version "
             << omClient->getDMTVersion();

    // 'finish this io' for qos accounting purposes, if we are
    // forwarding, the main time goes to waiting for response
    // from another DM, which is not really consuming local
    // DM resources
    qosCtrl->markIODone(*commitBlobReq);

    PerfTracer::tracePointEnd(commitBlobReq->opLatencyCtx);
    PerfTracer::tracePointEnd(commitBlobReq->opReqLatencyCtx);

    if (!err.ok()) {
        PerfTracer::incr(commitBlobReq->opReqFailedPerfEventType, commitBlobReq->getVolId(),
                commitBlobReq->perfNameStr);
    }

    // do forwarding if needed and commit was successfull
    if (error.ok() &&
        (commitBlobReq->dmt_version != dataMgr->omClient->getDMTVersion())) {
        VolumeMeta* vol_meta = NULL;
        fds_bool_t is_forwarding = false;

        // check if we need to forward this commit to receiving
        // DM if we are in progress of migrating volume catalog
        vol_map_mtx->lock();
        fds_verify(vol_meta_map.count(commitBlobReq->volId) > 0);
        vol_meta = vol_meta_map[commitBlobReq->volId];
        is_forwarding = vol_meta->isForwarding();
        vol_map_mtx->unlock();

        // move the state, once we drain  planned queue contents
        LOGTRACE << "is_forwarding ? " << is_forwarding << " req DMT ver "
                 << commitBlobReq->dmt_version << " DMT ver " << omClient->getDMTVersion();

        if (is_forwarding) {
            // DMT version must not match in order to forward the update!!!
            if (commitBlobReq->dmt_version != omClient->getDMTVersion()) {
                error = catSyncMgr->forwardCatalogUpdate(commitBlobReq, blob_version,
                                                         blob_obj_list, meta_list);
                if (error.ok()) {
                    // we forwarded the request!!!
                    forwarded_commit = true;
                }
            }
        } else {
            // DMT mismatch must not happen if volume is in 'not forwarding' state
            fds_verify(commitBlobReq->dmt_version != omClient->getDMTVersion());
        }
    }

    // check if we can finish forwarding if volume still forwards cat commits
    if (catSyncMgr->isSyncInProgress()) {
        fds_bool_t is_finish_forward = false;
        VolumeMeta* vol_meta = NULL;
        vol_map_mtx->lock();
        fds_verify(vol_meta_map.count(commitBlobReq->volId) > 0);
        vol_meta = vol_meta_map[commitBlobReq->volId];
        is_finish_forward = vol_meta->isForwardFinishing();
        vol_map_mtx->unlock();
        if (is_finish_forward) {
            if (!timeVolCat_->isPendingTx(commitBlobReq->volId, catSyncMgr->dmtCloseTs())) {
                finishForwarding(commitBlobReq->volId);
            }
        }
    }

    // if forwarding -- do not reply to AM yet, will reply when we receive response
    // for fwd cat update from destination DM
    if (!forwarded_commit) {
        commitBlobReq->dmio_commit_blob_tx_resp_cb(error, commitBlobReq);
    }
}

//
// Handle forwarded (committed) catalog update from another DM
//
void DataMgr::fwdUpdateCatalog(dmCatReq *io)
{
    Error err(ERR_OK);
    DmIoFwdCat *fwdCatReq = static_cast<DmIoFwdCat*>(io);

    LOGTRACE << "Will commit fwd blob " << *fwdCatReq << " to tvc";
    err = timeVolCat_->updateFwdCommittedBlob(fwdCatReq->volId,
                                              fwdCatReq->blob_name,
                                              fwdCatReq->blob_version,
                                              fwdCatReq->fwdCatMsg->obj_list,
                                              fwdCatReq->fwdCatMsg->meta_list,
                                              std::bind(&DataMgr::updateFwdBlobCb, this,
                                                        std::placeholders::_1, fwdCatReq));
    if (!err.ok()) {
        qosCtrl->markIODone(*fwdCatReq);
        fwdCatReq->dmio_fwdcat_resp_cb(err, fwdCatReq);
    }
}

void DataMgr::updateFwdBlobCb(const Error &err, DmIoFwdCat *fwdCatReq)
{
    LOGTRACE << "Committed fwd blob " << *fwdCatReq;
    qosCtrl->markIODone(*fwdCatReq);
    fwdCatReq->dmio_fwdcat_resp_cb(err, fwdCatReq);
}

void
DataMgr::ReqHandler::StartBlobTx(FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& msgHdr,
                                 boost::shared_ptr<std::string> &volumeName,
                                 boost::shared_ptr<std::string> &blobName,
                                 FDS_ProtocolInterface::TxDescriptorPtr &txDesc) {
    GLOGDEBUG << "Received start blob transction request for volume "
              << *volumeName << " and blob " << *blobName;
    fds_panic("must not get here");
}

void
DataMgr::ReqHandler::StatBlob(FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& msgHdr,
                              boost::shared_ptr<std::string> &volumeName,
                              boost::shared_ptr<std::string> &blobName) {
    GLOGDEBUG << "Received stat blob requested for volume "
              << *volumeName << " and blob " << *blobName;
    fds_panic("must not get here");
}

void DataMgr::ReqHandler::UpdateCatalogObject(FDS_ProtocolInterface::
                                              FDSP_MsgHdrTypePtr &msg_hdr,
                                              FDS_ProtocolInterface::
                                              FDSP_UpdateCatalogTypePtr
                                              &update_catalog) {
    Error err(ERR_OK);
    fds_panic("must not get here");
}

void
DataMgr::scheduleAbortBlobTxSvc(void * _io)
{
    Error err(ERR_OK);
    DmIoAbortBlobTx *abortBlobTx = static_cast<DmIoAbortBlobTx*>(_io);

    BlobTxId::const_ptr blobTxId = abortBlobTx->ioBlobTxDesc;
    fds_verify(*blobTxId != blobTxIdInvalid);

    // Call TVC abortTx
    timeVolCat_->abortBlobTx(abortBlobTx->volId, blobTxId);

    if (!err.ok()) {
        PerfTracer::incr(abortBlobTx->opReqFailedPerfEventType, abortBlobTx->getVolId(),
                abortBlobTx->perfNameStr);
    }
    qosCtrl->markIODone(*abortBlobTx);
    PerfTracer::tracePointEnd(abortBlobTx->opLatencyCtx);
    PerfTracer::tracePointEnd(abortBlobTx->opReqLatencyCtx);
    abortBlobTx->dmio_abort_blob_tx_resp_cb(err, abortBlobTx);
}

void
DataMgr::queryCatalogBackendSvc(void * _io)
{
    Error err(ERR_OK);
    DmIoQueryCat *qryCatReq = static_cast<DmIoQueryCat*>(_io);

    err = timeVolCat_->queryIface()->getBlob(qryCatReq->volId,
                                             qryCatReq->blob_name,
                                             &(qryCatReq->blob_version),
                                             &(qryCatReq->queryMsg->meta_list),
                                             &(qryCatReq->queryMsg->obj_list));
    if (!err.ok()) {
        PerfTracer::incr(qryCatReq->opReqFailedPerfEventType, qryCatReq->getVolId(),
                qryCatReq->perfNameStr);
    }

    qosCtrl->markIODone(*qryCatReq);
    // TODO(Andrew): Note the cat request gets freed
    // by the callback
    PerfTracer::tracePointEnd(qryCatReq->opLatencyCtx);
    PerfTracer::tracePointEnd(qryCatReq->opReqLatencyCtx);
    qryCatReq->dmio_querycat_resp_cb(err, qryCatReq);
}

void DataMgr::ReqHandler::QueryCatalogObject(FDS_ProtocolInterface::
                                             FDSP_MsgHdrTypePtr &msg_hdr,
                                             FDS_ProtocolInterface::
                                             FDSP_QueryCatalogTypePtr
                                             &query_catalog) {
    Error err(ERR_OK);
    fds_panic("must not get here");
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

    LOGDEBUG << "Will do first or second rsync for volume "
             << std::hex << snapReq->volId << " to node "
             << (snapReq->node_uuid).uuid_get_val() << std::dec;

    if (io->io_type == FDS_DM_SNAPDELTA_VOLCAT) {
        // we are doing second rsync, set volume state to forward
        // We are doing this before we do catalog rsync, so some
        // updates that will make it to the persistent volume catalog
        // will also be sent to the destination node; this is ok
        // because we are serializing the updates to the same blob.
        // so they will also make it in order (assuming that sending
        // update A before update B will also cause receiving DM to receive
        // udpdate A before update B -- otherwise we will have a race
        // which will also happen with other forwarded updates).
        vol_map_mtx->lock();
        fds_verify(vol_meta_map.count(snapReq->volId) > 0);
        VolumeMeta *vol_meta = vol_meta_map[snapReq->volId];
        vol_meta->setForwardInProgress();
        vol_map_mtx->unlock();
    }

    // sync the catalog
    err = timeVolCat_->queryIface()->syncCatalog(snapReq->volId,
                                                 snapReq->node_uuid);

    // notify sync mgr that we did rsync
    snapReq->dmio_snap_vcat_cb(snapReq->volId, err);

    // mark this request as complete
    qosCtrl->markIODone(*snapReq);
    delete snapReq;
}

/**
 * Populates an fdsp message header with stock fields.
 *
 * @param[in] Ptr to msg header to modify
 */
void
DataMgr::initSmMsgHdr(FDSP_MsgHdrTypePtr msgHdr) {
    msgHdr->minor_ver = 0;
    msgHdr->msg_id    = 1;

    msgHdr->major_ver = 0xa5;
    msgHdr->minor_ver = 0x5a;

    msgHdr->num_objects = 1;
    msgHdr->frag_len    = 0;
    msgHdr->frag_num    = 0;

    msgHdr->tennant_id      = 0;
    msgHdr->local_domain_id = 0;

    msgHdr->src_id = FDSP_DATA_MGR;
    msgHdr->dst_id = FDSP_STOR_MGR;

    msgHdr->src_node_name = *(modProvider_->get_plf_manager()->plf_get_my_name());

    msgHdr->origin_timestamp = fds::get_fds_timestamp_ms();

    msgHdr->err_code = ERR_OK;
    msgHdr->result   = FDSP_ERR_OK;
}

/**
 * Issues delete calls for a set of objects in 'oids' list
 * if DM is primary for volume 'volid'
 */
Error
DataMgr::expungeObjectsIfPrimary(fds_volid_t volid,
                                 const std::vector<ObjectID>& oids) {
    Error err(ERR_OK);
    if (runMode == TEST_MODE) return err;  // no SMs, noone to notify
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
    DeleteObjectMsgPtr expReq(new DeleteObjectMsg());

    // Set message parameters
    expReq->volId = volId;
    fds::assign(expReq->objId, objId);
    expReq->origin_timestamp = fds::get_fds_timestamp_ms();

    // Make RPC call

    auto asyncExpReq = gSvcRequestPool->newQuorumSvcRequest(
        boost::make_shared<DltObjectIdEpProvider>(omClient->getDLTNodesForDoidKey(objId)));
    asyncExpReq->setPayload(FDSP_MSG_TYPEID(fpi::DeleteObjectMsg), expReq);
    asyncExpReq->setTimeoutMs(5000);
    // TODO(brian): How to do cb?
    // asyncExpReq->onResponseCb(NULL);  // in other areas respcb is a parameter
    asyncExpReq->invoke();
    // Return any errors
    return err;
}

/**
 * Callback for expungeObject call
 */
void
DataMgr::expungeObjectCb(QuorumSvcRequest* svcReq,
                         const Error& error,
                         boost::shared_ptr<std::string> payload) {
    DBG(GLOGDEBUG << "Expunge cb called");
}

void DataMgr::scheduleGetBlobMetaDataSvc(void *_io) {
    Error err(ERR_OK);
    DmIoGetBlobMetaData *getBlbMeta = static_cast<DmIoGetBlobMetaData*>(_io);

    // TODO(Andrew): We're not using the size...we can remove it
    fds_uint64_t blobSize;
    err = timeVolCat_->queryIface()->getBlobMeta(getBlbMeta->volId,
                                             getBlbMeta->blob_name,
                                             &(getBlbMeta->blob_version),
                                             &(blobSize),
                                             &(getBlbMeta->message->metaDataList));
    if (!err.ok()) {
        PerfTracer::incr(getBlbMeta->opReqFailedPerfEventType, getBlbMeta->getVolId(),
                getBlbMeta->perfNameStr);
    }
    getBlbMeta->message->byteCount = blobSize;
    qosCtrl->markIODone(*getBlbMeta);
    PerfTracer::tracePointEnd(getBlbMeta->opLatencyCtx);
    PerfTracer::tracePointEnd(getBlbMeta->opReqLatencyCtx);
    // TODO(Andrew): Note the cat request gets freed
    // by the callback
    getBlbMeta->dmio_getmd_resp_cb(err, getBlbMeta);
}

void DataMgr::setBlobMetaDataSvc(void *io) {
    Error err;
    DmIoSetBlobMetaData *setBlbMetaReq = static_cast<DmIoSetBlobMetaData*>(io);
    err = timeVolCat_->updateBlobTx(setBlbMetaReq->volId,
                                    setBlbMetaReq->ioBlobTxDesc,
                                    setBlbMetaReq->md_list);
    if (!err.ok()) {
        PerfTracer::incr(setBlbMetaReq->opReqFailedPerfEventType, setBlbMetaReq->getVolId(),
                setBlbMetaReq->perfNameStr);
    }
    qosCtrl->markIODone(*setBlbMetaReq);
    PerfTracer::tracePointEnd(setBlbMetaReq->opLatencyCtx);
    PerfTracer::tracePointEnd(setBlbMetaReq->opReqLatencyCtx);
    setBlbMetaReq->dmio_setmd_resp_cb(err, setBlbMetaReq);
}

void DataMgr::getVolumeMetaData(dmCatReq *io) {
    Error err(ERR_OK);
    DmIoGetVolumeMetaData * getVolMDReq = static_cast<DmIoGetVolumeMetaData *>(io);
    err = timeVolCat_->queryIface()->getVolumeMeta(getVolMDReq->getVolId(),
            reinterpret_cast<fds_uint64_t *>(&getVolMDReq->msg->volume_meta_data.size),
            reinterpret_cast<fds_uint64_t *>(&getVolMDReq->msg->volume_meta_data.blobCount),
            reinterpret_cast<fds_uint64_t *>(&getVolMDReq->msg->volume_meta_data.objectCount));
    if (!err.ok()) {
        PerfTracer::incr(getVolMDReq->opReqFailedPerfEventType, getVolMDReq->getVolId(),
                getVolMDReq->perfNameStr);
    }
    qosCtrl->markIODone(*getVolMDReq);
    PerfTracer::tracePointEnd(getVolMDReq->opLatencyCtx);
    PerfTracer::tracePointEnd(getVolMDReq->opReqLatencyCtx);
    getVolMDReq->dmio_get_volmd_resp_cb(err, getVolMDReq);
}

void DataMgr::ReqHandler::DeleteCatalogObject(FDS_ProtocolInterface::
                                              FDSP_MsgHdrTypePtr &msg_hdr,
                                              FDS_ProtocolInterface::
                                              FDSP_DeleteCatalogTypePtr
                                              &delete_catalog) {
    Error err(ERR_OK);
    fds_panic("must not get here");
}

void DataMgr::ReqHandler::SetBlobMetaData(boost::shared_ptr<FDSP_MsgHdrType>& msgHeader,
                                          boost::shared_ptr<std::string>& volumeName,
                                          boost::shared_ptr<std::string>& blobName,
                                          boost::shared_ptr<FDSP_MetaDataList>& metaDataList) {
    GLOGDEBUG << " Set metadata for volume:" << *volumeName
              << " blob:" << *blobName;
    fds_panic("must not get here");
}

void DataMgr::ReqHandler::GetBlobMetaData(boost::shared_ptr<FDSP_MsgHdrType>& msgHeader,
                                          boost::shared_ptr<std::string>& volumeName,
                                          boost::shared_ptr<std::string>& blobName) {
    GLOGDEBUG << " volume:" << *volumeName
             << " blob:" << *blobName;
    fds_panic("must not get here");
}

void DataMgr::ReqHandler::GetVolumeMetaData(boost::shared_ptr<FDSP_MsgHdrType>& header,
                                            boost::shared_ptr<std::string>& volumeName) {
    Error err(ERR_OK);
    GLOGDEBUG << " volume:" << *volumeName << " txnid:" << header->req_cookie;

    RequestHeader reqHeader(header);
    dmCatReq* request = new dmCatReq(reqHeader, FDS_GET_VOLUME_METADATA);
    err = dataMgr->qosCtrl->enqueueIO(request->volId, request);
    fds_verify(err == ERR_OK);
}

void DataMgr::InitMsgHdr(const FDSP_MsgHdrTypePtr& msg_hdr)
{
    msg_hdr->minor_ver = 0;
    msg_hdr->msg_code = FDSP_MSG_PUT_OBJ_REQ;
    msg_hdr->msg_id =  1;

    msg_hdr->major_ver = 0xa5;
    msg_hdr->minor_ver = 0x5a;

    msg_hdr->num_objects = 1;
    msg_hdr->frag_len = 0;
    msg_hdr->frag_num = 0;

    msg_hdr->tennant_id = 0;
    msg_hdr->local_domain_id = 0;

    msg_hdr->src_id = FDSP_DATA_MGR;
    msg_hdr->dst_id = FDSP_STOR_MGR;

    msg_hdr->src_node_name = "";

    msg_hdr->err_code = ERR_OK;
    msg_hdr->result = FDSP_ERR_OK;
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

}  // namespace fds
