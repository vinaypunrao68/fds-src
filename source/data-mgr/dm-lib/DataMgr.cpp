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
#include <util/disk_utils.h>
#include <fiu-control.h>
#include <fiu-local.h>
#include <fdsp/event_api_types.h>
#include <fdsp/svc_types_types.h>



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

const std::hash<fds_volid_t> DataMgr::dmQosCtrl::volIdHash;
const std::hash<std::string> DataMgr::dmQosCtrl::blobNameHash;
const DataMgr::dmQosCtrl::SerialKeyHash DataMgr::dmQosCtrl::keyHash;

/**
 * Send an Event to OM using the events API
 */

void DataMgr::sendEventMessageToOM(fpi::EventType eventType,
                                   fpi::EventCategory eventCategory,
                                   fpi::EventSeverity eventSeverity,
                                   fpi::EventState eventState,
                                   const std::string& messageKey,
                                   std::vector<fpi::MessageArgs> messageArgs,
                                   const std::string& messageFormat) {

    fpi::NotifyEventMsgPtr eventMsg(new fpi::NotifyEventMsg());

    eventMsg->event.type = eventType;
    eventMsg->event.category = eventCategory;
    eventMsg->event.severity = eventSeverity;
    eventMsg->event.state = eventState;

    auto now = std::chrono::system_clock::now();
    fds_uint64_t seconds = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
    eventMsg->event.initialTimestamp = seconds;
    eventMsg->event.modifiedTimestamp = seconds;
    eventMsg->event.messageKey = messageKey;
    eventMsg->event.messageArgs = messageArgs;
    eventMsg->event.messageFormat = messageFormat;
    eventMsg->event.defaultMessage = "DEFAULT MESSAGE";

    auto svcMgr = MODULEPROVIDER()->getSvcMgr()->getSvcRequestMgr();
    auto request = svcMgr->newEPSvcRequest (MODULEPROVIDER()->getSvcMgr()->getOmSvcUuid());

    request->setPayload(FDSP_MSG_TYPEID(fpi::NotifyEventMsg), eventMsg);
    request->invoke();

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
void DataMgr::handleForwardComplete(DmRequest *io) {
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

    /*
     * Piggyback on this method to determine if we're nearing disk capacity
     */
    if (sampleCounter % 5 == 0) {
        LOGDEBUG << "Checking disk utilization!";
        float_t pct_used = timeVolCat_->getUsedCapacityAsPct();

        // We want to log which disk is too full here
        if (pct_used >= DISK_CAPACITY_ERROR_THRESHOLD &&
            lastCapacityMessageSentAt < DISK_CAPACITY_ERROR_THRESHOLD) {
            LOGERROR << "ERROR: DM is utilizing " << pct_used << "% of available storage space!";
            lastCapacityMessageSentAt = pct_used;

            // set time volume catalog to unavailable -- no available storage space
            timeVolCat_->setUnavailable();

            // Send message to OM
            sendHealthCheckMsgToOM(fpi::HEALTH_STATE_ERROR, ERR_SERVICE_CAPACITY_FULL, "DM capacity is FULL! ");

        } else if (pct_used >= DISK_CAPACITY_ALERT_THRESHOLD &&
                   lastCapacityMessageSentAt < DISK_CAPACITY_ALERT_THRESHOLD) {
            LOGWARN << "ATTENTION: DM is utilizing " << pct_used << " of available storage space!";
            lastCapacityMessageSentAt = pct_used;

            // Send message to OM
            sendHealthCheckMsgToOM(fpi::HEALTH_STATE_LIMITED,
                                   ERR_SERVICE_CAPACITY_DANGEROUS, "DM is reaching dangerous capacity levels!");

        } else if (pct_used >= DISK_CAPACITY_WARNING_THRESHOLD &&
                   lastCapacityMessageSentAt < DISK_CAPACITY_WARNING_THRESHOLD) {
            LOGNORMAL << "DM is utilizing " << pct_used << " of available storage space!";

            sendEventMessageToOM(fpi::SYSTEM_EVENT, fpi::EVENT_CATEGORY_STORAGE,
                                 fpi::EVENT_SEVERITY_WARNING,
                                 fpi::EVENT_STATE_SOFT,
                                 "dm.capacity.warn",
                                 std::vector<fpi::MessageArgs>(), "");

            lastCapacityMessageSentAt = pct_used;
        } else {
            // If the used pct drops below alert levels reset so we resend the message when
            // we re-hit this condition
            if (pct_used < DISK_CAPACITY_ALERT_THRESHOLD) {
                lastCapacityMessageSentAt = 0;

                // Send message to OM resetting us to OK state
                sendHealthCheckMsgToOM(fpi::HEALTH_STATE_RUNNING, ERR_OK,
                                       "DM utilization no longer at dangerous levels.");
            }
        }
        sampleCounter = 0;
    }
    sampleCounter++;

}

void DataMgr::sendHealthCheckMsgToOM(fpi::HealthState serviceState,
                                     fds_errno_t statusCode,
                                     const std::string& statusInfo) {

    fpi::SvcInfo info = MODULEPROVIDER()->getSvcMgr()->getSelfSvcInfo();

    // Send health check thrift message to OM
    fpi::NotifyHealthReportPtr healthRepMsg(new fpi::NotifyHealthReport());
    healthRepMsg->healthReport.serviceInfo.svc_id = info.svc_id;
    healthRepMsg->healthReport.serviceInfo.name = info.name;
    healthRepMsg->healthReport.serviceInfo.svc_port = info.svc_port;
    healthRepMsg->healthReport.serviceState = serviceState;
    healthRepMsg->healthReport.statusCode = statusCode;
    healthRepMsg->healthReport.statusInfo = statusInfo;

    auto svcMgr = MODULEPROVIDER()->getSvcMgr()->getSvcRequestMgr();
    auto request = svcMgr->newEPSvcRequest (MODULEPROVIDER()->getSvcMgr()->getOmSvcUuid());

    request->setPayload (FDSP_MSG_TYPEID (fpi::NotifyHealthReport), healthRepMsg);
    request->invoke();

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

const VolumeDesc* DataMgr::getVolumeDesc(fds_volid_t volId) const {
    FDSGUARD(vol_map_mtx);
    auto iter = vol_meta_map.find(volId);
    return (vol_meta_map.end() != iter && iter->second ?
            iter->second->vol_desc : 0);
}

void DataMgr::getActiveVolumes(std::vector<fds_volid_t>& vecVolIds) {
    vecVolIds.reserve(vol_meta_map.size());
    for (const auto& iter : vol_meta_map) {
        const VolumeDesc* vdesc = iter.second->vol_desc;
        if (vdesc->isStateActive() && !vdesc->isSnapshot()) {
            vecVolIds.push_back(vdesc->volUUID);
        }
    }
}

std::string DataMgr::getSnapDirName(const fds_volid_t &volId,
                                    const fds_volid_t snapId) const
{
    std::stringstream stream;
    stream << getSnapDirBase() << "/" << volId << "/" << snapId;
    return stream.str();
}

void DataMgr::deleteUnownedVolumes() {
    LOGNORMAL << "DELETING unowned volumes";

    std::vector<fds_volid_t> volIds;
    std::vector<fds_volid_t> deleteList;
    auto mySvcUuid = MODULEPROVIDER()->getSvcMgr()->getSelfSvcUuid();

    /* Filter out unowned volumes */
    dmutil::getVolumeIds(MODULEPROVIDER()->proc_fdsroot(), volIds);
    for (const auto &volId : volIds) {
        DmtColumnPtr nodes = MODULEPROVIDER()->getSvcMgr()->getDMTNodesForVolume(volId);
        if (nodes->find(NodeUuid(mySvcUuid)) == -1) {
            /* Volume is not owned..add for delete*/
            deleteList.push_back(volId);
        }
    }

    /* Delete unowned volumes one at a time */
    for (const auto &volId : deleteList) {
        LOGNORMAL << "DELETING volume: " << volId;
        /* Flow for deleting the volume is to mark it for delete first
         * followed by acutally deleting it
         */
        Error err = process_rm_vol(volId, true);        // Mark for delete
        if (err == ERR_OK) {
            err = process_rm_vol(volId, false);         // Do the actual delete
        }
        // TODO(xxx): Remove snapshots once the api is available
        if (err != ERR_OK) {
            LOGWARN << "Ecountered error: " << err << " while deleting volume: " << volId;
        }
    }

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
                          DmRequest* ioReq) {
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

    err = _add_vol_locked(vol_name, vol_uuid, desc);

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
                               VolumeDesc *vdesc) {
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

    if (vdesc->isClone()) {
        // clone happens only on primary
        fPrimary = amIPrimary(vdesc->srcVolumeId);
    } else if (vdesc->isSnapshot()) {
        // snapshot happens on all nodes
        fPrimary = amIPrimaryGroup(vdesc->srcVolumeId);
    } else {
        fPrimary = amIPrimary(vdesc->volUUID);
    }

    if (vdesc->isSnapshot() && !fPrimary) {
        LOGWARN << "not primary - nothing to do for snapshot "
                << "for vol:" << vdesc->srcVolumeId;
        return err;
    }

    // do this processing only in the case..
    if (vdesc->isSnapshot() || (vdesc->isClone() && fPrimary)) {
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
            err = timelineMgr->createSnapshot(vdesc);
        } else if (features.isTimelineEnabled()) {
            err = timelineMgr->createClone(vdesc);
        }
        if (err.ok()) fActivated = true;

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

    if (err.ok() && !fActivated) {
        // not going to sync this volume, activate volume
        // so that we can do get/put/del cat ops to this volume
        err = timeVolCat_->activateVolume(vdesc->volUUID);
        if (err.ok()) {
            fActivated = true;
        }
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
             << vol_uuid << std::dec << ", created catalogs";

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
        return  err;
    }

    if (vdesc->isSnapshot()) {
        return err;
    }

    // Primary is responsible for persisting the latest seq number.
    // latest seq_number is provided to AM on volume open.
    // XXX: (JLL) However, if we aren't the primary we can still become primary,
    // so we should also set the SequenceId when we sync with the primary. And we
    // should do the calculation if the primary fails over to us. but if we set it
    // here, before failover, we could have a seq_id ahead of the primary.
    // is this a problem?
    if (fActivated) {
        sequence_id_t seq_id;
        err = timeVolCat_->queryIface()->getVolumeSequenceId(vol_uuid, seq_id);

        if (!err.ok()) {
            LOGERROR << "failed to read persisted sequence id for vol: " << vol_uuid << " error: " << err;
            return err;
        }else{
            volmeta->setSequenceId(seq_id);
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
        timelineMgr->loadSnapshot(vol_uuid);
    }

    return err;
}

Error DataMgr::_process_add_vol(const std::string& vol_name,
                                fds_volid_t vol_uuid,
                                VolumeDesc *desc) {
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

    err = _add_vol_locked(vol_name, vol_uuid, desc);
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
        detachVolume(vol_uuid);
    } else {
        LOGERROR << "Failed to remove volume " << std::hex
                 << vol_uuid << std::dec << " " << err;
    }

    return err;
}

void DataMgr::detachVolume(fds_volid_t vol_uuid) {
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
    LOGNORMAL << "Detached vol meta for vol uuid "
        << std::hex << vol_uuid << std::dec;
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
 * For all the volumes under going Active Migration forwarding,
 * turn them off.
 */
Error DataMgr::notifyDMTClose() {
    Error err(ERR_OK);

    // TODO(Andrew): Um, no where to we have a useful error statue
    // to even return.
    sendDmtCloseCb(err);
    LOGMIGRATE << "Sent DMT close message to OM";
    dmMigrationMgr->stopAllClientForwarding();
    err = dmMigrationMgr->finishActiveMigration();
    return err;
}

//
// Handle DMT close for the volume in io req -- sends
// push meta done to destination DM
//
void DataMgr::handleDMTClose(DmRequest *io) {
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

    initHandlers();
    standalone = modProvider_->get_fds_config()->get<bool>("fds.dm.testing.standalone", false);
    numTestVols = 10;
    scheduleRate = 10000;
    shuttingDown = false;

    lastCapacityMessageSentAt = 0;
    sampleCounter = 0;

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
        timelineMgr.reset(new timeline::TimelineManager(this));
    }
    /**
     * FEATURE TOGGLE: Volume Open Support
     * Thu 02 Apr 2015 12:39:27 PM PDT
     */
    features.setVolumeTokensEnabled(modProvider_->get_fds_config()->get<bool>(
            "fds.feature_toggle.common.volume_open_support", false));

    features.setExpungeEnabled(modProvider_->get_fds_config()->get<bool>(
            "fds.dm.enable_expunge", true));

    // FEATURE TOGGLE: Serialization for consistency. Meant to ensure that
    // requests for a given serialization key are applied in the order they
    // are received.
    features.setSerializeReqsEnabled(modProvider_->get_fds_config()->get<bool>(
            "fds.dm.req_serialization", true));

    vol_map_mtx = new fds_mutex("Volume map mutex");

    /*
     * Retrieves the number of primary DMs from the config file.
     */
    int primary_check = MODULEPROVIDER()->get_fds_config()->
                                  get<int>("fds.dm.number_of_primary");
    fds_verify(primary_check > 0);
    setNumOfPrimary((unsigned)primary_check);

    /**
     * Instantiate migration manager.
     */
    dmMigrationMgr = DmMigrationMgr::unique_ptr(new DmMigrationMgr(this, *this));

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
    handlers[FDS_RENAME_BLOB] = new dm::RenameBlobHandler(*this);
    handlers[FDS_ABORT_BLOB_TX] = new dm::AbortBlobTxHandler(*this);
    handlers[FDS_DM_FWD_CAT_UPD] = new dm::ForwardCatalogUpdateHandler(*this);
    handlers[FDS_STAT_VOLUME] = new dm::StatVolumeHandler(*this);
    handlers[FDS_SET_VOLUME_METADATA] = new dm::SetVolumeMetadataHandler(*this);
    handlers[FDS_GET_VOLUME_METADATA] = new dm::GetVolumeMetadataHandler(*this);
    handlers[FDS_OPEN_VOLUME] = new dm::VolumeOpenHandler(*this);
    handlers[FDS_CLOSE_VOLUME] = new dm::VolumeCloseHandler(*this);
    handlers[FDS_DM_RELOAD_VOLUME] = new dm::ReloadVolumeHandler(*this);
    handlers[FDS_DM_MIGRATION] = new dm::DmMigrationHandler(*this);
    handlers[FDS_DM_RESYNC_INIT_BLOB] = new dm::DmMigrationBlobFilterHandler(*this);
    handlers[FDS_DM_MIG_DELTA_BLOBDESC] = new dm::DmMigrationDeltaBlobDescHandler(*this);
    handlers[FDS_DM_MIG_DELTA_BLOB] = new dm::DmMigrationDeltaBlobHandler(*this);
    handlers[FDS_DM_MIG_TX_STATE] = new dm::DmMigrationTxStateHandler(*this);
}

DataMgr::~DataMgr()
{
    // shutdown all data manager modules
    LOGDEBUG << "Received shutdown message DM ... shutdown modules..";
    mod_shutdown();
}

int DataMgr::run()
{

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
        features.setTestModeEnabled(true);
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
    fds_uint32_t diskIOPsMin = standalone ? 60*1000 :
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
    if (!standalone) {
        // since aggregator is in the same module, for stats that need to go to
        // local aggregator, we just directly stream to aggregator (not over network)
        StatsCollector::singleton()->startStreaming(
            std::bind(&DataMgr::sampleDMStats, this, std::placeholders::_1),
            std::bind(&DataMgr::handleLocalStatStream, this,
                      std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
        statStreamAggr_->mod_startup();

        // get DMT from OM if DMT already exist
        MODULEPROVIDER()->getSvcMgr()->getDMT();
        MODULEPROVIDER()->getSvcMgr()->getDLT();
    }

    root->fds_mkdir(root->dir_sys_repo_dm().c_str());
    root->fds_mkdir(root->dir_user_repo_dm().c_str());

    expungeMgr.reset(new ExpungeManager(this));
    // finish setting up time volume catalog
    timeVolCat_->mod_startup();

    // Register the DLT manager with service layer so that
    // outbound requests have the correct dlt_version.
    if (!features.isTestModeEnabled()) {
        gSvcRequestPool->setDltManager(MODULEPROVIDER()->getSvcMgr()->getDltManager());
    }

    if (timeVolCat_->isUnavailable()) {
        // send health check msg to OM
        sendHealthCheckMsgToOM(fpi::HEALTH_STATE_ERROR, ERR_NODE_NOT_ACTIVE, "DM failed to start! ");
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
DataMgr::snapVolCat(DmRequest *io) {
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
                           root->dir_user_repo_dm().c_str(), volId);
}

std::string getVolumeMetaDir(fds_volid_t volId) {
    const FdsRootDir* root = g_fdsprocess->proc_fdsroot();
    return util::strformat("%s/%ld/volumemeta",
                           root->dir_user_repo_dm().c_str(), volId);
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

void getVolumeIds(const FdsRootDir* root, std::vector<fds_volid_t>& vecVolumes) {
    std::vector<std::string> vecNames;

    util::getSubDirectories(root->dir_sys_repo_dm(), vecNames);

    for (const auto& name : vecNames) {
        vecVolumes.push_back(fds_volid_t(std::atoll(name.c_str())));
    }
    std::sort(vecVolumes.begin(), vecVolumes.end());
}

std::string getTimelineDBPath() {
    const FdsRootDir* root = g_fdsprocess->proc_fdsroot();
    const std::string dmDir = root->dir_sys_repo_dm();
    return util::strformat("%s/timeline.db", dmDir.c_str());
}

std::string getExpungeDBPath() {
    const FdsRootDir* root = g_fdsprocess->proc_fdsroot();
    const std::string dmDir = root->dir_user_repo_dm();
    return util::strformat("%s/expunge.ldb", dmDir.c_str());
}

}  // namespace dmutil



}  // namespace fds
