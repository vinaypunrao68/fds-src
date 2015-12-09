/* Copyright 2015 Formation Data Systems, Inc.
 */
#include <Volume.h>
#include <net/SvcMgr.h>
#include <net/SvcRequestPool.h>
#include <util/stringutils.h>

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

#define VERSION_CHECK(expected__, actual__) \
    do { \
    if (expected__ != actual__) { \
       GLOGWARN << "Version check failed.  expected: " \
            << expected__ << " actual: " << actual__; \
       return; \
    } \
    } while (false)

#define VOLUME_IO_VERSION_CHECK(volIo__) \
    do { \
        if (getVersion() != volIo__->getVersion()) { \
            GLOGWARN << volIo__->logString() \
                    << "Version check failed.  Current version: " << getVersion(); \
            volIo__->respStatus = ERR_INVALID_VOLUME_VERSION; \
            return; \
        } \
    } while (false)

#define ENSURE_IO_ORDER(io) \
do { \
    auto opId = getVolumeIoHdrRef(*(io->reqMsg)).opId; \
    if (opId != opInfo_.appliedOpId+1) { \
        fds_assert(!"opid mismatch"); \
        io->respStatus = ERR_BLOB_SEQUENCE_ID_REGRESSION; \
        io->respMsg.reset(new fpi::EmptyMsg()); \
        return; \
    } \
} while (false)

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
const std::string Volume::VERSIONKEY = "VERSION";

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
    os << " applidOpId: " << info.appliedOpId << " appliedCommmitId: "  << info.appliedCommitId;
    return os;
}

std::string VolumeIoBase::logString() const
{
    std::stringstream ss;
    ss << "VolumeIoBase volId: " << io_vol_id
        << " version: " << version
        << " msgType:  " << fpi::_FDSPMsgTypeId_VALUES_TO_NAMES.at(msgType);
    return ss.str();
}

Volume::Volume(CommonModuleProviderIf *provider,
               FDS_QoSControl *qosCtrl,
               const fds_volid_t &volId)
    : HasModuleProvider(provider),
    functional_("Functional"),
    syncing_("Syncing")
{
    logStr_ = util::strformat("Volume: %ld", volId.get());
    qosCtrl_ = qosCtrl;
    volId_ = volId;
    initBehaviors();
}

