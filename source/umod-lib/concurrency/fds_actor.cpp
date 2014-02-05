/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#include <concurrency/fds_actor.h>
#include <fds_process.h>

namespace fds {

FdsRequestQueueActor::FdsRequestQueueActor(fds_threadpoolPtr threadpool)
    : threadpool_(threadpool),
      lock_("FdsRequestQueueActor"),
      scheduled_(false)
{
}

Error FdsRequestQueueActor::send_actor_request(FdsActorRequestPtr req) {
    lock_.lock();
    queue_.push(req);
    if (!scheduled_) {
        scheduled_ = true;
        req->prev_owner_ = req->owner_;
        req->owner_ = this;
        threadpool_->schedule(&FdsRequestQueueActor::request_loop, this);
    }
    lock_.unlock();
}

void FdsRequestQueueActor::request_loop() {
    while (true) {
        FdsActorRequestPtr req;
        {
            fds_spinlock::scoped_lock l(lock_);
            fds_assert(scheduled_ == true);
            if (queue_.empty()) {
                scheduled_ = false;
                break;
            }
            req = queue_.front();
            queue_.pop();
        }

        Error err = this->handle_actor_request(req);
        if (err != ERR_OK) {
            fds_verify(!"Request resulted in an error");
            FDS_PLOG_WARN(g_fdslog) << " Request type: " << req->type << " error: " << err;
        }

        // TODO(Rao): We may starve other threads when queue_ is busy. We should
        // put a limit on number of requests that we handle
    }
}

}  // namespace fds
