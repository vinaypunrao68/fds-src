/*
 * Copyright 2015 Formation Data Systems, Inc.
 */
#include <fds_assert.h>

#include <net/SvcRequest.h>
#include <net/SvcRequestPool.h>
#include <net/SvcMgr.h>
#include <SMSvcHandler.h>
#include <ObjMeta.h>
#include <dlt.h>

#include <SmIo.h>
#include <MigrationClient.h>
#include <fds_process.h>
#include <MigrationTools.h>
#include "fdsp/om_api_types.h"

#include <object-store/SmDiskMap.h>

namespace fds {


MigrationClient::MigrationClient(SmIoReqHandler *_dataStore,
                                 NodeUuid& _destSMNodeID,
                                 fds_uint64_t& _targetDltVersion,
                                 fds_uint32_t bitsPerToken,
                                 bool resync)
    : dataStore(_dataStore),
      destSMNodeID(_destSMNodeID),
      targetDltVersion(_targetDltVersion),
      bitsPerDltToken(bitsPerToken),
      maxDeltaSetSize(16),
      forwardingIO(false),
      onePhaseMigration(resync)
{

    migClientState = ATOMIC_VAR_INIT(MC_INIT);
    seqNumDeltaSet = ATOMIC_VAR_INIT(0UL);

    snapshotRequest.io_type = FDS_SM_SNAPSHOT_TOKEN;
    snapshotRequest.isPersistent = true;
    SMTokenID = SMTokenInvalidID;
    executorID = SM_INVALID_EXECUTOR_ID;
    filterObjectSet.clear();
    dltTokenIDs.clear();

    maxDeltaSetSize = g_fdsprocess->get_fds_config()->get<int>("fds.sm.migration.max_delta_set_size");
}

MigrationClient::~MigrationClient()
{
}

void
MigrationClient::setForwardingFlagIfSecondPhase(fds_token_id smTok) {
    if (getMigClientState() == MC_SECOND_PHASE_DELTA_SET ||
        (getMigClientState() == MC_FIRST_PHASE_DELTA_SET &&
        onePhaseMigration == true)) {
        fds_verify(smTok == SMTokenID);
        LOGMIGRATE << "Setting forwarding flag for SM token " << smTok
                   << " executorId " << std::hex << executorID << std::dec;
        forwardingIO = true;
    }
}

fds_bool_t
MigrationClient::forwardAddObjRefIfNeeded(fds_token_id dltToken,
                                          fpi::AddObjectRefMsgPtr addObjRefReq)
{
    fds_bool_t forwarded = false;

    if (!forwardingIO || (dltTokenIDs.count(dltToken) == 0)) {
        // don't need to forward
        return false;
    }

    addObjRefReq->forwardedReq = true;

    auto asyncAddObjRefReq = gSvcRequestPool->newEPSvcRequest(destSMNodeID.toSvcUuid());
    asyncAddObjRefReq->setPayload(FDSP_MSG_TYPEID(fpi::AddObjectRefMsg),
                                  addObjRefReq);
    // TODO(Sean):
    // Should we wait for response?  Since this is do as much as possible, does it
    // matter?  Will address it when this is re-written as part of DM work.
    asyncAddObjRefReq->invoke();


    return forwarded;
}

fds_bool_t
MigrationClient::forwardIfNeeded(fds_token_id dltToken,
                                 FDS_IOType* req)
{
    if (!forwardingIO || (dltTokenIDs.count(dltToken) == 0)) {
        // don't need to forward
        return false;
    }

    /**
     * Start tracking forwarded IO request sent to the destination SM.
     * Tracking will be stopped in the callback of the forwared IO request.
     */
    if (!trackIOReqs.startTrackIOReqs()) {
        handleMigrationError(ERR_SM_TOK_MIGRATION_ABORTED);
        return ERR_SM_TOK_MIGRATION_ABORTED;
    }

    // forward to destination SM
    if (req->io_type == FDS_SM_PUT_OBJECT) {
        SmIoPutObjectReq* putReq = static_cast<SmIoPutObjectReq *>(req);

        LOGMIGRATE << "Forwarding " << *putReq
                   << " to Uuid " << std::hex << destSMNodeID << std::dec;

        // Set the forwarded flag, so the destination can appropriately handle
        // forwarded request.
        putReq->putObjectNetReq->forwardedReq = true;

        auto asyncPutReq = gSvcRequestPool->newEPSvcRequest(destSMNodeID.toSvcUuid());
        asyncPutReq->setPayload(FDSP_MSG_TYPEID(fpi::PutObjectMsg),
                                putReq->putObjectNetReq);
        asyncPutReq->setTimeoutMs(1000);
        asyncPutReq->onResponseCb(RESPONSE_MSG_HANDLER(MigrationClient::fwdPutObjectCb, putReq));
        asyncPutReq->invoke();
    } else if (req->io_type == FDS_SM_DELETE_OBJECT) {
        SmIoDeleteObjectReq* delReq = static_cast<SmIoDeleteObjectReq *>(req);

        // we are currently not forwarding 'delete' during resync on restart
        if (onePhaseMigration) {
            LOGMIGRATE << "Not forwarding delete during resync on restart " << *delReq;
            return false;
        }

        LOGMIGRATE << "Forwarding " << *delReq
                   << " to Uuid " << std::hex << destSMNodeID << std::dec;

        // Set the forwarded flag, so the destination can appropriately handle
        // forwarded request.
        delReq->delObjectNetReq->forwardedReq = true;

        auto asyncDelReq = gSvcRequestPool->newEPSvcRequest(destSMNodeID.toSvcUuid());
        asyncDelReq->setPayload(FDSP_MSG_TYPEID(fpi::DeleteObjectMsg),
                                delReq->delObjectNetReq);
        asyncDelReq->setTimeoutMs(1000);
        asyncDelReq->onResponseCb(RESPONSE_MSG_HANDLER(MigrationClient::fwdDelObjectCb, delReq));
        asyncDelReq->invoke();
    } else {
        trackIOReqs.finishTrackIOReqs();
    }

    return true;
}

void
MigrationClient::fwdPutObjectCb(SmIoPutObjectReq* putReq,
                                EPSvcRequest* svcReq,
                                const Error& error,
                                boost::shared_ptr<std::string> payload) {
    LOGMIGRATE << "Ack for forwarded PUT request " << putReq->getObjId()
               << " " << error;

    // Stop tracking this request, that was sent to destination SM.
    trackIOReqs.finishTrackIOReqs();

    // on error, set error state
    if (!error.ok()) {
        handleMigrationError(error);
    }
}

void
MigrationClient::fwdDelObjectCb(SmIoDeleteObjectReq* delReq,
                                EPSvcRequest* svcReq,
                                const Error& error,
                                boost::shared_ptr<std::string> payload) {
    LOGMIGRATE << "Ack for forwarded DELETE request " << delReq->getObjId()
               << " " << error;

    // Stop tracking this request, that was sent to destination SM.
    trackIOReqs.finishTrackIOReqs();

    // on error, set error state
    if (!error.ok()) {
        handleMigrationError(error);
    }
}

Error
MigrationClient::migClientSnapshotMetaData()
{
    Error err(ERR_OK);

    // if migration client in error state, don't do anything
    if (getMigClientState() == MC_ERROR) {
        return ERR_SM_TOK_MIGRATION_ABORTED;
    }

    // Track IO request for snapshot.
    // If the snapshot is successful, the IO request tracked will be finished in the
    // callback routine.
    // If the snapshot is unsuccessful, then IO request tracking will be finished in the
    // error check.
    if (!trackIOReqs.startTrackIOReqs()) {
        handleMigrationError(ERR_SM_TOK_MIGRATION_ABORTED);
        return ERR_SM_TOK_MIGRATION_ABORTED;
    }


    /* Should already have a valid sm token
     */
    fds_verify(SMTokenID != SMTokenInvalidID);
    fds_verify(executorID != SM_INVALID_EXECUTOR_ID);
    snapshotRequest.token_id = SMTokenID;
    snapshotRequest.executorId = executorID;
    snapshotRequest.targetDltVersion = targetDltVersion;

    fds_assert((getMigClientState() == MC_FIRST_PHASE_DELTA_SET) ||
               (getMigClientState() == MC_SECOND_PHASE_DELTA_SET));
    if (getMigClientState() == MC_FIRST_PHASE_DELTA_SET) {
        snapshotRequest.snapNum = "1";
        snapshotRequest.smio_persist_snap_resp_cb = std::bind(&MigrationClient::migClientSnapshotFirstPhaseCb,
                                                      this,
                                                      std::placeholders::_1,
                                                      std::placeholders::_2,
                                                      std::placeholders::_3,
                                                      std::placeholders::_4);
    } else {
        snapshotRequest.snapNum = "2";
        snapshotRequest.smio_persist_snap_resp_cb = std::bind(&MigrationClient::migClientSnapshotSecondPhaseCb,
                                                      this,
                                                      std::placeholders::_1,
                                                      std::placeholders::_2,
                                                      std::placeholders::_3,
                                                      std::placeholders::_4);
    }

    err = dataStore->enqueueMsg(FdsSysTaskQueueId, &snapshotRequest);
    if (!err.ok()) {
        LOGERROR << "Failed to snapshot. err=" << err;

        // Since the IO request for snapshot failed, stop tracking the snapshot request.
        trackIOReqs.finishTrackIOReqs();

        return err;
    }

    /* Not waiting for completion.  We will filter the objects in the callback
     * or somewhere else.
     */
    return err;
}

void
MigrationClient::migClientReadObjDeltaSetCb(const Error& error,
                                            SmIoReadObjDeltaSetReq *req,
                                            continueWorkFn nextWork)
{
    fds_verify(NULL != req);
    LOGMIGRATE << "MigClientState=" << getMigClientState()
               << ": Complete ReadObjectDelta: "
               << " seqNum=" << req->seqNum
               << " executorID=" << std::hex << req->executorId << std::dec
               << " DeltSetSize=" << req->deltaSet.size()
               << " lastSet=" << (req->lastSet ? "TRUE" : "FALSE");


    // Finish tracking IO request.
    trackIOReqs.finishTrackIOReqs();

    // on error, set error state (abort migration)
    if (!error.ok()) {
        handleMigrationError(error);
    }

    // Fire the code to execute the next delta set builder
    nextWork();
}

/* TODO(Gurpreet): Propogate error to Token Migration Manager
 */
void
MigrationClient::migClientAddMetaData(std::vector<std::pair<ObjMetaData::ptr,
                                                            fpi::ObjectMetaDataReconcileFlags>>& objMetaDataSet,
                                      fds_bool_t lastSet, continueWorkFn nextWork)
{
    Error err(ERR_OK);

    if (!trackIOReqs.startTrackIOReqs()) {
        handleMigrationError(ERR_SM_TOK_MIGRATION_ABORTED);
        return;
    }
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
                                std::placeholders::_2,
                                nextWork);

