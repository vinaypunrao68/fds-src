/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

#include <MigrationExecutor.h>

namespace fds {

MigrationExecutor::MigrationExecutor(SmIoReqHandler *_dataStore,
                                     NodeUuid _sourceNodeID,
                                     fds_token_id _sm_tokenID)
    : dataStore(_dataStore),
      tokenID(_sm_tokenID),
      sourceSMNodeID(_sourceNodeID)
{
    snapshotRequest.io_type = FDS_SM_SNAPSHOT_TOKEN;
    snapshotRequest.token_id = tokenID;
    snapshotRequest.smio_snap_resp_cb = std::bind(&MigrationExecutor::getObjectRebalanceSet,
                                                  this,
                                                  std::placeholders::_1,
                                                  std::placeholders::_2,
                                                  std::placeholders::_3,
                                                  std::placeholders::_4);
}

MigrationExecutor::~MigrationExecutor()
{
}

Error
MigrationExecutor::startObjectRebalance()
{
    Error err(ERR_OK);

    LOGDEBUG << "Request to get set of objects to rebalance token "
             << tokenID
             << " from "
             << sourceSMNodeID;

    err = dataStore->enqueueMsg(FdsSysTaskQueueId, &snapshotRequest);
    if (!err.ok()) {
        LOGERROR << "Failed to snapshot. err=" << err;
        return err;
    }

    return err;
}

void
MigrationExecutor::getObjectRebalanceSet(const Error& error,
                                         SmIoSnapshotObjectDB* snapRequest,
                                         leveldb::ReadOptions& options,
                                         leveldb::DB *db)
{
    /**
     * Iterate through the level db and add to set of objects to rebalance.
     */
    leveldb::Iterator* it = db->NewIterator(options);
    for (it->SeekToFirst(); it->Valid(); it->Next()) {
        ObjectID id(it->key().ToString());

        /**
         * TODO(Sean): add object id to the thrift paired set of object ids and ref count.
         */
    }

    /**
     * After generating a set of objects with the SM token ID, release the snapshot.
     */
    /**
     * TODO(Sean):  To support active IO on both the source and destination SMs,
     *              we need to keep this snapshot until initial set of objects
     *              are relanced.  After that, we need to take another snapshot
     *              to determine if active IOs have changed the state of the existing
     *              objects (i.e. ref cnt) or additional object are written.
     */
    db->ReleaseSnapshot(options.snapshot);

    /**
     * TODO(Sean): Send the set to the source SM.
     */

    LOGDEBUG << "Generated destination SM rebalance set of objects.";
}

}  // namespace fds
