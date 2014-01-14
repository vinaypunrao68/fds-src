/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#include <fds_timer.h>
namespace fds
{


FdsTimerTask::FdsTimerTask(FdsTimer &fds_timer) // NOLINT
    : timer_(fds_timer.io_service_),
    lock_("FdsTimerTask"),
    scheduled_(false)
{
}

FdsTimerTask::~FdsTimerTask() { }

FdsTimer::FdsTimer()
    : work_(io_service_),
      io_thread_(&FdsTimer::start_io_service, this)
{
}

void FdsTimer::destroy()
{
    io_service_.stop();
    io_thread_.join();
}

bool FdsTimer::cancel(const FdsTimerTaskPtr& task)
{
    {
        fds_spinlock::scoped_lock l(task->lock_);
        if (!task->scheduled_) {
            return false;
        }
        task->scheduled_ = false;
    }
    /* Note, igoring the return value.  May be we shouldn't? */
    task->timer_.cancel();
    return true;
}

void FdsTimer::start_io_service()
{
    try {
        io_service_.run();
    } catch(const std::exception &e) {
        FDS_PLOG_WARN(g_fdslog) << e.what();
    }
}

}  // namespace fds
