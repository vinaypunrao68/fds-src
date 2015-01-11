/*
 * Copyright 2015 Formation Data Systems, Inc.
 */
#include <fds_assert.h>

#include <SMSvcHandler.h>
#include <ObjMeta.h>
#include <dlt.h>

#include <MigrationClient.h>

#include <object-store/SmDiskMap.h>

namespace fds {


MigrationClient::MigrationClient(SmIoReqHandler *_dataStore,
                                 NodeUuid& _destSMNodeID,
                                 fds_uint32_t bitsPerToken)
    : dataStore(_dataStore),
      destSMNodeID(_destSMNodeID),
      bitsPerDltToken(bitsPerToken),
      maxDeltaSetSize(128)
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
    ObjMetaData objMetaData;
    uint64_t msgSeqNum = 0;

    /* TODO(Sean): Save off the levelDB information.  Not sure if we need to do this, but depends on the
     *             threading model. if we want to re-queue on QoS, then we need to save the state.  However,
     *             if we continue with the same thread, there is no need to save state.
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
        fds_token_id dltTokenId = DLT::getToken(objId, bitsPerDltToken);

        /* If the object is not in the DLT token set, then it's safe
         * to skip it.
         */
        auto dltTokenFiltered = dltTokenIDs.find(dltTokenId);
        /* Didn't fint it in the dlt token set.  So we can skip this
         * object and not send it back.
         */
        if (dltTokenFiltered == dltTokenIDs.end()) {
            continue;
        }

        /* Now look for object in the filtered set.
         */
        auto objectIdFiltered = filterObjectSet.find(objId);
        /* 1) Didn't find the object in the filtered set.  or
         * 2) Found it, and ref cnt is > 0,
         * Add to the list of object to send back to the destionation SM.
         *
         */
        if (objectIdFiltered == filterObjectSet.end()) {

            /* Get metadata associated with the object. */
            objMetaData.deserializeFrom(iterDB->value());

        } else {
            /* Found the object.  Let's look at the refcnt to see if we need
             * to add to the delta set.
             */
            fds_verify(objectIdFiltered->first == objId);
            /* Get metadata associated with the object. */
            objMetaData.deserializeFrom(iterDB->value());

            if (objMetaData.getRefCnt() > 0) {

            }
        }
    }
}


bool
MigrationClient::migClientVerifyDestination(fds_token_id dltToken,
                                           fds_uint64_t executorId)
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
    fds_uint64_t executorId = filterSet->executorID;
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
        /* All filter objects sets from the destination SM is received.
         * Safe to snapshot.
         */
        err = migClientSnapshotMetaData();
        if (!err.ok()) {
            return err;
        }
    }

    return err;
}

}  // namespace fds