void Volume::initBehaviors()
{
#define on(__msgType__) \
    OnMsg<fpi::FDSPMsgTypeId, VolumeIoBase>(FDSP_MSG_TYPEID(__msgType__))
    VolumeBehavior common("Common");
    common = {
        on(fpi::QosFunction) >> [this](const SHPTR<VolumeIoBase>& io_) {
            auto io = SHPTR_CAST(QosFunctionIo, io_);
            io->func();
        }
    };

    functional_ = common;
    functional_ += {
        on(fpi::StartTxMsg) >> [this](const SHPTR<VolumeIoBase>& io_) {
            auto io = SHPTR_CAST(StartTxIo, io_);
            VOLUME_IO_VERSION_CHECK(io);
            ENSURE_IO_ORDER(io);
            handleStartTx_(io);
        },
        on(fpi::UpdateTxMsg) >> [this](const SHPTR<VolumeIoBase>& io_) {
            auto io = SHPTR_CAST(UpdateTxIo, io_);
            VOLUME_IO_VERSION_CHECK(io);
            ENSURE_IO_ORDER(io);
            handleUpdateTxCommon_(io, true);
        },
        on(fpi::CommitTxMsg) >> [this](const SHPTR<VolumeIoBase>& io_) {
            auto io = SHPTR_CAST(CommitTxIo, io_);
            VOLUME_IO_VERSION_CHECK(io);
            ENSURE_IO_ORDER(io);
            handleCommitTxCommon_(io, true, nullptr);
        },
        on(fpi::PullActiveTxsMsg) >> [this](const SHPTR<VolumeIoBase>& io_) {
            auto io = SHPTR_CAST(PullActiveTxsIo, io_);
            handlePullActiveTxs_(io);
        },
        on(fpi::PullCommitLogEntriesMsg) >> [this](const SHPTR<VolumeIoBase>& io_) {
            auto io = SHPTR_CAST(PullCommitLogEntriesIo, io_);
            handlePullCommitLogEntries_(io);
        }
    };

    syncing_ = common;
    syncing_ += {
        on(fpi::StartTxMsg) >> [this](const SHPTR<VolumeIoBase>& io_) {
            auto io = SHPTR_CAST(StartTxIo, io_);
            VOLUME_IO_VERSION_CHECK(io);
            QUICKSYNC_BUFFERIO_CHECK(io);
            ENSURE_IO_ORDER(io);
            handleStartTx_(io);
        },
        on(fpi::UpdateTxMsg) >> [this](const SHPTR<VolumeIoBase>& io_) {
            auto io = SHPTR_CAST(UpdateTxIo, io_);
            VOLUME_IO_VERSION_CHECK(io);
            QUICKSYNC_BUFFERIO_CHECK(io);
            ENSURE_IO_ORDER(io);
            bool txMustExist = txTable_.size() > 0;
            handleUpdateTxCommon_(io, true);
        },
        on(fpi::CommitTxMsg) >> [this](const SHPTR<VolumeIoBase>& io_) {
            auto io = SHPTR_CAST(CommitTxIo, io_);
            VOLUME_IO_VERSION_CHECK(io);
            QUICKSYNC_BUFFERIO_CHECK(io);
            ENSURE_IO_ORDER(io);
            /* All commits are sent bufferCommitLog */
            handleCommitTxCommon_(io, true, &(quicksyncCtx_->bufferCommitLog));
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
    /* Read the version info */
    std::string versionStr;
    status = db_->Get(leveldb::ReadOptions(), VERSIONKEY, &versionStr);
    if (status.ok()) {
        version_ = atoi(versionStr.c_str());
    } else {
        version_ = VolumeGroupConstants::VERSION_START;
    }
    LOGNORMAL << logString() << " Loaded version: " << version_;

    /* Read the opinfo */
    status = db_->Get(leveldb::ReadOptions(), OPINFOKEY, &opinfoStr);
    if (status.ok()) {
        opInfo_.fromString(opinfoStr);
    } else {
        opInfo_.appliedOpId = VolumeGroupConstants::OPSTARTID;
        opInfo_.appliedCommitId = VolumeGroupConstants::COMMITSTARTID;
    }
    LOGNORMAL << logString() << " Current loaded opinfo state " << opInfo_;
    // TODO(Rao):
    // 1. Make sure existing volume on disk isn't destroyed

    commitLog_.init(MAX_COMMITLOG_ENTRIES, opInfo_.appliedCommitId);

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
    LOGDEBUG << *io;

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
    LOGDEBUG << *io;

    auto &reqMsg = *(io->reqMsg);
    auto &ioHdr = getVolumeIoHdrRef(reqMsg);
    auto itr = txTable_.find(ioHdr.txId);
    if (itr == txTable_.end()) {
        if (txMustExist) {
            // TODO(Rao): Handle this case
            fds_panic("tx not found");
        }
        return;
    }
    auto &batch = itr->second;
    for (const auto &kv : reqMsg.kvPairs) {
        batch->Put(kv.first, kv.second); 
    }
    io->respMsg.reset(new fpi::EmptyMsg());

    opInfo_.appliedOpId++;
}

void Volume::handleCommitTxCommon_(const CommitTxIoPtr &io,
                                   bool txMustExist,
                                   VolumeCommitLog *alternateLog)
{
    LOGDEBUG << *io;

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
            LOGNORMAL << "Skipping commit " << *io;
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
}

void Volume::commitBatch_(int64_t commitId, const CatWriteBatchPtr& batchPtr)
{
    fds_verify(opInfo_.appliedCommitId+1 == commitId);
    /* Update log and commit to catalog */
    auto logRet = commitLog_.append(commitId, batchPtr);
    fds_verify(logRet == ERR_OK);

    /* Commit the batch to leveldb */
    batchPtr->Put(OPINFOKEY, opInfo_.toSlice());
#ifdef USECATALOG
    auto catRet = db_->Update(batchPtr.get());
    fds_verify(catRet == ERR_OK);
#else
    auto catRet = db_->Write(leveldb::WriteOptions(), batchPtr.get());
    fds_verify(catRet.ok());
#endif
    opInfo_.appliedCommitId++;
}

void Volume::handlePullActiveTxs_(const PullActiveTxsIoPtr &io)
{
    LOGNORMAL << *io;
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
    LOGNORMAL << "Sending tx count: " << respMsg->txIds.size() << " active txs info: " << opInfo_;
}

void Volume::handlePullCommitLogEntries_(const PullCommitLogEntriesIoPtr &io)
{
    LOGNORMAL << *io;

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
    LOGNORMAL << logString() << " Behavior change from: " << currentBehavior_->logString()
        << " to: " << target->logString();
    currentBehavior_ = target;
}

void Volume::setError_(const Error &e)
{
    LOGWARN << "Setting error: " << e;
    fds_panic("Not handled");
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
    /**
     * Quicks sync protocol description
     * 1. Send a message to coordinator requsting to be added to io group
     * 2. Cooridantor adds the volume to io group, from this point volume will receive any
     *    active io
     * 3. Turn on buffer io flag so that active io is buffered in memory.  This should be ok because
     *    we only need to buffer a short while.
     * 4. Send request to peer volume to fetch all active transactions
     * 5. Turn off io buffering.  Apply buffered io on top of received transactions.
     *    From this point any active io can directly be applied against active transactions.
     *    We also know up what commit # we need to sync.  After this point any active commit
     *    requests will be logged to different commit log until our commit log is hole free.
     * 6. Send a request to the peer volume requesting for missing commit range in our commit log
     *    history.
     * 7. Apply the received commits followed by buffered commits from active io.  At this point
     *    commit log history is hole free.
     * 8. Notify coordinator we are functional again.
     */

    /* Do a version change */
    version_++;
    std::string versionStr = util::strformat("%d", version_);
    auto status = db_->Put(leveldb::WriteOptions(), VERSIONKEY, versionStr);
    fds_verify(status.ok());
    LOGNORMAL << logString() << " Starting sync check.  Version change: " << version_;

    /* Reset quick sync context.  Turn on IO buffering here itself.  Note, it's possible
     * to postpone buffering we send a message to pull active transactions.  Starting to
     * buffer here itself won't hurt either.  When replaying buffered IO we will ignore
     * IO already covered in by active transactions
     */
    fds_verify(txTable_.size() == 0);

    quicksyncCtx_.reset(new QuickSyncCtx);
    quicksyncCtx_->bufferIo = true;

    auto msg = fpi::AddToVolumeGroupCtrlMsgPtr(new fpi::AddToVolumeGroupCtrlMsg);
    msg->targetState = fpi::VolumeState::VOLUME_SYNCING;
    msg->groupId = volId_.get();
    msg->replicaVersion = version_;
    msg->svcUuid = MODULEPROVIDER()->getSvcMgr()->getSelfSvcUuid();
    msg->lastOpId = opInfo_.appliedOpId;
    msg->lastCommitId = opInfo_.appliedCommitId;

    /* Send message to coordinator requesting to be added to the group */
    auto requestMgr = MODULEPROVIDER()->getSvcMgr()->getSvcRequestMgr();
    auto req = requestMgr->newEPSvcRequest(coordinatorUuid_);
    req->setPayload(FDSP_MSG_TYPEID(fpi::AddToVolumeGroupCtrlMsg), msg);
    auto prevVersion = getVersion();
    req->onResponseCb(synchronizedResponseCb([this, prevVersion](const Error &e_, StringPtr payload) {
        VERSION_CHECK(prevVersion, getVersion());
        Error err = e_;
        auto responseMsg = fds::deserializeFdspMsg<fpi::AddToVolumeGroupRespCtrlMsg>(err, payload);
        if (err != ERR_OK) {
            // TODO(Rao): Multiple types of errors can be returned here..handle them
            LOGERROR << "Failed to receive sync info from coordinator: " << err
                    << quicksyncLogStr();
            setError_(err);
            return;
        }
        // TODO(Rao): Check if sync is even required.  If sync not required we can become functional
        LOGNORMAL << logString()
                  << " Sync check coordinator response:  " << fds::logString(*responseMsg);

        sendPullActiveTxsMsg_(responseMsg);
    }));
    req->invoke();
}

void Volume::sendPullActiveTxsMsg_(const fpi::AddToVolumeGroupRespCtrlMsgPtr &syncInfo)
{
    /* Pick sync peer.  For now pick the first functional service */
    fds_verify(syncInfo->group.functionalReplicas.size() > 0);

    quicksyncCtx_->syncPeer = syncInfo->group.functionalReplicas[0];

    LOGNORMAL << "Sending PullActiveTxsMsg " << quicksyncLogStr();

    auto pullTxsMsg = fpi::PullActiveTxsMsgPtr(new fpi::PullActiveTxsMsg); 
    pullTxsMsg->groupId = volId_.get();

    auto requestMgr = MODULEPROVIDER()->getSvcMgr()->getSvcRequestMgr();
    auto req = requestMgr->newEPSvcRequest(quicksyncCtx_->syncPeer);
    req->setPayload(FDSP_MSG_TYPEID(fpi::PullActiveTxsMsg), pullTxsMsg);
    auto prevVersion = getVersion();
    req->onResponseCb(synchronizedResponseCb([this, prevVersion](const Error &e_, StringPtr payload) {
        VERSION_CHECK(prevVersion, getVersion());
        Error err = e_;
        auto responseMsg = fds::deserializeFdspMsg<fpi::PullActiveTxsRespMsg>(err, payload);
        if (err != ERR_OK) {
            // TODO(Rao): Multiple types of errors can be returned here..handle them
            LOGERROR << "Failed to receive sync entries: " << err
                    << quicksyncLogStr();
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

    /* Copy the transaction table to reflect that of the sync peer */
    for (uint32_t i = 0; i < activeTxsMsg->txIds.size(); i++) {
        auto batchPtr = MAKE_SHARED<CatWriteBatch>();
        leveldb::Slice s((activeTxsMsg->txData)[i]);
        leveldb::WriteBatchInternal::SetContents(batchPtr.get(), s);
        txTable_[activeTxsMsg->txIds[i]] = batchPtr;
    }

    /* Adjust the op ids so buffered ops can be replayed on top of copied transaction table
     * in opid order
     */
    opInfo_.appliedOpId = activeTxsMsg->lastOpId;
    quicksyncCtx_->bufferCommitLog.init(MAX_COMMITLOG_ENTRIES, activeTxsMsg->lastCommitId);

    LOGNORMAL << "Applied pulled active transactions.  opinfo: " << quicksyncLogStr();

    /* Apply buffered ops on top of active transactions */
    applyBufferedIo_();

    /* Now sync the commit log */
    sendPullCommitLogEntriesMsg_(activeTxsMsg->lastCommitId);
}

void Volume::applyBufferedIo_()
{
    /* Unset bufferIo, so that buffered io can be applied and not buffered again */
    quicksyncCtx_->bufferIo = false;

    /* Typically startingBufferOpId should be less than that of appliedOpId.  If it's a greater
     * we will need to go through quinck sync process again.  For now assert
     */
    fds_assert(quicksyncCtx_->startingBufferOpId <= opInfo_.appliedOpId);

    /* Prune out buffered io whose opid is < than our current opid */
    if (quicksyncCtx_->startingBufferOpId <= opInfo_.appliedOpId) {
        int32_t diff = (opInfo_.appliedOpId - quicksyncCtx_->startingBufferOpId) + 1;
        quicksyncCtx_->bufferedIo.erase(quicksyncCtx_->bufferedIo.begin(),
                                        std::next(quicksyncCtx_->bufferedIo.begin(), diff));
    }
    /* NOTE: At this point io->cb isn't set.  Cb has already been called when we buffered */
    for (auto &io : quicksyncCtx_->bufferedIo) {
        getCurrentBehavior()->handle(io->msgType, io);
        // TODO(Rao): Handle errors here
    }
    LOGNORMAL << "Applied buffered io.  opinfo: " << quicksyncLogStr();
}

void Volume::sendPullCommitLogEntriesMsg_(const int64_t &syncCommitId)
{
    auto syncPeerUuid = quicksyncCtx_->syncPeer;
    auto pullEntriesMsg = fpi::PullCommitLogEntriesMsgPtr(new fpi::PullCommitLogEntriesMsg); 
    pullEntriesMsg->groupId = volId_.get();
    pullEntriesMsg->startCommitId = opInfo_.appliedCommitId+1;
    pullEntriesMsg->endCommitId = syncCommitId;

    auto requestMgr = MODULEPROVIDER()->getSvcMgr()->getSvcRequestMgr();
    auto req = requestMgr->newEPSvcRequest(syncPeerUuid);
    req->setPayload(FDSP_MSG_TYPEID(fpi::PullCommitLogEntriesMsg), pullEntriesMsg);
    auto prevVersion = getVersion();
    req->onResponseCb(synchronizedResponseCb([this, prevVersion](const Error &e_, StringPtr payload) {
        VERSION_CHECK(prevVersion, getVersion());
        Error err = e_;
        auto responseMsg = fds::deserializeFdspMsg<fpi::PullCommitLogEntriesRespMsg>(err, payload);
        if (err != ERR_OK) {
            // TODO(Rao): Multiple types of errors can be returned here..handle them
            LOGERROR << logString() << " Failed to receive sync entries: " << err;
            setError_(err);
            return;
        }
        concludeQuickSync_(responseMsg);
    }));
    LOGNORMAL << logString() << "Sending PullCommitLogEntriesMsg: "
        << fds::logString(*pullEntriesMsg);
    req->invoke();
}

void Volume::concludeQuickSync_(const fpi::PullCommitLogEntriesRespMsgPtr &pulledCommitsMsg)
{
    applyPulledCommitLogEntries_(pulledCommitsMsg);
    applyBufferedCommits_();

    quicksyncCtx_.reset();
    changeBehavior_(&functional_);

    /* Notify coordinator we are functional */
    auto msg = fpi::AddToVolumeGroupCtrlMsgPtr(new fpi::AddToVolumeGroupCtrlMsg);
    msg->targetState = fpi::VolumeState::VOLUME_FUNCTIONAL;
    msg->groupId = volId_.get();
    msg->replicaVersion = version_;
    msg->svcUuid = MODULEPROVIDER()->getSvcMgr()->getSelfSvcUuid();
    msg->lastOpId = opInfo_.appliedOpId;
    msg->lastCommitId = opInfo_.appliedCommitId;

    auto requestMgr = MODULEPROVIDER()->getSvcMgr()->getSvcRequestMgr();
    auto req = requestMgr->newEPSvcRequest(coordinatorUuid_);
    req->setPayload(FDSP_MSG_TYPEID(fpi::AddToVolumeGroupCtrlMsg), msg);
    auto prevVersion = getVersion();
    req->onResponseCb(synchronizedResponseCb([this, prevVersion](const Error &e_, StringPtr payload) {
        VERSION_CHECK(prevVersion, getVersion());
        Error err = e_;
        auto responseMsg = fds::deserializeFdspMsg<fpi::AddToVolumeGroupRespCtrlMsg>(err, payload);
        if (err != ERR_OK) {
            // TODO(Rao): Multiple types of errors can be returned here..handle them
            LOGERROR << logString() << " Failed to receive sync info from coordinator: " << err; 
            setError_(err);
            return;
        }
        LOGNORMAL << logString() << " is Functional from perspective of coordinator";
    }));
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
    LOGNORMAL " Applied commit log entries. " << quicksyncLogStr();
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
    LOGNORMAL << " Applied buffered commits: " << quicksyncLogStr();
}

std::string Volume::quicksyncLogStr() const
{
    std::stringstream ss;
    ss << logString() << opInfo_;
    ss << " syncPeer: " << SvcMgr::mapToSvcUuidAndName(quicksyncCtx_->syncPeer);
    if (quicksyncCtx_->bufferIo) {
        ss << " bufferIo: true "
            << " startingBufferOpId: "  << quicksyncCtx_->startingBufferOpId
            << " endBufferOpId: "
            << quicksyncCtx_->startingBufferOpId + quicksyncCtx_->bufferedIo.size()-1;
    }
    ss << " bufferCommitLog: " << quicksyncCtx_->bufferCommitLog.logString();
    return ss.str();
}
}  // namespace fds
