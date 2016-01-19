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
            cbToVGMgr(NULL)
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

    auto err = dataManager->timeVolCat_->getCommitlog(volId, commitLog);
    if (!err.ok()) {
        return err;
    }

    commitLog->clear();
    err = commitLog->applySerializedTxs(txs);
    if (!err.ok()) {
        return err;
    }

    setOpId(highestOpId);

    return ERR_OK;
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
    vol_desc->state = state;
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

    std::stringstream ss;
    ss << state;
    return ss.str();
}

std::function<void()> VolumeMeta::makeSynchronized(const std::function<void()> &f)
{
    auto qosCtrl = dataManager->getQosCtrl();
    fds_volid_t volId(getId());
    auto newCb = [f, qosCtrl, volId]() {
        auto ioReq = new DmFunctor(volId, std::bind(f));
        auto err = qosCtrl->enqueueIO(volId, ioReq);
        if (err != ERR_OK) {
            GLOGWARN << "Failed to enqueue volume synchronized DmFunctor.  Dropping. " << err;
            delete ioReq;
        }
    };
    return newCb;
}

EPSvcRequestRespCb
VolumeMeta::makeSynchronized(const EPSvcRequestRespCb &f)
{
    auto qosCtrl = dataManager->getQosCtrl();
    fds_volid_t volId(getId());
    auto newCb = [f, qosCtrl, volId](EPSvcRequest* req, const Error &e, StringPtr payload) {
        auto ioReq = new DmFunctor(volId, std::bind(f, req, e, payload));
        auto err = qosCtrl->enqueueIO(volId, ioReq);
        if (err != ERR_OK) {
            GLOGWARN << "Failed to enqueue volume synchronized DmFunctor.  Dropping. " << err;
            delete ioReq;
        }
    };
    return newCb;
}

StatusCb VolumeMeta::makeSynchronized(const StatusCb &f)
{
    auto qosCtrl = dataManager->getQosCtrl();
    fds_volid_t volId(getId());
    auto newCb = [f, qosCtrl, volId](const Error &e) {
        auto ioReq = new DmFunctor(volId, std::bind(f, e));
        auto err = qosCtrl->enqueueIO(volId, ioReq);
        if (err != ERR_OK) {
            GLOGWARN << "Failed to enqueue volume synchronized DmFunctor.  Dropping. " << err;
            delete ioReq;
        }
    };
    return newCb;
}

BufferReplay::ProgressCb VolumeMeta::synchronizedProgressCb(const BufferReplay::ProgressCb &f)
{
    auto qosCtrl = dataManager->getQosCtrl();
    fds_volid_t volId(getId());
    auto newCb = [f, qosCtrl, volId](BufferReplay::Progress status) {
        auto ioReq = new DmFunctor(volId, std::bind(f, status));
        auto err = qosCtrl->enqueueIO(volId, ioReq);
        if (err != ERR_OK) {
            GLOGWARN << "Failed to enqueue volume synchronized DmFunctor.  Dropping. " << err;
            delete ioReq;
        }
    };
    return newCb;
}
void VolumeMeta::startInitializer()
{
    fds_assert(getState() == fpi::Offline);
    fds_assert(!isInitializerInProgress());

    LOGDEBUG << "Starting initializer: " << logString();

    /* Coordinator is set. We can go through sync protocol */
    setState(fpi::Loading, " - startInitializer");
    initializer = MAKE_SHARED<VolumeInitializer>(MODULEPROVIDER(), this);
    initializer->run();
}

void VolumeMeta::cleanupInitializer()
{
    fds_assert(initializer->getProgress() == VolumeInitializer::COMPLETE);
    initializer.reset();
    LOGDEBUG << "Cleanedup initializer: " << logString();
}

Error VolumeMeta::startMigration(const fpi::SvcUuid &srcDmUuid,
                                 const int64_t &volId,
                                 const StatusCb &doneCb)
{
    Error err(ERR_OK);
    fpi::FDSP_VolumeDescType vol;
    vol.volUUID = volId;

    fds_assert(migrationDest == nullptr);
    cbToVGMgr = doneCb;
    uint32_t deltaBlobTimeout = uint32_t(MODULEPROVIDER()->get_fds_config()->
                                get<int32_t>("fds.dm.migration.migration_max_delta_blobs_to", 5));


    // DataMgr *nonConstDm = dataManager;

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
    auto typedRequest = static_cast<DmIoMigrationDeltaBlobDesc*>(dmRequest);
    auto err = migrationDest->processDeltaBlobDescs(typedRequest->deltaBlobDescMsg,
                                                    typedRequest->localCb);
    return err;
}

Error VolumeMeta::handleMigrationDeltaBlobs(DmRequest *dmRequest)
{
    auto typedRequest = static_cast<DmIoMigrationDeltaBlobs*>(dmRequest);
    auto err = migrationDest->processDeltaBlobs(typedRequest->deltaBlobsMsg);
    return err;
}

