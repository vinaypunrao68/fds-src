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
#include "volumegroup_extensions.h"

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
    const fpi::FDSPMsgTypeId reqMsgTypeId = ReqTypeId;
    const fpi::FDSPMsgTypeId respMsgTypeId = RespTypeId;

    using CbType = std::function<void (QosVolumeIo<ReqMsgT, ReqTypeId, RespMsgT, RespTypeId>*)>;

    QosVolumeIo(fds_volid_t volId,
                const SHPTR<ReqMsgT> &reqMsg,
                const CbType &cb)
    : SvcMsgIo(msgType)
    {
        this->ioType = FDS_DM_VOLUME_IO;
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
           const fds_volid_t &volId)
    : HasModuleProvider(provider)
    {
        qosCtrl_ = qosCtrl;
        volId_ = volId;
        state_ = fpi::VolumeState::VOLUME_UNINIT;
    }

    void init()
    {
        std::string catName = MODULEPROVIDER()->proc_fdsroot()->dir_sys_repo_dm() +
            "/" + std::to_string(volId_.get());
        catalog_.reset(new Catalog(catName));
        catalog_->GetWriteOptions().sync = false;
        // TODO(Rao):
        // 1. Make sure existing volume on disk isn't destroyed
        // 2. Load state from disk
        
    }

    void handleStartTx(StartTxIo *io)
    {
        ScopedVolumeIo<StartTxIo>  s(io, qosCtrl_);
        /* Start a new transaction */
        auto &reqMsg = *(io->reqMsg);
        auto &ioHdr = getVolumeIoHdrRef(reqMsg);
        auto insRet = txTable_.insert(
            std::make_pair(ioHdr.txId, std::unique_ptr<CatWriteBatch>(new CatWriteBatch)));
        fds_verify(insRet.second == true);
        // TODO(Rao): Incase insertion fails due to duplicate entry return an error
    }
    void handleUpdateTx(UpdateTxIo *io)
    {
        auto &reqMsg = *(io->reqMsg);
        auto &ioHdr = getVolumeIoHdrRef(reqMsg);
        auto itr = txTable_.find(ioHdr.txId);
        if (itr == txTable_.end()) {
            fds_panic("tx not found");
            // TODO(Rao): Handle this case
        }
        auto &batch = itr->second;
        for (const auto &kv : reqMsg.kvPairs) {
           batch->Put(kv.first, kv.second); 
        }
    }
    void handleCommitTx(CommitTxIo *io)
    {
        auto &reqMsg = *(io->reqMsg);
        auto &ioHdr = getVolumeIoHdrRef(reqMsg);
        auto itr = txTable_.find(ioHdr.txId);
        if (itr == txTable_.end()) {
            fds_panic("tx not found");
            // TODO(Rao): Handle this case
        }
        auto ret = catalog_->Update(itr->second.get());
        fds_verify(ret == ERR_OK);

        /* Update the log */
        // TODO(Rao):

        /* Clear out transaction */
        txTable_.erase(itr);
    }

    using TxTbl                 = std::unordered_map<int64_t, std::unique_ptr<CatWriteBatch>>;
    FDS_QoSControl              *qosCtrl_;
    fds_volid_t                 volId_;
    fpi::VolumeState            state_;
    int64_t                     appliedOpId_;
    int64_t                     appliedCommitId_;
    std::unique_ptr<Catalog>    catalog_;
    TxTbl                       txTable_;
};
using VolumePtr = SHPTR<Volume>;
}  // namespace fds

#endif
