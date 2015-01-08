/*
 * Copyright 2015 Formation Data Systems, Inc.
 */
#include <MigrationClient.h>

namespace fds {

MigrationClient::MigrationClient(SmIoReqHandler *_dataStore,
                                 NodeUuid _destinationSMNodeID,
                                 fds_token_id _sm_tokenID)
    : dataStore(_dataStore),
      destinatiomSMNodeID(_destinationSMNodeID),
      SMTokenID(_sm_tokenID)
{
    snapshotRequest.io_type = FDS_SM_SNAPSHOT_TOKEN;
    snapshotRequest.token_id = tokenID;
    snapshotRequest.smio_snap_resp_cb = std::bind(&MigrationClient::migClientSnapshotCB,
                                                  this,
                                                  std::placeholders::_1,
                                                  std::placeholders::_2,
                                                  std::placeholders::_3,
                                                  std::placeholders::_4);
}

MigrationClient::~MigrationClient()
{
}


Error
MigrationClient::snapshotObjects()
{
    Error err(ERR_OK);

    LOGDEBUG << "Request to snapshot token "
             << tokenID
             << " from "
             << sourceSMNodeID;

    err = dataStore->enqueueMsg(FdsSysTaskQueueId, &snapshotRequest);
    if (!err.ok()) {
        LOGERROR << "Failed to snapshot. err=" << err;
        return err;
    }

    /**
     * Not waiting for completion.  We will filter the objects in the callback
     * or somewhere else.
     */
    return err;
}

void
MigrationClient::migrationClientSnapshotCB(const Error& error,
                                           SmIoSnapshotObjectDB* snapRequest,
                                           leveldb::ReadOptions& options,
                                           leveldb::DB *db)
{
    /**
     * Save off the levelDB information.
     */
    snapshotLevelDB = db;
    iteratorLevelDB = db->NewIterator(options);

    /**
     * TODO(Sean): For now, do nothing
     */

    /**
     * Intentionally not releasing snapshot, since there is more work to do.
     */
}

}  // namespace fds
