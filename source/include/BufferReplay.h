/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

#ifndef SOURCE_INCLUDE_BUFFERREPLAY_H_
#define SOURCE_INCLUDE_BUFFERREPLAY_H_

#include <memory>
#include <concurrency/Mutex.h>
#include <serialize.h>

namespace fds {
class fds_threadpool;
using StringPtr = boost::shared_ptr<std::string>;

/* Buffer Replay module
 * - Buffers operations to a file
 * - Replays operations from the file in the order they were buffered
 * - Once replay is started further bufffering is only allowed until replay catched up
 *   with buffered ops.  Once replay is caught up futher buffering calls will result
 *   in ERR_UNAVAILABLE
 * - Both reads and writes to the buffer file are protected by lock
 */
struct BufferReplay {
    enum Progress {
        BUFFERING,
        BUFFERING_REPLAYING,
        REPLAY_CAUGHTUP,
        ABORTED,
        COMPLETE,
    };
    using ProgressCb = std::function<void (Progress status)>;
    using ReplayCb = std::function<void (int64_t, std::list<StringPtr>&)>;

    explicit BufferReplay(const std::string &bufferFileName,
                          int32_t maxReplayCnt,
                          fds_threadpool *threadpool);
    virtual ~BufferReplay();
    Error init();
    void abort();
    Error buffer(const StringPtr &s);
    void startReplay();
    void notifyOpsReplayed(int32_t cnt = 1);
    Progress getProgress();
    inline void setProgressCb(const ProgressCb& cb) {
        progressCb_ = cb;
    }
    inline void setReplayOpsCb(const ReplayCb& cb) {
        replayOpsCb_ = cb;
    }
 protected:
    void replayWork_();

    fds_mutex                                   lock_;
    fds_threadpool                              *threadpool_;
    std::string                                 bufferFileName_;
    int32_t                                     writefd_;
    std::unique_ptr<serialize::Serializer>      serializer_;
    std::unique_ptr<serialize::Deserializer>    deserializer_;
    int64_t                                     nBufferedOps_;
    int64_t                                     nReplayOpsIssued_;
    int32_t                                     nOutstandingReplayOps_;
    int32_t                                     maxReplayCnt_;
    ProgressCb                                  progressCb_;
    ReplayCb                                    replayOpsCb_;
    Progress                                    progress_;
};
} // namespace fds

#endif  // SOURCE_INCLUDE_BUFFERREPLAY_H_
