/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_STOR_MGR_INCLUDE_MOCKSVCHANDLER_H_
#define SOURCE_STOR_MGR_INCLUDE_MOCKSVCHANDLER_H_

#include <thrift/concurrency/TimerManager.h>
#include <thrift/concurrency/PlatformThreadFactory.h>
#include <boost/lockfree/queue.hpp>
#include <thread>

namespace fds {

namespace tc  = apache::thrift::concurrency;
struct MockSvcTimerTask {
    MockSvcTimerTask(fds_threadpool *tp, std::function<void()> taskF)
        : tp_(tp),
        taskF_(taskF)
    {
    }
    virtual ~MockSvcTimerTask()
    {
    }
    virtual void run()
    {
        if (tp_) {
            // tp_->schedule(taskF_);
        } else {
            taskF_();
        }
    }

    std::function<void()> taskF_;
    std::chrono::system_clock::time_point expTime_;
    fds_threadpool* tp_;
};
typedef MockSvcTimerTask* MockSvcTimerTaskPtr;

struct MockSvcHandler {
    void run();

    MockSvcHandler()
    : tasks_(100)
    {
        tp_ = nullptr;
        aborted_ = false;
        t_.reset(new std::thread(&MockSvcHandler::run, this));
        sleepTime_ = 200;
        nextTask_ = nullptr;
    }
    virtual ~MockSvcHandler()
    {
        aborted_ = true;
    }
    void setThreadpool(fds_threadpool* tp)
    {
        tp_ = tp;
    }

    /*
    template <class F, class... Args>
    void schedule(uint64_t latencyUs, F&& f, Args&&... args)
    {
        auto fun = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
        auto task = new MockSvcTimerTask(tp_, fun);
        task->expTime_ = std::chrono::system_clock::now() +
            std::chrono::microseconds(latencyUs);
        tasks_.push(task);
    }
    */

    void schedule(uint64_t latencyUs, std::function<void()> f)
    {
        auto task = new MockSvcTimerTask(tp_, f);
        task->expTime_ = std::chrono::system_clock::now() +
            std::chrono::microseconds(latencyUs);
        tasks_.push(task);
    }

 protected:
    fds_threadpool* tp_;
    bool aborted_;
    std::unique_ptr<std::thread> t_;
    uint32_t sleepTime_;
    boost::lockfree::queue<MockSvcTimerTaskPtr> tasks_; // NOLINT
    MockSvcTimerTaskPtr nextTask_;
};

void MockSvcHandler::run()
{
    while (!aborted_) {
        auto curTimePt = std::chrono::system_clock::now();
        while (true) {
            if (nextTask_ == nullptr) {
                bool ret = tasks_.pop(nextTask_);
                if (ret == false) {
                    break;
                }
            }
            fds_verify(nextTask_ != nullptr);
            if (nextTask_->expTime_ > curTimePt) {
                break;
            }
            nextTask_->run();
            delete nextTask_;
            nextTask_ = nullptr;
        }
        std::this_thread::sleep_for(std::chrono::microseconds(sleepTime_));
    }
}

}  // namespace fds

#endif  // SOURCE_STOR_MGR_INCLUDE_MOCKSVCHANDLER_H_