    std::vector<std::pair<ObjMetaData::ptr, fpi::ObjectMetaDataReconcileFlags>>::iterator itFirst, itLast;
    itFirst = objMetaDataSet.begin();
    itLast = objMetaDataSet.end();
    readDeltaSetReq->deltaSet.assign(itFirst, itLast);

    LOGMIGRATE << "MigClientState=" << getMigClientState()
               << ": QoS Enqueue with ReadObjDelta: "
               << " seqNum=" << readDeltaSetReq->seqNum
               << " executorID=" << std::hex << readDeltaSetReq->executorId << std::dec
               << " DeltSetSize=" << readDeltaSetReq->deltaSet.size()
               << " lastSet=" << (lastSet ? "TRUE" : "FALSE");

    /* enqueue to QoS queue */
    err = dataStore->enqueueMsg(FdsSysTaskQueueId, readDeltaSetReq);
    fds_verify(err.ok());
}

void MigrationClient::buildDeltaSetWorkerFirstPhase(leveldb::Iterator *iterDB,
                                                    leveldb::DB *dbFromFirstSnap,
                                                    std::string &firstPhaseSnapshotDir,
                                                    leveldb::CopyEnv *env) {

    // If we hit this the iterator is no longer valid, so we've finished our work
    if (!iterDB->Valid()) {
        delete iterDB;
        delete dbFromFirstSnap;

        /* If this is a one phase migration(For ex: SM resync)
         * We no longer need this snapshot.
         * Delete the snapshot directory and files.
         */
        if (onePhaseMigration && env) {
            env->DeleteDir(firstPhaseSnapshotDir);
        }

        setMigClientState(MC_FIRST_PHASE_DELTA_SET_COMPLETE);

        // Finish tracking IO request.
        trackIOReqs.finishTrackIOReqs();
        return;
    }

    /* The reconcileFlag in the second is used to determine what action is needed for
     * metada data.  One of 3 possibilities:
     * 1) no reconcile -- write out both meta data and object data on the destination SM.
     * 2) reconcile -- need to reconcile meta data on the destination SM.
     * 3) overwrite -- overwrite meta data.  The destination SM already has object data, but
     *                 metadata may be stale.
     */
    std::vector<std::pair<ObjMetaData::ptr, fpi::ObjectMetaDataReconcileFlags>> objMetaDataSet;

    /* Iterate through level db and filter against the objectFilterSet.
     */
    for (fds_uint64_t i = 0;
         iterDB->Valid(), i < maxDeltaSetSize;
         i++, iterDB->Next()) {

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
         * 2) Found it, and state of the object metadat is not the same.
         * Add to the list of object to send back to the destionation SM.
         *
         */
        if (objectIdFiltered == filterObjectSet.end()) {

            /* Didn't find the object in the filter set.  Need to send both
             * metadata and object
             */
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
             *
             * Send over metadata and object if not in the filterset, if object
             * is not corrupted and refcnt > 0.
             */
            if (!objMetaDataPtr->isObjCorrupted() && (objMetaDataPtr->getRefCnt() > 0UL)) {
                LOGMIGRATE << "MigClientState=" << getMigClientState()
                            << ": Selecting object " << objMetaDataPtr->logString();
                objMetaDataSet.emplace_back(objMetaDataPtr, fpi::OBJ_METADATA_NO_RECONCILE);
            } else {
                if (objMetaDataPtr->isObjCorrupted()) {
                    LOGCRITICAL << "CORRUPTION: Skipping object: " << objMetaDataPtr->logString();
                } else {
                    LOGMIGRATE << "MigClientState=" << getMigClientState()
                                << ": skipping object: " << objMetaDataPtr->logString();
                }
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
            if (!objMetaDataPtr->isEqualSyncObjectMetaData(objectIdFiltered->second)) {
                /* add to the set of objec IDs to be sent to the QoS.
                 */

                /* Note:  we deal with snapshot metadata, not from the disk
                 *        state.  With active IO, we need to look at if
                 *        active IOs have change the metadata state, and change
                 *        accordingly on the destination SM.
                 */
                if (!objMetaDataPtr->isObjCorrupted()) {
                    LOGMIGRATE << "MigClientState=" << getMigClientState()
                                << ": Found in filter set with object state change: "
                                << objMetaDataPtr->logString();
                    objMetaDataSet.emplace_back(objMetaDataPtr, fpi::OBJ_METADATA_OVERWRITE);
                } else {
                    LOGCRITICAL << "CORRUPTION: Skipping object: " << objMetaDataPtr->logString();
                }
            }
        }
    }

    continueWorkFn nextStep = std::bind(&MigrationClient::buildDeltaSetWorkerFirstPhase, this, iterDB,
                                        dbFromFirstSnap, firstPhaseSnapshotDir, env);

    // This is the last message if iterDB is no longer valid
    /* The last message can be empty. */
    migClientAddMetaData(objMetaDataSet, (!iterDB->Valid()), nextStep);
}

