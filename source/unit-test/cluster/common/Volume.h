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
    virtual ~QosVolumeIo()
    {
        if (cb) {
            LOGNOTIFY << "Responding: " << *this;
            cb(*this);
        }
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

using SyncPullLogEntriesIo  = DECLARE_QOSVOLUMEIO(fpi::SyncPullLogEntriesMsg, fpi::SyncPullLogEntriesRespMsg);
using SyncPullLogEntriesIoPtr = SHPTR<SyncPullLogEntriesIo>;
std::ostream& operator<<(std::ostream &out, const SyncPullLogEntriesIo& io);

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
using QosFunctionIoPtr = SHPTR<QosFunctionIo>;

#if 0
template <class T>
struct ScopedVolumeIo {
    ScopedVolumeIo(T *_io, FDS_QoSControl *_qosCtrl)
    : io(_io),
    qosCtrl(_qosCtrl)
    {}
    ~ScopedVolumeIo() {
        if (io->cb) {
            LOGNOTIFY << "Responding: " << *io;
            io->cb(io);
        }
        qosCtrl->markIODone(io);
        delete io;
    }
    T                   *io;
    FDS_QoSControl      *qosCtrl;
};
#endif

using CatWriteBatchPtr      = SHPTR<CatWriteBatch>;

struct Volume : HasModuleProvider {

    Volume(CommonModuleProviderIf *provider,
           FDS_QoSControl *qosCtrl,
           const fds_volid_t &volId);
    virtual ~Volume() {}

    void init();
    /* Functional State handlers */
    void handleStartTx(StartTxIoPtr io);
    void handleUpdateTx(UpdateTxIoPtr io);
    void handleCommitTx(CommitTxIoPtr io);
    void handleSyncPullLogEntries(SyncPullLogEntriesIoPtr io);

    /* Any state handlers */
    void handleQosFunctionIo(QosFunctionIoPtr io);

    /* Sync state handlers */
    /**
    * @brief Initiates running sync protocol
    */
    void startSyncCheck();

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
    void changeState_(fpi::VolumeState targetState);
    void setError_(const Error &e);
    void commitBatch_(int64_t commitId, const CatWriteBatchPtr& writeBatch);

    void startSyncCheck_();
    void applySyncPullLogEntries_(int64_t startCommitId,
                                  std::vector<std::string> &entries);
    void sendSyncPullLogEntriesMsg_(const fpi::AddToVolumeGroupRespCtrlMsgPtr &syncInfo);
    void applySyncPullLogEntries_(const fpi::SyncPullLogEntriesRespMsgPtr &entriesMsg);

    using TxTbl                 = std::unordered_map<int64_t, CatWriteBatchPtr>;
    FDS_QoSControl                          *qosCtrl_;
    fds_volid_t                             volId_;
    std::unique_ptr<FDS_VolumeQueue>        volQueue_;;
    fpi::VolumeState                        state_;
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
    TxLog<CatWriteBatch>                    txLog_;

    static const std::string                OPINFOKEY;
    static const uint32_t                   MAX_SYNCENTRIES_BYTES = 1 * MB;
};
using VolumePtr = SHPTR<Volume>;

std::ostream& operator<< (std::ostream& os, const Volume::OpInfo &info);

}  // namespace fds

#endif
