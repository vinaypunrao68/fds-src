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

#define GROUPHANDLE_FUNCTIONAL_CHECK(cb) \
    if (state_ != fpi::ResourceState::Active) { \
        LOGWARN << logString() << " Unavailable"; \
        cb(ERR_VOLUMEGROUP_DOWN, nullptr); \
        return; \
    }

namespace fds {
struct VolumeGroupHandle;

std::ostream& operator << (std::ostream &out, const fpi::VolumeIoHdr &h);

struct VolumeReplicaHandle {
    explicit VolumeReplicaHandle(const fpi::SvcUuid &id)
        : svcUuid(id),
        state(fpi::ResourceState::Unknown),
        lastError(ERR_OK),
        appliedOpId(VolumeGroupConstants::OPSTARTID),
        appliedCommitId(VolumeGroupConstants::COMMITSTARTID)
    {
    }
    inline static bool isUnitialized(const fpi::ResourceState& s)
    {
        return s == fpi::ResourceState::Unknown;
    }
    inline static bool isFunctional(const fpi::ResourceState& s)
    {
        return s == fpi::ResourceState::Active;
    }
    inline static bool isSyncing(const fpi::ResourceState& s)
    {
        return s == fpi::ResourceState::Syncing;
    }
    inline static bool isNonFunctional(const fpi::ResourceState& s)
    {
        return s == fpi::ResourceState::Offline;
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
    inline void setState(const fpi::ResourceState &state)
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
                        const fpi::ResourceState &state,
                        int64_t opId, int64_t commitId)
    {
        setState(state);
        this->version = version;
        appliedOpId = opId;
        appliedCommitId = commitId;
    }

    int32_t                 version;
    fpi::SvcUuid            svcUuid;
    fpi::ResourceState      state;
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
                      const fds_volid_t& volId,
                      uint32_t quorumCnt);

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
    std::vector<fpi::SvcUuid> getAllReplicas() const;

    inline bool isFunctional() const { return state_ == fpi::ResourceState::Active; }
    inline int64_t getGroupId() const { return groupId_; }
    inline int32_t getDmtVersion() const { return dmtVersion_; }
    inline int32_t size() const {
        return functionalReplicas_.size() +
            nonfunctionalReplicas_.size() +
            syncingReplicas_.size();
    }
    inline std::string logString() const;

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
    void resetGroup_();
    EPSvcRequestPtr createSetVolumeGroupCoordinatorMsgReq_();
    QuorumSvcRequestPtr createPreareOpenVolumeGroupMsgReq_();
    void determineFunctaionalReplicas_(QuorumSvcRequest* openReq);
    void broadcastGroupInfo_();
    void changeState_(const fpi::ResourceState &targetState);
    Error changeVolumeReplicaState_(VolumeReplicaHandleItr &volumeHandle,
                                    const int32_t &replicaVersion,
                                    const fpi::ResourceState &targetState,
                                    const Error &e);
    void setGroupInfo_(const fpi::VolumeGroupInfo &groupInfo);
    fpi::VolumeGroupInfo getGroupInfoForExternalUse_();
    void setVolumeIoHdr_(fpi::VolumeIoHdr &hdr);
    VolumeReplicaHandleItr getVolumeReplicaHandle_(const fpi::SvcUuid &svcUuid);
    VolumeReplicaHandleList& getVolumeReplicaHandleList_(const fpi::ResourceState& s);

    SynchronizedTaskExecutor<uint64_t>  *taskExecutor_;
    SvcRequestPool                      *requestMgr_;
    VolumeReplicaHandleList             functionalReplicas_;
    VolumeReplicaHandleList             nonfunctionalReplicas_;
    VolumeReplicaHandleList             syncingReplicas_;
    uint32_t                            quorumCnt_;
    fpi::ResourceState                  state_;
    int64_t                             groupId_;
    fpi::VolumeGroupVersion             version_;
    int64_t                             opSeqNo_;
    int64_t                             commitNo_;
    uint64_t                            dmtVersion_;
    // TDOO(Rao): Need the state of the replica group.  Based on the state accept/reject
    // io

    friend class VolumeGroupBroadcastRequest;
    
};

template<class MsgT>
void VolumeGroupHandle::sendReadMsg(const fpi::FDSPMsgTypeId &msgTypeId,
                                    SHPTR<MsgT> &msg, const VolumeResponseCb &cb)
{
    runSynchronized([this, msgTypeId, msg, cb]() mutable {
        GROUPHANDLE_FUNCTIONAL_CHECK(cb);

        invokeCommon_<MsgT, VolumeGroupFailoverRequest>(msgTypeId, msg, cb);
    });
}

template<class MsgT>
void VolumeGroupHandle::sendModifyMsg(const fpi::FDSPMsgTypeId &msgTypeId,
                                      SHPTR<MsgT> &msg, const VolumeResponseCb &cb) {
    runSynchronized([this, msgTypeId, msg, cb]() mutable {
        GROUPHANDLE_FUNCTIONAL_CHECK(cb);

        opSeqNo_++;
        invokeCommon_<MsgT, VolumeGroupBroadcastRequest>(msgTypeId, msg, cb);
    });
}

template<class MsgT>
void VolumeGroupHandle::sendWriteMsg(const fpi::FDSPMsgTypeId &msgTypeId,
                                     SHPTR<MsgT> &msg, const VolumeResponseCb &cb) {
    runSynchronized([this, msgTypeId, msg, cb]() mutable {
        GROUPHANDLE_FUNCTIONAL_CHECK(cb);

        opSeqNo_++;
        commitNo_++;
        invokeCommon_<MsgT, VolumeGroupBroadcastRequest>(msgTypeId, msg, cb);
    });
}


}  // namespace fds

#endif   // SOURCE_INCLUDE_NET_VOLUMEGROUPHANDLE_H_
