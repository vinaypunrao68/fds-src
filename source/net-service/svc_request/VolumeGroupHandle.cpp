/* Copyright 2015 Formation Data Systems, Inc.
 */
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

std::ostream& operator << (std::ostream &out, const VolumeReplicaHandle &h)
{
    out << "VolumeReplicaHandle" 
        << " svcUuid: " << SvcMgr::mapToSvcUuidAndName(h.svcUuid)
        << " version: " << h.version
        << " state: " << fpi::_VolumeState_VALUES_TO_NAMES.at(static_cast<int>(h.state))
        << " lasterr: " << h.lastError
        << " appliedOp: " << h.appliedOpId
        << " appliedCommit: " << h.appliedCommitId;
    return out;
}

VolumeGroupHandle::VolumeGroupHandle(CommonModuleProviderIf* provider,
                                     const fpi::VolumeGroupInfo &groupInfo)
: HasModuleProvider(provider)
{
    taskExecutor_ = MODULEPROVIDER()->getSvcMgr()->getTaskExecutor();
    requestMgr_ = MODULEPROVIDER()->getSvcMgr()->getSvcRequestMgr();
    state_ = fpi::VolumeState::VOLUME_UNINIT;
    setGroupInfo_(groupInfo);
    // TODO(Rao): Go through protocol figure out the states of the replicas
    // For now set every handle as functional and start their op/commit ids
    // from beginning
    for (auto &r : functionalReplicas_) {
        r.setInfo(VolumeGroupConstants::VERSION_START,
                  fpi::VolumeState::VOLUME_FUNCTIONAL,
                  VolumeGroupConstants::OPSTARTID,
                  VolumeGroupConstants::COMMITSTARTID);
    }
    // TODO(Rao): Set this to be the latest ids after querying the replicas
    opSeqNo_ = VolumeGroupConstants::OPSTARTID;
    commitNo_ = VolumeGroupConstants::COMMITSTARTID;
}


void VolumeGroupHandle::open(const SHPTR<fpi::OpenVolumeMsg>& msg,
                             const StatusCb &clientCb)
{
#if  0
    runSynchronized([this, msg]() mutable {
        /* Send a message to OM requesting to be coordinator for the group */
        auto req = createSetVolumeGroupCoordinatorMsgReq_();
        auto respCb = [this, clientCb](const Error& e) {
           if (e != ERR_OK) {
            LOGWARN << "Failed set volume group coordinator.  Received " << e << " from om";
            clientCb(e);
            return;
           }
           auto openReq = createPreareOpenVolumeGroupMsgReq_();
           auto openCb = [this, clientCb](const Error *e) {
            if (e != ERR_OK) {
                LOGWARN << "Prepare for open failed: " << e;
                clientCb(e);
                return;
            }
            determineFunctaionalReplicas();
            if (functionalReplicas_.size() == 0) {
                LOGWARN << "Not enough replicas.  Group remains non-functional";
                clientCb(ERR_VOLUME_UNAVAILABLE);
                return;
            }
            auto commitReq = createCommitOpenVolumeGroupMsgReq_();
            commitReq->invoke();
           };
           openReq->invoke();
        };
        req->onResponseCb(respCb);
        req->invoke();
    });
#endif
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
        auto err = changeVolumeReplicaState_(volumeHandle,
                                             addMsg->replicaVersion,
                                             addMsg->targetState,
                                             ERR_OK);
        respMsg->group = getGroupInfoForExternalUse_();
        cb(err, respMsg);
    });
}

void VolumeGroupHandle::handleVolumeResponse(const fpi::SvcUuid &srcSvcUuid,
                                             const int32_t &replicaVersion,
                                             const fpi::VolumeIoHdr &hdr,
                                             const Error &inStatus,
                                             Error &outStatus,
                                             uint8_t &successAcks)
{
    ASSERT_SYNCHRONIZED();

    outStatus = inStatus;

    LOGDEBUG << "svcuuid: " << SvcMgr::mapToSvcUuidAndName(srcSvcUuid)
        << fds::logString(hdr) << inStatus;

    auto volumeHandle = getVolumeReplicaHandle_(srcSvcUuid);
    if (replicaVersion != volumeHandle->version) {
        LOGWARN << "Ignoring response.  Version check failed. svcuuid: "
            << SvcMgr::mapToSvcUuidAndName(srcSvcUuid)
            << *volumeHandle
            << " incoming replica version:  " << replicaVersion
            << fds::logString(hdr) << inStatus;
        return;
    }
    if (volumeHandle->isFunctional() ||
        volumeHandle->isSyncing()) {
        if (inStatus == ERR_OK) {
#ifdef IOHEADER_SUPPORTED
            fds_verify(volumeHandle->appliedOpId+1 == hdr.opId);
            fds_verify(volumeHandle->appliedCommitId == hdr.commitId ||
                       volumeHandle->appliedCommitId+1 == hdr.commitId);
            volumeHandle->appliedOpId = hdr.opId;
            volumeHandle->appliedCommitId = hdr.commitId;
#endif
            if (volumeHandle->isFunctional()) {
                successAcks++;
            }
        } else {
            auto changeErr = changeVolumeReplicaState_(volumeHandle,
                                                       volumeHandle->version,
                                                       fpi::VolumeState::VOLUME_DOWN,
                                                       inStatus);
            fds_verify(changeErr == ERR_OK);
        }
    } else if (volumeHandle->isUnitialized()) {
        /* We get this when we are trying to open the volume */
        fds_verify(state_ == fpi::VolumeState::VOLUME_UNINIT);
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

VolumeGroupHandle::VolumeReplicaHandleList&
VolumeGroupHandle::getVolumeReplicaHandleList_(const fpi::VolumeState& s)
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
                                                   const fpi::VolumeState &targetState,
                                                   const Error &e)
{
    LOGNORMAL << " Target state: "
        << fpi::_VolumeState_VALUES_TO_NAMES.at(static_cast<int>(targetState))
        << " Prior update handle: " << *volumeHandle;

    auto srcState = volumeHandle->state;

    if (VolumeReplicaHandle::isSyncing(targetState)) {
        if (replicaVersion <= volumeHandle->version) {
            fds_assert(!"Invalid version");
            return ERR_INVALID_VOLUME_VERSION;
        }
        volumeHandle->setInfo(replicaVersion, targetState, opSeqNo_, commitNo_);
    } else if (VolumeReplicaHandle::isNonFunctional(targetState)) {
        volumeHandle->setState(targetState);
        volumeHandle->setError(e);
    } else if (VolumeReplicaHandle::isFunctional(targetState)){
        if (replicaVersion < volumeHandle->version) {
            fds_assert(!"Invalid version");
            return ERR_INVALID_VOLUME_VERSION;
        }
        volumeHandle->setVersion(replicaVersion);
        volumeHandle->setState(targetState);
        volumeHandle->setError(ERR_OK);
    }
    LOGNORMAL << "After update handle: " << *volumeHandle;

    /* Move the handle from the appropriate replica list */
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
    ss << volumeIoHdr_;
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
                                       header->msg_code,
                                       outStatus,
                                       nSuccessAcked_);
    if (nSuccessAcked_ > 0 && responseCb_) {
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
