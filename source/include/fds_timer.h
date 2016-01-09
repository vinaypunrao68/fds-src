/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_FDS_TIMER_H_
#define SOURCE_INCLUDE_FDS_TIMER_H_

#include <atomic>
#include <chrono>
#include <set>
#include <functional>
#include <boost/shared_ptr.hpp>
#include <thread>
#include <fds_defines.h>
#include <fds_assert.h>
#include <concurrency/Mutex.h>

namespace fds
{
/* Forward declarations */
class FdsTimer;
typedef boost::shared_ptr<FdsTimer> FdsTimerPtr;

enum TimerTaskState {
    TASK_STATE_UNINIT = 0,
    TASK_STATE_SCHEDULED_ONCE,
    TASK_STATE_SCHEDULED_REPEAT,
    TASK_STATE_CANCELLED,
    /* Only valid for schedule once task */
    TASK_STATE_COMPLETE
};

class FdsTimerTask
{
public:
    FdsTimerTask();

    // TODO(Rao): Deprecate this constructor
    FdsTimerTask(FdsTimer &fds_timer);

    virtual ~FdsTimerTask();

    virtual void runTimerTask() = 0;

    void setExpiryTime(const std::chrono::system_clock::time_point &t);

    std::chrono::system_clock::time_point getExpiryTime() const;
protected:
    /* Expiration time */
    std::chrono::system_clock::time_point expTime_;
    std::chrono::milliseconds durationMs_;
    /* Task state */
    TimerTaskState state_;
    friend class FdsTimer;
};

typedef boost::shared_ptr<FdsTimerTask> FdsTimerTaskPtr;

struct LessFdsTimerTaskPtr {
    bool operator()(const FdsTimerTaskPtr& lhs, const FdsTimerTaskPtr& rhs) {
        if (lhs->getExpiryTime() == rhs->getExpiryTime()) {
            return lhs.get() < rhs.get();
        }
        return lhs->getExpiryTime() < rhs->getExpiryTime();
    }
};

/**
* @brief Timer task to wrap std::function
*/
struct FdsTimerFunctionTask : FdsTimerTask {
    explicit FdsTimerFunctionTask(const std::function<void()> &f);
    virtual void runTimerTask() override;

 protected:
    std::function<void()> f_;
};

/**
 * The timer class is used to schedule tasks for one-time execution or
 * repeated execution. Tasks are executed by the dedicated timer thread
 * sequentially.
 * IMPORTANT: Firing of the timer tasks isn't very accurate.  It's better to schedule
 * timer tasks in the granularity of seconds.
 */
class FdsTimer
{
public:

    /**
     * Constructor
     */
    FdsTimer();

    /**
     * Constructor
     */
    explicit FdsTimer(const std::string &id);

    /**
     * Destructor
     */
    ~FdsTimer()
    { destroy(); }

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
        return scheduleInternal_(task, time, false);
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
        return scheduleInternal_(task, time, true);
    }

    /**
     * Cancel a task. 
     * @return true if task was succsfully cancelled.  Note, false
     * returned when task wasn't scheduled to begin with.
     */
    bool cancel(const FdsTimerTaskPtr& task);

    virtual std::string log_string();

private:
    template<typename Rep, typename Period>
    bool scheduleInternal_(FdsTimerTaskPtr& task,
            const std::chrono::duration<Rep, Period>& time,
            const bool &repeated)
    {
        fds_assert(time >= std::chrono::seconds(1));

        lock_.lock();
        task->durationMs_ = std::chrono::duration_cast<std::chrono::milliseconds>(time);
        task->expTime_ = std::chrono::system_clock::now() + task->durationMs_;
        pendingTasks_.insert(task);
        if (repeated) {
            task->state_ = TASK_STATE_SCHEDULED_REPEAT;
        } else {
            task->state_ = TASK_STATE_SCHEDULED_ONCE;
        }
        lock_.unlock();
        return true;
    }

    void runTimerThread_();

    /* Lock for protecting scheduled variable */
    fds_mutex lock_;
    /* Id of the timer */
    std::string id_;
    /* Pending timer objects */
    std::set<FdsTimerTaskPtr, LessFdsTimerTaskPtr> pendingTasks_;
    /*
     * Whether timer thread should abort or not. Zero means not aborted.
     * value held indicates the # of times destroy() is called
     * We only destroy, i.e join on timer thread once
     */
    std::atomic<int> abortCntr_;
    /* Timer thread sleep time */
    int timerThreadSleepMs_;
    /* Timer thread */
    std::thread timerThread_;
};

/**
* @brief Convenince function for scheduling a task on timer
*/
SHPTR<FdsTimerTask> scheduleTimerTask(FdsTimer &timer,
                                      const std::chrono::seconds &time,
                                      const std::function<void()> &f);

SHPTR<FdsTimerTask> scheduleRepeatedTimerTask(FdsTimer &timer,
                                              const std::chrono::seconds &time,
                                              const std::function<void()> &f);
}  // namespace fds

#endif  // SOURCE_INCLUDE_FDS_TIMER_H_

