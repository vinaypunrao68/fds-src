/* Copyright 2015 Formation Data Systems, Inc.
 */
#ifndef _VOLUME_H_
#define _VOLUME_H_

#include <unordered_map>
#include <fds_qos.h>
#include <fdsp/svc_api_types.h>
#include <fdsp/volumegroup_types.h>
#include <lib/Catalog.h>
#include <fdsp_utils.h>
#include <volumegroup_extensions.h>
#include <TxLog.h>
#include <Behavior.h>

namespace std {
template<>
struct hash<fpi::FDSPMsgTypeId> {
    size_t operator()(const fpi::FDSPMsgTypeId& id) const
    {
        return static_cast<size_t>(id);
    }
};
}

namespace fds {

#define DECLARE_QOSVOLUMEIO(ReqT, RespT) \
    QosVolumeIo<ReqT, FDSP_MSG_TYPEID(ReqT), RespT, FDSP_MSG_TYPEID(RespT)>

struct EPSvcRequest;

struct SvcMsgIo : public FDS_IOType {
    SvcMsgIo(const fpi::FDSPMsgTypeId &msgType,
             const fds_volid_t &volId,
             FDS_QoSControl *qosCtrl)
    {
        this->msgType = msgType;
        this->io_type = FDS_DM_VOLUME_IO;
        this->io_vol_id = volId;
        this->qosCtrl = qosCtrl;
    }
    virtual ~SvcMsgIo() {
        qosCtrl->markIODone(this);
    }
    fds_volid_t getVolumeId() const {
        return fds_volid_t(io_vol_id);
    }
 
    fpi::FDSPMsgTypeId      msgType;
    FDS_QoSControl          *qosCtrl;
};
using SvcMsgIoPtr = SHPTR<SvcMsgIo>;

template <class ReqT,
          fpi::FDSPMsgTypeId ReqTypeId,
          class RespT,
          fpi::FDSPMsgTypeId RespTypeId>
struct QosVolumeIo : public SvcMsgIo {
    using ReqMsgT = ReqT;
    using RespMsgT = RespT;
    const static fpi::FDSPMsgTypeId reqMsgTypeId = ReqTypeId;
    const static fpi::FDSPMsgTypeId respMsgTypeId = RespTypeId;

    using ThisType = QosVolumeIo<ReqMsgT, ReqTypeId, RespMsgT, RespTypeId>;
    // TODO(Rao): Consider making this const ref
    using CbType = std::function<void (const ThisType&)>;

    QosVolumeIo(const fds_volid_t &volId,
                FDS_QoSControl *qosCtrl,
                const SHPTR<ReqMsgT> &reqMsg,
                const CbType &cb)
    : SvcMsgIo(ReqTypeId, volId, qosCtrl)
    {
        this->reqMsg = reqMsg;
        this->respStatus = ERR_OK;
        this->cb = cb;
    }
    void respondBack() {
        if (cb) {
            LOGNOTIFY << "Responding: " << *this;
            cb(*this);
            cb = 0;
        }
    }
    virtual ~QosVolumeIo()
    {
        respondBack();
    }

    SHPTR<ReqMsgT>          reqMsg;
    Error                   respStatus;
    SHPTR<RespMsgT>         respMsg;
    CbType                  cb;
};

template <class ReqT,
          fpi::FDSPMsgTypeId ReqTypeId,
          class RespT,
          fpi::FDSPMsgTypeId RespTypeId>
const fpi::FDSPMsgTypeId QosVolumeIo<ReqT, ReqTypeId, RespT, RespTypeId>::reqMsgTypeId;

template <class ReqT,
          fpi::FDSPMsgTypeId ReqTypeId,
          class RespT,
          fpi::FDSPMsgTypeId RespTypeId>
const fpi::FDSPMsgTypeId QosVolumeIo<ReqT, ReqTypeId, RespT, RespTypeId>::respMsgTypeId;

template <class ReqT,
          fpi::FDSPMsgTypeId ReqTypeId,
          class RespT,
          fpi::FDSPMsgTypeId RespTypeId>
std::ostream& operator<<(std::ostream &out,
                         const QosVolumeIo<ReqT, ReqTypeId, RespT, RespTypeId> & io) {
    out << " msgType: " << static_cast<int>(ReqTypeId)
        << fds::logString(getVolumeIoHdrRef(*(io.reqMsg)));
        // TODO: Print message
    return out;
}

using StartTxIo         = DECLARE_QOSVOLUMEIO(fpi::StartTxMsg, fpi::EmptyMsg);
using StartTxIoPtr      = SHPTR<StartTxIo>;

using UpdateTxIo        = DECLARE_QOSVOLUMEIO(fpi::UpdateTxMsg, fpi::EmptyMsg);
using UpdateTxIoPtr     = SHPTR<UpdateTxIo>;

using CommitTxIo        = DECLARE_QOSVOLUMEIO(fpi::CommitTxMsg, fpi::EmptyMsg);
using CommitTxIoPtr     = SHPTR<CommitTxIo>;

using PullActiveTxsIo  = DECLARE_QOSVOLUMEIO(fpi::PullActiveTxsMsg, fpi::PullActiveTxsRespMsg);
using PullActiveTxsIoPtr = SHPTR<PullActiveTxsIo>;
std::ostream& operator<<(std::ostream &out, const PullActiveTxsIo &io);

using PullCommitLogEntriesIo  = DECLARE_QOSVOLUMEIO(fpi::PullCommitLogEntriesMsg, fpi::PullCommitLogEntriesRespMsg);
using PullCommitLogEntriesIoPtr = SHPTR<PullCommitLogEntriesIo>;

std::ostream& operator<<(std::ostream &out, const PullCommitLogEntriesIo& io);

/**
* @brief Function to be executed on qos
*/
struct QosFunctionIo : SvcMsgIo {
    using Func = std::function<void()>;
    QosFunctionIo(const fds_volid_t &volId, FDS_QoSControl *qosCtrl)
        : SvcMsgIo(FDSP_MSG_TYPEID(fpi::QosFunction), volId, qosCtrl)
    {}
    Func                                            func;
};
using QosFunctionIoPtr      = SHPTR<QosFunctionIo>;

using CatWriteBatchPtr      = SHPTR<CatWriteBatch>;
using VolumeCommitLog       = TxLog<CatWriteBatch> ;

struct QuickSyncCtx {
    QuickSyncCtx()
    : bufferIo(false)
    {
    }
    fpi::SvcUuid                            syncPeer;
    bool                                    bufferIo;
    int64_t                                 startingBufferOpId; 
    /* Active IO that's been buffered */
    std::list<SvcMsgIoPtr>                  bufferedIo;
    /* Commits that are buffered during sync */
    VolumeCommitLog                         bufferCommitLog;
};
using QuickSyncCtxPtr = std::unique_ptr<QuickSyncCtx>;

struct Volume : HasModuleProvider {
    using VolumeBehavior        = Behavior<fpi::FDSPMsgTypeId, SvcMsgIo>;
    using TxTbl                 = std::map<int64_t, CatWriteBatchPtr>;

