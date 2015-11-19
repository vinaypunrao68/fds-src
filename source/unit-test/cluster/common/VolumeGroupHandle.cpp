/* Copyright 2015 Formation Data Systems, Inc.
 */
#include "VolumeGroupHandle.h"
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
    out << " version: " << h.version
        << " groupId: " << h.groupId 
        << " svcUuid: " << h.svcUuid.svc_uuid
        << " opId: " << h.opId
        << " commitId: " << h.commitId;
    return out;
}

std::ostream& operator << (std::ostream &out, const VolumeReplicaHandle &h)
{
    out << " svcUuid: " << h.svcUuid.svc_uuid
        << " state: " << h.state
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
    setGroupInfo_(groupInfo);
    // TODO(Rao): Go through protocol figure out the states of the replicas
    // For now set every handle as functional and start their op/commit ids
    // from beginning
    for (auto &r : functionalReplicas_) {
        r.setInfo(fpi::VolumeState::VOLUME_FUNCTIONAL,
                   VolumeGroupConstants::OPSTARTID,
                   VolumeGroupConstants::COMMITSTARTID);
    }
    // TODO(Rao): Set this to be the latest ids after querying the replicas
    opSeqNo_ = VolumeGroupConstants::OPSTARTID;
    commitNo_ = VolumeGroupConstants::COMMITSTARTID;
}

void VolumeGroupHandle::handleAddToVolumeGroupMsg(
    const fpi::AddToVolumeGroupCtrlMsgPtr &addMsg,
    const std::function<void(const Error&, const fpi::AddToVolumeGroupRespCtrlMsgPtr&)> &cb)

{
    runSynchronized([this, addMsg, cb]() mutable {
        auto err = changeVolumeReplicaState_(*addMsg);
        auto respMsg = MAKE_SHARED<fpi::AddToVolumeGroupRespCtrlMsg>();
        respMsg->group = getGroupInfoForExternalUse_();
        cb(err, respMsg);
    });
}

void VolumeGroupHandle::handleVolumeResponse(const fpi::SvcUuid &srcSvcUuid,
                                             const fpi::VolumeIoHdr &hdr,
                                             const Error &inStatus,
                                             Error &outStatus,
                                             uint8_t &successAcks)
{
    ASSERT_SYNCHRONIZED();

    outStatus = inStatus;

    GLOGNOTIFY << "svcuuid: " << SvcMgr::mapToSvcUuidAndName(srcSvcUuid)
        << fds::logString(hdr) << inStatus;

    auto volumeHandle = getVolumeReplicaHandle_(srcSvcUuid);
    if (volumeHandle->isFunctional() ||
        volumeHandle->isSyncing()) {
        if (inStatus == ERR_OK) {
            fds_verify(volumeHandle->appliedOpId+1 == hdr.opId);
            fds_verify(volumeHandle->appliedCommitId == hdr.commitId ||
                       volumeHandle->appliedCommitId+1 == hdr.commitId);
            volumeHandle->appliedOpId = hdr.opId;
            volumeHandle->appliedCommitId = hdr.commitId;
            if (volumeHandle->isFunctional()) {
                successAcks++;
            }
        } else {
            volumeHandle->state = fpi::VolumeState::VOLUME_DOWN;
            volumeHandle->lastError = inStatus;
            // GLOGWARN << hdr << " Volume marked down: " << *volumeHandle;
            GLOGWARN << " Replica marked down: " << *volumeHandle;
            // TODO(Rao): Change group state
            // changeGroupState();
        }
    } else {
        /* When replica isn't functional we don't expect subsequent IO to return with
         * success status
         */
        fds_verify(inStatus != ERR_OK);
    }
}

std::vector<fpi::SvcUuid> VolumeGroupHandle::getIoReadySvcUuids() const
{
    std::vector<fpi::SvcUuid> svcUuids;   
    for (const auto &r : functionalReplicas_) {
        svcUuids.push_back(r.svcUuid);
    }
    for (const auto &r : syncingReplicas_) {
        svcUuids.push_back(r.svcUuid);
    }
    return svcUuids;
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

Error VolumeGroupHandle::changeVolumeReplicaState_(
    const fpi::AddToVolumeGroupCtrlMsg& update)
{
    GET_VOLUMEREPLICA_HANDLE_RETURN(srcItr, update.svcUuid, ERR_INVALID);
    LOGNORMAL << " Incoming update: " << fds::logString(update)
        << " handler: " << *srcItr;

    auto srcState = srcItr->state;

    fds_verify(VolumeReplicaHandle::isSyncing(update.targetState));

    if (VolumeReplicaHandle::isSyncing(update.targetState)) {
        srcItr->setInfo(update.targetState, opSeqNo_, commitNo_);
    }

    /* Move the handle from the appropriate replica list */
    auto &srcList = getVolumeReplicaHandleList_(srcState);
    auto &dstList = getVolumeReplicaHandleList_(update.targetState);
    auto dstItr = std::move(srcItr, srcItr+1, std::back_inserter(dstList));
    srcList.erase(srcItr, srcItr+1);

    // LOGNORMAL << " After update handler: " << dstList.back();
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
: MultiEpSvcRequest(provider, id, myEpId, 0, groupHandle->getIoReadySvcUuids()),
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
    state_ = SvcRequestState::INVOCATION_PROGRESS;
    for (auto &ep : epReqs_) {
        ep->setPayloadBuf(msgTypeId_, payloadBuf_);
        ep->setTimeoutMs(timeoutMs_);
        ep.get()->invokeWork_();
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
    epReq->complete(header->msg_code, header, payload);

    Error outStatus = ERR_OK;
    groupHandle_->handleVolumeResponse(header->msg_src_uuid,
                                       volumeIoHdr_,
                                       header->msg_code,
                                       outStatus,
                                       nSuccessAcked_);
    if (nSuccessAcked_ > 0 && responseCb_) {
        responseCb_(ERR_OK, payload); 
        responseCb_ = 0;
    }
    if (outStatus != ERR_OK) {
        GLOGWARN << "Volume response: " << fds::logString(*header)
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

}  // namespace fds
