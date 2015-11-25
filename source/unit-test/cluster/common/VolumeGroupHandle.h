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
    inline static bool isFunctional(const fpi::VolumeState& s)
    {
        return s == fpi::VolumeState::VOLUME_FUNCTIONAL;
    }
    inline static bool isSyncing(const fpi::VolumeState& s)
    {
        return s == fpi::VolumeState::VOLUME_QUICKSYNC_CHECK;
    }
    inline static bool isNonFunctional(const fpi::VolumeState& s)
    {
        return s == fpi::VolumeState::VOLUME_DOWN;
    }
    inline bool isFunctional() const {
        return isFunctional(state);
    }
    inline bool isSyncing() const {
        return isSyncing(state);
    }
    inline bool isNonFunctional() const {
        return isNonFunctional(state);
    }
    inline void setState(const fpi::VolumeState &state)
    {
        this->state = state;
    }
    inline void setError(const Error &e)
    {
        this->lastError = e;
    }
    inline void setInfo(const fpi::VolumeState &state, int64_t opId, int64_t commitId)
    {
        setState(state);
        appliedOpId = opId;
        appliedCommitId = commitId;
    }

    fpi::SvcUuid            svcUuid;
    fpi::VolumeState        state;
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
    using VolumeReplicaHandleList       = std::vector<VolumeReplicaHandle>;
    using VolumeReplicaHandleItr        = VolumeReplicaHandleList::iterator;

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

    virtual void handleAddToVolumeGroupMsg(
        const fpi::AddToVolumeGroupCtrlMsgPtr &addMsg,
        const std::function<void(const Error&, const fpi::AddToVolumeGroupRespCtrlMsgPtr&)> &cb);

    virtual void handleVolumeResponse(const fpi::SvcUuid &srcSvcUuid,
                                      const fpi::VolumeIoHdr &hdr,
                                      const Error &inStatus,
                                      Error &outStatus,
                                      uint8_t &successAcks);
    std::vector<fpi::SvcUuid> getIoReadySvcUuids() const;

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
        LOGDEBUG << fds::logString(req->volumeIoHdr_);
        req->invoke();
    }
    Error changeVolumeReplicaState_(VolumeReplicaHandleItr &volumeHandle,
                                    const fpi::VolumeState &targetState,
                                    const Error &e);
    void setGroupInfo_(const fpi::VolumeGroupInfo &groupInfo);
    fpi::VolumeGroupInfo getGroupInfoForExternalUse_();
    void setVolumeIoHdr_(fpi::VolumeIoHdr &hdr);
    VolumeReplicaHandleItr getVolumeReplicaHandle_(const fpi::SvcUuid &svcUuid);
    VolumeReplicaHandleList& getVolumeReplicaHandleList_(const fpi::VolumeState& s);

    SynchronizedTaskExecutor<uint64_t>  *taskExecutor_;
    SvcRequestPool                      *requestMgr_;
    VolumeReplicaHandleList             functionalReplicas_;
    VolumeReplicaHandleList             nonfunctionalReplicas_;
    VolumeReplicaHandleList             syncingReplicas_;
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
