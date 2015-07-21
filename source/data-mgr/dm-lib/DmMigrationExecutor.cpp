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
										 DmMigrationExecutorDoneCb _callback)
	: dataMgr(_dataMgr),
      srcDmSvcUuid(_srcDmUuid),
      volDesc(_volDesc),
	  autoIncrement(_autoIncrement),
      migrDoneCb(_callback),
	  randNumGen(RandNumGenerator::getRandSeed()),
	  timerInterval(fds_uint32_t(MODULEPROVIDER()->get_fds_config()->
			  get<int>("fds.dm.migration.migration_max_delta_blobs_to"))),
	  seqTimer(new FdsTimer),
	  deltaBlobSetHelper(seqTimer, timerInterval,
						  std::bind(&DmMigrationExecutor::sequenceTimeoutHandler, this))
{
    volumeUuid = volDesc.volUUID;
	LOGMIGRATE << "Migration executor received for volume ID " << volDesc;
    deltaBlobSetCbHelper.expectedCount = 0;
    deltaBlobSetCbHelper.actualCbCounted = 0;
    deltaBlobSetCbHelper.expectedCountFinalized = false;
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
    }

	if (migrDoneCb) {
		migrDoneCb(volDesc.volUUID, err);
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
	/**
     * TODO: Need to hold descriptors until all blobs are applied.
	 */
	fds_verify(volumeUuid == fds_volid_t(msg->volume_id));
	LOGMIGRATE << "Processing incoming CtrlNotifyDeltaBlobDescMsg for volume="
               << std::hex << volumeUuid << std::dec;

	return ERR_OK;
}

Error
DmMigrationExecutor::processDeltaBlobs(fpi::CtrlNotifyDeltaBlobsMsgPtr& msg)
{
	LOGMIGRATE << "Processing incoming CtrlNotifyDeltaBlobsMsg for volume " << volumeUuid;
	fds_verify(volumeUuid == fds_volid_t(msg->volume_id));
	deltaBlobSetCbHelper.mtx.lock();
	deltaBlobSetCbHelper.expectedCount++;
	deltaBlobSetCbHelper.expectedCountFinalized =
			deltaBlobSetHelper.setSeqNum(msg->msg_seq_id, msg->last_msg_seq_id);
	deltaBlobSetCbHelper.mtx.unlock();
	if (msg->blob_obj_list.size() == 0) {
		LOGERROR << "Volume Blob object list size is 0 for volume: "
				<< msg->volume_id;
		return ERR_INVALID_ARG;
	}

	Error err(ERR_OK);
	fds_uint32_t blobMode = 0;
	fds_bool_t atLeastOneBlobCommitted = false;
	for (blobObjListIter bolIter = msg->blob_obj_list.begin();
			bolIter != msg->blob_obj_list.end(); ++bolIter) {
		// Each iter represents a blob and its diff list
		// The version numbers will not matter since we're going to overwrite it w/ blobdesc later
		DmIoCommitBlobTx* commitBlobReq = new DmIoCommitBlobTx(volumeUuid, bolIter->blob_name, 0, 0, 0);
		BlobTxId newTx(randNumGen.genNumSafe());
		BlobTxId::ptr txPtr = boost::make_shared<BlobTxId>(newTx);
		err = dataMgr.timeVolCat_->startBlobTx(volumeUuid, bolIter->blob_name, blobMode, txPtr);
		if(!err.OK()) {
			LOGERROR << "Failed to start transaction for volume " << volumeUuid
					<< " blob: " << bolIter->blob_name;
			return err;
		}
		dataMgr.timeVolCat_->updateBlobTx(volumeUuid, txPtr, bolIter->blob_diff_list);
		if(!err.OK()) {
			LOGERROR << "Failed to update blob " << bolIter->blob_name << " for volume " << volumeUuid;
			err = dataMgr.timeVolCat_->abortBlobTx(volumeUuid, txPtr);
			if (!err.OK()) {
				LOGERROR << "Failed to abort blob " << bolIter->blob_name << " for volume " << volumeUuid;
			}
			return err;
		}
		err = dataMgr.timeVolCat_->commitBlobTx(volumeUuid, bolIter->blob_name, txPtr, msg->msg_seq_id,
						std::bind(&dm::DmMigrationDeltaBlobHandler::volumeCatalogCb,
						static_cast<dm::DmMigrationDeltaBlobHandler*>(dataMgr.handlers[FDS_DM_MIG_DELTA_BLOB]),
						std::placeholders::_1, std::placeholders::_2, std::placeholders::_3,
						std::placeholders::_4, std::placeholders::_5, commitBlobReq));
		if(!err.OK()) {
			LOGERROR << "Failed to commit blob " << bolIter->blob_name << " for volume " << volumeUuid;
			err = dataMgr.timeVolCat_->abortBlobTx(volumeUuid, txPtr);
			if (!err.OK()) {
				LOGERROR << "Failed to abort blob " << bolIter->blob_name << " for volume " << volumeUuid;
			}
			/**
			 * It's ok to return because commitBlobReq gets freed in the callback that gets called
			 * regardless of the error code.
			 */
			return err;
		} else if (!atLeastOneBlobCommitted) {
			/**
			 * This message is used as part of migration tests, to make sure that
			 * there's no regression in our DM Migration commit transactions.
			 */
			LOGMIGRATE << "At least one delta set was applied and committed";
			atLeastOneBlobCommitted = true;
		}
	}
	return err;
}

Error
DmMigrationExecutor::processIncomingDeltaSetCb()
{
	fds_bool_t allCbCalled = false;
	deltaBlobSetCbHelper.mtx.lock();
	fds_verify(deltaBlobSetCbHelper.expectedCount > deltaBlobSetCbHelper.actualCbCounted);
	++deltaBlobSetCbHelper.actualCbCounted;
	allCbCalled = ((deltaBlobSetCbHelper.actualCbCounted == deltaBlobSetCbHelper.expectedCount) &&
			deltaBlobSetCbHelper.expectedCountFinalized);
	deltaBlobSetCbHelper.mtx.unlock();

	if (allCbCalled) {
		// TODO: Call the hook to start applying Blob Descriptors
		LOGMIGRATE << "All Blobs applied for volume " << volumeUuid;
	}

	return ERR_OK;
}

void
DmMigrationExecutor::sequenceTimeoutHandler()
{
	// TODO - part of error handling (FS-2619)
}

}  // namespace fds
