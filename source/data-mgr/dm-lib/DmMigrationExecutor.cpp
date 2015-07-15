/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

#include <DataMgr.h>
#include <DmMigrationExecutor.h>
#include <fdsp/dm_types_types.h>
#include <fdsp/dm_api_types.h>

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
	fds_verify(volumeUuid == fds_volid_t(msg->volume_id));
	if (msg->blob_obj_list.size() == 0) {
		return ERR_INVALID_ARG;
	}
	dsHelper.recordMsgSeqId(msg);

	LOGMIGRATE << "Processing incoming CtrlNotifyDeltaBlobsMsg for volume " << volumeUuid;
	Error err(ERR_OK);
	std::string blobName;
	for (blobObjListIter bolIter = msg->blob_obj_list.begin();
			bolIter != msg->blob_obj_list.end(); ++bolIter) {
		// Each iter represents a blob and its diff list
		blobName = blobName.empty() ? bolIter->blob_name : blobName;
		if (bolIter->blob_name != blobName) {
			// TODO NEW BLOB HANDLE
		}
		BlobTxId newTx;
		BlobTxId::ptr txPtr = boost::make_shared<BlobTxId>(newTx);
		dataMgr.timeVolCat_->startBlobTx(volumeUuid, blobName, 0, txPtr);
	}


#if 0
    Error startBlobTx(fds_volid_t volId,
                      const std::string &blobName,
                      fds_int32_t blobMode,
                      BlobTxId::const_ptr txDesc);

	err = dataMgr.timeVolCat_->volcat->putPartialBlob(volumeUuid, blobName,
			boost::make_shared<BlobObjList>(list));
#endif

	return err;
}
}  // namespace fds
