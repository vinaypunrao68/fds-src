/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_FDS_TIMER_H_
#define SOURCE_INCLUDE_FDS_TIMER_H_

#include <chrono>
#include <boost/shared_ptr.hpp>
#include <boost/asio.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/bind.hpp>
#include <boost/bind.hpp>
#include <thread>

#include <fds_globals.h>
#include <util/Log.h>
#include <fds_assert.h>
#include <concurrency/Mutex.h>

namespace fds
{

class FdsTimer;
typedef boost::shared_ptr<FdsTimer> FdsTimerPtr;

class FdsTimerTask
{
public:
    FdsTimerTask(FdsTimer &fds_timer);

    virtual ~FdsTimerTask();

    virtual void runTimerTask() = 0;

protected:
    /* Lock for protecting scheduled variable */
    fds_spinlock lock_;
    /* Whether timer is scheduled or not */
    bool scheduled_;
    /* Timer object */
    boost::asio::steady_timer timer_;
    friend class FdsTimer;
};

typedef boost::shared_ptr<FdsTimerTask> FdsTimerTaskPtr;

/**
 * The timer class is used to schedule tasks for one-time execution or
 * repeated execution. Tasks are executed by the dedicated timer thread
 * sequentially.
 */
class FdsTimer
{
public:

    /**
     * Constructor
     */
    FdsTimer();

    /**
     * Destroy the timer service 
     */
    void destroy();

    /**
     * Schedule a task for execution after a given delay.
     * It's safe to re-use the same task either after runTimerTask() has been
     * invoked or the task object is cancelled.
     * @param task - task to execute
     * @param time - duration in micro, milli, sec, etc.
     * @return true if task is scheduled fasle otherwise
     */
    template<typename Rep, typename Period>
    bool schedule(FdsTimerTaskPtr& task,
            const std::chrono::duration<Rep, Period>& time)
    {
        return schedule_internal(task, time, false);
    }

    /**
     * Schedule a task for repeated execution with the given delay
     * between each execution.
     * @param task - task to execute
     * @param time - duration in micro, milli, sec, etc.
     * @return true if task is scheduled fasle otherwise
     */
    template<typename Rep, typename Period>
    bool scheduleRepeated(FdsTimerTaskPtr& task,
            const std::chrono::duration<Rep, Period>& time)
    {
        return schedule_internal(task, time, true);
    }

    /**
     * Cancel a task. 
     * @return true if task was succsfully cancelled.  Note, false
     * returned when task wasn't scheduled to begin with.
     */
    bool cancel(const FdsTimerTaskPtr& task);

private:
    void start_io_service();

    template<typename Rep, typename Period>
    bool schedule_internal(FdsTimerTaskPtr& task,
            const std::chrono::duration<Rep, Period>& time,
            const bool& repeated)
    {
        {
            fds_spinlock::scoped_lock l(task->lock_);
            if (task->scheduled_) {
                return false;
            }
            task->scheduled_ = true;
        }

        std::size_t pending_cnt;
        boost::system::error_code error;

        pending_cnt = task->timer_.expires_from_now(time, error);
        if (pending_cnt != 0 || error) {
            /* This really shouldn't happen */ 
            fds_verify(!"Failed to schedule task");
        }
        task->timer_.async_wait(boost::bind(&FdsTimer::handler<Rep, Period>, this,
                boost::asio::placeholders::error, task, repeated, time));
        return true;
    }

    template<typename Rep, typename Period>
    void handler(const boost::system::error_code& error,
            FdsTimerTaskPtr& task,
            const bool& repeated,
            const std::chrono::duration<Rep, Period>& time)
    {
        {
            fds_spinlock::scoped_lock l(task->lock_);
            if (!task->scheduled_) {
                /* task has been cancelled.  Don't invoke the handler */
                return;
            }

            /* We will not set scheduled_ to false for repeated task here */
            if (!repeated) {
                task->scheduled_ = false;
            }
        }

        if (error ==  boost::asio::error::operation_aborted) {
            /* Handler isn't invoked for cancelled timers */
            return;
        } else if (error) {
            FDS_PLOG_WARN(g_fdslog) << "Failed to invoked handler for task.  Error: " << error;
            return;
        }
        task->runTimerTask();

        if (repeated) {
            scheduleRepeated(task, time);
        }
    }

    boost::asio::io_service io_service_;
    boost::asio::io_service::work work_;
    std::thread io_thread_;

    friend class FdsTimerTask;
};
}

#endif

