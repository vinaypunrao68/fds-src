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
#ifdef USECATALOG
        db_.reset(new Catalog(catName));
        db_->GetWriteOptions().sync = false;
#else
        leveldb::DB* tempDb;
        leveldb::Options options;
        options.create_if_missing = true;
        auto status = leveldb::DB::Open(options, catName, &tempDb);
        fds_verify(status.ok());
        db_.reset(tempDb);
#endif
        // TODO(Rao):
        // 1. Make sure existing volume on disk isn't destroyed
        // 2. Load state from disk
        // TODO(Rao): Init commit id from what's read on disk
        appliedOpId_ = VolumeGroupConstants::OPSTARTID;
        appliedCommitId_ = VolumeGroupConstants::COMMITSTARTID;

        txLog_.init(1000, 0);

        /* Register with Qos ctrl */
        // TODO(Rao): Set right qos params
        // TODO(Rao): This shouldn't be here
#define FdsDmSysTaskPrio    5
        volQueue_.reset(new FDS_VolumeQueue(1024, 10000, 20, FdsDmSysTaskPrio));
        volQueue_->activate();
        auto err = qosCtrl_->registerVolume(volId_, volQueue_.get());
        fds_verify(err == ERR_OK);
        
    }

    void handleStartTx(StartTxIo *io)
    {
        LOGNOTIFY << *io;

        ScopedVolumeIo<StartTxIo>  s(io, qosCtrl_);
        /* Start a new transaction */
        auto &reqMsg = *(io->reqMsg);
        auto &ioHdr = getVolumeIoHdrRef(reqMsg);
        auto insRet = txTable_.insert(
            std::make_pair(ioHdr.txId, CatWriteBatchPtr(new CatWriteBatch)));
        fds_verify(insRet.second == true);
        io->respMsg.reset(new fpi::EmptyMsg());
        // TODO(Rao): Incase insertion fails due to duplicate entry return an error

        appliedOpId_++;
    }
    void handleUpdateTx(UpdateTxIo *io)
    {
        LOGNOTIFY << *io;

        ScopedVolumeIo<UpdateTxIo>  s(io, qosCtrl_);
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
        io->respMsg.reset(new fpi::EmptyMsg());

        appliedOpId_++;
    }
    void handleCommitTx(CommitTxIo *io)
    {
        LOGNOTIFY << *io;

        ScopedVolumeIo<CommitTxIo>  s(io, qosCtrl_);
        auto &reqMsg = *(io->reqMsg);
        auto &ioHdr = getVolumeIoHdrRef(reqMsg);
        auto itr = txTable_.find(ioHdr.txId);
        if (itr == txTable_.end()) {
            fds_panic("tx not found");
            // TODO(Rao): Handle this case
        }
        auto &batchPtr = itr->second;

        /* Make sure commit id is correct */
        fds_verify(appliedCommitId_+1 == ioHdr.commitId);

        /* Update log and commit to catalog */
        auto logRet = txLog_.append(ioHdr.commitId, batchPtr);
        fds_verify(logRet == ERR_OK);

        /* Commit the batch to leveldb */
        batchPtr->Put("commitid", std::to_string(ioHdr.commitId));
        LOGNOTIFY << "Prior leveldb update";
#ifdef USECATALOG
        auto catRet = db_->Update(batchPtr.get());
        LOGNOTIFY << "Post leveldb update";
        fds_verify(catRet == ERR_OK);
#else
        auto catRet = db_->Write(leveldb::WriteOptions(), batchPtr.get());
        LOGNOTIFY << "Post leveldb update";
        fds_verify(catRet.ok());
#endif
        io->respMsg.reset(new fpi::EmptyMsg());

        appliedOpId_++;
        appliedCommitId_++;

        /* Clear out transaction */
        txTable_.erase(itr);

        LOGNOTIFY << "Completed commit " << *io;
    }

    using CatWriteBatchPtr      = SHPTR<CatWriteBatch>;
    using TxTbl                 = std::unordered_map<int64_t, CatWriteBatchPtr>;
    FDS_QoSControl                          *qosCtrl_;
    fds_volid_t                             volId_;
    std::unique_ptr<FDS_VolumeQueue>        volQueue_;;
    fpi::VolumeState                        state_;
    int64_t                                 appliedOpId_;
    int64_t                                 appliedCommitId_;
#ifdef USECATALOG
    std::unique_ptr<Catalog>                db_;
#else
    std::unique_ptr<leveldb::DB>            db_; 
    leveldb::DB*                            db2_;
#endif
    TxTbl                                   txTable_;
    TxLog<CatWriteBatch>                    txLog_;
};
using VolumePtr = SHPTR<Volume>;
}  // namespace fds

#endif
