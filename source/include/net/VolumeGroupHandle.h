/* Copyright 2015 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_NET_VOLUMEGROUPHANDLE_H_
#define SOURCE_INCLUDE_NET_VOLUMEGROUPHANDLE_H_

#include <net/SvcMgr.h>
#include <net/SvcRequest.h>
#include <net/SvcRequestPool.h>
#include <fdsp/volumegroup_types.h>
#include <fdsp/dm_api_types.h>
#include <net/volumegroup_extensions.h>

namespace fds {
struct VolumeGroupHandle;

std::ostream& operator << (std::ostream &out, const fpi::VolumeIoHdr &h);

struct VolumeReplicaHandle {
    explicit VolumeReplicaHandle(const fpi::SvcUuid &id)
        : svcUuid(id),
        state(fpi::VolumeState::VOLUME_UNINIT),
        lastError(ERR_OK),
        appliedOpId(VolumeGroupConstants::OPSTARTID),
        appliedCommitId(VolumeGroupConstants::COMMITSTARTID)
    {
    }
    inline static bool isUnitialized(const fpi::VolumeState& s)
    {
        return s == fpi::VolumeState::VOLUME_UNINIT;
    }
    inline static bool isFunctional(const fpi::VolumeState& s)
    {
        return s == fpi::VolumeState::VOLUME_FUNCTIONAL;
    }
    inline static bool isSyncing(const fpi::VolumeState& s)
    {
        return s == fpi::VolumeState::VOLUME_SYNCING;
    }
    inline static bool isNonFunctional(const fpi::VolumeState& s)
    {
        return s == fpi::VolumeState::VOLUME_DOWN;
    }
    inline bool isUnitialized() const {
        return isUnitialized(state);
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
    inline void setVersion(const int32_t &version)
    {
        this->version = version;
    }
    inline void setInfo(const int32_t &version,
                        const fpi::VolumeState &state,
                        int64_t opId, int64_t commitId)
    {
        setState(state);
        this->version = version;
        appliedOpId = opId;
        appliedCommitId = commitId;
    }

    int32_t                 version;
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
using StatusCb = std::function<void(const Error&)>;

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

struct VolumeGroupFailoverRequest : VolumeGroupRequest {
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

    VolumeGroupHandle(CommonModuleProviderIf* provider,
                      const fds_volid_t& volId);
    VolumeGroupHandle(CommonModuleProviderIf* provider,
                      const fpi::VolumeGroupInfo &groupInfo);

    void open(const SHPTR<fpi::OpenVolumeMsg>& msg, const StatusCb &cb);

    void close();

    template<class MsgT>
    void sendReadMsg(const fpi::FDSPMsgTypeId &msgTypeId,
                     SHPTR<MsgT> &msg, const VolumeResponseCb &cb);

    template<class MsgT>
    void sendWriteMsg(const fpi::FDSPMsgTypeId &msgTypeId,
                      SHPTR<MsgT> &msg, const VolumeResponseCb &cb);

    template<class MsgT>
    void sendModifyMsg(const fpi::FDSPMsgTypeId &msgTypeId,
                      SHPTR<MsgT> &msg, const VolumeResponseCb &cb);

    template<class F>
    void runSynchronized(F&& f)
    {
        // TODO(Rao): Use threadpool directly
        taskExecutor_->scheduleOnHashKey(groupId_, std::forward<F>(f));
    }


    virtual void handleAddToVolumeGroupMsg(
        const fpi::AddToVolumeGroupCtrlMsgPtr &addMsg,
        const std::function<void(const Error&, const fpi::AddToVolumeGroupRespCtrlMsgPtr&)> &cb);

    virtual void handleVolumeResponse(const fpi::SvcUuid &srcSvcUuid,
                                      const int32_t &replicaVersion,
                                      const fpi::VolumeIoHdr &hdr,
                                      const Error &inStatus,
                                      Error &outStatus,
                                      uint8_t &successAcks);
    std::vector<VolumeReplicaHandle*> getIoReadyReplicaHandles();
    VolumeReplicaHandle* getFunctionalReplicaHandle();

    inline bool isFunctional() const { return state_ == fpi::VolumeState::VOLUME_FUNCTIONAL; }
    inline int64_t getGroupId() const { return groupId_; }
    inline int32_t getDmtVersion() const { return DLT_VER_INVALID; }
    inline int32_t size() const {
        return functionalReplicas_.size() +
            nonfunctionalReplicas_.size() +
            syncingReplicas_.size();
    }

 protected:
    template<class MsgT, class ReqT>
    SHPTR<ReqT> invokeCommon_(const fpi::FDSPMsgTypeId &msgTypeId,
                              SHPTR<MsgT> &msg, const VolumeResponseCb &cb) {
        /* Update the header in the message */
        setVolumeIoHdr_(getVolumeIoHdrRef(*msg));

        /* Create a request and send */
        auto req = requestMgr_->newSvcRequest<ReqT>(this);
        req->setPayloadBuf(msgTypeId, serializeFdspMsg(*msg));
        req->volumeIoHdr_ = getVolumeIoHdrRef(*msg);
        req->responseCb_ = cb;
        req->setTaskExecutorId(groupId_);
        LOGTRACE << fds::logString(req->volumeIoHdr_);
        req->invoke();
        return req;
    }
    Error changeVolumeReplicaState_(VolumeReplicaHandleItr &volumeHandle,
                                    const int32_t &replicaVersion,
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
    fpi::VolumeState                    state_;
    int64_t                             groupId_;
    fpi::VolumeGroupVersion             version_;
    int64_t                             opSeqNo_;
    int64_t                             commitNo_;
    // TDOO(Rao): Need the state of the replica group.  Based on the state accept/reject
    // io

    friend class VolumeGroupBroadcastRequest;
    
};

template<class MsgT>
void VolumeGroupHandle::sendModifyMsg(const fpi::FDSPMsgTypeId &msgTypeId,
                                      SHPTR<MsgT> &msg, const VolumeResponseCb &cb) {
    runSynchronized([this, msgTypeId, msg, cb]() mutable {
        opSeqNo_++;
        invokeCommon_<MsgT, VolumeGroupBroadcastRequest>(msgTypeId, msg, cb);
    });
}

template<class MsgT>
void VolumeGroupHandle::sendWriteMsg(const fpi::FDSPMsgTypeId &msgTypeId,
                                     SHPTR<MsgT> &msg, const VolumeResponseCb &cb) {
    runSynchronized([this, msgTypeId, msg, cb]() mutable {
        opSeqNo_++;
        commitNo_++;
        invokeCommon_<MsgT, VolumeGroupBroadcastRequest>(msgTypeId, msg, cb);
    });
}


}  // namespace fds

#endif   // SOURCE_INCLUDE_NET_VOLUMEGROUPHANDLE_H_
