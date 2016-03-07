/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#include <string>
#include <list>
#include <VolumeMeta.h>
#include <fds_process.h>
#include <util/timeutils.h>
#include <net/SvcRequest.h>
#include <DataMgr.h>
#include <net/volumegroup_extensions.h>
#include <util/stringutils.h>
#include <dmhandler.h>
#include <json/json.h>
#include <ObjectId.h>

namespace fds {

VolumeMeta::VolumeMeta(CommonModuleProviderIf *modProvider,
                       const std::string& _name,
                       fds_volid_t _uuid,
                       fds_log* _dm_log,
                       VolumeDesc* _desc,
                       DataMgr *_dm)
          : HasModuleProvider(modProvider),
            fwd_state(VFORWARD_STATE_NONE),
            dmVolQueue(0),
            dataManager(_dm),
            cbToVGMgr(NULL),
            initializerTriesCnt(0),
            maxInitializerTriesCnt(10)
{
    const FdsRootDir *root = MODULEPROVIDER()->proc_fdsroot();

    vol_mtx = new fds_mutex("Volume Meta Mutex");
    vol_desc = new VolumeDesc(_name, _uuid);
    dmCopyVolumeDesc(vol_desc, _desc);

    root->fds_mkdir(root->dir_sys_repo_dm().c_str());
    root->fds_mkdir(root->dir_user_repo_dm().c_str());

    /* Enable ability to query state via StateProvider api */
    stateProviderId = "volume." + std::to_string(_uuid.get());
    MODULEPROVIDER()->get_cntrs_mgr()->add_for_export(this);

    selfSvcUuid = MODULEPROVIDER()->getSvcMgr()->getSelfSvcUuid();

    // this should be overwritten when volume add triggers read of the persisted value
    sequence_id = 0;

    opId = VolumeGroupConstants::OPSTARTID;
    version = VolumeGroupConstants::VERSION_INVALID;

    threadId = dataManager->getQosCtrl()->threadPool->getThreadId(_uuid.get());
}

VolumeMeta::~VolumeMeta()
{
    MODULEPROVIDER()->get_cntrs_mgr()->remove_from_export(this);

    delete vol_desc;
    delete vol_mtx;
    // volDb.reset();
}

std::string VolumeMeta::getBaseDirPath() const
{
    return dmutil::getVolumeDir(MODULEPROVIDER()->proc_fdsroot(),
                                     fds_volid_t(getId()),
                                     invalid_vol_id);
}

std::string VolumeMeta::getBufferfilePrefix() const
{
    return util::strformat("%s/bufferfile_", getBaseDirPath().c_str());
}

// Returns true if volume is in forwarding state, and qos queue is empty;
// otherwise returns false (volume not in forwarding state or qos queue
// is not empty
//
void VolumeMeta::finishForwarding() {
    vol_mtx->lock();
    setForwardFinish();
    vol_mtx->unlock();
    LOGMIGRATE << "finishForwarding for volume " << *vol_desc
               << ", state " << fwd_state;
}

void VolumeMeta::setPersistVolDB(DmPersistVolDB::ptr dbPtr)
{
    // volDb = dbPtr;
}

void VolumeMeta::dmCopyVolumeDesc(VolumeDesc *v_desc, VolumeDesc *pVol) {
    v_desc->name = pVol->name;
    v_desc->volUUID = pVol->volUUID;
    v_desc->tennantId = pVol->tennantId;
    v_desc->localDomainId = pVol->localDomainId;
    v_desc->globDomainId = pVol->globDomainId;

    v_desc->maxObjSizeInBytes = pVol->maxObjSizeInBytes;
    v_desc->capacity = pVol->capacity;
    v_desc->volType = pVol->volType;
    v_desc->maxQuota = pVol->maxQuota;
    v_desc->redundancyCnt = pVol->redundancyCnt;

    v_desc->consisProtocol = fpi::FDSP_ConsisProtoType(pVol->consisProtocol);
    v_desc->appWorkload = pVol->appWorkload;
    v_desc->mediaPolicy = pVol->mediaPolicy;

    v_desc->volPolicyId = pVol->volPolicyId;
    v_desc->iops_throttle = pVol->iops_throttle;
    v_desc->iops_assured = pVol->iops_assured;
    v_desc->relativePrio = pVol->relativePrio;
    v_desc->fSnapshot = pVol->fSnapshot;
    v_desc->srcVolumeId = pVol->srcVolumeId;
    v_desc->qosQueueId = pVol->qosQueueId;
    v_desc->contCommitlogRetention = pVol->contCommitlogRetention;
    v_desc->timelineTime = pVol->timelineTime;
    v_desc->coordinator = pVol->coordinator;
}

Error VolumeMeta::applyActiveTxState(const int64_t &highestOpId,
                                     const std::vector<std::string> &txs)
{
    fds_volid_t volId = fds_volid_t(getId());
    DmCommitLog::ptr commitLog;
    Error err = ERR_OK;

    err = dataManager->timeVolCat_->getCommitlog(volId, commitLog);
    if (!err.ok()) {
        return err;
    }

    commitLog->clear();
    err = commitLog->applySerializedTxs(txs);
    if (!err.ok()) {
        return err;
    }

    setOpId(highestOpId);

    return err;
}

void VolumeMeta::setSequenceId(sequence_id_t new_seq_id){
    fds_mutex::scoped_lock l(sequence_lock);

    if (new_seq_id > sequence_id) {
        sequence_id = new_seq_id;
    }
}

sequence_id_t VolumeMeta::getSequenceId(){
    fds_mutex::scoped_lock l(sequence_lock);

    return sequence_id;
}

std::string VolumeMeta::logString() const
{
    std::stringstream ss;
    ss << " ["
        << "volid: " << vol_desc->volUUID
        << " version: " << version
        << " opid: " << getOpId()
        << " sequenceid: " << sequence_id
        << " state: " << fpi::_ResourceState_VALUES_TO_NAMES.at(static_cast<int>(getState()))
        << "] ";
    return ss.str();
}

void VolumeMeta::setState(const fpi::ResourceState &state,
                          const std::string &logCtx)
{
    // TODO(Rao): Ensure setState is invoked under volume synchronized context
    vol_desc->state = state;
    if (state == fpi::ResourceState::Loading) {
        /* Every time volume goes into loading state version is incremented */
        version = dataManager->getPersistDB(vol_desc->volUUID)->updateVersion();
    } else if (state == fpi::ResourceState::Active) {
        initializerTriesCnt = 0;
    }
    LOGNORMAL << logString() << logCtx;
}

std::string VolumeMeta::getStateProviderId()
{
    return stateProviderId;
}

std::string VolumeMeta::getStateInfo()
{
    /* NOTE: Getting the stateinfo isn't synchronized.  It may be a bit stale */
    Json::Value state;
    state["state"] = fpi::_ResourceState_VALUES_TO_NAMES.at(static_cast<int>(getState()));
    state["version"] = version;
    state["opid"] = static_cast<Json::Value::Int64>(getOpId());
    state["sequenceid"] = static_cast<Json::Value::Int64>(sequence_id);
    state["coordinator"] = static_cast<Json::Value::Int64>(getCoordinatorId().svc_uuid);

    std::stringstream ss;
    ss << state;
    return ss.str();
}

void VolumeMeta::enqueDmIoReq(DataMgr &dataManager, DmRequest *dmReq)
{
    auto err = dataManager.getQosCtrl()->enqueueIO(dmReq->getVolId(), dmReq);
    if (err != ERR_OK) {
        GLOGWARN << "Failed to enqueue volume synchronized DmFunctor.  Dropping. "
            << err;
        delete dmReq;
    }
}

void VolumeMeta::startInitializer(bool force)
{
    fds_assert(isSynchronized());
    fds_assert(getState() == fpi::Offline);
    fds_assert(!isInitializerInProgress());

    /* Coordinator is set. We can go through sync protocol */
    initializerTriesCnt = force ? 1 : initializerTriesCnt + 1;

    setState(fpi::Loading,
             util::strformat(" - startInitializer with coordinator: %ld.  Try #: %d",
                             getCoordinatorId().svc_uuid, initializerTriesCnt));
    initializer = MAKE_SHARED<VolumeInitializer>(MODULEPROVIDER(), this);
    initializer->run();
}

void VolumeMeta::notifyInitializerComplete(const Error &completionError)
{
    fds_assert(isSynchronized());
    fds_assert(initializer->getProgress() == VolumeInitializer::COMPLETE);
    initializer.reset();
    LOGDEBUG << "Cleanedup initializer: " << logString();

    if (completionError != ERR_OK) {
        scheduleInitializer(false);
    } else {
        initializerTriesCnt = 0;
    }
}

void VolumeMeta::scheduleInitializer(bool fNow)
{
    auto func = makeSynchronized([this]() {
        if (getState() == fpi::Offline) {
            startInitializer();
        } else {
            LOGNORMAL << "Not going to start initializer.  Volume is not offline"
                << logString();
        }
    });

    if (fNow) {
        func();
    } else if (initializerTriesCnt > maxInitializerTriesCnt) {
        setState(fpi::Offline,
                util::strformat(" - scheduleInitializer.  Failed too many times #: %d",
                        initializerTriesCnt));
        LOGERROR << "Volume initialization failed too many times. Making the volume offline.";
    } else {
        auto nextScheduleTime =  std::min(1 << initializerTriesCnt, 60);
        LOGNORMAL << "Scheduling volume initializer retry in: " << nextScheduleTime
            << " seconds. " << logString();
        MODULEPROVIDER()->getTimer()->scheduleFunction(
            std::chrono::seconds(nextScheduleTime), func);
    }
}

Error VolumeMeta::startMigration(const fpi::SvcUuid &srcDmUuid,
                                 const int64_t &volId,
                                 const StatusCb &doneCb)
{
    fds_assert(isSynchronized());

    Error err(ERR_OK);
    fpi::FDSP_VolumeDescType vol;
    vol.volUUID = volId;

    fds_assert(migrationDest == nullptr);
    cbToVGMgr = doneCb;
    uint32_t deltaBlobTimeout = uint32_t(MODULEPROVIDER()->get_fds_config()->
                                get<int32_t>("fds.dm.migration.migration_max_delta_blobs_to", 5));

    // dataManager->counters->migrationLastRun.set(util::getTimeStampSeconds());
    dataManager->counters->numberOfActiveMigrExecutors.incr(1);

    auto dummyId = 0;
    auto srcDmNodeid = NodeUuid(srcDmUuid);
    migrationDest.reset(new DmMigrationDest(dummyId,
                                            *dataManager,
                                            srcDmNodeid,
                                            vol,
                                            deltaBlobTimeout,
                                            std::bind(&VolumeMeta::cleanUpMigrationDestination,
                                                      this,
                                                      std::placeholders::_1,
                                                      std::placeholders::_2,
                                                      std::placeholders::_3)));

    // TODO : once processInitialBlobFilterSet has been fixed to be
    // iterative, then we won't block here.
    err = migrationDest->start();

    return err;
}

Error VolumeMeta::handleMigrationDeltaBlobDescs(DmRequest *dmRequest)
{
    fds_assert(isSynchronized());

    auto typedRequest = static_cast<DmIoMigrationDeltaBlobDesc*>(dmRequest);

    auto err = migrationDest->checkVolmetaVersion(dmRequest->version);
    if (err.OK()) {
        err = migrationDest->processDeltaBlobDescs(typedRequest->deltaBlobDescMsg,
                                                    typedRequest->localCb);
    } else {
        typedRequest->localCb(err);
    }
    return err;
}

Error VolumeMeta::handleMigrationDeltaBlobs(DmRequest *dmRequest)
{
    fds_assert(isSynchronized());

    auto typedRequest = static_cast<DmIoMigrationDeltaBlobs*>(dmRequest);
    auto err = migrationDest->checkVolmetaVersion(dmRequest->version);
    if (err.OK()) {
        err = migrationDest->processDeltaBlobs(typedRequest->deltaBlobsMsg);
    }
    return err;
}

Error VolumeMeta::handleMigrationActiveTx(DmRequest *dmRequest)
{
    fds_assert(isSynchronized());

    /**
     * This is a weird re-route to dest to executor then back to volmeta again
     * because we want to ensure similar failure handling path.
     */
    auto typedRequest = static_cast<DmIoMigrationTxState*>(dmRequest);
    auto err = migrationDest->checkVolmetaVersion(dmRequest->version);
    if (err.OK()) {
        err = migrationDest->processTxState(typedRequest->txStateMsg);
    }
    return err;
}

Error VolumeMeta::serveMigration(DmRequest *dmRequest)
{
    fds_assert(isSynchronized());

    Error err(ERR_OK);
    NodeUuid mySvcUuid(MODULEPROVIDER()->getSvcMgr()->getSelfSvcUuid().svc_uuid);
    DmIoResyncInitialBlob* typedRequest = static_cast<DmIoResyncInitialBlob*>(dmRequest);
    NodeUuid destDmUuid(typedRequest->destNodeUuid);
    fpi::CtrlNotifyInitialBlobFilterSetMsgPtr migReqMsg = typedRequest->message;
    StatusCb cleanupCb = typedRequest->localCb;

    if (getState() != fpi::ResourceState::Active) {
        LOGWARN << "Rejecting serve migration request: " << *migReqMsg
            << logString();
        return ERR_NOT_READY;
    }

    LOGNOTIFY << "migrationid: " << migReqMsg->DMT_version
        <<" received msg for volume " << migReqMsg->volume_id
        << " on svcuuid: " << destDmUuid << " version: " << dmRequest->version;

    err = createMigrationSource(destDmUuid,
                                mySvcUuid,
                                migReqMsg,
                                cleanupCb,
                                dmRequest->version);

    return err;
}

Error VolumeMeta::createMigrationSource(NodeUuid destDmUuid,
                                        const NodeUuid &mySvcUuid,
                                        fpi::CtrlNotifyInitialBlobFilterSetMsgPtr filterSet,
                                        StatusCb cleanup,
                                        int32_t version) {
    Error err(ERR_OK);
    auto maxNumBlobs = uint64_t(MODULEPROVIDER()->get_fds_config()->
                           get<int64_t>("fds.dm.migration.migration_max_delta_blobs"));
    auto maxNumBlobDesc = uint64_t(MODULEPROVIDER()->get_fds_config()->
                              get<int64_t>("fds.dm.migration.migration_max_delta_blob_desc"));


    auto fds_volid = fds_volid_t(filterSet->volume_id);
    fds_verify(fds_volid == vol_desc->GetID());

    auto search = migrationSrcMap.find(destDmUuid);
    if (search != migrationSrcMap.end()) {
        LOGERROR << "migrationid: " << filterSet->DMT_version
            << " Source received request for destination node: " << destDmUuid
            << " volume " << filterSet->volume_id << " but it already exists";
        err = ERR_DUPLICATE;
    } else {
        LOGMIGRATE << "Creating migration source for dest: " << destDmUuid <<
                " for volume: " << vol_desc->name << "(ID: " << vol_desc->volUUID <<
                ") with meta version " << version;
        DmMigrationSrc::shared_ptr source;
        {
            SCOPEDWRITE(migrationSrcMapLock);
            source = DmMigrationSrc::shared_ptr(
                    new DmMigrationSrc(*dataManager,
                                       mySvcUuid,
                                       destDmUuid,
                                       filterSet->DMT_version,
                                       filterSet,
                                       std::bind(&VolumeMeta::cleanUpMigrationSource,
                                                 this,
                                                 std::placeholders::_1,
                                                 std::placeholders::_2,
                                                 destDmUuid),
                                       cleanup,
                                       maxNumBlobs,
                                       maxNumBlobDesc,
                                       version));
            migrationSrcMap.insert(std::make_pair(destDmUuid, source));
        }
        source->run();
    }
    return err;
}

void VolumeMeta::cleanUpMigrationSource(fds_volid_t volId,
                                        const Error &err,
                                        const NodeUuid destDmUuid) {

    if (!err.ok()) {
        LOGERROR << "Cleaning up DmMigrationSrc for vol: " << volId
            << " dest node: " << destDmUuid << " with error: " << err;
    } else {
        LOGNORMAL << "[migrate] Cleaning up DmMigrationSrc for vol: " << volId
            << " dest node: " << destDmUuid << " with error: " << err;

    }
    DmMigrationSrc::shared_ptr source;
    {
        SCOPEDWRITE(migrationSrcMapLock);
        auto search = migrationSrcMap.find(destDmUuid);
        if (search == migrationSrcMap.end()) {
            LOGERROR << "Volume " << volId << " Unable to find entry " << destDmUuid;
            return;
        } else {
            source = search->second;
            // source->sendFinishFwdMsg();
            // source->finish();
            migrationSrcMap.erase(search);
        }
    }
    if (err.OK()) {
        dataManager->counters->totalVolumesSentMigration.incr(1);
    } else {
        dataManager->counters->totalMigrationsAborted.incr(1);
    }
    dataManager->dmMigrationMgr->dumpStats();
}

void VolumeMeta::handleFinishStaticMigration(DmRequest *dmRequest)
{
    fds_assert(isSynchronized());

    dm::QueueHelper helper(*dataManager, dmRequest);
    DmIoFinishStaticMigration *request = static_cast<DmIoFinishStaticMigration*>(dmRequest);
    NodeUuid srcUuid;
    fds_volid_t volId;

    Error err(request->reqMessage->status);
    srcUuid = request->reqMessage->srcUuid.svc_uuid;
    volId.v = request->reqMessage->volume_id;

    /** volId and srcNodeUuid is there for formality */
    if (!err.ok()) {
        LOGERROR << "Cleaning up DmMigrationDest " << logString();
    } else {
        LOGNORMAL << "[migrate] Cleaning up DmMigrationDest " << logString();
    }

    cleanUpMigrationDestination(srcUuid, volId, err);

}

void VolumeMeta::cleanUpMigrationDestination(NodeUuid srcNodeUuid,
                                             fds_volid_t volId,
                                             const Error &err) {

    /** volId and srcNodeUuid is there for formality */
    fds_assert(volId == vol_desc->volUUID);
    fds_assert(isSynchronized());
    if (!err.ok()) {
        LOGERROR << "Cleaning up DmMigrationDest for vol: "
            << volId << " dest node: " << srcNodeUuid << " with error: " << err;
    } else {
        LOGNORMAL << "[migrate] Cleaning up DmMigrationDest for vol: " << volId
            << " dest node: " << srcNodeUuid << " with error: " << err;
    }

    if (cbToVGMgr) {
        /**
         * If this cleanup has been called as part of executor's failure handling,
         * then we do not execute the following. Since everything is done on a volume context
         * we can guarantee there will not be races.
         */
        migrationDest->waitForAsyncMsgs();
        migrationDest.reset();
        dataManager->counters->numberOfActiveMigrExecutors.decr(1);
        if (err.OK()) {
            dataManager->counters->totalVolumesReceivedMigration.incr(1);
        } else {
            dataManager->counters->totalMigrationsAborted.incr(1);
        }
        dataManager->dmMigrationMgr->dumpStats();

        cbToVGMgr(err);
        cbToVGMgr=nullptr;
    }

}


void VolumeMeta::handleVolumegroupUpdate(DmRequest *dmRequest)
{
    fds_assert(isSynchronized());

    dm::QueueHelper helper(*dataManager, dmRequest);
    DmIoVolumegroupUpdate* request = static_cast<DmIoVolumegroupUpdate*>(dmRequest);
    auto &group = request->reqMessage->group;

    LOGDEBUG << logString() << " volume group update ";
    if (getCoordinatorId() != group.coordinator.id) {
        LOGDEBUG << logString() << " Coordinator mismatch.  Ignoring volume group update";
        return;
    }
    if (getState() == fpi::Active) {
        if (group.nonfunctionalReplicas.end() !=
            std::find(group.nonfunctionalReplicas.begin(),
                      group.nonfunctionalReplicas.end(),
                      selfSvcUuid)) {
            setState(fpi::Offline,
                     " - VolumegroupUpdateHandler:coordinator has this volume as nonfunctional");
            scheduleInitializer(false);
        }
        return;
    } else if (isInitializerInProgress()) {
        LOGDEBUG << "Initializer in progress.  Ignoring volume group update";
        return;
    } else if (getState() == fpi::Loading) {  // This branch is when open is in progress
        if (group.functionalReplicas.end() !=
            std::find(group.functionalReplicas.begin(),
                      group.functionalReplicas.end(),
                      selfSvcUuid)) {
            fds_assert(getSequenceId() == static_cast<uint64_t>(group.lastCommitId));
            setOpId(group.lastOpId);
            setState(fpi::Active, " - VolumegroupUpdateHandler:state matched with coordinator");
        } else {
            fds_assert(getSequenceId() != static_cast<uint64_t>(group.lastCommitId));
            LOGWARN << "vol: " << request->volId << " doesn't have active state."
                << " current sequence id: " << getSequenceId()
                << " expected sequence id: " << group.lastCommitId;
            setState(fpi::Offline, " - VolumegroupUpdateHandler:sequence id mismatch");
            scheduleInitializer(false);
        }
        return;
    }
}

VolumeMeta::HashCalcContext::HashCalcContext(fpi::SvcUuid _reqUUID,
                                             int _batchSize) :
        requesterUUID(_reqUUID),
        contextErr(ERR_OK),
        batchSize(_batchSize)
{
    hashResult[0] = '\0';
}

VolumeMeta::HashCalcContext::~HashCalcContext() {
    // typedRequest gets deleted as callback of registerDmVolumeReqHandler<DmIoVolumeCheck>();
}

void
VolumeMeta::printDebugSlice(CatalogKVPair &pair) {

    auto keyType = reinterpret_cast<CatalogKeyType const*>(pair.first.data());
    int type = ((unsigned char)(*keyType) - (unsigned char)CatalogKeyType::ERROR);
    auto objID = ObjIdGen::genObjectId((const char*)&pair, sizeof(pair));
    LOGDEBUG << "Hashing one KV pair type: " << type << " with hash: " << objID.ToString();
}

void
VolumeMeta::HashCalcContext::hashThisSlice(CatalogKVPair &pair) {
    auto keyType = reinterpret_cast<CatalogKeyType const*>(pair.first.data());
    int type = ((unsigned char)(*keyType) - (unsigned char)CatalogKeyType::ERROR);
    // We do a whitelist type of hashing.
    if ((*keyType == CatalogKeyType::BLOB_METADATA) ||
        (*keyType == CatalogKeyType::BLOB_OBJECTS)) {
        hasher.update((const fds_byte_t *)&pair, sizeof(pair));
    }
}

void
VolumeMeta::HashCalcContext::computeCompleteHash() {
    hasher.final(hashResult);
}

Error
VolumeMeta::createHashCalcContext(DmRequest *req,
                                  fpi::SvcUuid _reqUUID,
                                  int batchSize) {
    std::unique_lock<fds_mutex> l(testMutex);
    if (hashCalcContextPtr) {
        LOGERROR << "A hash request has already been sent and is processing.";
        return ERR_DUPLICATE;
    } else {
        // As part of development, execute this in an online fashion
#if 0
        if (getState() != fpi::Offline) {
            LOGERROR << "Volume is not offline.";

            // For now, just set it offline... this needs another code path
            setState(fpi::Offline, " Volume hash operation requested.");
        }
#endif

        scanReq = static_cast<DmIoVolumeCheck*>(req);

        // Pass everything needed to HashCalcContext's constructor
        hashCalcContextPtr = new HashCalcContext (_reqUUID,
                                                  batchSize);

        auto req = std::bind(&VolumeMeta::doHashTaskOnContext, this);
        dataManager->addToQueue(req, FdsDmSysTaskId);
        return ERR_OK;
    }
}
void
VolumeMeta::cleanupHashOnContext() {
    if (hashCalcContextPtr != nullptr) {
        scanReq->respStatus = hashCalcContextPtr->contextErr;
        scanReq->respMessage->hash_result = std::string(reinterpret_cast<char*>(hashCalcContextPtr->hashResult));
        scanReq->cb(scanReq->respStatus, scanReq);
        delete hashCalcContextPtr;
    }
}

void
VolumeMeta::doHashTaskOnContext() {

    fds_assert(hashCalcContextPtr);
    if (hashCalcContextPtr == nullptr) {
        LOGERROR << "Hash calculation error";
        return;
    }
    // We first init the CatalogScanner
    auto catalogPtr = dataManager->getPersistDB(vol_desc->volUUID)->getCatalog();
    auto threadPoolPtr = dataManager->qosCtrl->threadPool;

    // For each batch, what do we do?
    auto batchCb = [this](std::list<CatalogKVPair> &batchSlice) {
        for (auto &kvPair : batchSlice) {
            printDebugSlice(kvPair);
            hashCalcContextPtr->hashThisSlice(kvPair);
        }
    };

    // For the last batch, batchCb ALWAYS gets called before scannerCb
    auto scannerCb = [this](CatalogScanner::progress scannerProgress) {
        switch (scannerProgress) {
            case (CatalogScanner::CS_DONE):
            {
                hashCalcContextPtr->computeCompleteHash();
                break;
            }
            case (CatalogScanner::CS_ERROR):
            {
                break;
            }
            default:
            {
                fds_assert(!"We shouldn't hit this in dev.");
                LOGERROR << "This is a weird case.";
                break;
            }
        }
        cleanupHashOnContext();
    };

    scannerPtr = new CatalogScanner(*catalogPtr,
                                 threadPoolPtr,
                                 static_cast<unsigned>(hashCalcContextPtr->batchSize),
                                 batchCb,
                                 scannerCb);

    scannerPtr->start();
}
}  // namespace fds