/* TODO(Gurpreet): Propogate error to Token Migration Manager
 */
void
MigrationClient::migClientSnapshotFirstPhaseCb(const Error& error,
                                               SmIoSnapshotObjectDB* snapRequest,
                                               std::string &snapDir,
                                               leveldb::CopyEnv *env)
{
    // First phase snapshot directory
    firstPhaseSnapshotDir = snapDir;

    // on error, set error state (abort migration)
    if (!error.ok()) {
        // This will set the ClientState to MC_ERROR
        handleMigrationError(error);
    }

    if (getMigClientState() == MC_ERROR) {
        LOGMIGRATE << "Migration Client in error state, not processing snapshot";
        // already in error state, don't do anything
        if (env) {
            env->DeleteDir(firstPhaseSnapshotDir);
        }
        // Finish tracking IO request.
        trackIOReqs.finishTrackIOReqs();
        return;
    }

    /* Setup db Options and create leveldb from the snapshot.
     */
    leveldb::DB* dbFromFirstSnap;
    leveldb::Options options;
    leveldb::ReadOptions read_options;

    leveldb::Status status = leveldb::DB::Open(options, firstPhaseSnapshotDir, &dbFromFirstSnap);
    if (!status.ok()) {
        LOGCRITICAL << "Could not open leveldb instance for First Phase snapshot."
                   << "status " << status.ToString();
        if (env) {
            env->DeleteDir(firstPhaseSnapshotDir);
        }
        // Finish tracking IO request.
        trackIOReqs.finishTrackIOReqs();
        return;
    }

    leveldb::Iterator *iterDB = dbFromFirstSnap->NewIterator(read_options);
    iterDB->SeekToFirst();

    // This will build the delta set and call the next method w/ "resume method" bound
    buildDeltaSetWorkerFirstPhase(iterDB, dbFromFirstSnap, firstPhaseSnapshotDir, env);
}

