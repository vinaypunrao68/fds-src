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

const std::string Volume::OPINFOKEY = "OPINFO";

std::ostream& operator<<(std::ostream &out, const SyncPullLogEntriesIo& io)
{
    out << " msgType: SyncPullLogEntriesIo "
        << " groupId: " << io.reqMsg->groupId
        << " lastCommitVersion: " << io.reqMsg->lastCommitVersion
        << " startCommitId: " << io.reqMsg->startCommitId
        << " endCommitId: " << io.reqMsg->endCommitId;
    return out;
}

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

void Volume::handleStartTx(StartTxIoPtr io)
{
    LOGNOTIFY << *io;

    ASSERT_SYNCHRONIZED();
    FUNCTIONAL_STATE_CHECK();
    // TODO(Rao): Op id checks

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
void Volume::handleUpdateTx(UpdateTxIoPtr io)
{
    LOGNOTIFY << *io;

    ASSERT_SYNCHRONIZED();
    FUNCTIONAL_STATE_CHECK();
    // TODO(Rao): Op id checks

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

void Volume::handleCommitTx(CommitTxIoPtr io)
{
    LOGNOTIFY << *io;

    ASSERT_SYNCHRONIZED();
    FUNCTIONAL_STATE_CHECK();
    // TODO(Rao): Op id checks

    auto &reqMsg = *(io->reqMsg);
    auto &ioHdr = getVolumeIoHdrRef(reqMsg);
    auto itr = txTable_.find(ioHdr.txId);
    if (itr == txTable_.end()) {
        fds_panic("tx not found");
        // TODO(Rao): Handle this case
    }
    auto &batchPtr = itr->second;
    commitBatch_(ioHdr.commitId, batchPtr);
    io->respMsg.reset(new fpi::EmptyMsg());

    opInfo_.appliedOpId++;

    /* Clear out transaction */
    txTable_.erase(itr);

    LOGNOTIFY << "Completed commit " << *io;
}

void Volume::handleQosFunctionIo(QosFunctionIoPtr io)
{
    ASSERT_SYNCHRONIZED();
    io->func();
}

void Volume::commitBatch_(int64_t commitId, const CatWriteBatchPtr& batchPtr)
{
    fds_verify(opInfo_.appliedCommitId+1 == commitId);
    /* Update log and commit to catalog */
    auto logRet = txLog_.append(commitId, batchPtr);
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

std::ostream& operator<< (std::ostream& os, const Volume::OpInfo &info)
{
    os << "applidOpId: " << info.appliedOpId << " commmitId: "  << info.appliedCommitId;
    return os;
}

void Volume::changeState_(fpi::VolumeState targetState)
{
    state_ = targetState;
}

void Volume::setError_(const Error &e)
{
    fds_verify(state_ == fpi::VolumeState::VOLUME_FUNCTIONAL);
    fds_verify(lastError_ == ERR_OK);
    changeState_(fpi::VolumeState::VOLUME_DOWN);
    lastError_ = e;
}

void Volume::startSyncCheck_()
{
    changeState_(fpi::VolumeState::VOLUME_QUICKSYNC_CHECK);

    auto requestMgr = MODULEPROVIDER()->getSvcMgr()->getSvcRequestMgr();

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
        changeState_(fpi::VolumeState::VOLUME_QUICKSYNC_INPROGRESS);
        sendSyncPullLogEntriesMsg_(responseMsg);
    }));
    req->invoke();
}

void Volume::sendSyncPullLogEntriesMsg_(const fpi::AddToVolumeGroupRespCtrlMsgPtr &syncInfo)
{
    /* Pick sync peer.  For now pick the first functional service */
    fds_verify(syncInfo->group.functionalReplicas.size() > 0);

    auto syncPeerUuid = syncInfo->group.functionalReplicas[0];
    auto pullEntriesMsg = fpi::SyncPullLogEntriesMsgPtr(new fpi::SyncPullLogEntriesMsg); 
    pullEntriesMsg->groupId = volId_.get();
    pullEntriesMsg->startCommitId = opInfo_.appliedCommitId;
    pullEntriesMsg->endCommitId = syncInfo->group.lastCommitId;

    auto requestMgr = MODULEPROVIDER()->getSvcMgr()->getSvcRequestMgr();
    auto req = requestMgr->newEPSvcRequest(syncPeerUuid);
    req->setPayload(FDSP_MSG_TYPEID(fpi::SyncPullLogEntriesMsg), pullEntriesMsg);
    req->onResponseCb(synchronizedResponseCb([this](const Error &e_, StringPtr payload) {
        SYNC_STATE_CHECK();
        Error err = e_;
        auto responseMsg = fds::deserializeFdspMsg<fpi::SyncPullLogEntriesRespMsg>(err, payload);
        if (err != ERR_OK) {
            // TODO(Rao): Multiple types of errors can be returned here..handle them
            LOGERROR << "Failed to receive sync entries: " << err;
            setError_(err);
            return;
        }
        applySyncPullLogEntries_(responseMsg);
    }));
    req->invoke();
}

void Volume::applySyncPullLogEntries_(const fpi::SyncPullLogEntriesRespMsgPtr &entriesMsg)
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
    /* Once applying if out commit log history is hole free, we can switch to being
     * in functional state.
     */
}

void Volume::handleSyncPullLogEntries(SyncPullLogEntriesIoPtr io)
{
    LOGNOTIFY << *io;

    ASSERT_SYNCHRONIZED();
    FUNCTIONAL_STATE_CHECK();

    auto &reqMsg = *(io->reqMsg);
    /* TODO(Rao): Do a common point version check */

    io->respMsg.reset(new fpi::SyncPullLogEntriesRespMsg());
    io->respMsg->startCommitId = reqMsg.startCommitId;
    uint32_t bytesAddedCnt = 0;
    auto itr = txLog_.iterator(reqMsg.startCommitId);
    for (auto id = reqMsg.startCommitId;
         txLog_.isValid(itr) && id <= reqMsg.endCommitId;
         ++id, itr++) {
        auto entry = txLog_.get(itr);
        leveldb::Slice s = leveldb::WriteBatchInternal::Contents(entry.get());
        if (bytesAddedCnt + s.size() > MAX_SYNCENTRIES_BYTES) {
            break;
        }
        io->respMsg->entries.push_back(s.ToString());
        bytesAddedCnt += s.size();
    }
}

}  // namespace fds
