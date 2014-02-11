/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#include <concurrency/fds_actor.h>
#include <fds_process.h>

namespace fds {

FdsRequestQueueActor::FdsRequestQueueActor()
    : FdsRequestQueueActor(nullptr)
{
}

FdsRequestQueueActor::FdsRequestQueueActor(fds_threadpoolPtr threadpool)
    : lock_("FdsRequestQueueActor"),
      scheduled_(false)
{
    init(threadpool);
}

void FdsRequestQueueActor::init(fds_threadpoolPtr threadpool)
{
    fds_assert(threadpool_ == nullptr);
    threadpool_ = threadpool;
}

int FdsRequestQueueActor::get_queue_size()
{
    fds_spinlock::scoped_lock l(lock_);
    return queue_.size();
}

Error FdsRequestQueueActor::send_actor_request(FdsActorRequestPtr req)
{
    lock_.lock();
    queue_.push(req);
    if (!scheduled_) {
        scheduled_ = true;
        req->prev_owner_ = req->owner_;
        req->owner_ = this;
        threadpool_->schedule(&FdsRequestQueueActor::request_loop, this);
    }
    lock_.unlock();

    return ERR_OK;
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
            FDS_PLOG_WARN(g_fdslog) << " Request type: " << req->type
                    << " error: " << err;
        }
    }
}

}  // namespace fds
