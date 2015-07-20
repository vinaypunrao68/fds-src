/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

#include <DataMgr.h>
#include <DmMigrationClient.h>

namespace fds {

extern std::string logString(const fpi::CtrlNotifyDeltaBlobDescMsg &msg);
extern std::string logString(const fpi::CtrlNotifyDeltaBlobsMsg &msg);

DmMigrationClient::DmMigrationClient(DmIoReqHandler* _DmReqHandle,
										DataMgr& _dataMgr,
										const NodeUuid& _myUuid,
										NodeUuid& _destDmUuid,
										fpi::CtrlNotifyInitialBlobFilterSetMsgPtr& _ribfsm,
										DmMigrationClientDoneHandler _handle,
                                        uint64_t _maxDeltaBlobs,
                                        uint64_t _maxDeltaBlobDescs)
    : DmReqHandler(_DmReqHandle), migrDoneHandler(_handle), mySvcUuid(_myUuid),
	  destDmUuid(_destDmUuid), dataMgr(_dataMgr), ribfsm(_ribfsm),
      maxNumBlobs(_maxDeltaBlobs), maxNumBlobDescs(_maxDeltaBlobDescs)
{
	volId = fds_volid_t(_ribfsm->volumeId);
    seqNumBlobs = ATOMIC_VAR_INIT(0UL);
    seqNumBlobDescs = ATOMIC_VAR_INIT(0UL);
}

DmMigrationClient::~DmMigrationClient()
{

}

uint64_t
DmMigrationClient::getSeqNumBlobs()
{
    return std::atomic_fetch_add(&seqNumBlobs, 1UL);
}

void
DmMigrationClient::resetSeqNumBlobs()
{
    LOGMIGRATE << "Resetting seqNumBlobs=0";
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
    LOGMIGRATE << "Resetting seqNumBlobs=0";
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
    auto source_it = source.cbegin();
    auto dest_it = dest.cbegin();

    while (dest_it != dest.cend() && source_it != source.cend()) {
        if (dest_it->first == source_it->first) {
            /* NOTE: this assumes we overwrite more recent versions on the Dest.
               Switch the comparison to '<' to only overwite older versions */
            if (dest_it->second != source_it->second) {
                // update blob on dest to source's version
                update_list.push_back(source_it->first);
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
        // delete blob from dest
        delete_list.push_back(dest_it->first);
        ++dest_it;
    }

    // send the remainder of the Source list
    while (source_it != source.cend()) {
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
	err = dataMgr.timeVolCat_->queryIface()->getAllBlobsWithSequenceId(volId,
                                                                       localBlobMap,
                                                                       snap_);

    if (ERR_OK != err) {
        LOGERROR << "Failed to get blob descriptors with sequence id for volume=" << volId
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
        LOGERROR << "Failed to get blob update list and blob delete list for volume=" << volId
                 << " with error=" << err;
        return ERR_DM_CAT_MIGRATION_DIFF_FAILED;
    }

    LOGMIGRATE << "num blobs update=" << blobUpdateList.size()
               << "num blobs delete=" << blobDeleteList.size();

    // Now generate and set the blob delta set, which consists of list
    // blobs to be updated and deleted (blobs + descriptors.
    err = generateBlobDeltaSets(blobUpdateList, blobDeleteList);
    if (ERR_OK != err) {
        LOGERROR << "Failed go generate blob delta set for volume=" << volId
                 << " with error=" << err;
        return ERR_DM_CAT_MIGRATION_DIFF_FAILED;
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
    deltaBlobsMsg->msg_seq_id = getSeqNumBlobs();

	fpi::CtrlNotifyDeltaBlobDescMsgPtr deltaBlobDescMsg(new fpi::CtrlNotifyDeltaBlobDescMsg());
	deltaBlobDescMsg->volume_id = volId.get();
    deltaBlobDescMsg->msg_seq_id = getSeqNumBlobDescs();

    for (const auto & blobName: updateBlobs) {

        BlobMetaDesc metaDesc;
        fpi::DMMigrationObjListDiff objList;
        objList.blob_name = blobName;

        LOGMIGRATE << "Getting blobs and blob descriptor for blob=" << blobName;

        // Now get blobs and blob descriptor for given blob name.
	    err = dataMgr.timeVolCat_->queryIface()->getBlobAndMetaFromSnapshot(volId,
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

        LOGMIGRATE << "Got blobs and blob descriptor for blob name=" << blobName
                   << ", Nblobs=" << deltaBlobDescMsg->blob_desc_list.size();

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
            deltaBlobDescMsg.reset(new fpi::CtrlNotifyDeltaBlobDescMsg);
            deltaBlobDescMsg->volume_id = volId.get();
            deltaBlobDescMsg->msg_seq_id = getSeqNumBlobDescs();
        }

        if (deltaBlobsMsg->blob_obj_list.size() >= maxNumBlobs) {
            /**
             * send the blob desc to thd destination dm.
             */
            err = sendDeltaBlobs(deltaBlobsMsg);
            fds_verify(ERR_OK == err);
            /**
             * Need to allocate a new delta blobs message, since the message
             * can be sitting in the service layer (or another possible threadpool)
             * that can execute asynchronously.
             */
            deltaBlobsMsg.reset(new fpi::CtrlNotifyDeltaBlobsMsg);
            deltaBlobsMsg->volume_id = volId.get();
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
    deltaBlobDescMsg->msg_seq_id = getSeqNumBlobDescs();

    /**
     * Loop and generate delta desc msg for the delete blobs.
     * Occassionaly send the message if the max payload is achieved.
     */
    for (const auto & blobName: deleteBlobs) {
        fpi::DMBlobDescListDiff blobDesc;

        /**
         * Add blob name to the descriptor list.
         */
        blobDesc.vol_blob_name = blobName;
        /**
         * Intentionally not mofidying vol_blob_desc, since it should be 0 strlen.
         */
        LOGMIGRATE << "Adding DELETE blob=" << blobDesc.vol_blob_name
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
        LOGERROR << "Failed generate delete blobs for volume=" << volId
                 << " with error=" << err;
        return err;
    }

    err = generateUpdateBlobDeltaSets(updateBlobs);
    if (ERR_OK != err) {
        LOGERROR << "Failed generate update blobs for volume=" << volId
                 << " with error=" << err;
        return err;
    }

    return err;
}

Error
DmMigrationClient::processBlobFilterSet()
{
    Error err(ERR_OK);

	LOGMIGRATE << "Taking snapshot for volume: " << volId;

    // Get snapshot for the volume.
	err = dataMgr.timeVolCat_->queryIface()->getVolumeSnapshot(volId, snap_);
    if (ERR_OK != err) {
        LOGERROR << "Failed to get snapshot volume=" << volId
                 << " with error=" << err;
        return err;
    }

    // process blob diff
    err = processBlobDiff();
    if (ERR_OK != err) {
       LOGERROR << "Failed to process blob diff on volume=" << volId
                 << " with error=" << err;
        return err;
    }

    // free the in-memory snapshot diff after completion.
    err = dataMgr.timeVolCat_->queryIface()->freeVolumeSnapshot(volId, snap_);
    if (ERR_OK != err) {
       LOGERROR << "Failed to free snapshot on volume=" << volId
                 << " with error=" << err;
        return err;
    }

    // if completion handler is registered call it.
	if (migrDoneHandler) {
        LOGMIGRATE << "Calling migration client done handler";
        migrDoneHandler(volId, err);
    }

	return err;
}

Error
DmMigrationClient::sendDeltaBlobs(fpi::CtrlNotifyDeltaBlobsMsgPtr& blobsMsg)
{
    Error err(ERR_OK);

	LOGMIGRATE << "Sending blobs to: " << std::hex << destDmUuid << std::dec
               << " " << logString(*blobsMsg);

    /**
     * Send fire and forget message consisting of blobs to the destination DM.
     */
    fds_verify(static_cast<fds_volid_t>(blobsMsg->volume_id) == volId);
    auto asyncDeltaBlobsMsg = gSvcRequestPool->newEPSvcRequest(destDmUuid.toSvcUuid());
    asyncDeltaBlobsMsg->setTimeoutMs(0);
	asyncDeltaBlobsMsg->setPayload(FDSP_MSG_TYPEID(fpi::CtrlNotifyDeltaBlobsMsg),
                                                   blobsMsg);
    asyncDeltaBlobsMsg->invoke();

    return err;
}

Error
DmMigrationClient::sendDeltaBlobDescs(fpi::CtrlNotifyDeltaBlobDescMsgPtr& blobDescMsg)
{
     Error err(ERR_OK);

       LOGMIGRATE << "Sending blob descs to: " << std::hex << destDmUuid << std::dec
                << " " << logString(*blobDescMsg);

     /**
      * Send fire and forget message consisting of blob descriptors to the destination DM.
      */
     fds_verify(static_cast<fds_volid_t>(blobDescMsg->volume_id) == volId);
     auto asyncDeltaBlobDescMsg = gSvcRequestPool->newEPSvcRequest(destDmUuid.toSvcUuid());
     asyncDeltaBlobDescMsg->setTimeoutMs(0);
       asyncDeltaBlobDescMsg->setPayload(FDSP_MSG_TYPEID(fpi::CtrlNotifyDeltaBlobDescMsg),
                                                       blobDescMsg);
     asyncDeltaBlobDescMsg->invoke();

       return err;
}
}  // namespace fds
