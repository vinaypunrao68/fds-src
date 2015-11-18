/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#ifndef USE_BOOSTBASED_TIMER
#include <fds_globals.h>
#include <fds_timer.h>
#include <util/Log.h>

namespace fds
{
FdsTimerTask::FdsTimerTask(FdsTimer &fds_timer)
{
    state_ = TASK_STATE_UNINIT;
}

FdsTimerTask::~FdsTimerTask()
{
}


void FdsTimerTask::setExpiryTime(const std::chrono::system_clock::time_point &t)
{
    expTime_ = t;
}

std::chrono::system_clock::time_point FdsTimerTask::getExpiryTime() const {
    return expTime_;
}

FdsTimerFunctionTask::FdsTimerFunctionTask(FdsTimer &timer,
                                           const std::function<void()> &f)
: FdsTimerTask(timer),
    f_(f)
{
}

void FdsTimerFunctionTask::runTimerTask()
{
    f_();
}

/**
 * Constructor
 */
FdsTimer::FdsTimer()
: FdsTimer("") {
}

/**
 * Constructor
 */
FdsTimer::FdsTimer(const std::string &id)
: id_(std::string("FdsTimer:") + id + std::string(": ")),
    abortCntr_(0),
    timerThreadSleepMs_(1000),
    timerThread_(std::bind(&FdsTimer::runTimerThread_, this))
{
}

/**
 * Destroy the timer service 
 */
void FdsTimer::destroy()
{
    int prevAbortCnt = abortCntr_++;
    if (prevAbortCnt == 0) {
        timerThread_.join();
    }
}

/**
 * Cancel a task. 
 * @return true if task was succsfully cancelled.  Note, false
 * returned when task wasn't scheduled to begin with.
 */
bool FdsTimer::cancel(const FdsTimerTaskPtr& task)
{
    lock_.lock();
    /*
       fds_assert(task->state_ == TASK_STATE_SCHEDULED_ONCE ||
       task->state_ == TASK_STATE_SCHEDULED_REPEAT ||
       task->state_ == TASK_STATE_COMPLETE);
       */
    pendingTasks_.erase(task);
    task->state_ = TASK_STATE_CANCELLED;
    lock_.unlock();
    return true;
}

std::string FdsTimer::log_string()
{
    return id_;
}

void FdsTimer::runTimerThread_()
{
    /* This wait is needed because of races in initializing g_fdslog.  There are global
     * variables in OM that use fds_timer.  Before fds_process is inited, if we try to log
     * anything from any of these globals g_fdslog may be inited twice from
     * init_process_globals() which isn't desirable.
     */
    while (g_fdslog == nullptr) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    GLOGNORMAL << log_string() << " Timer thread started...";

    while (abortCntr_ == 0) {
        // TODO(Rao): Improve this sleep below so that it services tasks
        // more promptly than every timerThreadSleepMs_
        auto cur_time_pt = std::chrono::system_clock::now();
        /* drain the expired tasks */
        lock_.lock();
        while (pendingTasks_.size() > 0) {
            auto task = *(pendingTasks_.begin());
            if (task->getExpiryTime() <= cur_time_pt) {
                pendingTasks_.erase(pendingTasks_.begin());
                fds_assert(task->state_ == TASK_STATE_SCHEDULED_ONCE ||
                           task->state_ == TASK_STATE_SCHEDULED_REPEAT);
                if (task->state_ == TASK_STATE_SCHEDULED_REPEAT) {
                    task->expTime_ = std::chrono::system_clock::now() + task->durationMs_;
                    pendingTasks_.insert(task);
                } else {
                    task->state_ = TASK_STATE_COMPLETE;
                }
                lock_.unlock();

                try {
                    task->runTimerTask();
                } catch (const std::exception &e) {
                    GLOGERROR << log_string() << "Exception on timer thread: " << e.what()
                        << ".  Ignoring and continuing timer thread";
                } catch (...) {
                    GLOGERROR << log_string() << "Unknown exception on timer thread: "
                        << ".  Ignoring and continuing timer thread";
                }

                lock_.lock();
            } else {
                break;
            }
        }
        lock_.unlock();
        /* go back to sleep */
        std::this_thread::sleep_for(std::chrono::milliseconds(timerThreadSleepMs_));
    }

    GLOGNORMAL << log_string() << "Timer thread exited...";
}
}  // namespace fds

#else  // USE_BOOSTBASED_TIMER

#include <string>
#include <fds_timer.h>  // NOLINT
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
        fds_mutex::scoped_lock l(task->lock_);
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
#endif  // USE_BOOSTBASED_TIMER

