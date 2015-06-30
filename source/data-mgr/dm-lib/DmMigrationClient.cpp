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
										fpi::ResyncInitialBlobFilterSetMsgPtr& _ribfsm,
										DmMigrationClientDoneHandler _handle)
    : DmReqHandler(_DmReqHandle), migrDoneHandler(_handle), mySvcUuid(_myUuid),
	  destDmUuid(_destDmUuid), dataMgr(_dataMgr), ribfsm(_ribfsm)
{

}

DmMigrationClient::~DmMigrationClient()
{

}

/**
 *  Diff algorithm operates on two sorted lists:
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
DmMigrationClient::diffBlobLists(const std::map<int64_t, sequence_id_t>& dest,
                                 const std::map<int64_t, sequence_id_t>& source)
{
    auto source_it = source.cbegin();
    auto dest_it = dest.cbegin();

    while (dest_it != dest.cend() && source_it != source.cend()) {
        if (dest_it.first == source_it.first){
            /* NOTE: this assumes we overwrite more recent versions on the Dest
               switch to < to only overwite older versions */
            if (dest_it.second != source_it.second) {
                // XXX: send source's blob here

            } // otherwise they match, do nothing

            ++dest_it;
            ++source_it;
        }else if (dest_it.first > source_it.first) {
            // XXX: send source's blob here

            ++source_it;
        }else{
            // XXX: send delete dest's blob here

            ++dest_it;
        }
    }

    while (dest_it != dest.cend()) {
        // delete the remainder of the Dest list

        // XXX: send delete dest's blob here

        ++dest_it;
    }

    if (source_it != source.cend()) {
        // send the remainder of the Source list

        // XXX: send source's blob here

        ++source_it;
    }

    return Error.OK;
}

}  // namespace fds
