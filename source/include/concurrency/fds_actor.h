/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#ifndef SOURCE_UTIL_ACTOR_H_
#define SOURCE_UTIL_ACTOR_H_

#include <string>
#include <boost/shared_ptr.hpp>
#include <atomic>
#include <memory>
#include <fds_error.h>
#include <concurrency/fds_actor_request.h>
#include <concurrency/ThreadPool.h>
#include <concurrency/Mutex.h>

namespace fds {
class FdsActor {
public:
    FdsActor(const std::string &id, FdsActor *parent);
    virtual ~FdsActor() {}
    virtual Error send_actor_request(FdsActorRequestPtr req) = 0;
    virtual Error handle_actor_request(FdsActorRequestPtr req) = 0;
    virtual int get_queue_size() = 0;
    virtual std::string log_string() {
        return "FdsActor";
    }
protected:
    std::string id_;
    FdsActor *parent_;
    bool shutdown_issued_;
};

typedef boost::shared_ptr<FdsActor> FdsActorPtr;
typedef std::unique_ptr<FdsActor> FdsActorUPtr;

class FdsRequestQueueActor : public FdsActor {
public:
    FdsRequestQueueActor(const std::string &id, FdsActor *parent);
    FdsRequestQueueActor(const std::string &id, FdsActor *parent,
            fds_threadpoolPtr threadpool);
    ~FdsRequestQueueActor();
    void init(fds_threadpoolPtr threadpool);
    virtual Error send_actor_request(FdsActorRequestPtr req) override;
    virtual int get_queue_size() override;
    virtual std::string log_string() override {
        return "FdsRequestQueueActor";
    }

protected:
    virtual void request_loop();

    fds_threadpoolPtr threadpool_;
    fds_mutex lock_;
    bool scheduled_;
    // TODO(rao):  Change to lockfree queue
    FdsActorRequestQueue queue_;
    DBG(std::atomic<int> sched_cnt_);
};
}  // namespace fds

#endif  // SOURCE_UTIL_ACTOR_H_
