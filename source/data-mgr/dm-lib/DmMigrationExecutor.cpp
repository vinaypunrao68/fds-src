/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

#include <DataMgr.h>
#include <DmMigrationExecutor.h>
#include <fdsp/dm_types_types.h>
#include <fdsp/dm_api_types.h>
#include <dmhandler.h>

#include "fds_module_provider.h"
#include <net/SvcMgr.h>
#include <net/SvcRequestPool.h>

namespace fds {

DmMigrationExecutor::DmMigrationExecutor(DataMgr& _dataMgr,
    							 	 	 const NodeUuid& _srcDmUuid,
	 	 								 fpi::FDSP_VolumeDescType& _volDesc,
										 const fds_bool_t& _autoIncrement,
										 DmMigrationExecutorDoneCb _callback,
                                         uint32_t _timeout)
	: dataMgr(_dataMgr),
      srcDmSvcUuid(_srcDmUuid),
      volDesc(_volDesc),
	  autoIncrement(_autoIncrement),
      migrDoneCb(_callback),
	  timerInterval(_timeout),
	  seqTimer(new FdsTimer),
      msgHandler(_dataMgr),
      migrationProgress(INIT)
{
    volumeUuid = volDesc.volUUID;

	LOGMIGRATE << "Migration executor received for volume ID " << volDesc;
}

DmMigrationExecutor::~DmMigrationExecutor()
{
}

Error
DmMigrationExecutor::startMigration()
{
	Error err(ERR_OK);

	LOGMIGRATE << "starting migration for VolDesc: " << volDesc;

    /** TODO(Sean):
     *
     * For now, assume that the volume exists on start migration.
     * Currently, OM sends two messages to DMs  :  1) notifyVolumeAdd  and 2) startMigration.
     *
     * notifyVolumeAdd is broadcasted to all DMs in the cluster, and depending on the DMT it
     * will create volumes.
     *
     * startMigration is sent only to the destination DM that needs sync volume(s) from the
     * primary DM in the redundancey group.
     *
     * So, for now, we should just check for the existence of the volume.
     */
    err = dataMgr._process_add_vol(dataMgr.getPrefix() + std::to_string(volumeUuid.get()),
                                   volumeUuid,
                                   &volDesc);

    /** TODO(Sean):
     * With current OM implementation, add node will send list of volumes and start migration
     * message, which contains the list of volume descriptors to sync.  So, it's expected
     * that at this point, we should be getting ERR_DUPLICATE.  When OM changes to send
     * only one message (startMigration), we update this block of code.
     *
     * OM could have sent the volume descriptor over already
     */
    if (err.ok() || (err == ERR_DUPLICATE)) {
    	fds_scoped_lock lock(progressLock);
    	fds_assert(migrationProgress == INIT);
    	migrationProgress = STATICMIGRATION_IN_PROGRESS;

    	/**
    	 * First do a StopDequeue on the volume
    	 */
    	LOGMIGRATE << "Stopping De-queing IO for volume " << volumeUuid;
    	dataMgr.qosCtrl->stopDequeue(volumeUuid);

    	/**
    	 * If the volume is successfully created with the given volume descriptor, process and generate the
    	 * initial blob filter set to be sent to the source DM.
    	 */
    	err = processInitialBlobFilterSet();
    	if (!err.ok()) {
    		LOGERROR << "processInitialBlobFilterSet failed on volume=" << volumeUuid
    				<< " with error=" << err;
    	}
    } else {
        LOGERROR << "process_add_vol failed on volume=" << volumeUuid
                 << " with error=" << err;
        if (migrDoneCb) {
        	migrDoneCb(volDesc.volUUID, err);
        }
    }

    return err;
}

Error
DmMigrationExecutor::processInitialBlobFilterSet()
{
	Error err(ERR_OK);
	LOGMIGRATE << "starting migration for VolDesc: " << volDesc;

    /**
     * create and initialize message for initial filter set.
     */
    fpi::CtrlNotifyInitialBlobFilterSetMsgPtr filterSet(new fpi::CtrlNotifyInitialBlobFilterSetMsg());
    filterSet->volumeId = volumeUuid.get();

    LOGMIGRATE << "processing to get list of <blobid, seqnum> for volume=" << volumeUuid;
    /**
     * Get the list of <blobid, seqnum> for a volume associted with this executor.
     */
    err = dataMgr.timeVolCat_->queryIface()->getAllBlobsWithSequenceId(fds_volid_t(volumeUuid),
                                                                       filterSet->blobFilterMap);
    if (!err.ok()) {
        LOGERROR << "failed to generatate list of <blobid, seqnum> for volume=" << volumeUuid
                 <<" with error=" << err;
        return err;
    }

    /**
     * If successfully generated the initial filter set, send it to the source DM.
     */
    auto asyncInitialBlobSetReq = gSvcRequestPool->newEPSvcRequest(srcDmSvcUuid.toSvcUuid());
    asyncInitialBlobSetReq->setPayload(FDSP_MSG_TYPEID(fpi::CtrlNotifyInitialBlobFilterSetMsg), filterSet);
    asyncInitialBlobSetReq->setTimeoutMs(0);
    asyncInitialBlobSetReq->invoke();

    return err;
}

Error
DmMigrationExecutor::processDeltaBlobDescs(fpi::CtrlNotifyDeltaBlobDescMsgPtr& msg)
{
    Error err(ERR_OK);
	/**
     * TODO: Need to hold descriptors until all blobs are applied.
	 */
	fds_verify(volumeUuid == fds_volid_t(msg->volume_id));
	LOGMIGRATE << "Processing incoming CtrlNotifyDeltaBlobDescMsg for volume="
               << std::hex << msg->volume_id << std::dec
               << " msgseqid=" << msg->msg_seq_id
               << " lastmsgseqid=" << msg->last_msg_seq_id
               << " numofblobdesc=" << msg->blob_desc_list.size();

    /**
     * Check if all blob offset is applied.  if applyBlobDescList is still
     * false, them queue them up to be applied later.
     */
    blobDescListMutex.lock();
    if (!deltaBlobsSeqNum.isSeqNumComplete()) {
        /**
         * add to the blob descriptor list while holding lock.
         */
        LOGMIGRATE << "Queueing incoming blob descriptor message vof volume="
                   << std::hex << msg->volume_id << std::dec
                   << " msgseqid=" << msg->msg_seq_id
                   << " lastmsgseqid=" << msg->last_msg_seq_id;
        blobDescList.emplace_back(msg);
        blobDescListMutex.unlock();
        return err;
    }
    blobDescListMutex.unlock();

    /**
     * No need to have lock at this point, since boolean is always set
     * one direction from false to true.
     */
    fds_verify(deltaBlobsSeqNum.isSeqNumComplete());

    LOGMIGRATE << "Applying blob descriptor for volume="
               << std::hex << volumeUuid << std::dec
               << ", sequencId=" << msg->msg_seq_id
               << ", lastMsg=" << msg->last_msg_seq_id;
    err = applyBlobDesc(msg);
    if (err != ERR_OK) {
        LOGERROR << "Applying blob descriptor failed on volume="
                 << std::hex << msg->volume_id << std::dec
                 << " msgseqid=" << msg->msg_seq_id
                 << " lastmsgseqid=" << msg->last_msg_seq_id
                 << " numofblobdesc=" << msg->blob_desc_list.size();
    }

	return err;
}

Error
DmMigrationExecutor::processDeltaBlobs(fpi::CtrlNotifyDeltaBlobsMsgPtr& msg)
{
	Error err(ERR_OK);

	fds_verify(volumeUuid == fds_volid_t(msg->volume_id));
	LOGMIGRATE << "Processing incoming CtrlNotifyDeltaBlobsMsg: "
               << std::hex << volumeUuid << std::hex
               << ", msg_seq_id=" << msg->msg_seq_id
               << ", last_seq_id=" << msg->last_msg_seq_id
               << ", num obj=" << msg->blob_obj_list.size();

    /**
     * It is possible to get en empty message and last_sequence_id == true.
     */
	if (0 == msg->blob_obj_list.size()) {
		LOGMIGRATE << "For volume=" << std::hex << volumeUuid << std::dec
                   << " received empty object list with"
                   << ", msg_seq_id=" << msg->msg_seq_id
                   << " last_mst_seq_id=" << msg->last_msg_seq_id;
    } else {
    	LOGMIGRATE << "Volume " << volumeUuid << " received non-empty blob message";
        /**
         * For each blob in the blob_obj_list, apply blob offset.
         */
        for (auto & blobObj : msg->blob_obj_list) {
            /**
             * TODO(Sean):
             * This can potentially be big, so might have move off stack and allocate.
             */
            BlobObjList blobList(blobObj.blob_diff_list);

            LOGMIGRATE << "put object on volume="
                         << std::hex << volumeUuid << std::dec
                         << ", blob_name=" << blobObj.blob_name
                         << ", num_blobs=" << blobList.size();

            /**
             * TODO(Sean):
             * This should really be directly called, since it's not query iface.
             * Will clean it up later.
             */
            err = dataMgr.timeVolCat_->queryIface()->putObject(fds_volid_t(volumeUuid),
                                                               blobObj.blob_name,
                                                               blobList);
            if (!err.ok()) {
                LOGERROR << "putObject failed on volume="
                         << std::hex << volumeUuid << std::dec
                         << ", blob_name=" << blobObj.blob_name;
                return err;
            }

        }
    }

    blobDescListMutex.lock();

    /**
     * Set the sequence number appropriately.
     */
    deltaBlobsSeqNum.setSeqNum(msg->msg_seq_id, msg->last_msg_seq_id);

    /**
     * If all the sequence numbers are present for the blobs, then send apply the queued
     * blob descriptors in this thread context.
     * It's possible that all desciptors have already been received.
     * So, any descriptors in the queue should be flushed.
     */
    if (deltaBlobsSeqNum.isSeqNumComplete()) {
        LOGMIGRATE << "blob sequence number is complete for volume="
                   << std::hex << volumeUuid << std::dec
                   << ".  Apply queued blob descriptors.";
        err = applyQueuedBlobDescs();
        fds_verify(ERR_OK == err);
    }
    blobDescListMutex.unlock();

	return err;
}


Error
DmMigrationExecutor::applyBlobDesc(fpi::CtrlNotifyDeltaBlobDescMsgPtr& msg)
{
    Error err(ERR_OK);

    for (auto & desc : msg->blob_desc_list) {
        LOGMIGRATE << "Applying blob descriptor for volume="
                   << std::hex << volumeUuid << std::dec
                   << ", blob_name=" << desc.vol_blob_name;

        err = dataMgr.timeVolCat_->migrateDescriptor(fds_volid_t(volumeUuid),
                                                     desc.vol_blob_name,
                                                     desc.vol_blob_desc);
        if (!err.ok()) {
            LOGERROR << "Failed to apply blob descriptor for volume="
                       << std::hex << volumeUuid << std::dec
                       << ", blob_name=" << desc.vol_blob_name;
            return err;
        }
    }

    /**
     * Record the sequence number after the blob descriptor is applied.
     */
    deltaBlobDescsSeqNum.setSeqNum(msg->msg_seq_id, msg->last_msg_seq_id);

    /**
     * If the blob descriptor seq number is complete, then notify the mgr that
     * the static migration is complete for this MigrationExecutor.
     */
    if (deltaBlobDescsSeqNum.isSeqNumComplete()) {
        LOGMIGRATE << "All Blob descriptors applied to volume="
                   << std::hex << volumeUuid << std::dec;
        notifyStaticMigrationComplete();
        if (migrDoneCb) {
            migrDoneCb(volDesc.volUUID, err);
        }
    }

    return err;
}

Error
DmMigrationExecutor::applyQueuedBlobDescs()
{
    Error err(ERR_OK);

    /**
     * process all queued blob descriptors.
     */
    for (auto & blobDescMsg : blobDescList) {
        LOGMIGRATE << "Applying queued blob descriptor for volume="
                   << std::hex << volumeUuid << std::dec
                   << ", sequencId=" << blobDescMsg->msg_seq_id
                   << ", lastMsg=" << blobDescMsg->last_msg_seq_id;
        err = applyBlobDesc(blobDescMsg);
        if (!err.ok()) {
            LOGERROR << "Failed applying queued blob descriptor for vlume="
                     << std::hex << volumeUuid << std::dec
                     << ", sequencId=" << blobDescMsg->msg_seq_id
                     << ", lastMsg=" << blobDescMsg->last_msg_seq_id;
            return err;
        }
    }

    return err;
}

Error
DmMigrationExecutor::processTxState(fpi::CtrlNotifyTxStateMsgPtr txStateMsg) {
    Error err;

    DmCommitLog::ptr commitLog;
    err = dataMgr.timeVolCat_->getCommitlog(volumeUuid, commitLog);

    if (!err.ok()) {
        LOGERROR << "Error getting commit log for vol: " << volumeUuid;
        return err;
    }

    err = commitLog->applySerializedTxs(txStateMsg->transactions);

    return err;
}

void
DmMigrationExecutor::sequenceTimeoutHandler()
{
	// TODO - part of error handling (FS-2619)
}

void
DmMigrationExecutor::notifyStaticMigrationComplete() {
    fds_scoped_lock lock(progressLock);
    fds_assert(migrationProgress == STATICMIGRATION_IN_PROGRESS);
    migrationProgress = APPLYING_FORWARDS_IN_PROGRESS;
    /* Send any buffered Forwarded messages to qos controller under system volume tag */
    for (const auto &msg : forwardedMsgs) {
        msgHandler.addToQueue(msg);
    }
}

Error
DmMigrationExecutor::processForwardedCommits(DmIoFwdCat* fwdCatReq) {
    /* Callback from QOS */
    fwdCatReq->cb = [this](const Error &e, DmRequest *dmReq) {
        if (e != ERR_OK) {
            // TODO: Abort migration
            fds_panic("Not handled");
            delete dmReq;
            return;
        }
        auto fwdCatReq = reinterpret_cast<DmIoFwdCat*>(dmReq);
        fds_assert((unsigned)(fwdCatReq->fwdCatMsg->volume_id) == volumeUuid.v);
        if (fwdCatReq->fwdCatMsg->lastForward) {
            fds_scoped_lock lock(progressLock);
            /* All forwards have been applied.  At this point we don't expect anything from
             * migration source.  We can resume active IO
             */
            migrationProgress = MIGRATION_COMPLETE;
            LOGMIGRATE << "Applying forwards is complete and resuming IO for volume: " << volumeUuid;
            dataMgr.qosCtrl->resumeIOs(volumeUuid);
        }
        delete dmReq;
    };
    
    fds_scoped_lock lock(progressLock);
    if (migrationProgress  == STATICMIGRATION_IN_PROGRESS) {
        forwardedMsgs.push_back(fwdCatReq);
    } else if (migrationProgress  == APPLYING_FORWARDS_IN_PROGRESS) {
        fds_assert(forwardedMsgs.size() == 0);
        msgHandler.addToQueue(fwdCatReq);
    } else {
        fds_panic("Unexpected state encountered");
    }
    return ERR_OK;
}

Error
DmMigrationExecutor::finishActiveMigration()
{
	fds_scoped_lock lock(progressLock);
	migrationProgress = MIGRATION_COMPLETE;
	LOGMIGRATE << "No forwards was sent, resuming IO for volume " << volumeUuid;
	dataMgr.qosCtrl->resumeIOs(volumeUuid);

	return ERR_OK;
}
}  // namespace fds
