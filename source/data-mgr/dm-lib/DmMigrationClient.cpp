/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

#include <DataMgr.h>
#include <DmMigrationClient.h>

namespace fds {

DmMigrationClient::DmMigrationClient(DmIoReqHandler* _DmReqHandle,
										DataMgr& _dataMgr,
										const NodeUuid& _myUuid,
										NodeUuid& _destDmUuid,
										fpi::CtrlNotifyInitialBlobFilterSetMsgPtr& _ribfsm,
										DmMigrationClientDoneHandler _handle)
    : DmReqHandler(_DmReqHandle), migrDoneHandler(_handle), mySvcUuid(_myUuid),
	  destDmUuid(_destDmUuid), dataMgr(_dataMgr), ribfsm(_ribfsm)
{
	volId = fds_volid_t(_ribfsm->volumeId);
}

DmMigrationClient::~DmMigrationClient()
{

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
DmMigrationClient::diffBlobLists(const std::map<int64_t, int64_t>& dest,
                                 const std::map<int64_t, int64_t>& source,
                                 std::vector<fds_uint64_t>& update_list,
                                 std::vector<fds_uint64_t>& delete_list)
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
DmMigrationClient::processBlobDescDiff()
{
    Error err(ERR_OK);

    // gather all blob blob descriptors with sequence id.
    // the snapshot should've been taken before calling this.
    std::map<int64_t, int64_t> localBlobMap;
	err = dataMgr.timeVolCat_->queryIface()->getAllBlobsWithSequenceIdSnap(volId,
                                                                           localBlobMap,
                                                                           opts);
    if (ERR_OK != err) {
        LOGERROR << "Failed to get blob descriptors with sequence id for volume=" << volId
                 << " with error=" << err;
        return ERR_DM_CAT_MIGRATION_DIFF_FAILED;
    }

    // using the destination DM's blob descs with seq id and
    // source DM's blob desc with seq ids, generate the list of blobs
    // to be updated or deleted on the destination side.
    std::vector<fds_uint64_t> blobUpdateList;
    std::vector<fds_uint64_t> blobDeleteList;
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

    // TODO(Sean): fs-2260

    return err;
}

/**
 * For testing only
 */
#define MAX_TEST_BLOB_MSGS 20
Error
DmMigrationClient::processBlobFilterSet()
{
    Error err(ERR_OK);

	LOGMIGRATE << "Taking snapshot for volume: " << volId;

    // Get snapshot for the volume.
	err = dataMgr.timeVolCat_->queryIface()->getVolumeSnapshot(volId, opts);
    if (ERR_OK != err) {
        LOGERROR << "Failed to get snapshot volume=" << volId
                 << " with error=" << err;
        return err;
    }

    // process blob diff
    err = processBlobDescDiff();
    if (ERR_OK != err) {
       LOGERROR << "Failed to process blob diff on volume=" << volId
                 << " with error=" << err;
        return err;
    }

	/**
     * TODO(Neil) Used for testing for sendCtrlNotifyDeltaBlobs()
	 */
	generateRandomDeltaBlobs(myBlobMsgs);
	fds_verify(myBlobMsgs.size() == MAX_TEST_BLOB_MSGS);

	/**
	 * TODO(Neil) Just a prototype. Currently coring, need more investigation.
	 */
	// sendCtrlNotifyDeltaBlobs();

    // free the in-memory snapshot diff after completion.
    err = dataMgr.timeVolCat_->queryIface()->freeVolumeSnapshot(volId, opts);
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

/**
  * For testing only.
  */
#define MAX_TEST_BLOB_MSGS 20
Error
DmMigrationClient::generateRandomDeltaBlobs(std::vector<fpi::CtrlNotifyDeltaBlobsPtr> &blobsMsg)
{
	fds_bool_t testLast = false;
	unsigned testBlobId = 0;
	for (int i = 0; i < MAX_TEST_BLOB_MSGS; i++) {
		fpi::CtrlNotifyDeltaBlobsPtr aPtr(new fpi::CtrlNotifyDeltaBlobs);
		aPtr->volume_id = volId.v;
		aPtr->msg_seq_id = i;
		if ((i+1) == MAX_TEST_BLOB_MSGS) {
			aPtr->last_msg_seq_id = true;
			testLast = true;
		}
		testBlobId = i > 9 ? 2 : 1;

		fpi::DMMigrationObjListDiff testObjList;
		testObjList.blob_id = testBlobId;
		fpi::DMBlobObjListDiff testBlobDiff;
		testBlobDiff.obj_offset = 0x50; // random
		// testBlobDiff.obj_id = fpi::FDS_ObjectIdType(0);
		testObjList.blob_diff_list.push_back(testBlobDiff);
		aPtr->blob_obj_list.push_back(testObjList);

		blobsMsg.push_back(aPtr);
	}

	fds_verify(testLast);

	return (ERR_OK);
}

void
DmMigrationClient::sendCtrlNotifyDeltaBlobs()
{
	if (myBlobMsgs.size() == 0) {
		return;
	}
	LOGMIGRATE << "Sending CtrlNotifyDeltaBlobs msgs to DM: " << destDmUuid;
	auto asyncDeltaBlobsMsg = gSvcRequestPool->newEPSvcRequest(destDmUuid.toSvcUuid());
	asyncDeltaBlobsMsg->setTimeoutMs(0);

	for (unsigned i = 0; i < myBlobMsgs.size(); i++) {
		fds_verify((unsigned)myBlobMsgs[i]->volume_id == volId.v);
		// TODO(Neil) - this needs to be fixed - currently coring.
		asyncDeltaBlobsMsg->setPayload(FDSP_MSG_TYPEID(fpi::CtrlNotifyDeltaBlobs), myBlobMsgs[i]);
		asyncDeltaBlobsMsg->invoke();
	}
}
}  // namespace fds
