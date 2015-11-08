/* Copyright 2015 Formation Data Systems, Inc.
 */
#ifndef SOURCE_VOLUMEGROUP_H_
#define SOURCE_VOLUMEGROUP_H_

#include <net/SvcMgr.h>
#include <net/SvcRequest.h>
#include <fdsp/volumegroup_types.h>

namespace fds {
struct VolumeGroupHandle;

struct VolumeGroupConstants {
    static const int64_t            OPSTARTID = 0;
    static const int64_t            COMMITSTARTID = 0;
};
std::ostream& operator << (std::ostream &out, const fpi::VolumeIoHdr &h);

struct VolumeReplicaHandle {
    VolumeReplicaHandle()
        : state(fpi::VolumeState::VOLUME_UNINIT),
        lastError(ERR_OK),
        appliedOpId(VolumeGroupConstants::OPSTARTID),
        appliedCommitId(VolumeGroupConstants::COMMITSTARTID)
    {
        svcUuid.svc_uuid = 0;
    }
    inline bool isFunctional() const {
        return state == fpi::VolumeState::VOLUME_FUNCTIONAL;
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
struct VolumeGroupRequest : SvcRequestIf {
    VolumeGroupRequest(CommonModuleProviderIf* provider,
                        const SvcRequestId &id,
                        const fpi::SvcUuid &myEpId,
                        VolumeGroupHandle *groupHandle);
    virtual std::string logString() override;
 protected:
    virtual void invokeWork_() override {
        fds_panic("Shouldn't come here");
    }

    VolumeGroupHandle          *groupHandle_; 
    fpi::VolumeIoHdr            volumeIoHdr_; 
    VolumeResponseCb            responseCb_;
    uint8_t                     nAcked_;
    uint8_t                     nSuccessAcked_;
    
    friend class VolumeGroupHandle;
};


struct VolumeGroupBroadcastRequest : VolumeGroupRequest {
    using VolumeRequestStatePair = std::pair<fpi::SvcUuid, SvcRequestState>;    

    /* Constructors inherited */
    using VolumeGroupRequest::VolumeGroupRequest;
    virtual void invoke() override;
    virtual void handleResponse(SHPTR<fpi::AsyncHdr>& header,
                                SHPTR<std::string>& payload);

 protected:
    VolumeRequestStatePair* getVolumeRequestStatePair_(const fpi::SvcUuid &svcUuid);

    std::vector<VolumeRequestStatePair> reqStates_;
};

struct VolumeGroupHandle : HasModuleProvider {
    explicit VolumeGroupHandle(CommonModuleProviderIf* provider);

    template<class F>
    void runSynchronized(F&& f)
    {
        taskExecutor_->scheduleOnHashKey(groupId_, std::forward<F>(f));
    }

    template<class MsgT>
    void sendModifyMsg(const fpi::FDSPMsgTypeId &msgTypeId,
                       SHPTR<MsgT> &msg, const VolumeResponseCb &cb) {
        runSynchronized([this, msgTypeId, msg, cb]() {
            opSeqNo_++;
            updateVolumeIoHdr(getVolumeIoHdrRef(*msg));
            sendVolumeBroadcastRequest_(msgTypeId, serializeFdspMsg(msg), cb);
        });
    }
    virtual void handleVolumeResponse(const fpi::VolumeIoHdr &hdr,
                                       const Error &inStatus,
                                       Error &outStatus);
 protected:
    VolumeReplicaHandle* getVolumeReplicaHandle_(const fpi::VolumeIoHdr &hdr);
    void sendVolumeBroadcastRequest_(const fpi::FDSPMsgTypeId &msgTypeId,
                                      const StringPtr &payload,
                                      const VolumeResponseCb &cb);

    SynchronizedTaskExecutor<uint64_t>  *taskExecutor_;
    SvcRequestPool                      *requestMgr_;
    std::vector<VolumeReplicaHandle>    functionalReplicas_;
    std::vector<VolumeReplicaHandle>    nonfunctionalReplicas_;
    int64_t                             groupId_;
    fpi::VolumeGroupVersion             version_;
    int64_t                             opSeqNo_;
    int64_t                             commitNo_;

    friend class VolumeGroupBroadcastRequest;
    
};

}  // namespace fds

#endif   // SOURCE_VOLUMEGROUP_H_
