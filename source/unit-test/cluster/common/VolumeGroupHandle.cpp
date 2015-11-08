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

VolumeGroupRequest::VolumeGroupRequest(CommonModuleProviderIf* provider,
                                         const SvcRequestId &id,
                                         const fpi::SvcUuid &myEpId,
                                         VolumeGroupHandle *groupHandle)
: SvcRequestIf(provider, id, myEpId),
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
    state_ = SvcRequestState::INVOCATION_PROGRESS;
    for (const auto &handle : groupHandle_->functionalReplicas_) {
        reqStates_.push_back(std::make_pair(handle.svcUuid,
                                            SvcRequestState::INVOCATION_PROGRESS));
        sendPayload_(handle.svcUuid);
    }
}

void VolumeGroupBroadcastRequest::handleResponse(SHPTR<fpi::AsyncHdr>& header,
                                                  SHPTR<std::string>& payload)
{
    DBG(GLOGDEBUG << fds::logString(*header));
    auto volumeStatePair = getVolumeRequestStatePair_(header->msg_src_uuid);
    if (isComplete()) {
        /* Drop completed requests */
        GLOGWARN << logString() << " Already completed";
        return;
    } else if (!volumeStatePair) {
        /* Drop responses from uknown endpoint src ids */
        GLOGWARN << fds::logString(*header) << " Unknown EpId";
        return;
    } else if (volumeStatePair->second == SvcRequestState::SVC_REQUEST_COMPLETE) {
        GLOGWARN << fds::logString(*header) << " Already completed";
        return;
    }
    volumeStatePair->second = SvcRequestState::SVC_REQUEST_COMPLETE;

    Error outStatus = ERR_OK;
    groupHandle_->handleVolumeResponse(volumeIoHdr_, header->msg_code, outStatus);
    if (outStatus == ERR_OK) {
        ++nSuccessAcked_;
        responseCb_(ERR_OK, payload); 
        responseCb_ = 0;
    } else {
        GLOGWARN << "Volume response: " << fds::logString(*header)
            << " outstatus: " << outStatus;
    }
    ++nAcked_;
    if (nAcked_ == reqStates_.size()) {
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

VolumeGroupBroadcastRequest::VolumeRequestStatePair* 
VolumeGroupBroadcastRequest::getVolumeRequestStatePair_(const fpi::SvcUuid &svcUuid)
{
    for (auto &s : reqStates_) {
        if (s.first == svcUuid) {
            return &s;
        }
    }
    return nullptr;
}

VolumeGroupHandle::VolumeGroupHandle(CommonModuleProviderIf* provider)
: HasModuleProvider(provider)
{
    taskExecutor_ = MODULEPROVIDER()->getSvcMgr()->getTaskExecutor();
    requestMgr_ = MODULEPROVIDER()->getSvcMgr()->getSvcRequestMgr();
}

void VolumeGroupHandle::handleVolumeResponse(const fpi::VolumeIoHdr &hdr,
                                               const Error &inStatus,
                                               Error &outStatus)
{
    outStatus = inStatus;

    auto volumeHandle = getVolumeReplicaHandle_(hdr);
    fds_verify(volumeHandle->appliedOpId+1 == hdr.opId);
    fds_verify(volumeHandle->appliedCommitId+1 == hdr.commitId);
    if (volumeHandle->isFunctional()) {
        if (inStatus == ERR_OK) {
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

VolumeReplicaHandle* VolumeGroupHandle::getVolumeReplicaHandle_(const fpi::VolumeIoHdr &hdr)
{
    for (auto &h : functionalReplicas_) {
        if (h.svcUuid == hdr.svcUuid) {
            return &h;
        }
    }
    for (auto &h : nonfunctionalReplicas_) {
        if (h.svcUuid == hdr.svcUuid) {
            return &h;
        }
    }
    return nullptr;
}
void VolumeGroupHandle::sendVolumeBroadcastRequest_(const fpi::FDSPMsgTypeId &msgTypeId,
                                                      const StringPtr &payload,
                                                      const VolumeResponseCb &cb)
{
    auto req = requestMgr_->newSvcRequest<VolumeGroupBroadcastRequest>(this);
    req->setPayloadBuf(msgTypeId, payload);
    req->responseCb_ = cb;
    req->setTaskExecutorId(groupId_);
    req->invoke();
}
}  // namespace fds
