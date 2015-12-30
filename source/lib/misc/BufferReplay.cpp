/*
 * Copyright 2015 Formation Data Systems, Inc.
 */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <concurrency/ThreadPool.h>
#include <BufferReplay.h>

namespace fds {

BufferReplay::BufferReplay(const std::string &bufferFileName,
                           int32_t maxReplayCnt,
                           fds_threadpool *threadpool)
{
    threadpool_ = threadpool;
    bufferFileName_ = bufferFileName;
    writefd_ = -1;
    nBufferedOps_ = 0;
    nReplayOpsIssued_ = 0;
    nOutstandingReplayOps_ = 0;
    maxReplayCnt_ = maxReplayCnt;
    progress_ = ABORTED;
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
    {
        fds_mutex::scoped_lock l(lock_);
        progress_ = ABORTED;
    }
    progressCb_(ABORTED);
}

Error BufferReplay::buffer(const BufferReplay::Op &op)
{
    fds_mutex::scoped_lock l(lock_);
    if (progress_ == ABORTED) {
        return ERR_ABORTED;
    } else if (progress_ == REPLAY_CAUGHTUP || progress_ == COMPLETE) {
        return ERR_UNAVAILABLE;
    }

    serializer_->writeI32(op.first);
    auto ret = serializer_->writeString(*(op.second));
    if (ret < op.second->size()) {
        progress_  = ABORTED;
        return ERR_DISK_WRITE_FAILED;
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
    threadpool_->schedule(&BufferReplay::replayWork_, this);
}

BufferReplay::Progress BufferReplay::getProgress()
{
    fds_mutex::scoped_lock l(lock_);
    return progress_;
}

void BufferReplay::notifyOpsReplayed(int32_t cnt)
{
    {
        fds_mutex::scoped_lock l(lock_);
        if (progress_ == ABORTED) return;
        fds_assert(progress_ == BUFFERING_REPLAYING || progress_  == REPLAY_CAUGHTUP);
        nOutstandingReplayOps_ -= cnt;
        fds_assert(nOutstandingReplayOps_ >= 0);
        if (nOutstandingReplayOps_ == 0 &&
            progress_ == REPLAY_CAUGHTUP) {
            progress_ = COMPLETE;
        }
    }

    if (nOutstandingReplayOps_ == 0) {  // lock protection not required
        if (progress_ == COMPLETE) {
           progressCb_(COMPLETE); 
           return;
        }
        /* schedule next batch for replay */
        threadpool_->schedule(&BufferReplay::replayWork_, this);
    }
}

void BufferReplay::replayWork_()
{
    Error err(ERR_OK);
    int64_t replayIdx;
    std::list<Op> replayList;
    {
        /* Read few entries */
        fds_mutex::scoped_lock l(lock_);
        if (progress_ == ABORTED) return;
        while (nReplayOpsIssued_ + static_cast<int32_t>(replayList.size()) < nBufferedOps_ &&
               static_cast<int32_t>(replayList.size()) < maxReplayCnt_) {
            OpType opType;
            StringPtr s = MAKE_SHARED<std::string>();

            deserializer_->readI32(opType);
            auto ret = deserializer_->readString(*s);
            if (ret < 0) {
                GLOGWARN << "Failed to deserialize: " << bufferFileName_;
                fds_assert(!"Failed to deserialize");
                err = ERR_DISK_READ_FAILED;
                progress_ = ABORTED;
                break;
            }
            replayList.emplace_back(std::make_pair(opType, std::move(s)));
        }
        fds_assert(nOutstandingReplayOps_  == 0);
        replayIdx = nReplayOpsIssued_;
        nOutstandingReplayOps_ = replayList.size();
        nReplayOpsIssued_ += nOutstandingReplayOps_;
    }

    if (!err.ok()) {
        progressCb_(ABORTED);
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

}  // namespace fds
