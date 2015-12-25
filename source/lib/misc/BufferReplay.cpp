/*
 * Copyright 2015 Formation Data Systems, Inc.
 */
#include <concurrency/ThreadPool.h>
#include <BufferReplay.h>

namespace fds {

BufferReplay::BufferReplay(const std::string &bufferFileName,
                           int32_t maxReplayCnt,
                           fds_threadpool *threadpool)
{
    threadpool_ = threadpool;
    bufferFileName_ = bufferFileName;
    nBufferedOps_ = 0;
    nReplayOpsIssued_ = 0;
    nOutstandingReplayOps_ = 0;
    maxReplayCnt_ = maxReplayCnt;
    progress_ = ABORTED;
}

BufferReplay::~BufferReplay()
{
}

Error BufferReplay::init()
{
    serializer_.reset(serialize::getFileSerializer(bufferFileName_));
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
        fds_mutex::scoped_lock l(bufferLock_);
        progress_ = ABORTED;
    }
    progressCb_(ABORTED);
}

Error BufferReplay::buffer(const StringPtr &s)
{
    fds_mutex::scoped_lock l(bufferLock_);
    if (progress_ == ABORTED) {
        return ERR_ABORTED;
    } else if (progress_ == REPLAY_CAUGHTUP) {
        return ERR_UNAVAILABLE;
    }

    auto ret = serializer_->writeString(*s);
    if (ret < s->size()) {
        progress_  = ABORTED;
        return ERR_DISK_WRITE_FAILED;
    }
    ++nBufferedOps_;
    return ERR_OK;
}

void BufferReplay::startReplay()
{
    {
        fds_mutex::scoped_lock l(bufferLock_);
        progress_ = BUFFERING_REPLAYING;
    }
    threadpool_->schedule(&BufferReplay::replayWork_, this);
}

BufferReplay::Progress BufferReplay::getProgress()
{
    fds_mutex::scoped_lock l(bufferLock_);
    return progress_;
}

void BufferReplay::notifyOpReplayed()
{
    {
        fds_mutex::scoped_lock l(bufferLock_);
        if (progress_ == ABORTED) {
            return;
        }
        fds_assert(nOutstandingReplayOps_ > 0);
        fds_assert(progress_ == BUFFERING_REPLAYING || progress_  == REPLAY_CAUGHTUP);
        nOutstandingReplayOps_--;
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

    std::list<StringPtr> replayList;
    {
        /* Read few entries */
        fds_mutex::scoped_lock l(bufferLock_);
        if (progress_ == ABORTED) return;
        while (nReplayOpsIssued_ + static_cast<int32_t>(replayList.size()) < nBufferedOps_ &&
               static_cast<int32_t>(replayList.size()) < maxReplayCnt_) {
            StringPtr s = MAKE_SHARED<std::string>();
            auto ret = deserializer_->readString(*s);
            if (ret < 0) {
                GLOGWARN << "Failed to deserialize: " << bufferFileName_;
                fds_assert(!"Failed to deserialize");
                err = ERR_DISK_READ_FAILED;
                progress_ = ABORTED;
                break;
            }
            replayList.emplace_back(std::move(s));
        }
        fds_assert(nOutstandingReplayOps_  == 0);
        nOutstandingReplayOps_ = replayList.size();
        nReplayOpsIssued_ += nOutstandingReplayOps_;
    }

    if (!err.ok()) {
        progressCb_(ABORTED);
        return;
    }

    /* Send for replay */
    replayOpsCb_(replayList);
    
    /* Check if replay's been caughtup with buffering.  Once replay's been caught up
     * we disallow further buffering
     */
    {
        fds_mutex::scoped_lock l(bufferLock_);
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
