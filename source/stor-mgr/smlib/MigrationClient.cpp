/*
 * Copyright 2015 Formation Data Systems, Inc.
 */
#include <fds_assert.h>

#include <SMSvcHandler.h>
#include <ObjMeta.h>
#include <dlt.h>

#include <SmIo.h>
#include <MigrationClient.h>
#include <fds_process.h>

#include <object-store/SmDiskMap.h>

namespace fds {


MigrationClient::MigrationClient(SmIoReqHandler *_dataStore,
                                 NodeUuid& _destSMNodeID,
                                 fds_uint32_t bitsPerToken)
    : dataStore(_dataStore),
      destSMNodeID(_destSMNodeID),
      bitsPerDltToken(bitsPerToken),
      maxDeltaSetSize(16),
      currDeltaSetSize(0),
      seqNumDeltaSet(0),
      readDeltaSetReq(NULL),
      testMode(false)
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

    maxDeltaSetSize = g_fdsprocess->get_fds_config()->get<int>("fds.sm.migration.max_delta_set");
    testMode = g_fdsprocess->get_fds_config()->get<bool>("fds.sm.testing.standalone");
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
MigrationClient::migClientReadObjDeltaSetCb(const Error& error,
                                            SmIoReadObjDeltaSetReq *req)
{
    fds_verify(NULL != req);
}

void
MigrationClient::migClientAddMetaData(ObjMetaData& objMetaData,
                                      fds_bool_t setObjMetaData,
                                      fds_bool_t lastSet)
{
    Error err(ERR_OK);

    /* create the delta set container. */
    if (NULL == readDeltaSetReq) {
        /* the current set size should be null */
        fds_verify(0 == currDeltaSetSize);
#if 0
        readDeltaSetReq = SmIoReadObjDeltaSetReq SharedPtr(
                              new(std::nothrow) SmIoReadObjDeltaSetReq(executorID,
                                                                    getSeqNumDeltaSet(),
                                                                    lastSet));
#endif
        readDeltaSetReq = new(std::nothrow) SmIoReadObjDeltaSetReq(destSMNodeID,
                                                                   executorID,
                                                                   getSeqNumDeltaSet(),
                                                                   lastSet);
        fds_verify(NULL != readDeltaSetReq);

        readDeltaSetReq->io_type = FDS_SM_READ_DELTA_SET;
        readDeltaSetReq->smioReadObjDeltaSetReqCb = std::bind(
                &MigrationClient::migClientReadObjDeltaSetCb,
                this,
                std::placeholders::_1,
                std::placeholders::_2);


        LOGDEBUG << "Allocating new ReadObjDelta with seqNum "
                 << readDeltaSetReq->seqNum;
    }

    if (true == setObjMetaData) {
        LOGDEBUG << "Adding object to delta set: " << objMetaData;

        /* add object metadata data to the set */
        readDeltaSetReq->deltaSet.push_back(objMetaData);

        /*increase the set size */
        ++currDeltaSetSize;
    }

#if 0
    fpi::CtrlObjectMetaDataPropagate objMetaDataPropagate;
    /* copy meta data to thrift message type */
    if (setObjMetaData) {
        objMetaData.propagateMetaData(objMetaDataPropagate);
        readDeltaSetReq->deltaSet.push_back(objMetaDataPropagate);
        /*increase the set size */
        ++currDeltaSetSize;

        LOGDEBUG << "Adding object to delta set: " << objMetaData;
    }
#endif

    if ((currDeltaSetSize == maxDeltaSetSize) || (true == lastSet)) {
        /* send the delta set to the QoS to be read.
         */
        fds_verify(NULL != readDeltaSetReq);

        LOGDEBUG << "QoS Enqueu with ReadObjDelta..."
                 << "seqNum=" << readDeltaSetReq->seqNum
                 << "executorID=" << readDeltaSetReq->executorId
                 << "DeltSetSize=" << readDeltaSetReq->deltaSet.size()
                 << "lastSet=" << lastSet ? "TRUE" : "FALSE";

        /* enqueue to QoS queue */
        err = dataStore->enqueueMsg(FdsSysTaskQueueId, readDeltaSetReq);
        fds_verify(err.ok());


        /* reset the batch pointer and current delta set size
         */
        readDeltaSetReq = NULL;
        currDeltaSetSize = 0;
    }
}

void
MigrationClient::migClientSnapshotCB(const Error& error,
                                     SmIoSnapshotObjectDB* snapRequest,
                                     leveldb::ReadOptions& options,
                                     leveldb::DB *db)
{
    ObjMetaData objMetaData;

    /* TODO(Sean): Save off the levelDB information.
     *             Need this to be on-diek snapshot, since the hold time on the
     *             snapshot can be days or weeks.
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
        /* Didn't find it in the dlt token set.  So we can skip this
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

            /* Note:  we have to deal with snapshot metadata, not from the disk
             *        state later.  With active IO, we need to look at if
             *        active IOs have change the metadat state, and change
             *        accordingly on the destination SM.
             */
            if (!objMetaData.isObjCorrupted()) {
                migClientAddMetaData(objMetaData, true, false);
            } else {
                LOGERROR << "CORRUPTION: Skipping object ID " << objId;
            }

        } else {
            /* Found the object.  Let's look at the refcnt to see if we need
             * to add to the delta set.  If the reference count is different, then
             * we add to the filter set.
             */
            fds_verify(objectIdFiltered->first == objId);
            /* Get metadata associated with the object. */
            objMetaData.deserializeFrom(iterDB->value());

            /* compare the refcnt of the filteredObject and object in the
             * leveldb snapshot.
             */
            if (objMetaData.getRefCnt() != objectIdFiltered->second) {
                /* add to the set of objec IDs to be sent to the QoS.
                 */

                /* Note:  we have to deal with snapshot metadata, not from the disk
                 *        state later.  With active IO, we need to look at if
                 *        active IOs have change the metadat state, and change
                 *        accordingly on the destination SM.
                 */
                if (!objMetaData.isObjCorrupted()) {
                    migClientAddMetaData(objMetaData, true, false);
                } else {
                    LOGERROR << "CORRUPTION: Skipping object ID " << objId;
                }
            }
        }
    }

    /* The last message is going to be empty.  It's just easier to deal
     * with it for now.
     * TODO(Sean):  check how easy it is check if the current item is
     *              the last one -- especially with disk based snapshot.
     */
    migClientAddMetaData(objMetaData, false, true);
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
MigrationClient::migClientAddObjToFilterSet(fpi::CtrlObjectRebalanceFilterSetPtr& filterSet)
{
    /* Verify that the token and executor ID matches known SM token and perviously
     * set exector ID.
     */
    uint64_t curSeqNum = filterSet->seqNum;
    fds_token_id dltToken = filterSet->tokenId;
    fds_uint64_t executorId = filterSet->executorID;
    bool completeSet = false;
    Error err(ERR_OK);

    filterSetLock.lock();

    /* Verify message destination.
     */
    bool verifySuccess = migClientVerifyDestination(dltToken, executorId);
    if (!verifySuccess) {
            filterSetLock.unlock();
            return ERR_SM_TOK_MIGRATION_DESTINATION_MSG_CORRUPT;
    }

    /* since the MigrationMgr may add tokenID and objects to filter asynchronously,
     * need to protect it.
     */
    /* For debugging, keep set of DLT token ids.
     */
    dltTokenIDs.insert(filterSet->tokenId);

    /* Now, add the objects and corresponding refcnts to the local filter set.
     */
    for (auto& objAndRefCnt : filterSet->objectsToFilter) {
      filterObjectSet.emplace(ObjectID(objAndRefCnt.objectID.digest),
                              objAndRefCnt.objRefCnt);
    }
    filterSetLock.unlock();

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
