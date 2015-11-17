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
    SvcMsgIo(const fpi::FDSPMsgTypeId &msgType)
    {
        this->msgType = msgType;
    }
    virtual ~SvcMsgIo() {}
 
    fpi::FDSPMsgTypeId      msgType;
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

    QosVolumeIo(fds_volid_t volId,
                const SHPTR<ReqMsgT> &reqMsg,
                const CbType &cb)
    : SvcMsgIo(ReqTypeId)
    {
        this->io_type = FDS_DM_VOLUME_IO;
        this->io_vol_id = volId;
        this->reqMsg = reqMsg;
        this->respStatus = ERR_OK;
        this->cb = cb;
    }
    virtual ~QosVolumeIo()
    {
    }
    fds_volid_t getVolumeId() {
        const auto &hdr = getVolumeIoHdrRef(*reqMsg);
        return fds_volid_t(static_cast<uint64_t>(hdr.groupId));
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

struct Volume : HasModuleProvider {
    Volume(CommonModuleProviderIf *provider,
           FDS_QoSControl *qosCtrl,
           const fds_volid_t &volId);
    virtual ~Volume() {}

    void init();
    void handleStartTx(StartTxIo *io);
    void handleUpdateTx(UpdateTxIo *io);
    void handleCommitTx(CommitTxIo *io);

    using CatWriteBatchPtr      = SHPTR<CatWriteBatch>;
    using TxTbl                 = std::unordered_map<int64_t, CatWriteBatchPtr>;
    FDS_QoSControl                          *qosCtrl_;
    fds_volid_t                             volId_;
    std::unique_ptr<FDS_VolumeQueue>        volQueue_;;
    fpi::VolumeState                        state_;
    struct OpInfo {
        int64_t                             appliedOpId;
        int64_t                             appliedCommitId;
        leveldb::Slice toSlice() {
            return leveldb::Slice((char*)this, sizeof(OpInfo));
        }
        void fromString(const std::string &s) {
            memcpy(this, s.data(), sizeof(OpInfo));
        }
    } opInfo_;
#ifdef USECATALOG
    std::unique_ptr<Catalog>                db_;
#else
    std::unique_ptr<leveldb::DB>            db_; 
    leveldb::DB*                            db2_;
#endif
    TxTbl                                   txTable_;
    TxLog<CatWriteBatch>                    txLog_;

    static const std::string                OPINFOKEY;
};
using VolumePtr = SHPTR<Volume>;

std::ostream& operator<< (std::ostream& os, const Volume::OpInfo &info);

}  // namespace fds

#endif
