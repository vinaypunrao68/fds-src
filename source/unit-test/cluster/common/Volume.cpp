/* Copyright 2015 Formation Data Systems, Inc.
 */
#include "Volume.h"
namespace fds {
const std::string Volume::OPINFOKEY = "OPINFO";

Volume::Volume(CommonModuleProviderIf *provider,
               FDS_QoSControl *qosCtrl,
               const fds_volid_t &volId)
    : HasModuleProvider(provider)
{
    qosCtrl_ = qosCtrl;
    volId_ = volId;
    state_ = fpi::VolumeState::VOLUME_UNINIT;
}
void Volume::init()
{
    leveldb::Status status;
    std::string opinfoStr;

    std::string catName = MODULEPROVIDER()->proc_fdsroot()->dir_sys_repo_dm() +
        "/" + std::to_string(volId_.get());
#ifdef USECATALOG
    db_.reset(new Catalog(catName));
    db_->GetWriteOptions().sync = false;
#else
    leveldb::DB* tempDb;
    leveldb::Options options;
    options.create_if_missing = true;
    status = leveldb::DB::Open(options, catName, &tempDb);
    fds_verify(status.ok());
    db_.reset(tempDb);
#endif
    status = db_->Get(leveldb::ReadOptions(), OPINFOKEY, &opinfoStr);
    if (status.ok()) {
        opInfo_.fromString(opinfoStr);
    } else {
        opInfo_.appliedOpId = VolumeGroupConstants::OPSTARTID;
        opInfo_.appliedCommitId = VolumeGroupConstants::COMMITSTARTID;
    }
    LOGNOTIFY << "Current loaded opinfo state " << opInfo_;
    // TODO(Rao):
    // 1. Make sure existing volume on disk isn't destroyed

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
void Volume::handleStartTx(StartTxIo *io)
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

    opInfo_.appliedOpId++;
}
void Volume::handleUpdateTx(UpdateTxIo *io)
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

    opInfo_.appliedOpId++;
}
void Volume::handleCommitTx(CommitTxIo *io)
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
    fds_verify(opInfo_.appliedCommitId+1 == ioHdr.commitId);

    /* Update log and commit to catalog */
    auto logRet = txLog_.append(ioHdr.commitId, batchPtr);
    fds_verify(logRet == ERR_OK);

    /* Commit the batch to leveldb */
    batchPtr->Put(OPINFOKEY, opInfo_.toSlice());
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

    opInfo_.appliedOpId++;
    opInfo_.appliedCommitId++;

    /* Clear out transaction */
    txTable_.erase(itr);

    LOGNOTIFY << "Completed commit " << *io;
}

std::ostream& operator<< (std::ostream& os, const Volume::OpInfo &info)
{
    os << "applidOpId: " << info.appliedOpId << " commmitId: "  << info.appliedCommitId;
    return os;
}

}  // namespace fds
