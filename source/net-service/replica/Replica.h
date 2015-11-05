/* Copyright 2015 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_REPLICA_REPLICA_H_
#define SOURCE_INCLUDE_REPLICA_REPLICA_H_

#include <net/SvcMgr.h>
#include <net/SvcRequest.h>

namespace fds {

struct ReplicaConstants {
    static const int64_t            OPSTARTID = 0;
    static const int64_t            COMMITSTARTID = 0;
};

std::ostream& operator << (std::ostream &out, const fpi::ReplicaIoHdr &h)
{
    out << " version: " << h.version
        << " groupId: " << h.groupId 
        << " svcUuid: " << h.svcUuid
        << " opId: " << h.opId
        << " commitId: " << h.commitId;
    return out;
}

struct ReplicaHandle {
    ReplicaHandle()
        : svcUuid(0),
        state(REPLICA_UNINIT),
        lastError(ERR_OK),
        appliedOpId(ReplicaConstants::OPSTARTID),
        appliedCommitId(ReplicaConstants::COMMITSTARTID)
    {
    }
    bool isFunctional() const {
        return state == fpi::ReplicaState::REPLICA_FUNCTIONAL;
    }
    fpi::SvcUuid            svcUuid;
    int                     state;
    Error                   lastError;
    /* Last succesfully applied operation id */
    int64_t                 appliedOpId;
    /* Last succesfully applied commit id */
    int64_t                 appliedCommitId;
};

std::ostream& operator << (std::ostream &out, const ReplicaHandle &h)
{
    out << " svcUuid: " << svcUuid.svc_uuid
        << " state: " << state
        << " lasterr: " << lastError
        << " appliedOp: " << appliedOpId
        << " appliedCommit: " << appliedCommitId;
    return out;
}

using ReplicaResponseCb = std::function<void(const Error&, StringPtr)>;

/**
* @brief Payload is sent to all replicas.
*/
struct ReplicaGroupRequest : SvcRequestIf {
    ReplicaGroupRequest(CommonModuleProviderIf* provider,
                        const SvcRequestId &id,
                        const fpi::SvcUuid &myEpId,
                        ReplicaGroupHandle *groupHandle)
        : SvcRequestIf(provider, id, myEpId),
        groupHandle_(groupHandle),
        nAcked_(0),
        nSuccessAcked_(0)
    {}
 protected:
    ReplicaGroupHandle          groupHandle_; 
    fpi::ReplicaIoHdr           replicaHdr_; 
    ReplicaResponseCb           responseCb_;
    uint8_t                     nAcked_;
    uint8_t                     nSuccessAcked_;
    
    friend class ReplicaGroupHandle;
};

struct ReplicaGroupBroadcastRequest : ReplicaGroupRequest {
    using ReplicaGroupRequest::ReplicaGroupRequest;

    virtual void invoke() override
    {
        state_ = SvcRequestState::INVOCATION_PROGRESS;
        for (const auto &handle : groupHandle_->functionalReplicas_) {
            reqStates_.push_back(std::make_pair(handle.svcUuid,
                                                SvcRequestState::INVOCATION_PROGRESS));
            sendPayload_(handle.svcUuid);
        }
    }

