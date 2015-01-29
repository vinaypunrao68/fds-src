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
      testMode(false)
{

    migClientState = ATOMIC_VAR_INIT(MIG_CLIENT_INIT);
    seqNumDeltaSet = ATOMIC_VAR_INIT(0UL);

    snapshotRequest.io_type = FDS_SM_SNAPSHOT_TOKEN;
    SMTokenID = SMTokenInvalidID;
    executorID = invalidExecutorID;

    maxDeltaSetSize = g_fdsprocess->get_fds_config()->get<int>("fds.sm.migration.max_delta_set_size");
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

    fds_assert((getMigClientState() == MIG_CLIENT_FIRST_PHASE_DELTA_SET) ||
               (getMigClientState() == MIG_CLIENT_SECOND_PHASE_DELTA_SET));
    if (getMigClientState() == MIG_CLIENT_FIRST_PHASE_DELTA_SET) {
        snapshotRequest.smio_snap_resp_cb = std::bind(&MigrationClient::migClientSnapshotFirstPhaseCb,
                                                      this,
                                                      std::placeholders::_1,
                                                      std::placeholders::_2,
                                                      std::placeholders::_3,
                                                      std::placeholders::_4);
    } else {
        snapshotRequest.smio_snap_resp_cb = std::bind(&MigrationClient::migClientSnapshotSecondPhaseCb,
                                                      this,
                                                      std::placeholders::_1,
                                                      std::placeholders::_2,
                                                      std::placeholders::_3,
                                                      std::placeholders::_4);
    }

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
    LOGMIGRATE << "MigClientState=" << getMigClientState()
               << ": Complete ReadObjectDelta: "
               << " seqNum=" << req->seqNum
               << " executorID=" << req->executorId
               << " DeltSetSize=" << req->deltaSet.size()
               << " lastSet=" << req->lastSet ? "TRUE" : "FALSE";
}

void
MigrationClient::migClientAddMetaData(std::vector<ObjMetaData::ptr>& objMetaDataSet,
                                      fds_bool_t lastSet)
{
    Error err(ERR_OK);

    SmIoReadObjDeltaSetReq *readDeltaSetReq =
                      new(std::nothrow) SmIoReadObjDeltaSetReq(destSMNodeID,
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

    std::vector<ObjMetaData::ptr>::iterator itFirst, itLast;
    itFirst = objMetaDataSet.begin();
    itLast = objMetaDataSet.end();
    readDeltaSetReq->deltaSet.assign(itFirst, itLast);

    LOGMIGRATE << "MigClientState=" << getMigClientState()
               << ": QoS Enqueue with ReadObjDelta: "
               << " seqNum=" << readDeltaSetReq->seqNum
               << " executorID=" << readDeltaSetReq->executorId
               << " DeltSetSize=" << readDeltaSetReq->deltaSet.size()
               << " lastSet=" << lastSet ? "TRUE" : "FALSE";

    /* enqueue to QoS queue */
    err = dataStore->enqueueMsg(FdsSysTaskQueueId, readDeltaSetReq);
    fds_verify(err.ok());
}

void
MigrationClient::migClientSnapshotFirstPhaseCb(const Error& error,
                                               SmIoSnapshotObjectDB* snapRequest,
                                               leveldb::ReadOptions& options,
                                               leveldb::DB *db)
{
    std::vector<ObjMetaData::ptr> objMetaDataSet;

    firstPhaseSnapDB = db;
    leveldb::Iterator *iterDB = firstPhaseSnapDB->NewIterator(options);

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

            ObjMetaData::ptr objMetaDataPtr = ObjMetaData::ptr(new ObjMetaData());

            /* Get metadata associated with the object. */
            objMetaDataPtr->deserializeFrom(iterDB->value());

#ifdef DEBUG
            {
                ObjectID objIdDebug(std::string((const char *)(objMetaDataPtr->obj_map.obj_id.metaDigest),
                                    sizeof(objMetaDataPtr->obj_map.obj_id.metaDigest)));
                fds_assert(objId == objIdDebug);
            }
#endif

            /* Note:  we have to deal with snapshot metadata, not from the disk
             *        state later.  With active IO, we need to look at if
             *        active IOs have change the metadat state, and change
             *        accordingly on the destination SM.
             */
            if (!objMetaDataPtr->isObjCorrupted()) {
                LOGMIGRATE << "MigClientState=" << getMigClientState()
                           << ": Selecting object" << objMetaDataPtr->logString();
                objMetaDataSet.push_back(objMetaDataPtr);
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
            ObjMetaData::ptr objMetaDataPtr = ObjMetaData::ptr(new ObjMetaData());
            objMetaDataPtr->deserializeFrom(iterDB->value());

#ifdef DEBUG
            {
                ObjectID objIdDebug(std::string((const char *)(objMetaDataPtr->obj_map.obj_id.metaDigest),
                                    sizeof(objMetaDataPtr->obj_map.obj_id.metaDigest)));
                fds_assert(objId == objIdDebug);
            }
#endif

            /* compare the refcnt of the filteredObject and object in the
             * leveldb snapshot.
             */
            /* TODO(Sean): Do we need to also check the volume association when filtering
             *             in the future?
             */
            if (objMetaDataPtr->getRefCnt() != objectIdFiltered->second) {
                /* add to the set of objec IDs to be sent to the QoS.
                 */

                /* Note:  we have to deal with snapshot metadata, not from the disk
                 *        state later.  With active IO, we need to look at if
                 *        active IOs have change the metadat state, and change
                 *        accordingly on the destination SM.
                 */
                if (!objMetaDataPtr->isObjCorrupted()) {
                    LOGMIGRATE << "MigClientState=" << getMigClientState()
                               << ": Selecting object" << objMetaDataPtr->logString();
                    objMetaDataSet.push_back(objMetaDataPtr);
                } else {
                    LOGERROR << "CORRUPTION: Skipping object ID " << objId;
                }
            }
        }

        /* If the size of the object data set is greater than the max
         * size, then add metadata set to be read.
         */
        if (objMetaDataSet.size() >= maxDeltaSetSize) {
            migClientAddMetaData(objMetaDataSet, false);
            objMetaDataSet.clear();
            fds_verify(objMetaDataSet.size() == 0);
        }
    }

    /* The last message can be empty. */
    migClientAddMetaData(objMetaDataSet, true);

    delete iterDB;

    setMigClientState(MIG_CLIENT_FIRST_PHASE_DELTA_SET_COMPLETE);
}



