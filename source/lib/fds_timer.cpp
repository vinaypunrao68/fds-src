/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#include <fds_timer.h>
namespace fds
{


FdsTimerTask::FdsTimerTask(FdsTimer &fds_timer) // NOLINT
: timer_(fds_timer.io_service_)
{
}

FdsTimerTask::~FdsTimerTask() { }

FdsTimer::FdsTimer(boost::shared_ptr<fds_log> log)
    : log_(log),
      work_(io_service_),
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
    std::size_t cancel_cnt = task->timer_.cancel();
    return cancel_cnt == 1;
}

void FdsTimer::start_io_service()
{
    try {
        io_service_.run();
    } catch(const std::exception &e) {
        FDS_PLOG_WARN(log_) << e.what();
    }
}

void FdsTimer::handler(const boost::system::error_code& error,
        FdsTimerTaskPtr &task)
{
    if (error ==  boost::asio::error::operation_aborted) {
        /* Handler isn't invoked for cancelled timers */
        return;
    } else if (error) {
        FDS_PLOG_WARN(log_) << "Failed to invoked handler for task.  Error: " << error;
        return;
    }
    task->runTimerTask();
}

}  // namespace fds
