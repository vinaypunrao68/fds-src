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
 * - Both reads and writes to the buffer file are protected by lock
 * - Replays operations from the file in the order they were buffered
 * - Operations are read in a batch and sent for replay.  There can only be one batch being
 *   replayed.  Once the all the ops in batch have been acked back we read another
 *   batch.
 * - Once replay is started further bufffering is only allowed until replay catches up
 *   with buffered ops.  Once replay is caught up futher buffering calls will result
 *   in ERR_UNAVAILABLE
 */
struct BufferReplay {
    using OpType = int32_t;
    using Op = std::pair<OpType, StringPtr>;
    enum Progress {
        BUFFERING,
        BUFFERING_REPLAYING,
        REPLAY_CAUGHTUP,
        ABORTED,
        COMPLETE,
    };
    static const constexpr char* const progressStr[] = 
               { "BUFFERING", "BUFFERING_REPLAYING", "REPLAY_CAUGHTUP", "ABORTED","COMPLETE" };
    using ProgressCb = std::function<void (Progress status)>;
    using ReplayCb = std::function<void (int64_t, std::list<Op>&)>;

    explicit BufferReplay(const std::string &bufferFileName,
                          int32_t maxReplayCnt,
                          fds_threadpool *threadpool);
    virtual ~BufferReplay();
    /**
    * @brief Initialize reader and writer
    *
    * @return 
    */
    Error init();

    /**
    * @brief Abort buffer replay.  Client will get a callback once all the pending
    * operations have completed indicating it's safe to delete BufferReplay
    * object.
    */
    void abort();

    /**
    * @brief Buffers the operation
    *
    * @param op
    *
    * @return 
    */
    Error buffer(const Op &op);

    /**
    * @brief To Initiate replay.  Only call this once.
    */
    void startReplay();

    /**
    * @brief After replay op[s] have been applied client should call this function
    * once all the replay ops have been acked back via this call, next batch of
    * replay ops are read
    *
    * @param cnt
    */
    void notifyOpsReplayed(int32_t cnt = 1);


    /**
    * @brief To query progress of replay
    *
    * @return 
    */
    Progress getProgress();

    /**
    * @brief As BufferReplay object goes through different states, this callback is 
    * invoked.  The important progress steps for which the callback is invoked are:
    * REPLAY_CAUGHTUP - Replay has caught up with buffering
    * ABORTED - Completed due to an error / explicit abort
    * COMPLETE - When Replay completed successfully
    *
    * @param cb
    */
    inline void setProgressCb(const ProgressCb& cb) {
        progressCb_ = cb;
    }

    /**
    * @brief 
    *
    * @param cb
    */
    inline void setReplayOpsCb(const ReplayCb& cb) {
        replayOpsCb_ = cb;
    }

    /**
    * @brief Return current outstanding replay ops
    *
    * @return 
    */
    int32_t getOutstandingReplayOpsCnt();

    std::string logString() const;

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
    bool                                        replayWorkPosted_;
    int32_t                                     maxReplayCnt_;
    ProgressCb                                  progressCb_;
    ReplayCb                                    replayOpsCb_;
    Progress                                    progress_;
};
} // namespace fds

#endif  // SOURCE_INCLUDE_BUFFERREPLAY_H_