    virtual void handleResponse(boost::shared_ptr<fpi::AsyncHdr>& header,
            boost::shared_ptr<std::string>& payload)
    {
        DBG(GLOGDEBUG << fds::logString(*header));
        auto replicaStatePair = getReplicaRequestStatePair(header->msg_src_uuid);
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
        groupHandle_->handleReplicaResponse(replicaHdr, header->msg_code, outStatus);
        if (outStatus == ERR_OK) {
            ++nSuccessAcked_;
            responseCb_(ERR_OK, payload); 
            responseCb_ = 0;
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

 protected:
    ReplicaRequestStatePair* getReplicaRequestStatePair_(const fpi::SvcUuid &svcUuid) {
        for (const auto &s : reqStates_) {
           if (s.first == svcUuid) {
            return &s;
           }
        }
        return nullptr;
    }
    using ReplicaRequestStatePair = std::pair<fpi::SvcUuid, SvcRequestState>;    
    std::vector<ReplicaRequestStatePair> reqStates_;
};

#if 0
struct ReplicaGroupFailoverRequest : SvcRequestIf {
    explicit ReplicaGroupFailoverRequest(ReplicaGroupHandle *groupHandle);
    virtual void invoke() override;
    virtual void handleResponse(SHPTR<fpi::AsyncHdr>& header,
                                SHPTR<std::string>& payload);
 protected:
    fpi::ReplicaIoHdr           replicaHdr; 
    uint8_t                     nAcked;
    uint8_t                     nSuccessAcked;

    friend class ReplicaGroupHandle;
};
#endif
struct ReplicaGroupHandle : HasModuleProvider {
    explicit ReplicaGroupHandle(CommonModuleProviderIf* provider);

    template<MsgT>
    void sendModifyMsg(const fpi::FDSPMsgTypeId &msgTypeId,
                       SHPTR<MsgT> &msg, const ReplicaResponseCb &cb) {
        runSynchronized([msgTypeId, msg, cb]() {
            opSeqNo_++;
            updateReplicaIoHdr(getReplicaIoHdrRef(*msg));
            sendReplicaBroadcastRequest_(msgTypeId, serializeFdspMsg(msg), cb);
        });
    }
    virtual void handleReplicaResponse(const fpi::ReplicaIoHdr &hdr,
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
                GLOGWARN << hdr << " Replica marked down: " << *replicaHandle;
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

 protected:
    ReplicaHandle* getReplicaHandle_(const fpi::ReplicaIoHdr &hdr) const {
        for (const auto &h : functionalReplicas_) {
            if (h.svcUuid == hdr.svcUuid) {
                return &h;
            }
        }
        for (const auto &h : nonfunctionalReplicas_) {
            if (h.svcUuid == hdr.svcUuid) {
                return &h;
            }
        }
        return nullptr;
    }
    void sendReplicaBroadcastRequest_(const fpi::FDSPMsgTypeId &msgTypeId,
                                      const StringPtr &payload, const ReplicaResponseCb &cb)
    {
        auto req = requestMgr_->newSvcRequest<ReplicaGroupBroadcastRequest>(this);
        req->setPayloadBuf(msgTypeId, payload);
        req->responseCb = cb;
        req->invoke();
    }

    SynchronizedTaskExecutor<uint64_t>* taskExecutor_;
    std::vector<ReplicHandle>           functionalReplicas_;
    std::vector<ReplicaHandle>          failedReplicas_;
    fpi::ReplicaVersion                 version_;
    int64_t                             opSeqNo_;
    int64_t                             commitNo_;

    friend class ReplicaGroupBroadcastRequest;
    
#if 0
    virtual void handleReplicaResponse(ReplicaGroupFailoverRequest* req,
                                       SHPTR<fpi::AsyncHdr>& header,
                                       SHPTR<std::string>& payload);
    virtual void handleCommitResp(SHPTR<fpi::AsyncHdr>& header,
                                  SHPTR<std::string>& payload);
    virtual void handleReadResp(SHPTR<fpi::AsyncHdr>& header,
                                SHPTR<std::string>& payload);
    virtual void handleSnapResp(SHPTR<fpi::AsyncHdr>& header,
                                SHPTR<std::string>& payload);
    template<MsgT>
    void sendCommitMsg();

    template<MsgT>
    void sendReadMsg();

    template<MsgT>
    void sendSnapMsg();
#endif
};

ReplicaGroupHandle::ReplicaGroupHandle(CommonModuleProviderIf* provider)
: HasModuleProvider(provider)
{
    taskExecutor = MODULEPROVIDER()->getSvcMgr()->getTaskExecutor();
}
#if 0
void ReplicaGroupHandle::handleCommitResp(SHPTR<fpi::AsyncHdr>& header,
                                          SHPTR<std::string>& payload)
{
}
void ReplicaGroupHandle::handleReadResp(SHPTR<fpi::AsyncHdr>& header,
                                        SHPTR<std::string>& payload)
{
}
void ReplicaGroupHandle::handleSnapResp(SHPTR<fpi::AsyncHdr>& header,
                                        SHPTR<std::string>& payload)
{
}
#endif


template<class TrasactionTblT, class JournalLogT, class DataStoreT>
struct Replica : HasModuleProvider {
    template<MsgT>
    void applyUpdateMsg(SHPTR<MsgT> msg);

#if 0
    template<MsgT>
    void applyCommitMsg();

    template<MsgT>
    void applyReadMsg();

    template<MsgT>
    void applySnapMsg();
#endif

 protected:
    std::shared_ptr<TrasactionTblT>         transactions_;
    std::shared_ptr<JournalLogT>            journal_;
    std::shared_ptr<DataStoreT>             dataStore_;
};

}  // namespace fds

#endif
