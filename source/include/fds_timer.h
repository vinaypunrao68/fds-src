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

#include <util/Log.h>
#include <fds_assert.h>

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

private:
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
     * @param log
     */
    FdsTimer(boost::shared_ptr<fds_log> log);

    /**
     * Destroy the timer and detach its execution thread if the calling thread
     * is the timer thread, join the timer execution thread otherwise.
     */
    void destroy();

    /**
     * Schedule a task for execution after a given delay.
     * NOTE: Do NOT invoke schedule() the same task object twice before the
     * runTimerTask() for the frist invocation has been called. Doing so will
     * will cancel the first task.
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
        std::size_t pending_cnt;
        boost::system::error_code error;

        pending_cnt = task->timer_.expires_from_now(time, error);
        if (pending_cnt != 0 || error) {
            FDS_PLOG_WARN(log_) << " Failed to schedule task. Error: " << error
                    << " Pending cnt: " << pending_cnt;
            return false;
        }
        task->timer_.async_wait(boost::bind(&FdsTimer::handler, this,
                boost::asio::placeholders::error, task));
        return true;
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
        return false;
    }

    /**
     * Cancel a task. Returns true if the task has not yet run or if
     * Cancel it's a task scheduled for repeated execution. Returns false if
     * the task has already run, was already cancelled or was never
     * schedulded.
     */
    bool cancel(const FdsTimerTaskPtr& task);

private:
    void start_io_service();

    void handler(const boost::system::error_code& error,
            FdsTimerTaskPtr &task);

    boost::shared_ptr<fds_log> log_;
    boost::asio::io_service io_service_;
    boost::asio::io_service::work work_;
    std::thread io_thread_;

    friend class FdsTimerTask;
};
}

#endif