bool
MigrationClient::migClientVerifyDestination(fds_token_id dltToken,
                                           fds_uint64_t executorId)
{
    /* Need to copy the set of objects to the
     */
    fds_token_id derivedSMTokenID = SmDiskMap::smTokenId(dltToken);

    LOGMIGRATE << "MigClientState=" << getMigClientState()
               << ": Dest SM=" << std::hex << destSMNodeID.uuid_get_val() << std::dec << ", "
               << "DLT Token=" << dltToken << ", "
               << "SM Token=" << derivedSMTokenID << ", "
               << "ExecutorID=" << executorId;

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


void
MigrationClient::migClientSnapshotSecondPhaseCb(const Error& error,
                                               SmIoSnapshotObjectDB* snapRequest,
                                               leveldb::ReadOptions& options,
                                               leveldb::DB *db)
{
    std::vector<ObjMetaData::ptr> objMetaDataSet;

    secondPhaseSnapDB = db;

    /*****************
     * TODO(Sean):
     * Here, we will diff between two snapshots, and calculate difference between
     * two snapshots.
     * Will have a set of
     */

    /* TODO(Sean):
     * Return empty set for now on the second phase.
     */
    migClientAddMetaData(objMetaDataSet, true);
}

Error
MigrationClient::migClientStartRebalanceFirstPhase(fpi::CtrlObjectRebalanceFilterSetPtr& filterSet)
{
    /* Verify that the token and executor ID matches known SM token and perviously
     * set exector ID.
     */
    uint64_t curSeqNum = filterSet->seqNum;
    fds_token_id dltToken = filterSet->tokenId;
    fds_uint64_t executorId = filterSet->executorID;
    bool completeSet = false;
    Error err(ERR_OK);

    LOGMIGRATE << "MigClientState=" << getMigClientState()
               << ": seqNum=" << curSeqNum << ", "
               << "DLT token=" << dltToken << ", "
               << "executorId=" << executorId;

    /* Transition to the filter set state.
     */
    setMigClientState(MIG_CLIENT_FILTER_SET);

    migClientLock.lock();

    /* Verify message destination.
     */
    bool verifySuccess = migClientVerifyDestination(dltToken, executorId);
    if (!verifySuccess) {
        migClientLock.unlock();
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
    migClientLock.unlock();

    /* Set the sequence number of the received message to ensure all messages are received.
     */
    completeSet = seqNumFilterSet.setSeqNum(filterSet->seqNum, filterSet->lastFilterSet);
    if (true == completeSet) {
        /* All filter objects sets from the destination SM is received.
         * Safe to snapshot.
         */
        LOGMIGRATE << "MigClientState=" << getMigClientState()
                   << ": Received complete FilterSet with last seqNum=" << filterSet->seqNum;

        /* Transition to the first phase of delta set generation.
         */
        setMigClientState(MIG_CLIENT_FIRST_PHASE_DELTA_SET);

        err = migClientSnapshotMetaData();
        if (!err.ok()) {
            LOGERROR << "Snapshot failed: error=" << err;
            return err;
        }
    }

    return err;
}

Error
MigrationClient::migClientStartRebalanceSecondPhase(fpi::CtrlGetSecondRebalanceDeltaSetPtr& secondPhase)
{
    Error err(ERR_OK);

    /* This can potentially be called repeatedly, but that's ok. */
    setMigClientState(MIG_CLIENT_SECOND_PHASE_DELTA_SET);

    /* since we are starting second set of delta set migration,
     * reset the sequence number to start from 0.
     */
    resetSeqNumDeltaSet();

    LOGMIGRATE << "MigClientState=" << getMigClientState()
               << ": Received Msg for executorID=" << secondPhase->executorID;

    /* Take second snapshot */
    err = migClientSnapshotMetaData();

    return err;
}


uint64_t
MigrationClient::getSeqNumDeltaSet()
{
    return std::atomic_fetch_add(&seqNumDeltaSet, 1UL);
}

void
MigrationClient::resetSeqNumDeltaSet() {
    LOGMIGRATE << "Resetting seqNumDeltaSet=0";
    std::atomic_store(&seqNumDeltaSet, 0UL);
}

MigrationClient::MigrationClientState
MigrationClient::getMigClientState() {
    return std::atomic_load(&migClientState);
}

void
MigrationClient::setMigClientState(MigrationClientState newState)
{
    MigrationClientState prevState;
    prevState = std::atomic_load(&migClientState);

    std::atomic_store(&migClientState, newState);

    LOGMIGRATE << "Setting MigrateClieentState: from " << prevState
               << "=> " << migClientState;
}

}  // namespace fds
