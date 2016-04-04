/*
 * Copyright 2015 Formation Data Systems, Inc.
 */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <concurrency/ThreadPool.h>
#include <BufferReplay.h>

namespace fds {

constexpr const char* const BufferReplay::progressStr[];

BufferReplay::BufferReplay(const std::string &bufferFileName,
                           int32_t maxReplayCnt,
                           int64_t affinity,
                           fds_threadpool *threadpool)
{
    threadpool_ = threadpool;
    bufferFileName_ = bufferFileName;
    writefd_ = -1;
    startingOpId_ = -1;
    nBufferedOps_ = 0;
    nReplayOpsIssued_ = 0;
    nOutstandingReplayOps_ = 0;
    maxReplayCnt_ = maxReplayCnt;
    progress_ = IDLE;
}

BufferReplay::~BufferReplay()
{
    if (writefd_ >= 0) {
        close(writefd_);
    }
}

Error BufferReplay::init()
{
    fds_mutex::scoped_lock l(lock_);
    writefd_ = open(bufferFileName_.c_str(), O_WRONLY | O_CREAT | O_TRUNC,
               S_IRUSR | S_IWUSR | S_IRGRP);
    if (writefd_ < 0) {
        LOGERROR << "Failed to open/ create file: '" << bufferFileName_ << "' errno: '"
                << errno << "'";
        return ERR_DISK_WRITE_FAILED;
    }
    /* Using fd based because by default thrift opens in append mode */
    serializer_.reset(serialize::getFileSerializer(writefd_));
    if (!serializer_) {
        GLOGWARN << "Failed to init serializer at: " << bufferFileName_;
        return ERR_INVALID;
    }

    deserializer_.reset(serialize::getFileDeserializer(bufferFileName_));
    if (!deserializer_) {
        GLOGWARN << "Failed to init deserializer at: " << bufferFileName_;
        return ERR_INVALID;
    }

    progress_ = BUFFERING;

    return ERR_OK;
}

void BufferReplay::abort()
{
    bool doCb = false;
    {
        fds_mutex::scoped_lock l(lock_);
        doCb = (progress_ == BUFFERING);
        progress_ = ABORTED;
    }
    /* Invoke progressCb_ from  here only if replay hasn't started.
     * Once replay starts all progress callbacks must be done
     * in replayWork_
     */
    if (doCb) progressCb_(ABORTED);
}

Error BufferReplay::buffer(const BufferReplay::Op &op)
{
    fds_mutex::scoped_lock l(lock_);
    if (progress_ == ABORTED) {
        return ERR_ABORTED;
    } else if (progress_ == REPLAY_CAUGHTUP || progress_ == COMPLETE) {
        return ERR_UNAVAILABLE;
    }

    serializer_->writeI64(op.opId);
    serializer_->writeI32(op.type);
    auto ret = serializer_->writeString(*(op.payload));
    if (ret < op.payload->size()) {
        progress_  = ABORTED;
        return ERR_DISK_WRITE_FAILED;
    }
    if (nBufferedOps_ == 0) {
        startingOpId_ = op.opId;
    }
    ++nBufferedOps_;
    return ERR_OK;
}

void BufferReplay::startReplay()
{
    {
        fds_mutex::scoped_lock l(lock_);
        if (progress_ == ABORTED) return;
        progress_ = BUFFERING_REPLAYING;

        replayWorkPosted_  = true;
    }
    threadpool_->scheduleWithAffinity(affinity_, &BufferReplay::replayWork_, this);
}

BufferReplay::Progress BufferReplay::getProgress()
{
    fds_mutex::scoped_lock l(lock_);
    return progress_;
}

/**
* @brief Client will call this function to notify cnt # of ops have been replayed.
* Once all the outstanding ops have been replayed, replayWork_() will be
* scheduled to send more ops for replay.
* NOTE: This function is mutually exclusive with replayWork_()
*
* @param cnt # of ops that have been replayed.
*/
void BufferReplay::notifyOpsReplayed(int32_t cnt)
{
    do {
        fds_mutex::scoped_lock l(lock_);
        /* Do the accounting of outstanding ops even if abort is issued */
        nOutstandingReplayOps_ -= cnt;
        
        fds_assert(!replayWorkPosted_);
        fds_assert(nOutstandingReplayOps_ >= 0);
        
        fds_assert(progress_ == BUFFERING_REPLAYING ||
                   progress_  == REPLAY_CAUGHTUP ||
                   progress_ == ABORTED);
        if (nOutstandingReplayOps_ == 0) {
            replayWorkPosted_ = true;
        }
    } while (false);

    if (replayWorkPosted_) {        // Ok to read out side lock
        threadpool_->scheduleWithAffinity(affinity_, &BufferReplay::replayWork_, this);
    }
}

int32_t BufferReplay::getOutstandingReplayOpsCnt()
{
    fds_mutex::scoped_lock l(lock_);
    return nOutstandingReplayOps_;
}

/**
* @brief Ops are read from file and sent out for replay
* Once replay starts all progress_ transitions (abort is special) are performed here.
* Once replay starts all progress callbacks are done here as well.
* NOTE: This function is mutually exclusive with notifyOpsReplayed()
*/
void BufferReplay::replayWork_()
{
    Error err(ERR_OK);
    int64_t replayIdx = -1;
    std::list<Op> replayList;
    bool complete = false;
    do {
        fds_mutex::scoped_lock l(lock_);

        /* replayWork_() and notifyOpsReplayed() are mutually exclusive */ 
        fds_assert(nOutstandingReplayOps_  == 0);
        fds_assert(replayWorkPosted_);
        fds_assert(progress_ == BUFFERING_REPLAYING ||
                   progress_  == REPLAY_CAUGHTUP ||
                   progress_ == ABORTED);

        replayWorkPosted_ = false;

        /* It's possible we received an abort while replayWork_ is schedule on threadpool */
        if (progress_ == ABORTED)  {
            err = ERR_ABORTED;
            break;
        }

        if (nBufferedOps_ == nReplayOpsIssued_) {
            /* Nothing more read and replay */
            progress_ = COMPLETE;
            complete = true;
            break;
        }

        /* Read few entries */
        while (nReplayOpsIssued_ + static_cast<int32_t>(replayList.size()) < nBufferedOps_ &&
               static_cast<int32_t>(replayList.size()) < maxReplayCnt_) {
            Op op;
            op.payload = MAKE_SHARED<std::string>();

            deserializer_->readI64(op.opId);
            deserializer_->readI32(op.type);
            auto ret = deserializer_->readString(*(op.payload));
            if (ret < 0) {
                GLOGWARN << "Failed to deserialize: " << bufferFileName_;
                fds_assert(!"Failed to deserialize");
                err = ERR_DISK_READ_FAILED;
                progress_ = ABORTED;
                replayList.clear();
                break;
            }
            replayList.emplace_back(op);
        }
        /* Op accounting */
        replayIdx = nReplayOpsIssued_;
        nOutstandingReplayOps_ = replayList.size();
        nReplayOpsIssued_ += nOutstandingReplayOps_;
    } while (false);

    if (!err.ok()) {
        progressCb_(ABORTED);
        return;
    } else if (complete) {
        progressCb_(COMPLETE);
        return;
    }

    /* Send for replay */
    replayOpsCb_(replayIdx, replayList);
    
    /* Check if replay's been caughtup with buffering.  Once replay's been caught up
     * we disallow further buffering
     */
    bool replayCaughtup = false;
    {
        fds_mutex::scoped_lock l(lock_);
        if (progress_ == ABORTED) return;
        if (nReplayOpsIssued_ == nBufferedOps_) {
            progress_ = REPLAY_CAUGHTUP;
            replayCaughtup = true;
        }
    }

    if (replayCaughtup) {
        progressCb_(REPLAY_CAUGHTUP);
    }
}

std::string BufferReplay::logString() const
{
    std::stringstream ss;
    ss << " BufferReplay progress:  " << progressStr[static_cast<int>(progress_)]
        << "range: [" << startingOpId_ << "," << startingOpId_ + nBufferedOps_ - 1 << "]"
        << " nBuffered: " << nBufferedOps_
        << " nReplayOpsIssued: " << nReplayOpsIssued_
        << " nOutstandingReplayOps: " << nOutstandingReplayOps_;
    return ss.str();
}

}  // namespace fds
