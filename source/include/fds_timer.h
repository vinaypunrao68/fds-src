/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_FDS_TIMER_H_
#define SOURCE_INCLUDE_FDS_TIMER_H_

#include <chrono>
#include <boost/shared_ptr.hpp>

namespace fds
{

class FdsTimer;
typedef boost::shared_ptr<FdsTimer> FdsTimerPtr;

class FdsTimerTask
{
public:

    virtual ~FdsTimerTask() { }

    virtual void runTimerTask() = 0;
};
typedef boost::shared_ptr<FdsTimerTask> FdsTimerTaskPtr;



/**
 * The timer class is used to schedule tasks for one-time execution or
 * repeated execution. Tasks are executed by the dedicated timer thread
 * sequentially.
 */
// todo: implementation of this interface
class FdsTimer
{
public:

    /**
     *
     */
    FdsTimer()
    {
    }

    /**
     * Constructor
     * @param priority
     */
    FdsTimer(int priority)
    {

    }

    /**
     * Destroy the timer and detach its execution thread if the calling thread
     * is the timer thread, join the timer execution thread otherwise.
     */
    void destroy()
    {

    }

    /**
     * Schedule a task for execution after a given delay.
     * @param task - task to execute
     * @param time - duration in micro, milli, sec, etc.
     */
    template<typename Rep, typename Period>
    void schedule(const FdsTimerTaskPtr& task,
            const std::chrono::duration<Rep, Period>& time)
    {

    }

    /**
     * Schedule a task for repeated execution with the given delay
     * between each execution.
     * @param task - task to execute
     * @param time - duration in micro, milli, sec, etc.
     */
    template<typename Rep, typename Period>
    void scheduleRepeated(const FdsTimerTaskPtr& task,
            const std::chrono::duration<Rep, Period>& time)
    {

    }

    /**
     * Cancel a task. Returns true if the task has not yet run or if
     * Cancel it's a task scheduled for repeated execution. Returns false if
     * the task has already run, was already cancelled or was never
     * schedulded.
     */
    bool cancel(const FdsTimerTaskPtr&)
    {
        return true;
    }

private:
};
}

#endif

