/* Copyright 2015 Formation Data Systems, Inc.
 */
#include "VolumeGroupHandle.h"
#include <net/SvcRequestPool.h>

namespace fds {
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

void VolumeGroupHandle::handleVolumeResponse(const fpi::SvcUuid &srcSvcUuid,
                                             const fpi::VolumeIoHdr &hdr,
                                             const Error &inStatus,
                                             Error &outStatus)
{
    outStatus = inStatus;

    GLOGNOTIFY << "svcuuid: " << SvcMgr::mapToSvcUuidAndName(srcSvcUuid)
        << fds::logString(hdr) << inStatus;

    auto volumeHandle = getVolumeReplicaHandle_(srcSvcUuid);
    if (volumeHandle->isFunctional()) {
        if (inStatus == ERR_OK) {
            fds_verify(volumeHandle->appliedOpId+1 == hdr.opId);
            fds_verify(volumeHandle->appliedCommitId == hdr.commitId ||
                       volumeHandle->appliedCommitId+1 == hdr.commitId);
            volumeHandle->appliedOpId = hdr.opId;
            volumeHandle->appliedCommitId = hdr.commitId;
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

std::vector<fpi::SvcUuid> VolumeGroupHandle::getFunctionReplicaSvcUuids() const
{
    std::vector<fpi::SvcUuid> svcUuids;   
    for (const auto &r : functionalReplicas_) {
        svcUuids.push_back(r.svcUuid);
    }
    return svcUuids;
}

void VolumeGroupHandle::setGroupInfo_(const fpi::VolumeGroupInfo &groupInfo)
{
    groupId_ = groupInfo.groupId;
    version_ = groupInfo.version;
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

VolumeReplicaHandle* VolumeGroupHandle::getVolumeReplicaHandle_(const fpi::SvcUuid &svcUuid)
{
    for (auto &h : functionalReplicas_) {
        if (h.svcUuid == svcUuid) {
            return &h;
        }
    }
    for (auto &h : nonfunctionalReplicas_) {
        if (h.svcUuid == svcUuid) {
            return &h;
        }
    }
    return nullptr;
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
: MultiEpSvcRequest(provider, id, myEpId, 0, groupHandle->getFunctionReplicaSvcUuids()),
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
                                       outStatus);
    if (outStatus == ERR_OK) {
        ++nSuccessAcked_;
        if (responseCb_) {
            responseCb_(ERR_OK, payload); 
            responseCb_ = 0;
        }
    } else {
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
