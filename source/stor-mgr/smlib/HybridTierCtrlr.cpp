/*
 * Copyright 2015 Formation Data Systems, Inc.
 */
#include <string>
#include <fds_module_provider.h>
#include <fds_timer.h>
#include <concurrency/ThreadPool.h>
#include <ObjMeta.h>
#include <HybridTierCtrlr.h>
#include <concurrency/Mutex.h>

namespace fds {

uint32_t HybridTierCtrlr::BATCH_SZ = 1024;
uint32_t HybridTierCtrlr::FREQUENCY = 10;

// TODO(Rao):
// -Handle unowned token case.
// -Move runTask into ObjectStore
// -Start garbage collection for ssd
// -Expose stats for testing
HybridTierCtrlr::HybridTierCtrlr(SmIoReqHandler* storMgr,
                                 SmDiskMap::ptr diskMap)
    : featureEnabled(false),
      hybridTierLock("HybridTierLock")
{
    threadpool_ = MODULEPROVIDER()->proc_thrpool();
    storMgr_ = storMgr;
    diskMap_ = diskMap;

    BATCH_SZ = MODULEPROVIDER()->get_fds_config()->\
                       get<uint32_t>("fds.sm.tiering.hybrid.batchSz");
    FREQUENCY = MODULEPROVIDER()->get_fds_config()->\
                       get<uint32_t>("fds.sm.tiering.hybrid.frequency");

    state_ = HTC_STOPPED;
    snapRequest_.io_type = FDS_SM_SNAPSHOT_TOKEN;
    snapRequest_.smio_snap_resp_cb = std::bind(&HybridTierCtrlr::snapTokenCb,
                                              this,
                                              std::placeholders::_1,
                                              std::placeholders::_2,
                                              std::placeholders::_3,
                                              std::placeholders::_4);
    auto &timer = *(MODULEPROVIDER()->getTimer());
    runTask_.reset(
        new FdsTimerFunctionTask(
            std::bind(&HybridTierCtrlr::run, this)));
}
void
HybridTierCtrlr::enableFeature()
{
    fds_mutex::scoped_lock l(hybridTierLock);
    featureEnabled = true;
}

void HybridTierCtrlr::start(bool manual)
{

    GLOGNOTIFY << "Starting hybrid tiering:  manual=" << manual;

    fds_mutex::scoped_lock l(hybridTierLock);
    /* Check if hybrid tiering is enabled or not.
     * Allow manual start even if the feature is disabled.
     */
    if (!featureEnabled && !manual) {
        GLOGNOTIFY << "Failed to starting hybrid tiering: enabled=" << featureEnabled << ", manual=" << manual;
        return;
    }

    auto nextScheduleInSecs = FREQUENCY;

    if (manual) {
        if (state_ == HTC_READY || state_ == HTC_INPROGRESS) {
            GLOGNOTIFY << "Already in progress...ignoring manual start";
            return;
        }
        if (state_ == HTC_SCHEDULED) {
            MODULEPROVIDER()->getTimer()->cancel(runTask_);
        }
        /* Schedule in the next 1 second */
        nextScheduleInSecs = 1;
    } else {
        /* First time starting hybrid tier */
        if (HTC_STOPPED == state_) {
            scheduleNextRun_(nextScheduleInSecs);
            GLOGNOTIFY << "Scheuduling Hybrid Tier Policy";
        }
    }

}

void HybridTierCtrlr::scheduleNextRun_(uint32_t nextRunInSeconds)
{
    /* Schedule run() on timer */
    state_ = HTC_SCHEDULED;
    MODULEPROVIDER()->getTimer()->\
        schedule(runTask_, std::chrono::seconds(nextRunInSeconds));
}

void HybridTierCtrlr::run()
{
    fds_assert(tokenSet_.empty() &&
               tokenItr_.get() == nullptr &&
               state_ == HTC_SCHEDULED);

    fds_mutex::scoped_lock l(hybridTierLock);
    state_ = HTC_READY;
    hybridMoveTs_ = util::getTimeStampSeconds() - FREQUENCY;
    tokenSet_ = diskMap_->getSmTokens();
    threadpool_->schedule(&HybridTierCtrlr::moveToNextToken, this);

    GLOGNOTIFY << "Move objects older than ts: " << hybridMoveTs_
        << " token cnt: " << tokenSet_.size();

}

void HybridTierCtrlr::stop()
{
    fds_panic("Feature not implemented yet");

    GLOGNOTIFY;

    MODULEPROVIDER()->getTimer()->cancel(runTask_);
}

void HybridTierCtrlr::moveToNextToken()
{
    fds_mutex::scoped_lock l(hybridTierLock);

    if (state_ == HTC_READY) {
        /* We are at first token..don't increment nextToken_ */
        state_ = HTC_INPROGRESS;
        nextToken_ = tokenSet_.begin();
    } else {
        fds_assert(state_ == HTC_INPROGRESS);
        nextToken_++;
    }

    if (nextToken_ != tokenSet_.end()) {
        GLOGDEBUG << "next token id: " << *nextToken_;
        /* Start moving objects for the next token.  First we take a snap */
        snapToken();
    } else {
        GLOGNOTIFY << "Completed processing all tokens. Scheduling hybrid tier work again";
        /* Completed moving objects.  Schedule the next relocation task */
        tokenSet_.clear();
        scheduleNextRun_(FREQUENCY);
    }
}

void HybridTierCtrlr::snapToken()
{
    GLOGDEBUG << "token id: " << *nextToken_;

    fds_assert(tokenItr_.get() == nullptr);

    snapRequest_.token_id = *nextToken_;
    Error err = storMgr_->enqueueMsg(FdsSysTaskQueueId, &snapRequest_);
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
                              std::shared_ptr<leveldb::DB> db)
{
    GLOGDEBUG << "token id: " << *nextToken_;

    if (!err.ok()) {
        GLOGERROR << "Failed to take snap.  Err: "
            << err << " token id: " << *nextToken_;
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
    initMoveTierRequest_();

    ObjMetaData omd;

    /* Construct object list to update tier location from flash to disk */
    auto &itr = tokenItr_->itr;
    for (; itr->Valid() && moveTierRequest_->oidList.size() < BATCH_SZ;
         itr->Next()) {
        omd.deserializeFrom(itr->value());
        if (omd.onFlashTier() && omd.getCreationTime() < hybridMoveTs_) {
            moveTierRequest_->oidList.push_back(ObjectID(itr->key().ToString()));
        }
    }

    /* Send message to move the objects */
    Error err = storMgr_->enqueueMsg(FdsSysTaskQueueId, moveTierRequest_);
    if (!err.ok()) {
        GLOGWARN << "Failed to enqueue move tier request: err " << err;
        leveldb::ReadOptions options;
        moveObjsToTierCb(err, moveTierRequest_);
        return;
    }
}

void HybridTierCtrlr::moveObjsToTierCb(const Error& e,
                                        SmIoMoveObjsToTier *req)
{
    if (e != ERR_OK) {
        LOGWARN << "Failed to move some objects to disk from flash for token: "
            << *nextToken_;
        /* On error we still continue processing */
    } else {
        LOGDEBUG << "Moved " << req->movedCnt << " objects for token: " << *nextToken_;
    }

    if (tokenItr_->itr->Valid()) {
        /* We haven't finished processing the current token..go back to building list */
        threadpool_->\
            schedule(&HybridTierCtrlr::constructTierMigrationList, this);
        return;
    }

    LOGDEBUG << "Completed moving objects for token: " << *nextToken_;

    /* Completed current token.  Release snapshot */
    tokenItr_->db->ReleaseSnapshot(tokenItr_->options.snapshot);
    tokenItr_.reset();

    /* Move on to next token */
    threadpool_->schedule(&HybridTierCtrlr::moveToNextToken, this);
}

void HybridTierCtrlr::initMoveTierRequest_()
{
    moveTierRequest_ = new SmIoMoveObjsToTier();
    moveTierRequest_->io_type = FDS_SM_TIER_PROMOTE_OBJECTS;
    moveTierRequest_->oidList.reserve(BATCH_SZ);
    moveTierRequest_->fromTier = diskio::DataTier::flashTier;
    moveTierRequest_->toTier = diskio::DataTier::diskTier;
    moveTierRequest_->relocate = true;
    moveTierRequest_->moveObjsRespCb = std::bind(&HybridTierCtrlr::moveObjsToTierCb,
                                                this,
                                                std::placeholders::_1,
                                                std::placeholders::_2);
}
}  // namespace fds
