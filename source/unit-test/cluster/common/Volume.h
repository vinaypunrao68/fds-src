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
        qosCtrl->markIODone(io);
    }
    fds_volid_t getVolumeId() {
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

    using CbType = std::function<void (QosVolumeIo<ReqMsgT, ReqTypeId, RespMsgT, RespTypeId>*)>;

    QosVolumeIo(const fds_volid_t &volId,
                const SHPTR<ReqMsgT> &reqMsg,
                const CbType &cb)
    : SvcMsgIo(ReqTypeId, volId)
    {
        this->reqMsg = reqMsg;
        this->respStatus = ERR_OK;
        this->cb = cb;
    }
    virtual ~QosVolumeIo()
    {
        if (cb) {
            LOGNOTIFY << "Responding: " << *this;
            cb(io);
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
using UpdateTxIo        = DECLARE_QOSVOLUMEIO(fpi::UpdateTxMsg, fpi::EmptyMsg);
using CommitTxIo        = DECLARE_QOSVOLUMEIO(fpi::CommitTxMsg, fpi::EmptyMsg);
using SyncPullLogEntriesIo  = DECLARE_QOSVOLUMEIO(fpi::SyncPullLogEntriesMsg, fpi::SyncPullLogEntriesRespMsg);

/**
* @brief Function to be executed on qos
*/
struct QosFunctionIo : SvcMsgIo {
    using Func = std::function<void(const Error&, StringPtr)>;
    QosFunctionIo(const fds_volid_t &volId)
        : SvcMsgIo(FDSP_MSG_TYPEID(fpi::QosFunction), volId)
    {}
    Func                                            func;
    /* Dummy, not needed..kept around for ScopeVolumeIo */
    std::function<void(QosFunctionIo*)>             cb;
};

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
    void handleStartTx(StartTxIo *io);
    void handleUpdateTx(UpdateTxIo *io);
    void handleCommitTx(CommitTxIo *io);

    /* Sync state handlers */
    /**
    * @brief Initiates running sync protocol
    */
    void startSyncCheck();

    template <class ReqT>
    std::function<void(ReqT*, const Error&, StringPtr)>
    qosFunction(const QosFunctionIo::Func &func )
    {
        auto qosCtrl = qosCtrl_;
        auto qosMsg = new QosFunctionIo(volId_);
        qosMsg->func = func;
        // TODO(Rao): qosctrl shouldn't be needed when qosMsg is holds qosCtrl as a reference
        return [qosCtrl, qosMsg](ReqT*, const Error& e, StringPtr payload) {
            auto enqRet = qosCtrl->enqueueIO(qosMsg->io_vol_id, qosMsg);
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
    void applySyncPullLogEntries_(int64_t startCommitId,
                                  std::vector<std::string> &entries);

    using TxTbl                 = std::unordered_map<int64_t, CatWriteBatchPtr>;
    FDS_QoSControl                          *qosCtrl_;
    fds_volid_t                             volId_;
    std::unique_ptr<FDS_VolumeQueue>        volQueue_;;
    fpi::VolumeState                        state_;
    Error                                   lastError_;
    OpInfo                                  opInfo_;
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
