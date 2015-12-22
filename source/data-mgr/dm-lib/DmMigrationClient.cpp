/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

#include <DataMgr.h>
#include <DmMigrationClient.h>

namespace fds {

extern std::string logString(const fpi::CtrlNotifyDeltaBlobDescMsg &msg);
extern std::string logString(const fpi::CtrlNotifyDeltaBlobsMsg &msg);

DmMigrationClient::DmMigrationClient(DmIoReqHandler* _DmReqHandle,
                                     DataMgr* _dataMgr,
                                     const NodeUuid& _myUuid,
                                     NodeUuid& _destDmUuid,
                                     int64_t migrationId,
                                     fpi::CtrlNotifyInitialBlobFilterSetMsgPtr _ribfsm,
                                     DmMigrationClientDoneHandler _handle,
                                     migrationCb _cleanup,
                                     uint64_t _maxDeltaBlobs,
                                     uint64_t _maxDeltaBlobDescs,
                                     bool _volumeGroupMode)
    : DmMigrationBase(migrationId, _dataMgr),
      DmReqHandler(_DmReqHandle), migrDoneHandler(_handle),
      mySvcUuid(_myUuid),
	  destDmUuid(_destDmUuid),
	  ribfsm(_ribfsm),
	  snap_(nullptr),
      maxNumBlobs(_maxDeltaBlobs),
      maxNumBlobDescs(_maxDeltaBlobDescs),
	  forwardingIO(false),
	  snapshotTaken(false),
	  abortFlag(false),
	  cleanUp(_cleanup),
	  volumeGroupMode(_volumeGroupMode)
{
	volId = fds_volid_t(_ribfsm->volumeId);
    seqNumBlobs = ATOMIC_VAR_INIT(0UL);
    seqNumBlobDescs = ATOMIC_VAR_INIT(0UL);
    dmtVersion = MODULEPROVIDER()->getSvcMgr()->getDMTVersion();
}

DmMigrationClient::~DmMigrationClient()
{
}

void
DmMigrationClient::run()
{
    thrPtr.reset(new std::thread([this] {
        Error err(ERR_OK);
        err = processBlobFilterSet();
        if (ERR_OK != err) {
            LOGERROR << logString() << " Processing filter set failed: " << err << ". Exiting run thread";
            routeAbortMigration();
            return;
        }

        err = processBlobFilterSet2();
        if (ERR_OK != err) {
            // This one doesn't have an async callback to decrement so we fail it manually
            LOGERROR << logString() << " Processing blob diff failed: " << err << ". Exiting run thread";
            routeAbortMigration();
            return;
        }
    }));
}

uint64_t
DmMigrationClient::getSeqNumBlobs()
{
    return std::atomic_fetch_add(&seqNumBlobs, 1UL);
}

void
DmMigrationClient::resetSeqNumBlobs()
{
    LOGMIGRATE << logString() << "Resetting seqNumBlobs=0";
    std::atomic_store(&seqNumBlobs, 0UL);
}

uint64_t
DmMigrationClient::getSeqNumBlobDescs()
{
    return std::atomic_fetch_add(&seqNumBlobDescs, 1UL);
}

void
DmMigrationClient::resetSeqNumBlobDescs()
{
    LOGMIGRATE << logString() << "Resetting seqNumBlobs=0";
    std::atomic_store(&seqNumBlobDescs, 0UL);
}

/**
 *  Diff algorithm operates on two sorted lists of (blob_id, sequence_id) pairs:
 *
 *    Dest | Source |   Action    | State Transition
 *   ______|________|_____________|_____________________
 *     A   |   A    |    no-op    | advance both lists
 *     A   |   A'   |   send A'   | advance both lists
 *     A   |   X    | send del A  | advance Dest list
 *     X   |   B    |   send B    | advance Source list
 *
 *  if Dest runs out first, send the remainder of the Source list.
 *  if Source runs out, delete the remainder of the Dest list.
 *
 *  Algorithm runtime is linear in the size of the input.
 */
Error
DmMigrationClient::diffBlobLists(const std::map<std::string, int64_t>& dest,
                                 const std::map<std::string, int64_t>& source,
                                 std::vector<std::string>& update_list,
                                 std::vector<std::string>& delete_list)
{
    fds_bool_t dummyFlag = false;
    return (diffBlobLists(dest, source, update_list, delete_list, dummyFlag));
}

Error
DmMigrationClient::diffBlobLists(const std::map<std::string, int64_t>& dest,
                                 const std::map<std::string, int64_t>& source,
                                 std::vector<std::string>& update_list,
                                 std::vector<std::string>& delete_list,
                                 const fds_bool_t &abortFlag)
{
    auto source_it = source.cbegin();
    auto dest_it = dest.cbegin();

    while (dest_it != dest.cend() && source_it != source.cend()) {
        // check abort flag
        if (abortFlag) {
            return (ERR_DM_MIGRATION_ABORTED);
        }

        if (dest_it->first == source_it->first) {
            /* NOTE: this assumes we overwrite more recent versions on the Dest.
               Switch the comparison to '<' to only overwite older versions */
            if (dest_it->second != source_it->second) {
                // update blob on dest to source's version
                update_list.push_back(source_it->first);

                if (dest_it->second > source_it->second) {
                    LOGMIGRATE << "Destination has more recent version of blob: "
                               << dest_it->first;
                }
            } // otherwise they match, do nothing

            ++dest_it;
            ++source_it;
        } else if (dest_it->first > source_it->first) {
            // add blob on dest
            update_list.push_back(source_it->first);
            ++source_it;
        } else {
            // delete blob from dest
            delete_list.push_back(dest_it->first);
            ++dest_it;
        }
    }

    // delete the remainder of the Dest list
    while (dest_it != dest.cend()) {
        if (abortFlag) {
            return (ERR_DM_MIGRATION_ABORTED);
        }
        // delete blob from dest
        delete_list.push_back(dest_it->first);
        ++dest_it;
    }

    // send the remainder of the Source list
    while (source_it != source.cend()) {
        if (abortFlag) {
            return (ERR_DM_MIGRATION_ABORTED);
        }
        // add blob on dest
        update_list.push_back(source_it->first);
        ++source_it;
    }

    return ERR_OK;
}


Error
DmMigrationClient::processBlobDiff()
{
    Error err(ERR_OK);

    // gather all blob blob descriptors with sequence id.
    // the snapshot should've been taken before calling this.
    std::map<std::string, int64_t> localBlobMap;
    err = dataMgr->timeVolCat_->queryIface()->getAllBlobsWithSequenceId(volId,
                                                                       localBlobMap,
                                                                       snap_,
                                                                       abortFlag);

    if (ERR_OK != err) {
        LOGERROR << logString() << "Failed to get blob descriptors with sequence id for volume=" << volId
            << " with error=" << err;
        return ERR_DM_CAT_MIGRATION_DIFF_FAILED;
    }

    // using the destination DM's blob descs with seq id and
    // source DM's blob desc with seq ids, generate the list of blobs
    // to be updated or deleted on the destination side.
    std::vector<std::string> blobUpdateList;
    std::vector<std::string> blobDeleteList;
    err = diffBlobLists(ribfsm->blobFilterMap,
                        localBlobMap,
                        blobUpdateList,
                        blobDeleteList);
    if (ERR_OK != err) {
        LOGERROR << logString() << "Failed to get blob update list and blob delete list for volume=" << volId
            << " with error=" << err;
        return err;
    }

    LOGMIGRATE << logString() << "num blobs update=" << blobUpdateList.size()
        << "num blobs delete=" << blobDeleteList.size();

    // Now generate and set the blob delta set, which consists of list
    // blobs to be updated and deleted (blobs + descriptors.
    err = generateBlobDeltaSets(blobUpdateList, blobDeleteList);
    if (ERR_OK != err) {
        LOGERROR << logString() << "Failed to generate blob delta set for volume=" << volId
            << " with error=" << err;
        return err;
    }

    return err;
}

Error
DmMigrationClient::generateUpdateBlobDeltaSets(const std::vector<std::string>& updateBlobs)
{
    Error err(ERR_OK);

    // Allocate the payload message and set the volume id and sequence number
    // Allocate both blobs and blob desc list.
    fpi::CtrlNotifyDeltaBlobsMsgPtr deltaBlobsMsg(new fpi::CtrlNotifyDeltaBlobsMsg());
    deltaBlobsMsg->volume_id = volId.get();
    deltaBlobsMsg->DMT_version = migrationId;
    deltaBlobsMsg->msg_seq_id = getSeqNumBlobs();
    deltaBlobsMsg->volume_group_mode = volumeGroupMode;

    fpi::CtrlNotifyDeltaBlobDescMsgPtr deltaBlobDescMsg(new fpi::CtrlNotifyDeltaBlobDescMsg());
    deltaBlobDescMsg->volume_id = volId.get();
    deltaBlobDescMsg->DMT_version = migrationId;
    deltaBlobDescMsg->msg_seq_id = getSeqNumBlobDescs();
    deltaBlobDescMsg->volume_group_mode = volumeGroupMode;

    for (const auto & blobName: updateBlobs) {
        if (abortFlag) {
            return (ERR_DM_MIGRATION_ABORTED);
        }

        BlobMetaDesc metaDesc;
        fpi::DMMigrationObjListDiff objList;
        objList.blob_name = blobName;

        LOGMIGRATE << logString() << "Getting blobs and blob descriptor for blob=" << blobName;

        // Now get blobs and blob descriptor for given blob name.
        err = dataMgr->timeVolCat_->queryIface()->getBlobAndMetaFromSnapshot(volId,
                                                                            blobName,
                                                                            metaDesc,
                                                                            objList.blob_diff_list,
                                                                            snap_);
        // for now, just panic if they don't work.
        fds_verify(ERR_OK == err);

        // Add blobs to the delta blobs msg.
        deltaBlobsMsg->blob_obj_list.emplace_back(objList);

        // Add blob descriptor to delta blob desc msg.
        fpi::DMBlobDescListDiff blobDesc;
        blobDesc.vol_blob_name = blobName;

        err = metaDesc.getSerialized(blobDesc.vol_blob_desc);
        // for now, just panic if they don't work.
        fds_verify(ERR_OK == err);

        deltaBlobDescMsg->blob_desc_list.emplace_back(blobDesc);

        LOGMIGRATE << logString() << "Got blobs and blob descriptor for blob name=" << blobName
            << ", Nblobs=" << deltaBlobDescMsg->blob_desc_list.size();

        if (deltaBlobDescMsg->blob_desc_list.size() >= maxNumBlobDescs) {
            /**
             * send the blob desc to thd destination dm.
             */
            dataMgr->counters->totalSizeOfDataMigrated.incr(sizeOfData(deltaBlobDescMsg));
            err = sendDeltaBlobDescs(deltaBlobDescMsg);
            fds_verify(ERR_OK == err);
            /**
             * Need to allocate a new delta blobs desc message, since the message
             * can be sitting in the service layer (or another possible threadpool)
             * that can execute asynchronously.
             */
            deltaBlobDescMsg.reset(new fpi::CtrlNotifyDeltaBlobDescMsg);
            deltaBlobDescMsg->volume_id = volId.get();
            deltaBlobDescMsg->DMT_version = migrationId;
            deltaBlobDescMsg->msg_seq_id = getSeqNumBlobDescs();
        }

        if (deltaBlobsMsg->blob_obj_list.size() >= maxNumBlobs) {
            /**
             * send the blob desc to thd destination dm.
             */
            dataMgr->counters->totalSizeOfDataMigrated.incr(sizeOfData(deltaBlobsMsg));
            err = sendDeltaBlobs(deltaBlobsMsg);
            fds_verify(ERR_OK == err);
            /**
             * Need to allocate a new delta blobs message, since the message
             * can be sitting in the service layer (or another possible threadpool)
             * that can execute asynchronously.
             */
            deltaBlobsMsg.reset(new fpi::CtrlNotifyDeltaBlobsMsg);
            deltaBlobsMsg->volume_id = volId.get();
            deltaBlobsMsg->DMT_version = migrationId;
            deltaBlobsMsg->msg_seq_id = getSeqNumBlobs();
        }
    }

    /**
     * Send the deltaBlobDescMsg to the destination DM.  This can be empty.
     * Note: Alway set the last_msg_seq_id to true here.
     */
    deltaBlobDescMsg->last_msg_seq_id = true;
    err = sendDeltaBlobDescs(deltaBlobDescMsg);
    fds_verify(ERR_OK == err);

    /**
     * Send the deltaBlobsMsg to the destination DM.  This can be empty.
     * Note: Alway set the last_msg_seq_id to true here.
     */
    deltaBlobsMsg->last_msg_seq_id = true;
    err = sendDeltaBlobs(deltaBlobsMsg);
    fds_verify(ERR_OK == err);

    return err;
}

Error
DmMigrationClient::generateDeleteBlobDeltaSets(const std::vector<std::string>& deleteBlobs)
{
    Error err(ERR_OK);

    /**
     * Since we are just dealing with delete blobs, use the blob desc msg to send
     * blobs to be deleted.
     * If the payload doesn't have vol_blob_desc set, then the blob will be deleted
     * on the destination side.
     */
    fpi::CtrlNotifyDeltaBlobDescMsgPtr deltaBlobDescMsg(new fpi::CtrlNotifyDeltaBlobDescMsg());
    deltaBlobDescMsg->volume_id = volId.get();
    deltaBlobDescMsg->DMT_version = migrationId;
    deltaBlobDescMsg->msg_seq_id = getSeqNumBlobDescs();
    deltaBlobDescMsg->volume_group_mode = volumeGroupMode;

    /**
     * Loop and generate delta desc msg for the delete blobs.
     * Occassionaly send the message if the max payload is achieved.
     */
    for (const auto & blobName: deleteBlobs) {
        if (abortFlag) {
            return (ERR_DM_MIGRATION_ABORTED);
        }

        fpi::DMBlobDescListDiff blobDesc;
        /**
         * Add blob name to the descriptor list.
         */
        blobDesc.vol_blob_name = blobName;
        /**
         * Intentionally not mofidying vol_blob_desc, since it should be 0 strlen.
         */
        LOGMIGRATE << logString() << "Adding DELETE blob=" << blobDesc.vol_blob_name
            << " to the descriptor list";
        deltaBlobDescMsg->blob_desc_list.emplace_back(blobDesc);

        if (deltaBlobDescMsg->blob_desc_list.size() >= maxNumBlobDescs) {
            /**
             * send the blob desc to thd destination dm.
             */
            err = sendDeltaBlobDescs(deltaBlobDescMsg);
            fds_verify(ERR_OK == err);
            /**
             * Need to allocate a new delta blobs desc message, since the message
             * can be sitting in the service layer (or another possible threadpool)
             * that can execute asynchronously.
             */
            deltaBlobDescMsg.reset(new fpi::CtrlNotifyDeltaBlobDescMsg());
            deltaBlobDescMsg->volume_id = volId.get();
            deltaBlobDescMsg->DMT_version = migrationId;
            deltaBlobDescMsg->msg_seq_id = getSeqNumBlobDescs();
        }
    }

    /**
     * Send remaining descriptors to destination dm.
     * This can be an empty message.
     */
    err = sendDeltaBlobDescs(deltaBlobDescMsg);
    fds_verify(ERR_OK == err);

    return err;
}

Error
DmMigrationClient::generateBlobDeltaSets(const std::vector<std::string>& updateBlobs,
                                         const std::vector<std::string>& deleteBlobs)
{
    Error err(ERR_OK);

    /**
     * First handle the delete blobs.
     * Then handle the update blobs.
     * Note: The order is very important since  generateUpdateBlobDeltaSets() will
     *       set the last sequence number to true.
     */
    err = generateDeleteBlobDeltaSets(deleteBlobs);
    if (ERR_OK != err) {
        LOGERROR << logString() << "Failed generate delete blobs for volume=" << volId
            << " with error=" << err;
        return err;
    }

    err = generateUpdateBlobDeltaSets(updateBlobs);
    if (ERR_OK != err) {
        LOGERROR << logString() << "Failed generate update blobs for volume=" << volId
            << " with error=" << err;
        return err;
    }

    return err;
}

Error
DmMigrationClient::processBlobFilterSet()
{
    LOGMIGRATE << logString() << "Taking snapshot for volume: " << volId;

    fiu_do_on("abort.dm.migration.processBlobFilter",\
              LOGNOTIFY << "abort.dm.migration processBlobFilter.fault point enabled";\
              return ERR_NOT_READY;);

    // Lookup commit log so we can take a snapshot of the volume while blocking
    // updates
    auto err = dataMgr->timeVolCat_->getCommitlog(volId, commitLog);
    if (!err.ok() || !commitLog) {
        LOGERROR << logString() << "Failed to get snapshot volume=" << volId
                 << " with error=" << err;
        return err;
    }

    // Block commit log and get snapshot for the volume.
    {
        fds_scoped_lock lock(ssTakenScopeLock);
		{
			auto auto_lock = commitLog->getCommitLock(true);
			err = dataMgr->timeVolCat_->queryIface()->getVolumeSnapshot(volId, snap_);
			turnOnForwarding();
		}
		if (ERR_OK != err) {
			LOGERROR << logString() << "Failed to get snapshot volume=" << volId
					 << " with error=" << err;
			return err;
		} else {
		    fds_assert(snap_ != nullptr);
			snapshotTaken = true;
		}
    }

    return err;
}

Error
DmMigrationClient::processBlobFilterSet2()
{
	Error err(ERR_OK);

    fiu_do_on("abort.dm.migration.processBlobFilter2",\
              LOGNOTIFY << "abort.dm.migration processBlobFilter2.fault point enabled";\
              return ERR_NOT_READY;);

	/**
     * This is the main entrance for migrationClient (source) work.
     * It will take care of generating the blobs diff, blobs descriptors, and send
     * them over to the destination side. By the time this method returns, we have already
     * fired and forgotten.
     */
    err = processBlobDiff();
    // free the in-memory snapshot diff after completion.
    {
        fds_scoped_lock lock(ssTakenScopeLock);
    	fds_verify(dataMgr->timeVolCat_->queryIface()->freeVolumeSnapshot(volId, snap_).ok());
    	snapshotTaken = false;
    }

    if (ERR_OK != err) {
        LOGERROR << logString() << "Failed to process blob diff on volume=" << volId
            << " with error=" << err;
        return err;
    }

    // if completion handler is registered call it.
    if (migrDoneHandler) {
        LOGMIGRATE << logString() << "Calling migration client done handler";
        migrDoneHandler(volId, err);
    }

    return err;
}

Error
DmMigrationClient::sendDeltaBlobs(fpi::CtrlNotifyDeltaBlobsMsgPtr& blobsMsg)
{
    Error err(ERR_OK);

    LOGMIGRATE << logString() << "Sending blobs to: " << std::hex << destDmUuid << std::dec
        << " " << fds::logString(*blobsMsg);

    fds_verify(static_cast<fds_volid_t>(blobsMsg->volume_id) == volId);
    auto asyncDeltaBlobsMsg = gSvcRequestPool->newEPSvcRequest(destDmUuid.toSvcUuid());
    asyncDeltaBlobsMsg->setTimeoutMs(dataMgr->dmMigrationMgr->getTimeoutValue());
    asyncDeltaBlobsMsg->setPayload(FDSP_MSG_TYPEID(fpi::CtrlNotifyDeltaBlobsMsg),
                                   blobsMsg);
    // A hack because g++ doesn't like a bind within a macro that does bind
    std::function<void()> abortBind = std::bind(&DmMigrationClient::asyncMsgFailed, this);
    std::function<void()> passBind = std::bind(&DmMigrationClient::asyncMsgPassed, this);
    asyncDeltaBlobsMsg->onResponseCb(RESPONSE_MSG_HANDLER(DmMigrationBase::dmMigrationCheckResp, abortBind, passBind));
	asyncDeltaBlobsMsg->setTaskExecutorId(volId.v);
	asyncMsgIssued();
    asyncDeltaBlobsMsg->invoke();

    return err;
}

Error
DmMigrationClient::sendDeltaBlobDescs(fpi::CtrlNotifyDeltaBlobDescMsgPtr& blobDescMsg)
{
    Error err(ERR_OK);

    LOGMIGRATE << logString() << "Sending blob descs to: " << std::hex << destDmUuid << std::dec
        << " " << fds::logString(*blobDescMsg);

    fds_verify(static_cast<fds_volid_t>(blobDescMsg->volume_id) == volId);
    auto asyncDeltaBlobDescMsg = gSvcRequestPool->newEPSvcRequest(destDmUuid.toSvcUuid());
    asyncDeltaBlobDescMsg->setTimeoutMs(dataMgr->dmMigrationMgr->getTimeoutValue());
    asyncDeltaBlobDescMsg->setPayload(FDSP_MSG_TYPEID(fpi::CtrlNotifyDeltaBlobDescMsg),
                                      blobDescMsg);
    // A hack because g++ doesn't like a bind within a macro that does bind
    std::function<void()> abortBind = std::bind(&DmMigrationClient::asyncMsgFailed, this);
    std::function<void()> passBind = std::bind(&DmMigrationClient::asyncMsgPassed, this);
    asyncDeltaBlobDescMsg->onResponseCb(RESPONSE_MSG_HANDLER(DmMigrationBase::dmMigrationCheckResp, abortBind, passBind));
    asyncDeltaBlobDescMsg->setTaskExecutorId(volId.v);
	asyncMsgIssued();
    asyncDeltaBlobDescMsg->invoke();

    return err;
}

Error
DmMigrationClient::forwardCatalogUpdate(DmIoCommitBlobTx *commitBlobReq,
                                        blob_version_t blob_version,
                                        const BlobObjList::const_ptr& blob_obj_list,
                                        const MetaDataList::const_ptr& meta_list)
{
    Error err(ERR_OK);

    /* Don't forward I/O if this particular client says no */
	if (!forwardingIO.load(std::memory_order_relaxed)) {
		return err;
	}

    LOGMIGRATE << logString() << "Forwarding cat update for vol " << std::hex << commitBlobReq->volId
               << std::dec << " blob " << commitBlobReq->blob_name;

    fpi::ForwardCatalogMsgPtr fwdMsg(new fpi::ForwardCatalogMsg());
    fwdMsg->volume_id = commitBlobReq->volId.get();
    fwdMsg->DMT_version = migrationId;
    fwdMsg->blob_name = commitBlobReq->blob_name;
    fwdMsg->blob_version = blob_version;
    fwdMsg->sequence_id = commitBlobReq->sequence_id;
    // fwdMsg->txId = commitBlobReq->ioBlobTxDesc->getValue();
    fwdMsg->txId = 0;
    blob_obj_list->toFdspPayload(fwdMsg->obj_list);
    meta_list->toFdspPayload(fwdMsg->meta_list);

    // send forward cat update, and pass commitBlobReq as context so we can
    // reply to AM on fwd cat update response
    // auto asyncCatUpdReq = gSvcRequestPool->newEPSvcRequest(this->node_uuid.toSvcUuid());
    auto asyncCatUpdReq = gSvcRequestPool->newEPSvcRequest(destDmUuid.toSvcUuid());
    asyncCatUpdReq->setPayload(FDSP_MSG_TYPEID(fpi::ForwardCatalogMsg), fwdMsg);
    asyncCatUpdReq->setTimeoutMs(dataMgr->dmMigrationMgr->getTimeoutValue());
    asyncCatUpdReq->onResponseCb(RESPONSE_MSG_HANDLER(DmMigrationClient::fwdCatalogUpdateMsgResp,
                                                      commitBlobReq));
    /**
     * There are 2 guarantees:
     * 1. AM will guarantee that the outstanding blob tx's are finished before a new one
     * is started.
     * 2. The source (us) guarantee that the transmission for a same volume
     * are sent over the wire synchronously.
     */
    asyncCatUpdReq->setTaskExecutorId(volId.v);
    asyncMsgIssued();
    asyncCatUpdReq->invoke();

    return err;
}

void DmMigrationClient::fwdCatalogUpdateMsgResp(DmIoCommitBlobTx *commitReq,
												EPSvcRequest* req,
												const Error& error,
												boost::shared_ptr<std::string> payload) {
    LOGMIGRATE << logString()
        << "Received forward catalog update response for blob " << commitReq->blob_name
        << " request that used DMT version " << commitReq->dmt_version
        << " with error " << error;
    // Set the error code to forward failed when we got a timeout so that
    // the caller can differentiate between our timeout and its own.
    if (!error.ok()) {
    	LOGERROR << logString() << "Forwarding failed: " << error << ". Aborting DM Migration";
        asyncMsgFailed();
    }else{
        asyncMsgPassed();
    }

    fds_assert(commitReq->usedForMigration);
    commitReq->localCb(error);
    // commitReq must not be accessed from this point.
    return;
}


fds_bool_t
DmMigrationClient::shouldForwardIO(fds_uint64_t dmtVersionIn)
{
	/**
	 * If the forwarding is turned ON at this point, then we need to check if we still
	 * have outstanding transactions. Otherwise, if it's off at this point, it means
	 * that we aren't on yet, so return false.
	 */
	if (forwardingIO.load(std::memory_order_relaxed) && (dmtVersionIn == dmtVersion)) {
		return true;
	} else {
		// Return false if forwarding is off or if DMT version for the transaction is newer
		return false;
	}
}


void DmMigrationClient::turnOnForwarding() {
	LOGMIGRATE << logString() << "Turning on forwarding for volume: " << volId;
	std::atomic_store(&forwardingIO, true);
}

void DmMigrationClient::turnOffForwardingInternal() {
	LOGMIGRATE << logString() << "Turning off forwarding for volume: " << volId;
	std::atomic_store(&forwardingIO, false);
}

void DmMigrationClient::turnOffForwarding() {
	turnOffForwardingInternal();
	sendFinishFwdMsg();
}

Error
DmMigrationClient::sendFinishFwdMsg()
{
	Error err(ERR_OK);
	fpi::ForwardCatalogMsgPtr finMsg(new fpi::ForwardCatalogMsg());

	LOGMIGRATE << logString() << "Sending an empty finish forwarding message for volume: " << volId;

	finMsg->volume_id = volId.v;
	finMsg->DMT_version = migrationId;
	finMsg->blob_name = "";
	finMsg->lastForward = true;

	auto thriftMsg = gSvcRequestPool->newEPSvcRequest(destDmUuid.toSvcUuid());
	thriftMsg->setPayload(FDSP_MSG_TYPEID(fpi::ForwardCatalogMsg), finMsg);
    thriftMsg->setTimeoutMs(dataMgr->dmMigrationMgr->getTimeoutValue());
    std::function<void()> abortBind = std::bind(&DmMigrationClient::asyncMsgFailed, this);
    std::function<void()> passBind = std::bind(&DmMigrationClient::asyncMsgPassed, this);
    thriftMsg->onResponseCb(RESPONSE_MSG_HANDLER(DmMigrationBase::dmMigrationCheckResp, abortBind, passBind));
	thriftMsg->setTaskExecutorId(volId.v);
	asyncMsgIssued();
	thriftMsg->invoke();

	return (err);
}

void
DmMigrationClient::routeAbortMigration()
{
    dataMgr->dmMigrationMgr->abortMigration();
}

void
DmMigrationClient::abortMigration()
{
    /**
     * Clean up tasks by notifying them via this abortFlag
     */
    abortFlag = true;

	/**
	 * Clean up after processBlobFilterSet()
	 */
	turnOffForwardingInternal();
}

void
DmMigrationClient::finish()
{
    waitForAsyncMsgs();
 	// join the work thread
	if (thrPtr) {
	    LOGDEBUG << "Waiting for volume " << volId << " work thread to complete";
	    thrPtr->join();
	    LOGDEBUG << "Volume " << volId << " work thread to completed";
	}
	{
        fds_scoped_lock lock(ssTakenScopeLock);
		if (snapshotTaken) {
		    fds_assert(snap_ != nullptr);
			dataMgr->timeVolCat_->queryIface()->freeVolumeSnapshot(volId, snap_);
			snapshotTaken = false;
		}
	}
	if (cleanUp) {
	    cleanUp(ERR_OK);
	}
}
}  // namespace fds
