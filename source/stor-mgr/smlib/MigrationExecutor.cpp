/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

#include <MigrationExecutor.h>

namespace fds {

MigrationExecutor::MigrationExecutor(SmIoReqHandler *_dataStore,
                                     uint64_t _sourceNodeID,
                                     fds_token_id _sm_tokenID)
    : dataStore(_dataStore),
      tokenID(_sm_tokenID),
      sourceSMNodeID(_sourceNodeID),
      metadataSnapshotCompleted(false)
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

    /**
     * If successfully enqueue to snapshot, then wait until the request is complete,
     * since it's done in another thread's context.
     */
    LOGDEBUG << "Waiting for snapshot to complete for token "
             << tokenID;
    std::unique_lock<std::mutex> myLock(metadataSnapshotMutex);
    metadataSnapshotCondVar.wait(myLock,
                                 [this]{ return this->metadataSnapshotCompleted; });

    /**
     * Reset metadata snapshot status.
     */
    metadataSnapshotCompleted = false;

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

    LOGDEBUG << "Generated destination SM rebalance set of objects.";

    /**
     * Now signal the water after the set of objects owned by the destination
     * SM is generated.
     */
    {
        std::lock_guard<std::mutex> myLock(metadataSnapshotMutex);
        metadataSnapshotCompleted = true;
        metadataSnapshotCondVar.notify_all();
    }
}


}  // namespace fds
