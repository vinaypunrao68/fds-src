/*
 * Copyright 2015 Formation Data Systems, Inc.
 */
#include <fds_assert.h>

#include <MigrationClient.h>
#include <MigrationExecutor.h>

#include <object-store/SmDiskMap.h>

namespace fds {


MigrationClient::MigrationClient(SmIoReqHandler *_dataStore,
                                 NodeUuid& _destSMNodeID)
    : dataStore(_dataStore),
      destSMNodeID(_destSMNodeID)
{
    snapshotRequest.io_type = FDS_SM_SNAPSHOT_TOKEN;
    snapshotRequest.smio_snap_resp_cb = std::bind(&MigrationClient::migClientSnapshotCB,
                                                  this,
                                                  std::placeholders::_1,
                                                  std::placeholders::_2,
                                                  std::placeholders::_3,
                                                  std::placeholders::_4);
    SMTokenID = SMTokenInvalidID;
    executorID = invalidExecutorID;
}

MigrationClient::~MigrationClient()
{
}


Error
MigrationClient::migClientSnapshotMetaData()
{
    Error err(ERR_OK);

    /* Should already have a valid sm token
     */
    fds_verify(SMTokenID != SMTokenInvalidID);
    snapshotRequest.token_id = SMTokenID;

    err = dataStore->enqueueMsg(FdsSysTaskQueueId, &snapshotRequest);
    if (!err.ok()) {
        LOGERROR << "Failed to snapshot. err=" << err;
        return err;
    }

    /* Not waiting for completion.  We will filter the objects in the callback
     * or somewhere else.
     */
    return err;
}

void
MigrationClient::migClientSnapshotCB(const Error& error,
                                     SmIoSnapshotObjectDB* snapRequest,
                                     leveldb::ReadOptions& options,
                                     leveldb::DB *db)
{
    /* Save off the levelDB information.
     */
    snapDB = db;
    iterDB = db->NewIterator(options);

    /* Iterate through level db and filter against the objectFilterSet.
     */
    for (iterDB->SeekToFirst(); iterDB->Valid(); iterDB->Next()) {
        ObjectID objId(iterDB->key().ToString());

        /* two level filter for now:
         * 1) filter against dltTokenIDs.
         * 2) filter against objectSet.
         */
    }
}

bool
MigrationClient::migClientVerifyDestination(fds_token_id dltToken,
                                           uint64_t executorId)
{
    /* Need to copy the set of objects to the
     */
    fds_token_id derivedSMTokenID = SmDiskMap::smTokenId(dltToken);

    LOGDEBUG << "Dest SM: " << destSMNodeID << ", "
             << "DLT Token: " << dltToken << ", "
             << "SM Token: " << derivedSMTokenID << ", "
             << "ExecutorID: " << executorId;

    /* Update the SM token with the given DLT token.
     */
    if (SMTokenID == SMTokenInvalidID) {
        SMTokenID = derivedSMTokenID;
    } else {
        /* Verify dltToken belongs to the same SM token ID.
         * In this case, panic is preferred.
         */
        if (SMTokenID != derivedSMTokenID) {
            return false;
        }
    }

    /* Save off executor ID, so when we send back, the migrationMgr knows
     * which instance it maps to.
     */
    if (executorID == invalidExecutorID) {
        executorID = executorId;
    } else {
        if (executorID != executorId) {
            return false;
        }
    }
    return true;
}


Error
MigrationClient::migClientAddObjectSet(fpi::CtrlObjectRebalanceFilterSetPtr &filterSet)
{
    /* Verify that the token and executor ID matches known SM token and perviously
     * set exector ID.
     */
    uint64_t curSeqNum = filterSet->seqNum;
    fds_token_id dltToken = filterSet->tokenId;
    uint64_t executorId = filterSet->executorID;
    bool completeSet = false;
    Error err(ERR_OK);

    /* Verify message destination.
     */
    bool verifySuccess = migClientVerifyDestination(dltToken, executorId);
    if (!verifySuccess) {
            return ERR_SM_TOK_MIGRATION_DESTINATION_MSG_CORRUPT;
    }

    /* For debugging, keep set of DLT token ids.
     */
    dltTokenIDs.insert(filterSet->tokenId);

    /* Now, add the objects and corresponding refcnts to the local filter set.
     */
    for (auto& objAndRefCnt : filterSet->objectsToFilter) {
      filterObjectSet.emplace(ObjectID(objAndRefCnt.objectID.digest),
                              objAndRefCnt.objRefCnt);
    }

    /* Set the sequence number of the received message to ensure all messages are received.
     */
    completeSet = seqNumFilterSet.setSeqNum(filterSet->seqNum, filterSet->lastFilterSet);
    if (true == completeSet) {
        /* All messages from destination SM is received.  filter object set is complete.
         */
        err = migClientSnapshotMetaData();
        if (!err.ok()) {
            return err;
        }
    }

    return err;
}

}  // namespace fds
