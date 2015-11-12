/* Copyright 2015 Formation Data Systems, Inc.
 */
#ifndef SOURCE_VOLUMEGROUP_H_
#define SOURCE_VOLUMEGROUP_H_

#include <net/SvcMgr.h>
#include <net/SvcRequest.h>
#include <net/SvcRequestPool.h>
#include <fdsp/volumegroup_types.h>
#include <volumegroup_extensions.h>

namespace fds {
struct VolumeGroupHandle;

std::ostream& operator << (std::ostream &out, const fpi::VolumeIoHdr &h);

struct VolumeReplicaHandle {
    VolumeReplicaHandle(const fpi::SvcUuid &id)
        : svcUuid(id),
        state(fpi::VolumeState::VOLUME_UNINIT),
        lastError(ERR_OK),
        appliedOpId(VolumeGroupConstants::OPSTARTID),
        appliedCommitId(VolumeGroupConstants::COMMITSTARTID)
    {
    }
    inline bool isFunctional() const {
        return state == fpi::VolumeState::VOLUME_FUNCTIONAL;
    }
    void setInfo(const fpi::VolumeState &state, int64_t opId, int64_t commitId)
    {
        this->state = state;
        appliedOpId = opId;
        appliedCommitId = commitId;
    }

    fpi::SvcUuid            svcUuid;
    int                     state;
    Error                   lastError;
    /* Last succesfully applied operation id */
    int64_t                 appliedOpId;
    /* Last succesfully applied commit id */
    int64_t                 appliedCommitId;
};
std::ostream& operator << (std::ostream &out, const VolumeReplicaHandle &h);

using VolumeResponseCb = std::function<void(const Error&, StringPtr)>;
/**
* @brief Payload is sent to all replicas.
*/
struct VolumeGroupRequest : MultiEpSvcRequest {
    VolumeGroupRequest(CommonModuleProviderIf* provider,
                        const SvcRequestId &id,
                        const fpi::SvcUuid &myEpId,
                        VolumeGroupHandle *groupHandle);
    virtual std::string logString() override;

    VolumeGroupHandle          *groupHandle_; 
    fpi::VolumeIoHdr            volumeIoHdr_; 
    VolumeResponseCb            responseCb_;
    uint8_t                     nAcked_;
    uint8_t                     nSuccessAcked_;
    
    friend class VolumeGroupHandle;
};


struct VolumeGroupBroadcastRequest : VolumeGroupRequest {
    /* Constructors inherited */
    using VolumeGroupRequest::VolumeGroupRequest;
    virtual void invoke() override;
    virtual void handleResponse(SHPTR<fpi::AsyncHdr>& header,
                                SHPTR<std::string>& payload) override;
 protected:
    virtual void invokeWork_() override;
};

struct VolumeGroupHandle : HasModuleProvider {
    explicit VolumeGroupHandle(CommonModuleProviderIf* provider,
                               const fpi::VolumeGroupInfo &groupInfo);

    template<class F>
    void runSynchronized(F&& f)
    {
        // TODO(Rao): Use threadpool directly
        taskExecutor_->scheduleOnHashKey(groupId_, std::forward<F>(f));
    }

    template<class MsgT>
    void sendModifyMsg(const fpi::FDSPMsgTypeId &msgTypeId,
                       SHPTR<MsgT> &msg, const VolumeResponseCb &cb) {
        runSynchronized([this, msgTypeId, msg, cb]() mutable {
            opSeqNo_++;
            invokeCommon_<MsgT, VolumeGroupBroadcastRequest>(msgTypeId, msg, cb);
        });
    }
    template<class MsgT>
    void sendWriteMsg(const fpi::FDSPMsgTypeId &msgTypeId,
                       SHPTR<MsgT> &msg, const VolumeResponseCb &cb) {
        runSynchronized([this, msgTypeId, msg, cb]() mutable {
            opSeqNo_++;
            commitNo_++;
            invokeCommon_<MsgT, VolumeGroupBroadcastRequest>(msgTypeId, msg, cb);
        });
    }

    virtual void handleVolumeResponse(const fpi::SvcUuid &srcSvcUuid,
                                      const fpi::VolumeIoHdr &hdr,
                                      const Error &inStatus,
                                      Error &outStatus);
    std::vector<fpi::SvcUuid> getFunctionReplicaSvcUuids() const;

 protected:
    template<class MsgT, class ReqT>
    void invokeCommon_(const fpi::FDSPMsgTypeId &msgTypeId,
                       SHPTR<MsgT> &msg, const VolumeResponseCb &cb) {
        /* Update the header in the message */
        setVolumeIoHdr_(getVolumeIoHdrRef(*msg));

        /* Create a request and send */
        auto req = requestMgr_->newSvcRequest<ReqT>(this);
        req->setPayloadBuf(msgTypeId, serializeFdspMsg(*msg));
        req->volumeIoHdr_ = getVolumeIoHdrRef(*msg);
        req->responseCb_ = cb;
        req->setTaskExecutorId(groupId_);
        LOGNOTIFY << fds::logString(req->volumeIoHdr_);
        req->invoke();
    }
    void setGroupInfo_(const fpi::VolumeGroupInfo &groupInfo);
    void setVolumeIoHdr_(fpi::VolumeIoHdr &hdr);
    VolumeReplicaHandle* getVolumeReplicaHandle_(const fpi::SvcUuid &svcUuid);
#if 0
    void sendVolumeBroadcastRequest_(const fpi::FDSPMsgTypeId &msgTypeId,
                                      const StringPtr &payload,
                                      const VolumeResponseCb &cb);
#endif

    SynchronizedTaskExecutor<uint64_t>  *taskExecutor_;
    SvcRequestPool                      *requestMgr_;
    std::vector<VolumeReplicaHandle>    functionalReplicas_;
    std::vector<VolumeReplicaHandle>    nonfunctionalReplicas_;
    int64_t                             groupId_;
    fpi::VolumeGroupVersion             version_;
    int64_t                             opSeqNo_;
    int64_t                             commitNo_;
    // TDOO(Rao): Need the state of the replica group.  Based on the state accept/reject
    // io

    friend class VolumeGroupBroadcastRequest;
    
};


}  // namespace fds

#endif   // SOURCE_VOLUMEGROUP_H_
