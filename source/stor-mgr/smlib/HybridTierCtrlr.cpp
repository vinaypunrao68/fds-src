/*
 * Copyright 2015 Formation Data Systems, Inc.
 */
#include <string>
#include <fds_module_provider.h>
#include <fds_timer.h>
#include <concurrency/ThreadPool.h>
#include <HybridTierCtrlr.h>

namespace fds {

// TODO(Rao): Get this from config file
uint32_t HybridTierCtrlr::BATCH_SZ = 1024;
uint32_t HybridTierCtrlr::FREQUENCY = 10;

// TODO(Rao):
// -Handle unowned token case.
// -Move runTask into ObjectStore
// -Start garbage collection for ssd
// -Expose stats for testing
HybridTierCtrlr::HybridTierCtrlr(CommonModuleProviderIf* modProvider,
                                   SmIoReqHandler* storMgr,
                                   SmDiskMap::ptr diskMap)
{
    modProvider_ = modProvider_;
    threadpool_ = modProvider_->proc_thrpool();
    storMgr_ = storMgr;
    diskMap_ = diskMap;
    snapRequest_.io_type = FDS_SM_SNAPSHOT_TOKEN;
    snapRequest_.smio_snap_resp_cb = std::bind(&HybridTierCtrlr::snapTokenCb,
                                              this,
                                              std::placeholders::_1,
                                              std::placeholders::_2,
                                              std::placeholders::_3,
                                              std::placeholders::_4);

    moveTierRequest_.oidList.reserve(BATCH_SZ);
    moveTierRequest_.fromTier = diskio::DataTier::flashTier;
    moveTierRequest_.toTier = diskio::DataTier::diskTier;
    moveTierRequest_.relocate = true;
    moveTierRequest_.moveObjsRespCb = std::bind(&HybridTierCtrlr::moveObjsToTierCb,
                                                this,
                                                std::placeholders::_1,
                                                std::placeholders::_2);
    /* Start from -1 b/c moveToNexToken() increments */
    curTokenIdx_ = -1;
    BATCH_SZ = modProvider->get_fds_config()->\
                       get<uint32_t>("fds.sm.tiering.batchSz");
    FREQUENCY = modProvider->get_fds_config()->\
                       get<uint32_t>("fds.sm.tiering.frequency");
}

void HybridTierCtrlr::start()
{
    GLOGNOTIFY;

    fds_assert(runTask_.get() == nullptr);
    auto &timer = *(modProvider_->getTimer());
    runTask_.reset(
        boost::make_shared<FdsTimerFunctionTask>(
            timer,
            std::bind(&HybridTierCtrlr::moveToNextToken, this)));
    timer.schedule(runTask_, std::chrono::seconds(FREQUENCY));
}

void HybridTierCtrlr::run()
{
    threadpool_.schedule(&HybridTierCtrlr::moveToNextToken, this);
}

void HybridTierCtrlr::stop()
{
    fds_panic("Feature not implemented yet");

    GLOGNOTIFY;

    fds_assert(runTask_);
    modProvider_->getTimer()->cancel(runTask_);
    runTask_.reset();
}

void HybridTierCtrlr::moveToNextToken()
{
    curTokenIdx_++;

    if (curTokenIdx_ < tokenList_.size()) {
        GLOGDEBUG << "next token id: " << tokenList_[curTokenIdx_];
        /* Start moving objects for the next token */
        snapToken();
    } else {
        GLOGNOTIFY << "Completed processing all tokens.  Scheduling hybrid tier work again";
        /* Completed moving objects.  Schedule the next relocation task */
        curTokenIdx_ = -1;
        // TODO(Rao): Get from config
        timer.schedule(runTask_, std::chrono::seconds(10));
    }
}

void HybridTierCtrlr::snapToken()
{
    GLOGDEBUG << "token id: " << tokenList_[curTokenIdx_];

    fds_assert(tokenItr_.get() == nullptr);
    fds_assert(moveTierRequest_.oidList.empty() == true);

    snapRequest_.token_id = tokenList_[curTokenIdx_];
    err = storMgr_->enqueueMsg(FdsSysTaskQueueId, &snapRequest_);
    if (!err.ok()) {
        GLOGWARN << "Failed to enqueue snapshot request: err " << err;
        leveldb::ReadOptions options;
        snapTokenCb(err, &snapRequest_, options, nullptr);
        return;
    }
}

void HybridTierCtrlr::snapTokenCb(const Error& err,
                              SmIoSnapshotObjectDB* snapReq,
                              leveldb::ReadOptions& options,
                              leveldb::DB* db)
{
    GLOGDEBUG << "token id: " << tokenList_[curTokenIdx_];

    if (!err.ok()) {
        GLOGERROR << "Failed to take snap.  Err: "
            << err << " token id: " << tokenList_[curTokenIdx_];
        /* Move on to next token */
        threadpool_->schedule(&HybridTierCtrlr::moveToNextToken, this);
        return;
    }

    /* Initialize the tokenItr */
    tokenItr_.reset(new SMTokenItr());
    tokenItr_->itr = db->NewIterator(options);
    tokenItr_->itr->SeekToFirst();
    tokenItr_->db = db;
    tokenItr_->options = options;

    threadpool_->\
        schedule(&HybridTierCtrlr::constructTierMigrationList, this);
}

void HybridTierCtrlr::constructTierMigrationList()
{
    fds_assert(moveTierRequest_.oidList.empty() == true);

    ObjMetaData omd;

    /* Construct object list to update tier location from flash to disk */
    auto &itr = tokenItr_->itr;
    for (; itr->Valid() && tierMigrationList_.size() < BATCH_SZ;
         itr->Next()) {
        omd.deserializeFrom(itr->Value());
        if (omd.onFlashTier() && omd.getCreationTime() < hybridMoveTs_) {
            moveTierRequest_.oidList.push_back(ObjectID(itr->Key()));
        }
    }

    /* Send message to move the objects */
    err = storMgr_->enqueueMsg(FdsSysTaskQueueId, &moveTierRequest_);
    if (!err.ok()) {
        GLOGWARN << "Failed to enqueue move tier request: err " << err;
        leveldb::ReadOptions options;
        moveObjsToTierCb(err, &snapRequest_);
        return;
    }
}

void HybridTierCtrlr::moveObjsToTierCb(const Error& e,
                                        SmIoMoveObjsToTier *req)
{
    if (e != ERR_OK) {
        LOGWARN << "Failed to move some objects to disk from flash for token: "
            << tokenList_[curTokenIdx_];
        /* On error we still continue processing */
    }

    if (tokenItr_->itr->Valid()) {
        /* We haven't finished processing the current token..go back to building list */
        moveTierRequest_.oidList.clear();
        threadpool_->\
            schedule(&HybridTierCtrlr::constructTierMigrationList, this);
        return;
    }
    
    LOGDEBUG << "Moved objects for token: " << tokenList_[curTokenIdx_];

    /* Completed current token.  Release snapshot */
    tokenItr_->itr = nullptr;
    tokenItr_->db->ReleaseSnapshot(tokenItr_->options.snapshot);
    tokenItr_->db = nullptr;
    tokenItr_->done = true;

    /* Move on to next token */
    threadpool_->schedule(&HybridTierCtrlr::moveToNextToken(), this);
}
}  // namespace fds
