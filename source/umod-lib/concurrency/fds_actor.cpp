/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#include <string>
#include <concurrency/fds_actor.h>
#include <fds_process.h>

namespace fds {

FdsActor::FdsActor(const std::string &id, FdsActor *parent)
{
    id_ = id;
    parent_ = parent;
    shutdown_issued_ = false;
}

FdsRequestQueueActor::FdsRequestQueueActor(const std::string &id,
        FdsActor *parent)
    : FdsRequestQueueActor(id, parent, nullptr)
{
}

FdsRequestQueueActor::FdsRequestQueueActor(const std::string &id,
        FdsActor *parent,
        fds_threadpoolPtr threadpool)
    : FdsActor(id, parent),
      lock_("FdsRequestQueueActor"),
      scheduled_(false)
{
    init(threadpool);
}

FdsRequestQueueActor::~FdsRequestQueueActor()
{
    fds_assert(scheduled_ == false);
    fds_assert(queue_.size() == 0);
}

void FdsRequestQueueActor::init(fds_threadpoolPtr threadpool)
{
    fds_assert(threadpool_ == nullptr);
    threadpool_ = threadpool;
}

int FdsRequestQueueActor::get_queue_size()
{
    fds_mutex::scoped_lock l(lock_);
    return queue_.size();
}

Error FdsRequestQueueActor::send_actor_request(FdsActorRequestPtr req)
{
    fds_mutex::scoped_lock l(lock_);

    /* Shutdown check.  We will not accept any more requests after
     * shutdown has been issued
     */
    if (shutdown_issued_) {
        fds_assert(!"actor already shutdown");
        LOGWARN << "Sending request: " << req->type << " When actor: "
                << id_ << " was already shutdown";
        return ERR_FAR_SHUTDOWN;
    }
    if (req->type == FAR_ID(FdsActorShutdown)) {
        shutdown_issued_ = true;
    }

    queue_.push(req);
    if (!scheduled_) {
        scheduled_ = true;
        req->prev_owner_ = req->owner_;
        req->owner_ = this;
        DBG(sched_cnt_++);
        threadpool_->schedule(&FdsRequestQueueActor::request_loop, this);
    }

    return ERR_OK;
}

void FdsRequestQueueActor::request_loop() {
    DBG(sched_cnt_--);
    bool notify_shutdown = false;

    while (true) {
        FdsActorRequestPtr req;
        {
            fds_mutex::scoped_lock l(lock_);
            fds_assert(scheduled_ == true);
            if (queue_.empty()) {
                scheduled_ = false;
                break;
            }
            req = queue_.front();
            queue_.pop();

            /* As part of shutting down we will notify the parent so that
             * parent can clean up any state
             */
            if (req->type == FAR_ID(FdsActorShutdown)) {
                LOGNORMAL << "Shutting down actor " << id_;
                fds_assert(queue_.size() == 0);
                scheduled_ = false;
                notify_shutdown = true;
                break;
            }
        }

        Error err = this->handle_actor_request(req);
        if (err != ERR_OK) {
            fds_verify(!"Request resulted in an error");
            FDS_PLOG_WARN(g_fdslog) << " Request type: " << req->type
                    << " error: " << err;
        }
    }

    if (notify_shutdown && parent_) {
        /* This should be the last thing that we do */
        FdsActorShutdownCompletePtr shutdown_complete(new FdsActorShutdownComplete(id_));
        parent_->send_actor_request(
                FdsActorRequestPtr(
                        new FdsActorRequest(FAR_ID(FdsActorShutdownComplete),
                                shutdown_complete)));
    }
}

}  // namespace fds
