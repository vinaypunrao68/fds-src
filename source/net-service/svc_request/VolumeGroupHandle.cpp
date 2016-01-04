/* Copyright 2015 Formation Data Systems, Inc.
 */
#include <vector>
#include <net/VolumeGroupHandle.h>
#include <net/SvcRequestPool.h>

namespace fds {

#define NONE_RET

#define ASSERT_SYNCHRONIZED()

#define INVALID_REAPLICA_HANDLE() functionalReplicas_.end()

#define GET_VOLUMEREPLICA_HANDLE_RETURN(volumeHandle__, svcUuid__, ret__) \
    auto volumeHandle__ = getVolumeReplicaHandle_(svcUuid__); \
    do { \
    if (volumeHandle__ == INVALID_REAPLICA_HANDLE()) { \
        LOGWARN << "Failed to lookup replica handle for: " << svcUuid__.svc_uuid; \
        return ret__; \
    } \
    } while (false)

#define GET_VOLUMEREPLICA_HANDLE(volumeHandle__, svcUuid__) \
        GET_VOLUMEREPLICA_HANDLE_RETURN(volumeHandle__, svcUuid__, NONE_RET)

std::ostream& operator << (std::ostream &out, const fpi::VolumeIoHdr &h)
{
    out << " groupId: " << h.groupId 
        << " svcUuid: " << SvcMgr::mapToSvcUuidAndName(h.svcUuid)
        << " opId: " << h.opId
        << " commitId: " << h.commitId;
    return out;
}

std::string VolumeReplicaHandle::logString() const
{
    std::stringstream ss;
    ss << " [VolumeReplicaHandle" 
        << " svcUuid: " << SvcMgr::mapToSvcUuidAndName(svcUuid)
        << " version: " << version
        << " state: " << fpi::_ResourceState_VALUES_TO_NAMES.at(static_cast<int>(state))
        << " lasterr: " << lastError
        << " appliedOp: " << appliedOpId
        << " appliedCommit: " << appliedCommitId
        << "] ";

    return ss.str();
}

VolumeGroupHandle::VolumeGroupHandle(CommonModuleProviderIf* provider,
                                     const fds_volid_t& volId,
                                     uint32_t quorumCnt)
: HasModuleProvider(provider)
{
    taskExecutor_ = MODULEPROVIDER()->getSvcMgr()->getTaskExecutor();
    requestMgr_ = MODULEPROVIDER()->getSvcMgr()->getSvcRequestMgr();
    quorumCnt_ = quorumCnt;
    state_ = fpi::ResourceState::Unknown;

    groupId_ = volId.get();
    version_ = VolumeGroupConstants::VERSION_INVALID;
}

#if 0
VolumeGroupHandle::init(CommonModuleProviderIf* provider,
                        const fpi::VolumeGroupInfo &groupInfo,
                        int32_t quorumCnt)
: HasModuleProvider(provider)
{
    taskExecutor_ = MODULEPROVIDER()->getSvcMgr()->getTaskExecutor();
    requestMgr_ = MODULEPROVIDER()->getSvcMgr()->getSvcRequestMgr();
    quorumCnt_ = quorumCnt;
    state_ = fpi::ResourceState::Unknown;
    setGroupInfo_(groupInfo);
    // TODO(Rao): Go through protocol figure out the states of the replicas
    // For now set every handle as functional and start their op/commit ids
    // from beginning
    for (auto &r : functionalReplicas_) {
        r.setInfo(VolumeGroupConstants::VERSION_START,
                  fpi::ResourceState::Active,
                  VolumeGroupConstants::OPSTARTID,
                  VolumeGroupConstants::COMMITSTARTID);
    }
    // TODO(Rao): Set this to be the latest ids after querying the replicas
    opSeqNo_ = VolumeGroupConstants::OPSTARTID;
    commitNo_ = VolumeGroupConstants::COMMITSTARTID;
}
#endif

void VolumeGroupHandle::setListener(VolumeGroupHandleListener *listener)
{
    listener_ = listener;
}

void VolumeGroupHandle::toggleWriteOpsBuffering_(bool enable)
{
    if (enable) {
        writeOpsBuffer_.reset(new WriteOpsBuffer(WRITEOPS_BUFFER_SZ));
        LOGNORMAL << logString() << " Enabled write ops buffering";
    } else {
        writeOpsBuffer_.reset();
        LOGNORMAL << logString() << " Disabled write ops buffering";
    }
}

bool VolumeGroupHandle::replayFromWriteOpsBuffer_(const fpi::SvcUuid &svcUuid,
                                                  const int64_t replayStartOpId)
{
    fds_assert(opSeqNo_ >= static_cast<int64_t>(writeOpsBuffer_->size()));
    fds_assert(replayStartOpId > VolumeGroupConstants::OPSTARTID &&
               replayStartOpId <= opSeqNo_+1);
    if (replayStartOpId == opSeqNo_+1) {
        /* There is nothing to replay */
        return true;
    }

    /* Figure out the range for ops buffered */
    int64_t fromOpId = opSeqNo_ - writeOpsBuffer_->size() + 1;
    int64_t toOpId = opSeqNo_;
    if (replayStartOpId < fromOpId || replayStartOpId > toOpId) {
        LOGWARN << logString()
            << "replayStartOpId: " << replayStartOpId << " doesn't fall in the range ["
            << fromOpId << ", " << toOpId << "]";
        return false;
    }

    LOGNORMAL << logString() << " replaying writeOpsBuffer from: " << replayStartOpId
        << " to: " << opSeqNo_;

    int32_t idx = replayStartOpId - fromOpId;
    for (; idx < static_cast<int64_t>(writeOpsBuffer_->size()); idx++) {
        auto req = requestMgr_->newEPSvcRequest(svcUuid);
        req->setTaskExecutorId(groupId_);
        req->setPayloadBuf((*writeOpsBuffer_)[idx].first, (*writeOpsBuffer_)[idx].second);
        /* NOTE: For now send as fire and forget.  This is fine because we haven't considered
         * the replica to be functional yet.  Also, on the DM side incase opid doens't match
         * DM will reject messages
         */
        req->invoke();
    }
    return true;
}

void VolumeGroupHandle::resetGroup_()
{
    /* NOTE: Consider not incrementing version every time open is called.  If open fails
     * we may not want to incrment the version.
     */
    version_++;

    changeState_(fpi::ResourceState::Offline, __FUNCTION__);

    opSeqNo_ = VolumeGroupConstants::OPSTARTID;
    commitNo_ = VolumeGroupConstants::COMMITSTARTID;

    /* Make all the replica handles non-functional */
    auto svcMgr = MODULEPROVIDER()->getSvcMgr();
    dmtVersion_ = svcMgr->getDMTVersion();
    auto svcs = svcMgr->getDMTNodesForVolume(fds_volid_t(groupId_))->toSvcUuids();
    functionalReplicas_.clear();
    nonfunctionalReplicas_.clear();
    syncingReplicas_.clear();
    for (const auto &svcUuId : svcs) {
        nonfunctionalReplicas_.push_back(VolumeReplicaHandle(svcUuId));
    }
}

void VolumeGroupHandle::open(const SHPTR<fpi::OpenVolumeMsg>& msg,
                             const StatusCb &clientCb)
{
    runSynchronized([this, msg, clientCb]() mutable {
        if (state_ == fpi::ResourceState::Active) {
            LOGWARN << logString() << " - Trying open an already opened volume";
            clientCb(ERR_INVALID);
            return;
        }

        try {
            resetGroup_();
        } catch (const Exception &e) {
            LOGWARN << logString() << " - Failed to get nodes from DMT";
            clientCb(e.getError());
            return;
        }
        
        /* Send a message to OM requesting to be coordinator for the group */
        auto setCoordinatorreq = createSetVolumeGroupCoordinatorMsgReq_();
        setCoordinatorreq->onResponseCb([this, clientCb](EPSvcRequest*,
                                                         const Error& e,
                                                         StringPtr) {
           if (e != ERR_OK) {
            LOGWARN << logString()
                    << " Failed set volume group coordinator.  Received "
                    << e << " from om";
            clientCb(e);
            return;
           }
           auto openReq = createPreareOpenVolumeGroupMsgReq_();
           openReq->onResponseCb([this, clientCb](QuorumSvcRequest* openReq,
                                                  const Error& e_,
                                                  boost::shared_ptr<std::string> payload) {
             Error e = e_;
             auto responseMsg = fds::deserializeFdspMsg<fpi::OpenVolumeRspMsg>(e, payload);
             if (e != ERR_OK) {
                 LOGWARN << logString() << "Prepare for open failed: " << e;
                 clientCb(e);
                 return;
             }
             determineFunctaionalReplicas_(openReq);
             if (functionalReplicas_.size() == 0) {
                 LOGWARN << logString()
                     << " Not enough members with latest state to start a group";
                 clientCb(ERR_VOLUMEGROUP_DOWN);
                 return;
             }
             changeState_(fpi::ResourceState::Active, "Open volume");
             broadcastGroupInfo_();
             clientCb(ERR_OK);
             return;
           });
           openReq->invoke();
        });
        LOGNORMAL << logString() << " - Setting coordinator request to OM";
        setCoordinatorreq->invoke();
    });
}

void VolumeGroupHandle::close()
{
    runSynchronized([this]() {
        changeState_(fpi::ResourceState::Offline, "Close");
    });
}

std::string VolumeGroupHandle::logString() const
{
    std::stringstream ss;
    ss << " [VolumeGroupHandle: " << groupId_
        << " version: " << version_
        << " state:  " << fpi::_ResourceState_VALUES_TO_NAMES.at(static_cast<int>(state_))
        << " up:" << functionalReplicas_.size()
        << " sync:" << syncingReplicas_.size()
        << " down:" << nonfunctionalReplicas_.size()
        << "] ";
    return ss.str();
}

EPSvcRequestPtr
VolumeGroupHandle::createSetVolumeGroupCoordinatorMsgReq_()
{
    auto msg = MAKE_SHARED<fpi::SetVolumeGroupCoordinatorMsg>();
    assign(msg->coordinator.id, groupId_);
    msg->coordinator.version = version_;
    msg->volumeId = groupId_;
    auto omUuid = MODULEPROVIDER()->getSvcMgr()->getOmSvcUuid();
    auto req = requestMgr_->newEPSvcRequest(omUuid);
    req->setTaskExecutorId(groupId_);
    req->setPayload(FDSP_MSG_TYPEID(fpi::SetVolumeGroupCoordinatorMsg), msg);
    return req;
}

QuorumSvcRequestPtr
VolumeGroupHandle::createPreareOpenVolumeGroupMsgReq_()
{
    fds_assert(functionalReplicas_.size() == 0 &&
               syncingReplicas_.size() == 0 &&
               nonfunctionalReplicas_.size() > 0);
    std::vector<fpi::SvcUuid> replicas;
    std::for_each(nonfunctionalReplicas_.begin(),
                  nonfunctionalReplicas_.end(),
                  [&replicas](const VolumeReplicaHandle &h) { replicas.push_back(h.svcUuid); });

    auto prepareMsg = boost::make_shared<fpi::OpenVolumeMsg>();
    prepareMsg->volume_id = groupId_;
    // TODO(Rao): Set the token.  Set cooridnator as well
    // prepareMsg->token = volReq->token;
    // prepareMsg->mode = volReq->mode;

    auto req = requestMgr_->newSvcRequest<QuorumSvcRequest>(getDmtVersion(), replicas);
    req->setTaskExecutorId(groupId_);
    req->setPayload(FDSP_MSG_TYPEID(fpi::OpenVolumeMsg), prepareMsg);
    req->setQuorumCnt(replicas.size());
    return req;
}

void VolumeGroupHandle::determineFunctaionalReplicas_(QuorumSvcRequest* openReq)
{
    using Response = std::pair<fpi::SvcUuid, fpi::OpenVolumeRspMsgPtr>;
    std::vector<Response> successSvcs;
    std::vector<fpi::SvcUuid> errdSvcs;

    /* Split services that responded succusfully from the ones that failed */
    auto sz = size();    
    for (int32_t i = 0; i < sz; i++) {
        const auto &ep = openReq->ep(i);
        if (ep->responseStatus() == ERR_OK) {
            Error e;
            auto resp = deserializeFdspMsg<fpi::OpenVolumeRspMsg>(e, ep->responsePayload());
            if (e != ERR_OK) {
                fds_assert(!"Deserialization shouldn't fail here");
                errdSvcs.push_back(ep->getPeerEpId());
                continue;
            }
            successSvcs.push_back(std::make_pair(ep->getPeerEpId(), resp));
            LOGNORMAL << logString()
                << " open success svc: " << fds::logString(ep->getPeerEpId())
                << " sequenceid: " << resp->sequence_id;
        } else {
            errdSvcs.push_back(ep->getPeerEpId());
            LOGNORMAL << logString()
                << " open errored svc: " << fds::logString(ep->getPeerEpId())
                << ep->responseStatus();
        }
    }

    /* Sort open responses by sequence id in descending order */
    std::sort(successSvcs.begin(),
              successSvcs.end(),
              [](const Response &lhs, const Response &rhs) {
                  return lhs.second->sequence_id > rhs.second->sequence_id;
              });

    /* Figure out if have quorum # of responses with latest state */
    uint32_t latestStateReplicas = 1;
    for (uint32_t i = 1; i < successSvcs.size(); ++i) {
        if (successSvcs[i-1].second->sequence_id != successSvcs[i].second->sequence_id) {
            break;
        }
        ++latestStateReplicas;
    }
    if (latestStateReplicas < quorumCnt_) {
        LOGWARN << logString() << " Not enough members with latest state to start a group";
        return;
    }
    for (uint32_t i = 0; i < latestStateReplicas; i++) {
        auto volumeHandle = getVolumeReplicaHandle_(successSvcs[i].first);
        /* For replica handle to be functional transition from syncing to functional so that
         * all the accounting is taken care for ids appropriately
         */
        changeVolumeReplicaState_(volumeHandle,
                                  successSvcs[i].second->replicaVersion,
                                  fpi::ResourceState::Syncing,
                                  opSeqNo_,
                                  ERR_OK,
                                  "Open");
        changeVolumeReplicaState_(volumeHandle,
                                  successSvcs[i].second->replicaVersion,
                                  fpi::ResourceState::Active,
                                  opSeqNo_,
                                  ERR_OK,
                                  "Open");
    }
}

void VolumeGroupHandle::broadcastGroupInfo_()
{
    auto msg = MAKE_SHARED<fpi::VolumeGroupInfoUpdateCtrlMsg>();
    msg->group = getGroupInfoForExternalUse_();
    auto req = requestMgr_->newSvcRequest<QuorumSvcRequest>(
        getDmtVersion(),
        getAllReplicas());
    req->setPayload(FDSP_MSG_TYPEID(fpi::VolumeGroupInfoUpdateCtrlMsg), msg);
    /* This broadcast intentionall is fire and forget */
    req->invoke();
}

void VolumeGroupHandle::changeState_(const fpi::ResourceState &targetState,
                                     const std::string& logCtx)
{
    if (targetState == fpi::ResourceState::Active) {
        fds_assert(functionalReplicas_.size() >= quorumCnt_);
    } else {
        functionalReplicas_.clear();
        nonfunctionalReplicas_.clear();
        syncingReplicas_.clear();
    }
    state_ = targetState;

    LOGNORMAL << logString() << " - State changed.  Context: " << logCtx;
}

void VolumeGroupHandle::handleAddToVolumeGroupMsg(
    const fpi::AddToVolumeGroupCtrlMsgPtr &addMsg,
    const std::function<void(const Error&, const fpi::AddToVolumeGroupRespCtrlMsgPtr&)> &cb)

{
    runSynchronized([this, addMsg, cb]() mutable {
        auto respMsg = MAKE_SHARED<fpi::AddToVolumeGroupRespCtrlMsg>();
        auto volumeHandle = getVolumeReplicaHandle_(addMsg->svcUuid);
        if (volumeHandle == INVALID_REAPLICA_HANDLE()) {
            LOGWARN << "Failed to lookup replica handle for: " << addMsg->svcUuid.svc_uuid;
            // TODO(Rao): Return better error code here
            cb(ERR_INVALID, respMsg);
            return;
        }

        Error err(ERR_OK);
        err = changeVolumeReplicaState_(volumeHandle,
                                        addMsg->replicaVersion,
                                        addMsg->targetState,
                                        addMsg->lastOpId,
                                        ERR_OK,
                                        __FUNCTION__);
        respMsg->group = getGroupInfoForExternalUse_();
        cb(err, respMsg);
    });
}

void VolumeGroupHandle::handleVolumeResponse(const fpi::SvcUuid &srcSvcUuid,
                                             const int32_t &replicaVersion,
                                             const fpi::VolumeIoHdr &hdr,
                                             const fpi::FDSPMsgTypeId &msgTypeId,
                                             const bool writeReq,
                                             const Error &inStatus,
                                             Error &outStatus,
                                             uint8_t &successAcks)
{
    ASSERT_SYNCHRONIZED();
    GROUPHANDLE_FUNCTIONAL_CHECK();

    outStatus = inStatus;

    LOGDEBUG << "svcuuid: " << SvcMgr::mapToSvcUuidAndName(srcSvcUuid)
        << fds::logString(hdr) << inStatus;

    auto volumeHandle = getVolumeReplicaHandle_(srcSvcUuid);

    /* Do a version check to ignore responses from previous incarnation of volume replica */
    if (replicaVersion != volumeHandle->version) {
        LOGWARN << "Ignoring response.  Version check failed. svcuuid: "
            << SvcMgr::mapToSvcUuidAndName(srcSvcUuid)
            << volumeHandle->logString()
            << " incoming replica version:  " << replicaVersion
            << fds::logString(hdr) << inStatus;
        return;
    }

    if (volumeHandle->isFunctional() ||
        volumeHandle->isSyncing()) {
        /* Check with listener if we should consider the incoming error to be not an error */
        if (inStatus != ERR_OK && 
            (listener_ && !listener_->isError(msgTypeId, inStatus))) {
            outStatus = ERR_OK;
        }

        if (outStatus == ERR_OK) {
#ifdef IOHEADER_SUPPORTED
            /* Do opid sequence checks only for requests that mutate state on replica */
            if (writeReq) {
                fds_verify(volumeHandle->appliedOpId+1 == hdr.opId);
                fds_verify(volumeHandle->appliedCommitId == hdr.commitId ||
                           volumeHandle->appliedCommitId+1 == hdr.commitId);
                volumeHandle->appliedOpId = hdr.opId;
                volumeHandle->appliedCommitId = hdr.commitId;
            }
#endif
            if (volumeHandle->isFunctional()) {
                successAcks++;
            }
        } else {
            auto changeErr = changeVolumeReplicaState_(volumeHandle,
                                                       volumeHandle->version,
                                                       fpi::ResourceState::Offline,
                                                       opSeqNo_,
                                                       outStatus,
                                                       __FUNCTION__);
            fds_verify(changeErr == ERR_OK);
            if (functionalReplicas_.size() < quorumCnt_) {
                changeState_(fpi::ResourceState::Offline, " not enough active replicas");
            }
        }
    } else {
        /* When replica isn't functional we don't expect subsequent IO to return with
         * success status
         */
        fds_verify(inStatus != ERR_OK);
    }
}

std::vector<VolumeReplicaHandle*> VolumeGroupHandle::getIoReadyReplicaHandles()
{
    std::vector<VolumeReplicaHandle*> replicas;
    for (auto &r : functionalReplicas_) {
        replicas.push_back(&r);
    }
    for (auto &r : syncingReplicas_) {
        replicas.push_back(&r);
    }
    return replicas;
}

VolumeReplicaHandle* VolumeGroupHandle::getFunctionalReplicaHandle()
{
    return &(functionalReplicas_.back());
}

std::vector<fpi::SvcUuid> VolumeGroupHandle::getAllReplicas() const
{
    std::vector<fpi::SvcUuid> svcs;
    auto appendF = [&svcs](const VolumeReplicaHandle& h) { svcs.push_back(h.svcUuid); };
    std::for_each(functionalReplicas_.begin(), functionalReplicas_.end(), appendF);
    std::for_each(nonfunctionalReplicas_.begin(), nonfunctionalReplicas_.end(), appendF);
    std::for_each(syncingReplicas_.begin(), syncingReplicas_.end(), appendF);
    return svcs;
}

VolumeGroupHandle::VolumeReplicaHandleList&
VolumeGroupHandle::getVolumeReplicaHandleList_(const fpi::ResourceState& s)
{
    if (VolumeReplicaHandle::isFunctional(s)) {
        return functionalReplicas_;
    } else if (VolumeReplicaHandle::isSyncing(s)) {
        return syncingReplicas_;
    } else {
        fds_verify(VolumeReplicaHandle::isNonFunctional(s));
        return nonfunctionalReplicas_;
    }
}

/**
* @brief 
* NOTE: Ensure all state changes for volume go through this function so that we have
* central function to coordinate state changes
*
* @param volumeHandle
* @param targetState
*
* @return 
*/
Error VolumeGroupHandle::changeVolumeReplicaState_(VolumeReplicaHandleItr &volumeHandle,
                                                   const int32_t &replicaVersion,
                                                   const fpi::ResourceState &targetState,
                                                   const int64_t &opId,
                                                   const Error &e,
                                                   const std::string &context)
{
    /* VolumeReplicaHandle state transitions
     * Transition cycle is expected to be Loading->Syncing -> Active -> Offline
     * Loading doesn't require any state change.  It's more of a query to figure
     * out the current group state.
     * On every new transition cycle starting with Syncing, version # is expected
     * to be incremented.
     * On a particual version # states transtions can only take place in the above cycle order.
     * For ex. at replica handle at version 1, can't go from state Active to Syncing on same
     * version #.
     */
    auto srcState = volumeHandle->state;

    if (targetState == fpi::ResourceState::Loading) {
        /* No state change is necessary.  Replica handle is still considered non-functional.
         * We just need to respond back with current group information so it can copy 
         * active transactions from a peer.
         * We will start buffering writes so that when replica handles come back again
         * after copying active transactions, we will replay buffered writes.
         */
        toggleWriteOpsBuffering_(true);
    } else if (VolumeReplicaHandle::isSyncing(targetState)) {
        if (replicaVersion != VolumeGroupConstants::VERSION_START &&
            replicaVersion <= volumeHandle->version) {
            fds_assert(!"Invalid version");
            return ERR_INVALID_VOLUME_VERSION;
        }
        replayFromWriteOpsBuffer_(volumeHandle->svcUuid, opId+1);
        toggleWriteOpsBuffering_(true);
        volumeHandle->setInfo(replicaVersion, targetState, opSeqNo_, commitNo_);
    } else if (VolumeReplicaHandle::isNonFunctional(targetState)) {
        volumeHandle->setState(targetState);
        volumeHandle->setError(e);
    } else if (VolumeReplicaHandle::isFunctional(targetState)){
        if (replicaVersion != volumeHandle->version) {
            fds_assert(!"Invalid version");
            return ERR_INVALID_VOLUME_VERSION;
        }
        volumeHandle->setVersion(replicaVersion);
        volumeHandle->setState(targetState);
        volumeHandle->setError(ERR_OK);
    }
    LOGNORMAL << volumeHandle->logString() << " state changed. Context - " << context;

    /* Move the handle from the appropriate replica list
     * NOTE: we move volumeHandle to the end in the destination list
     */
    auto &srcList = getVolumeReplicaHandleList_(srcState);
    auto &dstList = getVolumeReplicaHandleList_(targetState);
    auto dstItr = std::move(volumeHandle, volumeHandle+1, std::back_inserter(dstList));
    srcList.erase(volumeHandle, volumeHandle+1);
    volumeHandle = dstList.end() - 1;

    return ERR_OK;
}

fpi::VolumeGroupInfo VolumeGroupHandle::getGroupInfoForExternalUse_()
{
    fpi::VolumeGroupInfo groupInfo; 
    groupInfo.groupId = groupId_;
    groupInfo.version = version_;
    groupInfo.lastOpId = opSeqNo_;
    groupInfo.lastCommitId = commitNo_;
    for (const auto &replica : functionalReplicas_) {
        groupInfo.functionalReplicas.push_back(replica.svcUuid); 
    }
    for (const auto &replica : nonfunctionalReplicas_) {
        groupInfo.nonfunctionalReplicas.push_back(replica.svcUuid); 
    }
    for (const auto &replica : syncingReplicas_) {
        groupInfo.nonfunctionalReplicas.push_back(replica.svcUuid); 
    }
    return groupInfo;
}

void VolumeGroupHandle::setGroupInfo_(const fpi::VolumeGroupInfo &groupInfo)
{
    groupId_ = groupInfo.groupId;
    version_ = groupInfo.version;
    opSeqNo_ = groupInfo.lastOpId;
    commitNo_ = groupInfo.lastCommitId;
    functionalReplicas_.clear();
    for (const auto &svcUuId : groupInfo.functionalReplicas) {
        functionalReplicas_.push_back(VolumeReplicaHandle(svcUuId));
    }
    nonfunctionalReplicas_.clear();
    for (const auto &svcUuId : groupInfo.nonfunctionalReplicas) {
        nonfunctionalReplicas_.push_back(VolumeReplicaHandle(svcUuId));
    }
}

void VolumeGroupHandle::setVolumeIoHdr_(fpi::VolumeIoHdr &hdr)
{
    hdr.version = version_;
    hdr.groupId = groupId_;
    hdr.opId =  opSeqNo_;
    hdr.commitId = commitNo_;
}

VolumeGroupHandle::VolumeReplicaHandleItr
VolumeGroupHandle::getVolumeReplicaHandle_(const fpi::SvcUuid &svcUuid)
{
#define CHECKIN(__list) \
    for (auto itr = __list.begin(); itr != __list.end(); itr++) { \
        if (itr->svcUuid == svcUuid) { \
            return itr; \
        } \
    }
    
    CHECKIN(functionalReplicas_);
    CHECKIN(nonfunctionalReplicas_);
    CHECKIN(syncingReplicas_);
    return INVALID_REAPLICA_HANDLE();
}

#if 0
void VolumeGroupHandle::sendVolumeBroadcastRequest_(const fpi::FDSPMsgTypeId &msgTypeId,
                                                      const StringPtr &payload,
                                                      const VolumeResponseCb &cb)
{
    auto req = requestMgr_->newSvcRequest<VolumeGroupBroadcastRequest>(this);
    req->setPayloadBuf(msgTypeId, payload);
    req->responseCb_ = cb;
    req->onEPAppStatusCb(&VolumeGroupHandle::handleVolumeResponse, this, );
    req->setTaskExecutorId(groupId_);
    req->invoke();
}
#endif

VolumeGroupRequest::VolumeGroupRequest(CommonModuleProviderIf* provider,
                                       const SvcRequestId &id,
                                       const fpi::SvcUuid &myEpId,
                                       VolumeGroupHandle *groupHandle)
: MultiEpSvcRequest(provider, id, myEpId, 0, {}),
    groupHandle_(groupHandle),
    nAcked_(0),
    nSuccessAcked_(0)
{
}

std::string VolumeGroupRequest::logString()
{
    std::stringstream ss;
#ifdef IOHEADER_SUPPORTED
    ss << volumeIoHdr_;
#endif
    return ss.str();
}

void VolumeGroupBroadcastRequest::invoke()
{
    invokeWork_();
}

void VolumeGroupBroadcastRequest::invokeWork_()
{
    auto replicas = groupHandle_->getIoReadyReplicaHandles();
    state_ = SvcRequestState::INVOCATION_PROGRESS;
    for (const auto &r : replicas) {
        addEndpoint(r->svcUuid,
                    groupHandle_->getDmtVersion(),
                    groupHandle_->getGroupId(),
                    r->version);
        auto &ep = epReqs_.back();
        ep->setPayloadBuf(msgTypeId_, payloadBuf_);
        ep->setTimeoutMs(timeoutMs_);
        ep->invokeWork_();
    }
}

void VolumeGroupBroadcastRequest::handleResponse(SHPTR<fpi::AsyncHdr>& header,
                                                  SHPTR<std::string>& payload)
{
    DBG(GLOGDEBUG << fds::logString(*header));
    auto epReq = getEpReq_(header->msg_src_uuid);
    if (isComplete()) {
        /* Drop completed requests */
        GLOGWARN << logString() << " Already completed";
        return;
    } else if (!epReq) {
        /* Drop responses from uknown endpoint src ids */
        GLOGWARN << fds::logString(*header) << " Unknown EpId";
        return;
    } else if (epReq->isComplete()) {
        GLOGWARN << fds::logString(*header) << " Already completed";
        return;
    }
    epReq->completeReq(header->msg_code, header, payload);

    Error outStatus = ERR_OK;
    groupHandle_->handleVolumeResponse(header->msg_src_uuid,
                                       header->replicaVersion,
                                       volumeIoHdr_,
                                       msgTypeId_,
                                       true,
                                       header->msg_code,
                                       outStatus,
                                       nSuccessAcked_);
    if (nSuccessAcked_ == groupHandle_->getQuorumCnt() &&
        responseCb_) {
        responseCb_(ERR_OK, payload); 
        responseCb_ = 0;
    }
    if (outStatus != ERR_OK) {
        GLOGWARN << logString() << " " << fds::logString(*header)
            << " outstatus: " << outStatus;
    }
    ++nAcked_;
    if (nAcked_ == epReqs_.size()) {
        if (responseCb_) {
            /* All replicas have failed..return error code from last replica */
            responseCb_(header->msg_code, payload);
            responseCb_ = 0;
            complete(header->msg_code);
        } else {
            /* Atleast one replica succeeded */
            complete(ERR_OK);
        }
    }
}

void VolumeGroupFailoverRequest::invoke()
{
    invokeWork_();
}

void VolumeGroupFailoverRequest::invokeWork_()
{
    auto replica = groupHandle_->getFunctionalReplicaHandle();
    addEndpoint(replica->svcUuid,
                groupHandle_->getDmtVersion(),
                groupHandle_->getGroupId(),
                replica->version);
    auto &ep = epReqs_.back();
    ep->setPayloadBuf(msgTypeId_, payloadBuf_);
    ep->setTimeoutMs(timeoutMs_);
    ep->invokeWork_();
}

void VolumeGroupFailoverRequest::handleResponse(SHPTR<fpi::AsyncHdr>& header,
                                                  SHPTR<std::string>& payload)
{
    DBG(GLOGDEBUG << fds::logString(*header));
    auto epReq = getEpReq_(header->msg_src_uuid);
    if (isComplete()) {
        /* Drop completed requests */
        GLOGWARN << logString() << " Already completed";
        return;
    } else if (!epReq) {
        /* Drop responses from uknown endpoint src ids */
        GLOGWARN << fds::logString(*header) << " Unknown EpId";
        return;
    } else if (epReq->isComplete()) {
        GLOGWARN << fds::logString(*header) << " Already completed";
        return;
    }
    epReq->completeReq(header->msg_code, header, payload);

    ++nAcked_;
    fds_assert(nAcked_ <= groupHandle_->size());

    Error outStatus = ERR_OK;
    groupHandle_->handleVolumeResponse(header->msg_src_uuid,
                                       header->replicaVersion,
                                       volumeIoHdr_,
                                       msgTypeId_,
                                       false,
                                       header->msg_code,
                                       outStatus,
                                       nSuccessAcked_);
    if (nSuccessAcked_ == 1) {
        responseCb_(ERR_OK, payload); 
        responseCb_ = 0;
        /* Atleast one replica succeeded */
        complete(ERR_OK);
    } else {
        GLOGWARN << logString() << " " << fds::logString(*header)
            << " outstatus: " << outStatus;
        if (groupHandle_->isFunctional()) {
            /* Try against another replica */
            invokeWork_();
        } else {
            complete(ERR_SVC_REQUEST_FAILED);
        }
    }
}

}  // namespace fds
