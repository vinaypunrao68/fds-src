/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#ifndef SOURCE_UTIL_ACTOR_H_
#define SOURCE_UTIL_ACTOR_H_

#include <fds_err.h>
#include <concurrency/fds_actor_request.h>
#include <concurrency/ThreadPool.h>
#include <concurrency/Mutex.h>

namespace fds {
class FdsActor {
public:
    virtual ~FdsActor() {}
    virtual Error send_actor_request(FdsActorRequestPtr req) = 0;
    virtual Error handle_actor_request(FdsActorRequestPtr req) = 0;
    virtual std::string log_string() {
        return "FdsActor";
    }
};

class FdsRequestQueueActor : public FdsActor {
public:
    FdsRequestQueueActor(fds_threadpoolPtr threadpool);
    virtual Error send_actor_request(FdsActorRequestPtr req) override;
    virtual std::string log_string() {
        return "FdsRequestQueueActor";
    }

protected:
    virtual void request_loop();

    fds_threadpoolPtr threadpool_;
    fds_spinlock lock_;
    bool scheduled_;
    FdsActorRequestQueue queue_;
};
}  // namespace fds

#endif  // SOURCE_UTIL_ACTOR_H_
