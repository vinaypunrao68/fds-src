/* Copyright 2015 Formation Data Systems, Inc.
 */
#include <net/Replica.h>
#include <net/SvcRequestPool.h>

namespace fds {

std::ostream& operator << (std::ostream &out, const fpi::ReplicaIoHdr &h)
{
    out << " version: " << h.version
        << " groupId: " << h.groupId 
        << " svcUuid: " << h.svcUuid.svc_uuid
        << " opId: " << h.opId
        << " commitId: " << h.commitId;
    return out;
}

std::ostream& operator << (std::ostream &out, const ReplicaHandle &h)
{
    out << " svcUuid: " << h.svcUuid.svc_uuid
        << " state: " << h.state
        << " lasterr: " << h.lastError
        << " appliedOp: " << h.appliedOpId
        << " appliedCommit: " << h.appliedCommitId;
    return out;
}
std::string ReplicaGroupRequest::logString()
{
    std::stringstream ss;
    ss << replicaHdr_;
    return ss.str();
}

void ReplicaGroupBroadcastRequest::invoke()
{
    state_ = SvcRequestState::INVOCATION_PROGRESS;
    for (const auto &handle : groupHandle_->functionalReplicas_) {
        reqStates_.push_back(std::make_pair(handle.svcUuid,
                                            SvcRequestState::INVOCATION_PROGRESS));
        sendPayload_(handle.svcUuid);
    }
}

void ReplicaGroupBroadcastRequest::handleResponse(SHPTR<fpi::AsyncHdr>& header,
                                                  SHPTR<std::string>& payload)
{
    DBG(GLOGDEBUG << fds::logString(*header));
    auto replicaStatePair = getReplicaRequestStatePair_(header->msg_src_uuid);
    if (isComplete()) {
        /* Drop completed requests */
        GLOGWARN << logString() << " Already completed";
        return;
    } else if (!replicaStatePair) {
        /* Drop responses from uknown endpoint src ids */
        GLOGWARN << fds::logString(*header) << " Unknown EpId";
        return;
    } else if (replicaStatePair->second == SvcRequestState::SVC_REQUEST_COMPLETE) {
        GLOGWARN << fds::logString(*header) << " Already completed";
        return;
    }
    replicaStatePair->second = SvcRequestState::SVC_REQUEST_COMPLETE;

    Error outStatus = ERR_OK;
    groupHandle_->handleReplicaResponse(replicaHdr_, header->msg_code, outStatus);
    if (outStatus == ERR_OK) {
        ++nSuccessAcked_;
        responseCb_(ERR_OK, payload); 
        responseCb_ = 0;
    } else {
        GLOGWARN << "Replica response: " << fds::logString(*header)
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

ReplicaGroupBroadcastRequest::ReplicaRequestStatePair* 
ReplicaGroupBroadcastRequest::getReplicaRequestStatePair_(const fpi::SvcUuid &svcUuid)
{
    for (auto &s : reqStates_) {
        if (s.first == svcUuid) {
            return &s;
        }
    }
    return nullptr;
}

ReplicaGroupHandle::ReplicaGroupHandle(CommonModuleProviderIf* provider)
: HasModuleProvider(provider)
{
    taskExecutor_ = MODULEPROVIDER()->getSvcMgr()->getTaskExecutor();
    requestMgr_ = MODULEPROVIDER()->getSvcMgr()->getSvcRequestMgr();
}

void ReplicaGroupHandle::handleReplicaResponse(const fpi::ReplicaIoHdr &hdr,
                                               const Error &inStatus,
                                               Error &outStatus)
{
    outStatus = inStatus;

    auto replicaHandle = getReplicaHandle_(hdr);
    fds_verify(replicaHandle->appliedOpId+1 == hdr.opId);
    fds_verify(replicaHandle->appliedCommitId+1 == hdr.commitId);
    if (replicaHandle->isFunctional()) {
        if (inStatus == ERR_OK) {
            replicaHandle->appliedOpId = hdr.opId;
            replicaHandle->appliedCommitId = hdr.commitId;
        } else {
            replicaHandle->state = fpi::ReplicaState::REPLICA_DOWN;
            replicaHandle->lastError = inStatus;
            // GLOGWARN << hdr << " Replica marked down: " << *replicaHandle;
            GLOGWARN << " Replica marked down: " << *replicaHandle;
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

ReplicaHandle* ReplicaGroupHandle::getReplicaHandle_(const fpi::ReplicaIoHdr &hdr)
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
void ReplicaGroupHandle::sendReplicaBroadcastRequest_(const fpi::FDSPMsgTypeId &msgTypeId,
                                                      const StringPtr &payload,
                                                      const ReplicaResponseCb &cb)
{
    auto req = requestMgr_->newSvcRequest<ReplicaGroupBroadcastRequest>(this);
    req->setPayloadBuf(msgTypeId, payload);
    req->responseCb_ = cb;
    req->invoke();
}
}  // namespace fds
