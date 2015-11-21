/* Copyright 2015 Formation Data Systems, Inc.
 */
#include <Volume.h>
#include <net/SvcMgr.h>
#include <net/SvcRequestPool.h>

namespace leveldb {
typedef uint64_t SequenceNumber;
}
#include <db/write_batch_internal.h>

namespace fds {
// TODO(Rao): Complete these macros
/**
* @brief Ensure the volume is functional.  If not return an empty message with an error
*/
#define FUNCTIONAL_STATE_CHECK()
#define SYNC_STATE_CHECK()
#define ASSERT_SYNCHRONIZED()
#define ENSURE_IO_ORDER(io)
#define QUICKSYNC_BUFFERIO_CHECK(io) \
    do { \
    if (quicksyncCtx_->bufferIo) { \
        auto opId = getVolumeIoHdrRef(*(io->reqMsg)).opId; \
        if (quicksyncCtx_->bufferedIo.size() == 0) { \
            quicksyncCtx_->startingBufferOpId = opId; \
        } \
        /* Ensure buffered io also comes in order */ \
        if (opId != quicksyncCtx_->startingBufferOpId + static_cast<int64_t>(quicksyncCtx_->bufferedIo.size())) { \
            fds_panic("Op id check failed"); \
        } \
        quicksyncCtx_->bufferedIo.push_back(io); \
        io->respMsg.reset(new fpi::EmptyMsg()); \
        io->respondBack(); \
        return; \
    } \
    } while (false)

const std::string Volume::OPINFOKEY = "OPINFO";

std::ostream& operator<<(std::ostream &out, const PullCommitLogEntriesIo& io)
{
    out << " msgType: PullCommitLogEntriesIo "
        << " groupId: " << io.reqMsg->groupId
        << " lastCommitVersion: " << io.reqMsg->lastCommitVersion
        << " startCommitId: " << io.reqMsg->startCommitId
        << " endCommitId: " << io.reqMsg->endCommitId;
    return out;
}

std::ostream& operator<<(std::ostream &out, const PullActiveTxsIo& io)
{
    out << fds::logString(*(io.reqMsg));
    return out;
}

std::ostream& operator<< (std::ostream& os, const Volume::OpInfo &info)
{
    os << "applidOpId: " << info.appliedOpId << " commmitId: "  << info.appliedCommitId;
    return os;
}

Volume::Volume(CommonModuleProviderIf *provider,
               FDS_QoSControl *qosCtrl,
               const fds_volid_t &volId)
    : HasModuleProvider(provider),
    functional_("Functional"),
    syncing_("Syncing")
{
    qosCtrl_ = qosCtrl;
    volId_ = volId;
    initBehaviors();
}

void Volume::initBehaviors()
{
#define on(__msgType__) \
    OnMsg<fpi::FDSPMsgTypeId, SvcMsgIo>(FDSP_MSG_TYPEID(__msgType__))
    VolumeBehavior common("Common");
    common = {
        on(fpi::QosFunction) >> [this](const SHPTR<SvcMsgIo>& io_) {
            auto io = SHPTR_CAST(QosFunctionIo, io_);
            io->func();
        }
    };

    functional_ = common;
    functional_ += {
        on(fpi::StartTxMsg) >> [this](const SHPTR<SvcMsgIo>& io_) {
            auto io = SHPTR_CAST(StartTxIo, io_);
            ENSURE_IO_ORDER(io);
            handleStartTx_(io);
        },
        on(fpi::UpdateTxMsg) >> [this](const SHPTR<SvcMsgIo>& io_) {
            auto io = SHPTR_CAST(UpdateTxIo, io_);
            ENSURE_IO_ORDER(io);
            handleUpdateTxCommon_(io, true);
        },
        on(fpi::CommitTxMsg) >> [this](const SHPTR<SvcMsgIo>& io_) {
            auto io = SHPTR_CAST(CommitTxIo, io_);
            ENSURE_IO_ORDER(io);
            handleCommitTxCommon_(io, true, nullptr);
        },
        on(fpi::PullActiveTxsMsg) >> [this](const SHPTR<SvcMsgIo>& io_) {
            auto io = SHPTR_CAST(PullActiveTxsIo, io_);
            handlePullActiveTxs_(io);
        },
        on(fpi::PullCommitLogEntriesMsg) >> [this](const SHPTR<SvcMsgIo>& io_) {
            auto io = SHPTR_CAST(PullCommitLogEntriesIo, io_);
            handlePullCommitLogEntries_(io);
        }
    };

    syncing_ = common;
    syncing_ += {
        on(fpi::StartTxMsg) >> [this](const SHPTR<SvcMsgIo>& io_) {
            auto io = SHPTR_CAST(StartTxIo, io_);
            QUICKSYNC_BUFFERIO_CHECK(io);
            ENSURE_IO_ORDER(io);
            handleStartTx_(io);
        },
        on(fpi::UpdateTxMsg) >> [this](const SHPTR<SvcMsgIo>& io_) {
            auto io = SHPTR_CAST(UpdateTxIo, io_);
            QUICKSYNC_BUFFERIO_CHECK(io);
            ENSURE_IO_ORDER(io);
            bool txMustExist = txTable_.size() > 0;
            handleUpdateTxCommon_(io, txMustExist);
        },
        on(fpi::CommitTxMsg) >> [this](const SHPTR<SvcMsgIo>& io_) {
            auto io = SHPTR_CAST(CommitTxIo, io_);
            QUICKSYNC_BUFFERIO_CHECK(io);
            ENSURE_IO_ORDER(io);
            bool txMustExist = txTable_.size() > 0;
            /* All commits are sent bufferCommitLog */
            handleCommitTxCommon_(io, txMustExist, &(quicksyncCtx_->bufferCommitLog));
        }
    };
}

void Volume::init()
{
    leveldb::Status status;
    std::string opinfoStr;

    currentBehavior_ = &functional_;

    /* Load state from level db */
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

    commitLog_.init(1000, 0);

    /* Create system volume queue */
    /* Register with Qos ctrl */
    // TODO(Rao): Set right qos params
    // TODO(Rao): This shouldn't be here
#define FdsDmSysTaskPrio    5
    volQueue_.reset(new FDS_VolumeQueue(1024, 10000, 20, FdsDmSysTaskPrio));
    volQueue_->activate();
    auto err = qosCtrl_->registerVolume(volId_, volQueue_.get());
    fds_verify(err == ERR_OK);
}

void Volume::handleStartTx_(const StartTxIoPtr &io)
{
    LOGNOTIFY << *io;

    ASSERT_SYNCHRONIZED();
    FUNCTIONAL_STATE_CHECK();
    // TODO(Rao): Op id checks

    /* Start a new transaction */
    auto &reqMsg = *(io->reqMsg);
    auto &ioHdr = getVolumeIoHdrRef(reqMsg);
    auto insRet = txTable_.insert(
        std::make_pair(ioHdr.txId, MAKE_SHARED<CatWriteBatch>()));
    fds_verify(insRet.second == true);
    io->respMsg.reset(new fpi::EmptyMsg());
    // TODO(Rao): Incase insertion fails due to duplicate entry return an error

    opInfo_.appliedOpId++;
}

void Volume::handleUpdateTxCommon_(const UpdateTxIoPtr &io, bool txMustExist)
{
    auto &reqMsg = *(io->reqMsg);
    auto &ioHdr = getVolumeIoHdrRef(reqMsg);
    io->respStatus = updateTxTbl_(ioHdr.txId, reqMsg.kvPairs, txMustExist);
    io->respMsg.reset(new fpi::EmptyMsg());

    opInfo_.appliedOpId++;
}

Error Volume::updateTxTbl_(int64_t txId,
                           const fpi::TxUpdates &kvPairs,
                           bool txMustExist)
{
    auto itr = txTable_.find(txId);
    if (itr == txTable_.end()) {
        if (txMustExist) {
            // TODO(Rao): Handle this case
            fds_panic("tx not found");
        } else {
            return ERR_OK;
        }
    }
    auto &batch = itr->second;
    for (const auto &kv : kvPairs) {
        batch->Put(kv.first, kv.second); 
    }
    return ERR_OK;
}

void Volume::handleCommitTxCommon_(const CommitTxIoPtr &io,
                                   bool txMustExist,
                                   VolumeCommitLog *alternateLog)
{
    auto &reqMsg = *(io->reqMsg);
    auto &ioHdr = getVolumeIoHdrRef(reqMsg);

    /* Fetch transaction */
    auto itr = txTable_.find(ioHdr.txId);
    if (itr == txTable_.end()) {
        if (txMustExist) {
            // TODO(Rao): Handle this case
            fds_panic("tx not found");
        } else {
            /* Ignore and consider it a success*/
            LOGNOTIFY << "Skipping commit " << *io;
        }
        io->respMsg.reset(new fpi::EmptyMsg());
        opInfo_.appliedOpId++;
    }

    /* Commit transaction.  Commit to alternate log if set */
    auto &batchPtr = itr->second;
    if (!alternateLog) {
        commitBatch_(ioHdr.commitId, batchPtr);
    } else {
        io->respStatus = alternateLog->append(ioHdr.commitId, batchPtr);
    }
    io->respMsg.reset(new fpi::EmptyMsg());

    opInfo_.appliedOpId++;

    /* Clear out transaction */
    txTable_.erase(itr);

    LOGNOTIFY << "Completed commit " << *io;
}

void Volume::commitBatch_(int64_t commitId, const CatWriteBatchPtr& batchPtr)
{
    fds_verify(opInfo_.appliedCommitId+1 == commitId);
    /* Update log and commit to catalog */
    auto logRet = commitLog_.append(commitId, batchPtr);
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
    opInfo_.appliedCommitId++;
}

void Volume::handlePullActiveTxs_(const PullActiveTxsIoPtr &io)
{
    LOGNOTIFY << *io;
    auto &reqMsg = *(io->reqMsg);
    auto &respMsg = io->respMsg;

    respMsg.reset(new fpi::PullActiveTxsRespMsg());
    respMsg->lastOpId = opInfo_.appliedOpId;
    respMsg->lastCommitId = opInfo_.appliedCommitId;
    for (auto &itr : txTable_) {
        respMsg->txIds.push_back(itr.first);
        leveldb::Slice s = leveldb::WriteBatchInternal::Contents(itr.second.get());
        respMsg->txData.push_back(s.ToString());
    }
}

void Volume::handlePullCommitLogEntries_(const PullCommitLogEntriesIoPtr &io)
{
    LOGNOTIFY << *io;

    ASSERT_SYNCHRONIZED();
    FUNCTIONAL_STATE_CHECK();

    auto &reqMsg = *(io->reqMsg);
    /* TODO(Rao): Do a common point version check */

    io->respMsg.reset(new fpi::PullCommitLogEntriesRespMsg());
    io->respMsg->startCommitId = reqMsg.startCommitId;
    uint32_t bytesAddedCnt = 0;
    auto itr = commitLog_.iterator(reqMsg.startCommitId);
    for (auto id = reqMsg.startCommitId;
         commitLog_.isValid(itr) && id <= reqMsg.endCommitId;
         ++id, itr++) {
        auto entry = commitLog_.get(itr);
        leveldb::Slice s = leveldb::WriteBatchInternal::Contents(entry.get());
        if (bytesAddedCnt + s.size() > MAX_SYNCENTRIES_BYTES) {
            break;
        }
        io->respMsg->entries.push_back(s.ToString());
        bytesAddedCnt += s.size();
    }
}


#if 0
void Volume::handleQosFunctionIo(QosFunctionIoPtr io)
{
    ASSERT_SYNCHRONIZED();
    io->func();
}
#endif

void Volume::changeBehavior_(Volume::VolumeBehavior *target)
{
    currentBehavior_ = target;
}

void Volume::setError_(const Error &e)
{
    fds_verify(lastError_ == ERR_OK);
    // TODO(Rao): Go into non-functional behavior
    lastError_ = e;
}

void Volume::forceQuickSync(const fpi::SvcUuid &coordinator) {
    qosCtrl_->threadPool->scheduleWithAffinity(volId_.get(),[this, coordinator]() {
        coordinatorUuid_ = coordinator;
        changeBehavior_(&syncing_);
        startSyncCheck_();
    });
}

void Volume::startSyncCheck_()
{
    fds_verify(txTable_.size() == 0);
    quicksyncCtx_.reset(new QuickSyncCtx);

    auto requestMgr = MODULEPROVIDER()->getSvcMgr()->getSvcRequestMgr();

    /* First send message to coordinator requesting to be added to the group */
    auto msg = fpi::AddToVolumeGroupCtrlMsgPtr(new fpi::AddToVolumeGroupCtrlMsg);
    auto req = requestMgr->newEPSvcRequest(coordinatorUuid_);
    req->setPayload(FDSP_MSG_TYPEID(fpi::AddToVolumeGroupCtrlMsg), msg);
    req->onResponseCb(synchronizedResponseCb([this](const Error &e_, StringPtr payload) {
        SYNC_STATE_CHECK();
        Error err = e_;
        auto responseMsg = fds::deserializeFdspMsg<fpi::AddToVolumeGroupRespCtrlMsg>(err, payload);
        if (err != ERR_OK) {
            // TODO(Rao): Multiple types of errors can be returned here..handle them
            LOGERROR << "Failed to receive sync info from coordinator: " << err; 
            setError_(err);
            return;
        }
        // TODO(Rao): Check if sync is even required.  If sync not required we can become functional
        sendPullActiveTxsMsg_(responseMsg);
    }));
    req->invoke();
}

void Volume::sendPullActiveTxsMsg_(const fpi::AddToVolumeGroupRespCtrlMsgPtr &syncInfo)
{
    /* Pick sync peer.  For now pick the first functional service */
    fds_verify(syncInfo->group.functionalReplicas.size() > 0);

    quicksyncCtx_->syncPeer = syncInfo->group.functionalReplicas[0];

    auto pullTxsMsg = fpi::PullActiveTxsMsgPtr(new fpi::PullActiveTxsMsg); 
    pullTxsMsg->groupId = volId_.get();

    auto requestMgr = MODULEPROVIDER()->getSvcMgr()->getSvcRequestMgr();
    auto req = requestMgr->newEPSvcRequest(quicksyncCtx_->syncPeer);
    req->setPayload(FDSP_MSG_TYPEID(fpi::PullActiveTxsMsg), pullTxsMsg);
    req->onResponseCb(synchronizedResponseCb([this](const Error &e_, StringPtr payload) {
        SYNC_STATE_CHECK();
        Error err = e_;
        auto responseMsg = fds::deserializeFdspMsg<fpi::PullActiveTxsRespMsg>(err, payload);
        if (err != ERR_OK) {
            // TODO(Rao): Multiple types of errors can be returned here..handle them
            LOGERROR << "Failed to receive sync entries: " << err;
            setError_(err);
            return;
        }
        applyPulledActiveTxs_(responseMsg);
    }));
    req->invoke();
}

void Volume::applyPulledActiveTxs_(const fpi::PullActiveTxsRespMsgPtr &activeTxsMsg)
{
    fds_verify(txTable_.size() == 0);

    for (uint32_t i = 0; i < activeTxsMsg->txIds.size(); i++) {
        auto batchPtr = MAKE_SHARED<CatWriteBatch>();
        leveldb::Slice s((activeTxsMsg->txData)[i]);
        leveldb::WriteBatchInternal::SetContents(batchPtr.get(), s);
        txTable_[activeTxsMsg->txIds[i]] = batchPtr;
    }
    opInfo_.appliedOpId = activeTxsMsg->lastOpId;
    LOGNOTIFY << "Applied pulled active transactions.  opinfo: " << opInfo_;

    /* Apply buffered ops on top of active transactions */
    applyBufferedIo_();
    LOGNOTIFY << "Applied buffered io.  opinfo: " << opInfo_;

    /* Now sync the commit log */
    sendPullCommitLogEntriesMsg_(activeTxsMsg->lastCommitId);
}

void Volume::applyBufferedIo_()
{
    /* Unset bufferIo, so that buffered io can be applied and not buffered again */
    quicksyncCtx_->bufferIo = false;

    /* Prune out buffered io whose opid is < than our current opid */
    if (quicksyncCtx_->startingBufferOpId <= opInfo_.appliedOpId) {
        int32_t diff = opInfo_.appliedOpId - quicksyncCtx_->startingBufferOpId;
        quicksyncCtx_->bufferedIo.erase(quicksyncCtx_->bufferedIo.begin(),
                                        std::next(quicksyncCtx_->bufferedIo.begin(), diff));
    }
    for (auto &io : quicksyncCtx_->bufferedIo) {
        getCurrentBehavior()->handle(io->msgType, io);
        // TODO(Rao): Handle errors here
    }
}

void Volume::sendPullCommitLogEntriesMsg_(const int64_t &syncCommitId)
{
    auto syncPeerUuid = quicksyncCtx_->syncPeer;
    auto pullEntriesMsg = fpi::PullCommitLogEntriesMsgPtr(new fpi::PullCommitLogEntriesMsg); 
    pullEntriesMsg->groupId = volId_.get();
    pullEntriesMsg->startCommitId = opInfo_.appliedCommitId;
    pullEntriesMsg->endCommitId = syncCommitId;

    auto requestMgr = MODULEPROVIDER()->getSvcMgr()->getSvcRequestMgr();
    auto req = requestMgr->newEPSvcRequest(syncPeerUuid);
    req->setPayload(FDSP_MSG_TYPEID(fpi::PullCommitLogEntriesMsg), pullEntriesMsg);
    req->onResponseCb(synchronizedResponseCb([this](const Error &e_, StringPtr payload) {
        SYNC_STATE_CHECK();
        Error err = e_;
        auto responseMsg = fds::deserializeFdspMsg<fpi::PullCommitLogEntriesRespMsg>(err, payload);
        if (err != ERR_OK) {
            // TODO(Rao): Multiple types of errors can be returned here..handle them
            LOGERROR << "Failed to receive sync entries: " << err;
            setError_(err);
            return;
        }
        applyPulledCommitLogEntries_(responseMsg);
        applyBufferedCommits_();
        quicksyncCtx_.reset();
        changeBehavior_(&functional_);
        // TODO(Rao): Notify coordinator we are functional
    }));
    LOGNOTIFY << "Sending: " << fds::logString(*pullEntriesMsg);
    req->invoke();
}

void Volume::applyPulledCommitLogEntries_(const fpi::PullCommitLogEntriesRespMsgPtr &entriesMsg)
{
    fds_verify(opInfo_.appliedCommitId+1 == entriesMsg->startCommitId);
    auto commitId = entriesMsg->startCommitId;
    for (const auto &entry : entriesMsg->entries) {
        auto batchPtr = CatWriteBatchPtr(new CatWriteBatch());
        leveldb::Slice s(entry);
        leveldb::WriteBatchInternal::SetContents(batchPtr.get(), s);
        commitBatch_(commitId, batchPtr);
        commitId++;
    }
    LOGNOTIFY << "Applied commit log entries. From: " << entriesMsg->startCommitId
        << " size: " << entriesMsg->entries.size();
}

void Volume::applyBufferedCommits_()
{
    auto itr = quicksyncCtx_->bufferCommitLog.iterator();
    auto commitId = quicksyncCtx_->bufferCommitLog.startCommitId();
    while (quicksyncCtx_->bufferCommitLog.isValid(itr)) {
        commitBatch_(commitId, *itr);
        itr++;
        commitId++;
    }
    LOGNOTIFY << "Applied buffered commits: " << quicksyncCtx_->bufferCommitLog.startCommitId()
        << " to: " << commitId - 1;
}

}  // namespace fds