Error VolumeMeta::serveMigration(DmRequest *dmRequest) {
    Error err(ERR_OK);
    NodeUuid mySvcUuid(MODULEPROVIDER()->getSvcMgr()->getSelfSvcUuid().svc_uuid);
    DmIoResyncInitialBlob* typedRequest = static_cast<DmIoResyncInitialBlob*>(dmRequest);
    NodeUuid destDmUuid(typedRequest->destNodeUuid);
    fpi::CtrlNotifyInitialBlobFilterSetMsgPtr migReqMsg = typedRequest->message;
    StatusCb cleanupCb = typedRequest->localCb;

    LOGNOTIFY << "migrationid: " << migReqMsg->DMT_version
        <<" received msg for volume " << migReqMsg->volumeId
        << " on svcuuid: " << destDmUuid;

    err = createMigrationSource(destDmUuid, mySvcUuid, migReqMsg, cleanupCb);

    return err;
}

Error VolumeMeta::createMigrationSource(NodeUuid destDmUuid,
                                        const NodeUuid &mySvcUuid,
                                        fpi::CtrlNotifyInitialBlobFilterSetMsgPtr filterSet,
                                        StatusCb cleanup) {
    Error err(ERR_OK);
    auto maxNumBlobs = uint64_t(MODULEPROVIDER()->get_fds_config()->
                           get<int64_t>("fds.dm.migration.migration_max_delta_blobs"));
    auto maxNumBlobDesc = uint64_t(MODULEPROVIDER()->get_fds_config()->
                              get<int64_t>("fds.dm.migration.migration_max_delta_blob_desc"));


    auto fds_volid = fds_volid_t(filterSet->volumeId);
    fds_verify(fds_volid == vol_desc->GetID());

    auto search = migrationSrcMap.find(destDmUuid);
    if (search != migrationSrcMap.end()) {
        LOGERROR << "migrationid: " << filterSet->DMT_version
            << " Source received request for destination node: " << destDmUuid
            << " volume " << filterSet->volumeId << " but it already exists";
        err = ERR_DUPLICATE;
    } else {
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
                                       maxNumBlobDesc));
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
}

void VolumeMeta::handleFinishStaticMigration(DmRequest *dmRequest)
{
    dm::QueueHelper helper(*dataManager, dmRequest);
    DmIoFinishStaticMigration *request = static_cast<DmIoFinishStaticMigration*>(dmRequest);

    Error err(request->reqMessage->status);

    /** volId and srcNodeUuid is there for formality */
    if (!err.ok()) {
        LOGERROR << "Cleaning up DmMigrationDest " << logString();
    } else {
        LOGNORMAL << "[migrate] Cleaning up DmMigrationDest " << logString();
    }

    /* This shouldn't block.  This is there to avoid the assert ~MigrationTrackIOReqs() */
    migrationDest->waitForAsyncMsgs();
    migrationDest.reset();

    cbToVGMgr(err);
    cbToVGMgr=nullptr;
}

// TODO(Rao): remoe the following
void VolumeMeta::cleanUpMigrationDestination(NodeUuid srcNodeUuid,
                                             fds_volid_t volId,
                                             const Error &err) {

    /** volId and srcNodeUuid is there for formality */
    fds_assert(volId == vol_desc->volUUID);
    if (!err.ok()) {
        LOGERROR << "Cleaning up DmMigrationDest for vol: "
            << volId << " dest node: " << srcNodeUuid << " with error: " << err;
    } else {
        LOGNORMAL << "[migrate] Cleaning up DmMigrationDest for vol: " << volId
            << " dest node: " << srcNodeUuid << " with error: " << err;
    }

    migrationDest.reset();

    cbToVGMgr(err);
    cbToVGMgr=nullptr;
}


void VolumeMeta::handleVolumegroupUpdate(DmRequest *dmRequest)
{
    dm::QueueHelper helper(*dataManager, dmRequest);
    DmIoVolumegroupUpdate* request = static_cast<DmIoVolumegroupUpdate*>(dmRequest);

    LOGDEBUG << "Attempting to set volumegroup info for vol: '"
             << std::hex << request->volId << std::dec << "'";
    if (getState() != fpi::Loading) {
        LOGWARN << "Failed setting volumegroup info vol: " << request->volId
            << ". Volume isn't in loading state";
        helper.err = ERR_INVALID;
        return;
    } else if (getSequenceId() !=
               static_cast<uint64_t>(request->reqMessage->group.lastCommitId)) {
        LOGWARN << "vol: " << request->volId << " doesn't have active state."
            << " current sequence id: " << getSequenceId()
            << " expected sequence id: " << request->reqMessage->group.lastCommitId;
        setState(fpi::Offline, " - VolumegroupUpdateHandler:sequence id mismatch");
        // TODO(Rao): At this point we should trigger a sync
        return;
    }
    setOpId(request->reqMessage->group.lastOpId);
    setState(fpi::Active, " - VolumegroupUpdateHandler:state matched with coordinator");
}

}  // namespace fds
