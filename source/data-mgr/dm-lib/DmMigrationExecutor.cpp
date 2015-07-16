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
      migrDoneCb(_callback)
{
    volumeUuid = volDesc.volUUID;
	LOGMIGRATE << "Migration executor received for volume ID " << volDesc;
	dsHelper.lastMsg = 0;
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
                                   &volDesc,
                                   false);

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

void
DmMigrationExecutor::deltaSetHelper::recordMsgSeqId(fpi::CtrlNotifyDeltaBlobsMsgPtr &msg)
{
	fds_verify(msgMap.find(msg->msg_seq_id) == msgMap.end());
	msgMap[msg->msg_seq_id] = true;
	if (msg->last_msg_seq_id) {
		lastMsg = msg->msg_seq_id;
	}
}

fds_uint64_t
DmMigrationExecutor::deltaSetHelper::recallNumOfMsgsReceived()
{
	return (msgMap.size());
}

fds_bool_t
DmMigrationExecutor::deltaSetHelper::blobSetIsComplete()
{
	return (msgMap.size() == lastMsg);
}

Error
DmMigrationExecutor::processIncomingDeltaSet(fpi::CtrlNotifyDeltaBlobsMsgPtr &msg)
{
	/**
	 * TODO : start buffering process. FS-2486 is to be implemented here.
	 */
	LOGMIGRATE << "Processing incoming CtrlNotifyDeltaBlobsMsg for volume " << volumeUuid;
	fds_verify(volumeUuid == fds_volid_t(msg->volume_id));
	if (msg->blob_obj_list.size() == 0) {
		LOGERROR << "Volume Blob object list size is 0 for volume: "
				<< msg->volume_id;
		return ERR_INVALID_ARG;
	}

	LOGMIGRATE "NEIL DEBUG START volume: " << msg->volume_id;
	Error err(ERR_OK);
	std::string blobName;
	fds_uint32_t blobMode = 0;
	for (blobObjListIter bolIter = msg->blob_obj_list.begin();
			bolIter != msg->blob_obj_list.end(); ++bolIter) {
		// Each iter represents a blob and its diff list
		blobName = blobName.empty() ? bolIter->blob_name : blobName;
		if (bolIter->blob_name != blobName) {
			// TODO NEW BLOB HANDLE
		}
		// The version numbers will not matter since we're going to overwrite it w/ blobdesc later
		DmIoCommitBlobTx* commitBlobReq = new DmIoCommitBlobTx(volumeUuid, blobName, 0, 0, 0);
		BlobTxId newTx(dsHelper.randNumGen->genNumSafe());
		BlobTxId::ptr txPtr = boost::make_shared<BlobTxId>(newTx);
		err = dataMgr.timeVolCat_->startBlobTx(volumeUuid, blobName, blobMode, txPtr);
		if(!err.OK()) {
			LOGERROR << "Failed to start transaction for volume " << volumeUuid
					<< " blob: " << blobName;
			return err;
		}
		dataMgr.timeVolCat_->updateBlobTx(volumeUuid, txPtr, bolIter->blob_diff_list);
		if(!err.OK()) {
			LOGERROR << "Failed to update blob " << blobName << " for volume " << volumeUuid;
			err = dataMgr.timeVolCat_->abortBlobTx(volumeUuid, txPtr);
			if (!err.OK()) {
				LOGERROR << "Failed to abort blob " << blobName << " for volume " << volumeUuid;
			}
			return err;
		}
		LOGMIGRATE << "NEIL DEBUG COMMITTING BLOB";
		err = dataMgr.timeVolCat_->commitBlobTx(volumeUuid, blobName, txPtr, msg->msg_seq_id,
						std::bind(&dm::DmMigrationDeltablobHandler::volumeCatalogCb,
						static_cast<dm::DmMigrationDeltablobHandler*>(dataMgr.handlers[FDS_DM_MIG_DELT_BLB]),
						std::placeholders::_1, std::placeholders::_2, std::placeholders::_3,
						std::placeholders::_4, std::placeholders::_5, commitBlobReq));
		if(!err.OK()) {
			LOGERROR << "Failed to commit blob " << blobName << " for volume " << volumeUuid;
			err = dataMgr.timeVolCat_->abortBlobTx(volumeUuid, txPtr);
			if (!err.OK()) {
				LOGERROR << "Failed to abort blob " << blobName << " for volume " << volumeUuid;
			}
			/**
			 * It's ok to return because commitBlobReq gets freed in the callback that gets called
			 * regardless of the error code.
			 */
			return err;
		}
	}

#if 0
    /**
     * Applies a new offset update to an existing transaction
     * @param[in] volId volume ID
     * @param[in] txDesc  Transaction ID
     * @param[in] objList List of blob offsets being updated
     *
     * @return ERR_OK if the update was successfully applied
     * to the transaction
     */
    Error updateBlobTx(fds_volid_t volId,
                       BlobTxId::const_ptr txDesc,
                       const fpi::FDSP_BlobObjectList &objList);


    Error startBlobTx(fds_volid_t volId,
                      const std::string &blobName,
                      fds_int32_t blobMode,
                      BlobTxId::const_ptr txDesc);
    /**
     * Commits the updates associated with an existing transaction
     * @param[in] volId volume ID
     * @param[in] blobName Name of blob
     * @param[in] txDesc Transaction ID
     * @param[in] commitCb commit callback
     *
     * @return ERR_OK if the commit was successfully applied
     */
    Error commitBlobTx(fds_volid_t volId,
                       const std::string &blobName,
                       BlobTxId::const_ptr txDesc,
                       sequence_id_t seq_id,
                       const DmTimeVolCatalog::CommitCb &commitCb);

    typedef std::function<void (const Error &,
                                blob_version_t,
                                const BlobObjList::const_ptr&,
                                const MetaDataList::const_ptr&,
                                const fds_uint64_t)> CommitCb;


	err = dataMgr.timeVolCat_->volcat->putPartialBlob(volumeUuid, blobName,
			boost::make_shared<BlobObjList>(list));
#endif

	return err;
}
}  // namespace fds
