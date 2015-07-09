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
	volID = fds_volid_t(_ribfsm->volumeId);
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

void
DmMigrationClient::processDiff()
{

}

Error
DmMigrationClient::handleInitialBlobFilterMsg()
{
	LOGMIGRATE << "Taking snapshot for volume: " << volID;

	dataMgr.timeVolCat_->queryIface()->getVolumeSnapshot(volID, opts);

	dataMgr.timeVolCat_->queryIface()->getAllBlobsWithSequenceIdSnap(volID,
			ribfsm->blobFilterMap, opts);

	/**
	 * TODO Use the diff function (FS-2259) and genrate the actual diff set.
	 */

	// migrDoneHandler(uniqueId, threadErr);
	return (ERR_OK);
}
}  // namespace fds