    Volume(CommonModuleProviderIf *provider,
           FDS_QoSControl *qosCtrl,
           const fds_volid_t &volId);
    virtual ~Volume() {}

    void init();
    void initBehaviors();

    /* For testing */
    void forceQuickSync(const fpi::SvcUuid &coordinator);

    template <class ReqT=EPSvcRequest, class F>
    std::function<void(ReqT*, const Error&, StringPtr)>
    synchronizedResponseCb(F &&func)
    {
        auto qosMsg = new QosFunctionIo(volId_, qosCtrl_);
        return [qosMsg, func](ReqT*, const Error& e, StringPtr payload) {
            qosMsg->func = std::bind(func, e, payload);
            auto enqRet = qosMsg->qosCtrl->enqueueIO(qosMsg->io_vol_id, qosMsg);
            if (enqRet != ERR_OK) {
                fds_panic("Not handled");
                // TODO(Rao): Fix it
                // GLOGWARN << "Failed to enqueue " << *qosMsg;
                delete qosMsg;
            }
        };
    }
    inline VolumeBehavior* getCurrentBehavior() { return currentBehavior_; }

    struct OpInfo {
        int64_t                             appliedOpId;
        int64_t                             appliedCommitId;
        leveldb::Slice toSlice() {
            return leveldb::Slice((char*)this, sizeof(OpInfo));
        }
        void fromString(const std::string &s) {
            memcpy(this, s.data(), sizeof(OpInfo));
        }
    };
 protected:
    void changeBehavior_(VolumeBehavior *target);
    void setError_(const Error &e);

    void handleStartTx_(const StartTxIoPtr &io);
    void handlePullActiveTxs_(const PullActiveTxsIoPtr &io);
    void handlePullCommitLogEntries_(const PullCommitLogEntriesIoPtr &io);
    void handleUpdateTxCommon_(const UpdateTxIoPtr &io, bool txMustExist);
    void handleCommitTxCommon_(const CommitTxIoPtr &io,
                               bool txMustExist,
                               VolumeCommitLog *alternateLog);
    Error updateTxTbl_(int64_t txId,
                       const fpi::TxUpdates &kvPairs,
                       bool txMustExist);
    void commitBatch_(int64_t commitId, const CatWriteBatchPtr& writeBatch);

    void startSyncCheck_();
    void sendPullActiveTxsMsg_(const fpi::AddToVolumeGroupRespCtrlMsgPtr &syncInfo);
    void applyPulledActiveTxs_(const fpi::PullActiveTxsRespMsgPtr &txsResp);
    void applyBufferedIo_();
    void sendPullCommitLogEntriesMsg_(const int64_t &syncCommitId);
    void applyPulledCommitLogEntries_(const fpi::PullCommitLogEntriesRespMsgPtr &entriesMsg);
    void applyBufferedCommits_();


    FDS_QoSControl                          *qosCtrl_;
    fds_volid_t                             volId_;
    std::unique_ptr<FDS_VolumeQueue>        volQueue_;;
    VolumeBehavior                          functional_;
    VolumeBehavior                          syncing_;
    VolumeBehavior                          *currentBehavior_;
    Error                                   lastError_;
    OpInfo                                  opInfo_;
    fpi::SvcUuid                            coordinatorUuid_;
#ifdef USECATALOG
    std::unique_ptr<Catalog>                db_;
#else
    std::unique_ptr<leveldb::DB>            db_; 
    leveldb::DB*                            db2_;
#endif
    TxTbl                                   txTable_;
    VolumeCommitLog                         commitLog_;
    QuickSyncCtxPtr                         quicksyncCtx_;

    static const std::string                OPINFOKEY;
    static const uint32_t                   MAX_SYNCENTRIES_BYTES = 1 * MB;
};
using VolumePtr = SHPTR<Volume>;

std::ostream& operator<< (std::ostream& os, const Volume::OpInfo &info);

}  // namespace fds

#endif
