/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_SYNCHRONIZED_TASK_EXECUTOR_H_
#define SOURCE_INCLUDE_SYNCHRONIZED_TASK_EXECUTOR_H_

#include <unordered_map>
#include <deque>
#include <utility>
#include <concurrency/Mutex.h>
#include <concurrency/ThreadPool.h>

namespace fds {

template <class KeyT>
class SynchronizedTaskExecutor
{
 public:
    typedef std::function<void ()>TaskT;
 public:
    SynchronizedTaskExecutor(fds_threadpool &tp);
    ~SynchronizedTaskExecutor();

    void schedule(const KeyT &k, const TaskT &task);
    
    void run_(const KeyT &k, const TaskT &task);

 protected:
    /* Lock around taskMap_ */    
    fds_mutex lock_;
    /* Threadpool to execute task functions */
    fds_threadpool &threadpool_;
    /* Map of task qs */
    std::unordered_map<KeyT, std::deque<TaskT>> taskMap_;
};

template <class KeyT>
SynchronizedTaskExecutor<KeyT>::SynchronizedTaskExecutor(fds_threadpool &tp)
: threadpool_(tp)
{
}

template <class KeyT>
SynchronizedTaskExecutor<KeyT>::~SynchronizedTaskExecutor()
{
}

template <class KeyT>
void SynchronizedTaskExecutor<KeyT>::
schedule(const KeyT &k, const SynchronizedTaskExecutor::TaskT &task)
{
    threadpool_.schedule(&SynchronizedTaskExecutor<KeyT>::run_, this, k, task);
}

template <class KeyT>
void SynchronizedTaskExecutor<KeyT>::
run_(const KeyT &k, const SynchronizedTaskExecutor::TaskT &task)
{
    lock_.lock();
    auto itr = taskMap_.find(k);
    if (itr == taskMap_.end()) {
        /* No oustanding tasks with same id.  Start a new queue for incoming tasks with same id */
        const SynchronizedTaskExecutor::TaskT *taskP = &task;
        auto insRes = taskMap_.insert(std::make_pair(k, std::deque<SynchronizedTaskExecutor::TaskT>()));
        auto &sameIdTasks = insRes.first->second;
        sameIdTasks.push_back(task);
        lock_.unlock();

        /* Execute tasks with same id in this loop */
        while (true) {

            (*taskP)();

            fds_scoped_lock l(lock_);
            sameIdTasks.pop_front();
            if (sameIdTasks.size() == 0) {
                /* No more tasks with same id left.  Exit */
                taskMap_.erase(k);
                break;
            } else {
                /* More tasks were added with same id while we were executing this task.
                 * Move onto the next task
                 */
                taskP = &(sameIdTasks.front());
            }
        }
    } else {
        /* We have outstanding tasks with same id.  Add this task to that queue */
        itr->second.push_back(task);
        lock_.unlock();
    }
}

}  // namespace fds
#endif  // SOURCE_INCLUDE_SYNCHRONIZED_TASK_EXECUTOR_H_