/* TODO(Gurpreet): Propogate error to Token Migration Manager
 */
void
MigrationClient::migClientSnapshotSecondPhaseCb(const Error& error,
                                               SmIoSnapshotObjectDB* snapRequest,
                                               std::string &snapDir,
                                               leveldb::CopyEnv *env)
{
    // on error, set error state
    if (!error.ok()) {
        // This will set the ClientState to MC_ERROR
        handleMigrationError(error);
        // the remaining of the error is handled in the next
        // if statement, where cleanup snapshots and finish tracking req
    }

    // If the state is already set to error, then do nothing.
    if (getMigClientState() == MC_ERROR) {
        // already in error state, don't do anything
        LOGMIGRATE << "Migration Client in error state, not processing snapshot";

        // since we already took snapshot, cleanup dir
        if (env) {
            env->DeleteDir(firstPhaseSnapshotDir);
            env->DeleteDir(secondPhaseSnapshotDir);
        }

        // Finish tracking IO request.
        trackIOReqs.finishTrackIOReqs();
        return;
    }

    /* The reconcileFlag in the second is used to determine what action is needed for
     * metada data.  One of 3 possibilities:
     * 1) no reconcile -- write out both meta data and object data on the destination SM.
     * 2) reconcile -- need to reconcile meta data on the destination SM.
     * 3) overwrite -- overwrite meta data.  The destination SM already has object data, but
     *                 metadata may be stale.
     */
    std::vector<std::pair<ObjMetaData::ptr, fpi::ObjectMetaDataReconcileFlags>> objMetaDataSet;

    fds_verify(MC_SECOND_PHASE_DELTA_SET == getMigClientState());

    /* Second phase snapshot directory
     */
    secondPhaseSnapshotDir = snapDir;

    LOGMIGRATE << "MigClientState=" << getMigClientState()
               << "Starting diff between First and Second Phase snapshot.";

    /* Diff two snapshots and store the difference in the list.
     * TODO(Sean): This approach of keeping everything in the memory is sub-optimal
     *             since this set can grow since the first snapshot.  We need
     *             a way to persist both snapshots on the disk.
     *             Once we have two snapshots on the disk we can incrementally
     *             get diff between two snapshots, thus saving memory
     *             consumption.
     */
    metadata::metadata_diff_type *diffObjMetaData = new metadata::metadata_diff_type {};

    /* Setup db options and create leveldbs from the snapshots.
     * Leveldb instances then gets passed to the diff function.
     */

    leveldb::DB* dbFromFirstSnap;
    leveldb::DB* dbFromSecondSnap;
    leveldb::Options options;

    leveldb::Status status = leveldb::DB::Open(options, firstPhaseSnapshotDir, &dbFromFirstSnap);

    /* TODO(Gurpreet): Propogate error to Token Migration Manager.
     */
    if (!status.ok()) {
        LOGCRITICAL << "Could not open leveldb instance for First Phase snapshot."
                   << "status " << status.ToString();
        // since we already took snapshot, cleanup dir
        if (env) {
            env->DeleteDir(firstPhaseSnapshotDir);
            env->DeleteDir(secondPhaseSnapshotDir);
        }
        // Finish tracking IO request.
        trackIOReqs.finishTrackIOReqs();

        delete dbFromFirstSnap;
        delete dbFromSecondSnap;

        return;
    }

    status = leveldb::DB::Open(options, secondPhaseSnapshotDir, &dbFromSecondSnap);

    /* TODO(Gurpreet): Propogate error to Token Migration Manager.
     */
    if (!status.ok()) {
        LOGCRITICAL << "Could not open leveldb instance for Second Phase snapshot."
                   << "status " << status.ToString();
        // since we already took snapshot, cleanup dir
        if (env) {
            env->DeleteDir(firstPhaseSnapshotDir);
            env->DeleteDir(secondPhaseSnapshotDir);
        }
        // Finish tracking IO request.
        trackIOReqs.finishTrackIOReqs();

        delete dbFromFirstSnap;
        delete dbFromSecondSnap;

        return;
    }

    /* Figure out the differences between the first and second snapshot
     * of SM token
     */
    metadata::diff(dbFromFirstSnap,
                   dbFromSecondSnap,
                   *diffObjMetaData);

    // Safe to delete these now
    delete dbFromFirstSnap;
    delete dbFromSecondSnap;

    if (env) {
        env->DeleteDir(firstPhaseSnapshotDir);
        env->DeleteDir(secondPhaseSnapshotDir);
    }

    LOGMIGRATE << "MigClientState=" << getMigClientState()
               << "Completed diff between First and Second Phase snapshot: "
               << "diffObjMetaData.size=" << diffObjMetaData->size();

    /* Return value (diffObjMetaData) contains a set of ObjectMetaData Pair.
     * 1) <OMD.first, OMD.second>
     *    This indicates that the object meta data was modified since the
     *    first snapshot.  We will calculate the difference (for now onlye the per
     *    object ref_cnt and per volume association ref_cnt) and stuff the
     *    diff in OMD.second.
     *    Note that there is no need to send object data in this case.  We will
     *    just send object meta data that will be reconciled on the destination SM.
     *
     * 2) <null, OMD.second>
     *    This indicates that there is a new object since the first snapshot.  This
     *    means that we need to send both object metadata + data.
     */

    metadata::metadata_diff_type::iterator objMDIter = diffObjMetaData->begin();
    metadata::metadata_diff_type::iterator objMDIterEnd = diffObjMetaData->end();

    buildDeltaSetWorkerSecondPhase(objMDIter, objMDIterEnd);
}

