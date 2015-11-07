/* Copyright 2015 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_REPLICA_REPLICA_H_
#define SOURCE_INCLUDE_REPLICA_REPLICA_H_

#include <net/SvcMgr.h>
#include <net/SvcRequest.h>
#include <fdsp/replica_types.h>

namespace fds {

struct ReplicaGroupHandle;

struct ReplicaConstants {
    static const int64_t            OPSTARTID = 0;
    static const int64_t            COMMITSTARTID = 0;
};
std::ostream& operator << (std::ostream &out, const fpi::ReplicaIoHdr &h);

struct ReplicaHandle {
    ReplicaHandle()
        : state(fpi::ReplicaState::REPLICA_UNINIT),
        lastError(ERR_OK),
        appliedOpId(ReplicaConstants::OPSTARTID),
        appliedCommitId(ReplicaConstants::COMMITSTARTID)
    {
        svcUuid.svc_uuid = 0;
    }
    inline bool isFunctional() const {
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
std::ostream& operator << (std::ostream &out, const ReplicaHandle &h);

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
    virtual std::string logString() override;
 protected:
    virtual void invokeWork_() override {
        fds_panic("Shouldn't come here");
    }

    ReplicaGroupHandle          *groupHandle_; 
    fpi::ReplicaIoHdr           replicaHdr_; 
    ReplicaResponseCb           responseCb_;
    uint8_t                     nAcked_;
    uint8_t                     nSuccessAcked_;
    
    friend class ReplicaGroupHandle;
};


struct ReplicaGroupBroadcastRequest : ReplicaGroupRequest {
    using ReplicaRequestStatePair = std::pair<fpi::SvcUuid, SvcRequestState>;    

    /* Constructors inherited */
    using ReplicaGroupRequest::ReplicaGroupRequest;
    virtual void invoke() override;
    virtual void handleResponse(SHPTR<fpi::AsyncHdr>& header,
                                SHPTR<std::string>& payload);

 protected:
    ReplicaRequestStatePair* getReplicaRequestStatePair_(const fpi::SvcUuid &svcUuid);

    std::vector<ReplicaRequestStatePair> reqStates_;
};

struct ReplicaGroupHandle : HasModuleProvider {
    explicit ReplicaGroupHandle(CommonModuleProviderIf* provider);

    template<class F>
    void runSynchronized(F&& f)
    {
        taskExecutor_->schedule(std::forward<F>(f));
    }

    template<class MsgT>
    void sendModifyMsg(const fpi::FDSPMsgTypeId &msgTypeId,
                       SHPTR<MsgT> &msg, const ReplicaResponseCb &cb) {
#if 0
        runSynchronized([msgTypeId, msg, cb]() {
            opSeqNo_++;
            updateReplicaIoHdr(getReplicaIoHdrRef(*msg));
            sendReplicaBroadcastRequest_(msgTypeId, serializeFdspMsg(msg), cb);
        });
#endif
    }
    virtual void handleReplicaResponse(const fpi::ReplicaIoHdr &hdr,
                                       const Error &inStatus,
                                       Error &outStatus);
 protected:
    ReplicaHandle* getReplicaHandle_(const fpi::ReplicaIoHdr &hdr);
    void sendReplicaBroadcastRequest_(const fpi::FDSPMsgTypeId &msgTypeId,
                                      const StringPtr &payload,
                                      const ReplicaResponseCb &cb);

    SynchronizedTaskExecutor<uint64_t>  *taskExecutor_;
    SvcRequestPool                      *requestMgr_;
    std::vector<ReplicaHandle>          functionalReplicas_;
    std::vector<ReplicaHandle>          nonfunctionalReplicas_;
    fpi::ReplicaGroupVersion            version_;
    int64_t                             opSeqNo_;
    int64_t                             commitNo_;

    friend class ReplicaGroupBroadcastRequest;
    
};


}  // namespace fds

#endif
