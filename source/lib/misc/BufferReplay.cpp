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
        progress_ = ABORTED;
        doCb = (nOutstandingReplayOps_ == 0);
    }
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
    }
    replayWorkPosted_  = true;      // lock protection isn't required
    threadpool_->schedule(&BufferReplay::replayWork_, this);
}

BufferReplay::Progress BufferReplay::getProgress()
{
    fds_mutex::scoped_lock l(lock_);
    return progress_;
}

void BufferReplay::notifyOpsReplayed(int32_t cnt)
{
    do {
        fds_mutex::scoped_lock l(lock_);
        nOutstandingReplayOps_ -= cnt;
        fds_assert(nOutstandingReplayOps_ >= 0);
        
        if (progress_ == ABORTED) break;

        fds_assert(progress_ == BUFFERING_REPLAYING || progress_  == REPLAY_CAUGHTUP);
        if (nOutstandingReplayOps_ == 0 &&
            progress_ == REPLAY_CAUGHTUP) {
            /* Replayed all the ops */
            progress_ = COMPLETE;
        }
    } while (false);

    /* Do any necessary callbacks outside the lock
     * When we ABORTED/COMPLETED notify the client so that appropriate cleanups
     * can take place 
     */
    if (nOutstandingReplayOps_ == 0) {  // lock protection not required
        if (progress_ == ABORTED) {
           progressCb_(ABORTED); 
           return;
        } else if (progress_ == COMPLETE) {
           progressCb_(COMPLETE); 
           return;
        }
        /* Still have more to replay.  schedule next batch for replay */
        replayWorkPosted_ = true;           // lock protection not required
        threadpool_->schedule(&BufferReplay::replayWork_, this);
    }
}

int32_t BufferReplay::getOutstandingReplayOpsCnt()
{
    fds_mutex::scoped_lock l(lock_);
    return nOutstandingReplayOps_;
}

void BufferReplay::replayWork_()
{
    Error err(ERR_OK);
    int64_t replayIdx = -1;
    std::list<Op> replayList;
    bool complete = false;
    do {
        fds_mutex::scoped_lock l(lock_);

        fds_assert(nOutstandingReplayOps_  == 0);

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
        fds_assert(nOutstandingReplayOps_ == 0);
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
    {
        fds_mutex::scoped_lock l(lock_);
        if (progress_ == ABORTED) return;
        if (nReplayOpsIssued_ == nBufferedOps_) {
            progress_ = REPLAY_CAUGHTUP;
        }
    }

    if (progress_ == REPLAY_CAUGHTUP) {
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