void
MigrationClient::buildDeltaSetWorkerSecondPhase(metadata::metadata_diff_type::iterator start,
                                                metadata::metadata_diff_type::iterator end)
{

    // If start == end we're done with all of the work, time to clean up
    if (start == end) {
        /* Set the migration client state to indicate the second delta set is sent. */
        setMigClientState(MC_SECOND_PHASE_DELTA_SET_COMPLETE);

        // Finish tracking IO request.
        trackIOReqs.finishTrackIOReqs();
    }

    std::vector<std::pair<ObjMetaData::ptr, fpi::ObjectMetaDataReconcileFlags>> objMetaDataSet;

    for (fds_uint64_t i = 0; i < maxDeltaSetSize, start != end; i++, start++) {
        std::pair<metadata::elem_type, metadata::elem_type> objMD = *start;

        /* Ok.  New object.  Need to send both metadata + data */
        if (objMD.first == nullptr) {
            fds_verify(objMD.second != nullptr);

            /* only send if the object is not corrupted, or have a positive refcnt.
             * If object is new, but have refcnt == 0, then there is no need to migrate
             * them.
             */
            if (!objMD.second->isObjCorrupted() && (objMD.second->getRefCnt() > 0UL)) {

                LOGMIGRATE << "MigClientState=" << getMigClientState()
                            << ": Selecting object " << objMD.second->logString();

                /* Add to object metadata set */
                objMetaDataSet.emplace_back(objMD.second, fpi::OBJ_METADATA_NO_RECONCILE);
            } else {
                if (objMD.second->isObjCorrupted()) {
                    LOGCRITICAL << "CORRUPTION: Skipping object: " << objMD.second->logString();
                } else {
                    LOGMIGRATE << "MigClientState=" << getMigClientState()
                                << ": skipping object: " << objMD.second->logString();
                }
            }
        } else {
            /* Here we have 2 possible cases.
             * 1) the object was alive (i.e. ref_cnt > 0) on the first snapshot, and
             *    the metadata and object was migrated to the destination SM during the first phase.
             * 2) the object was dead (i.e. ref_cnt == 0) on the first snapshot, where the
             *    metadata and object didn't migrate, but was resurrected before the second
             *    snapshot.  In this case, we treat it as a new object.
             */
            if (objMD.first->getRefCnt() == 0UL) {
                /* This is counted as resurrection, but migrate only if the second snapshot of the object has
                 * the ref_cnt > 0.
                 */
                if (objMD.second->getRefCnt() > 0UL) {
                    /* Treat this as a new object. */
                    LOGMIGRATE << "MigClientState=" << getMigClientState()
                                << ": Object resurrected: Selecting object " << objMD.second->logString();

                    objMetaDataSet.emplace_back(objMD.second, fpi::OBJ_METADATA_NO_RECONCILE);
                } else {
                    LOGMIGRATE << "MigClientState=" << getMigClientState()
                                << ": skipping object: " << objMD.second->logString();
                }
            } else {
                /* Ok. There is change in object metadata.  Calculate the difference
                 * and stuff the diff (ref_cnt's only) to the second of a pair.
                 * Also, indicate that this is metadata update only when staging to
                 * form a delta set to be sent to the destination SM.
                 */
                objMD.second->diffObjectMetaData(objMD.first);

                LOGMIGRATE << "MigClientState=" << getMigClientState()
                            << ": Diff'ed MetaData " << objMD.second->logString();

                objMetaDataSet.emplace_back(objMD.second, fpi::OBJ_METADATA_RECONCILE);
            }
        }
    }

    continueWorkFn nextWork = std::bind(&MigrationClient::buildDeltaSetWorkerSecondPhase, this, start, end);

    migClientAddMetaData(objMetaDataSet, start == end, nextWork);
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
               << "ExecutorID=" << std::hex << executorId << std::dec;

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
    if (executorID == SM_INVALID_EXECUTOR_ID) {
        executorID = executorId;
    } else {
        if (executorID != executorId) {
            return false;
        }
    }
    return true;
}


Error
MigrationClient::migClientStartRebalanceFirstPhase(fpi::CtrlObjectRebalanceFilterSetPtr& filterSet,
                                                   fds_bool_t srcAccepted)
{
    /* Verify that the token and executor ID matches known SM token and perviously
     * set exector ID.
     */
    uint64_t curSeqNum = filterSet->seqNum;
    fds_token_id dltToken = filterSet->tokenId;
    fds_uint64_t executorId = filterSet->executorID;
    bool completeSet = false;
    Error err(ERR_OK);

    /**
     * Ideally this check should never be true. If it is, then something's
     * wrong on Migration Executor side.
     *
     * The check essentially says that if the sequence number set for the filter
     * set messages received from Migration Executor is already complete, then
     * this request will be ignored. Because the seqeunce number set was complete
     * the migration for this Migration Client - Executor pair is already
     * in progress.
     */
    if (seqNumFilterSet.isSeqNumComplete()) {
        LOGMIGRATE << "DLT token=" << dltToken << ", "
                   << "seqNum=" << curSeqNum << ", "
                   << "executorId=" << std::hex << executorId << std::dec
                   << " is received after the seqNumFilterSet was marked complete."
                   << " Probably a duplicate sequence number. Ignore filterSet message";
        return err;
    }

    LOGMIGRATE << "MigClientState=" << getMigClientState()
               << ": seqNum=" << curSeqNum << ", "
               << "DLT token=" << dltToken << ", "
               << "executorId=" << std::hex << executorId << std::dec
               << ", accepted? " << srcAccepted;

    if (getMigClientState() == MC_ERROR) {
        LOGMIGRATE << "Migration Client in error state";
        return ERR_SM_TOK_MIGRATION_ABORTED;
    }

    /* Transition to the filter set state.
     */
    setMigClientState(MC_FILTER_SET);

    // this method may be called if source did not accept to process this
    // filter set; we are calling the method in that case to ensure that
    // we properly detect end of sequence and go to the right state
    if (srcAccepted) {
        migClientLock.lock();

        /* Verify message destination.
         */
        bool verifySuccess = migClientVerifyDestination(dltToken, executorId);
        if (!verifySuccess) {
            migClientLock.unlock();
            LOGMIGRATE << "Filter set token migration message from destination is corrupt"
                       << std::hex << executorId << std::dec << dltToken;
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
                                    objAndRefCnt);
        }
        migClientLock.unlock();
    }

    /* Set the sequence number of the received message to ensure all messages are received.
     */
    completeSet = seqNumFilterSet.setSeqNum(filterSet->seqNum, filterSet->lastFilterSet);
    if (true == completeSet) {
        /* All filter objects sets from the destination SM is received.
         * Safe to snapshot.
         */
        LOGMIGRATE << "MigClientState=" << getMigClientState()
                   << ": Received complete FilterSet with last seqNum=" << filterSet->seqNum
                   << " for number of DLT tokens: " << dltTokenIDs.size()
                   << ", executorId " << std::hex << executorId << std::dec;

        if (dltTokenIDs.size() == 0) {
            LOGMIGRATE << "This SM declined to be a source for all DLT tokens from ExecutorID "
                       << std::hex << executorId << std::dec << "; no need to proceed with this client";
            // basically declining the whole set...
            return ERR_SM_RESYNC_SOURCE_DECLINE;
        }

        /* Transition to the first phase of delta set generation.
         */
        setMigClientState(MC_FIRST_PHASE_DELTA_SET);

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
    setMigClientState(MC_SECOND_PHASE_DELTA_SET);

    /* since we are starting second set of delta set migration,
     * reset the sequence number to start from 0.
     */
    resetSeqNumDeltaSet();

    LOGMIGRATE << "MigClientState=" << getMigClientState()
               << ": Received Msg for executorID=" << std::hex << secondPhase->executorID << std::dec;

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

    // do not over-write error state
    if (prevState == MC_ERROR) {
        LOGMIGRATE << "Not changing error state to " << newState;
        return;
    }

    std::atomic_store(&migClientState, newState);

    LOGMIGRATE << "Setting MigrateClieentState: from " << prevState
               << "=> " << migClientState;
}

void
MigrationClient::handleMigrationError(const Error& error) {
    if (getMigClientState() != MC_ERROR) {
        // first time we see error
        // do not abort migration; destination will see the error
        // or timeout, and either will retry with another client or
        // report to OM.
        setMigClientState(MC_ERROR);
    }
}

// Wait for IO completion for the MigrationClient.  This interface blocks
// until all outstanding IO requests are complete.
void
MigrationClient::waitForIOReqsCompletion(fds_uint64_t executorId)
{
    LOGMIGRATE << "Waiting for pending IO requests to complete: "
               << "ExecutorID=" << std::hex << executorId << std::dec;
    trackIOReqs.waitForTrackIOReqs();
    LOGMIGRATE << "No more pending IO requests: "
               << "ExecutorID=" << std::hex << executorId << std::dec;
}
}  // namespace fds
