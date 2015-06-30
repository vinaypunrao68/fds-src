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

    void scheduleOnTemplateKey(const KeyT &k, const TaskT &task);
    void scheduleOnHashKey(const size_t &k, const TaskT &task);

    void runTemplateKey_(const KeyT &k, const TaskT &task);
    void runHashKey_(const size_t &k, const TaskT &task);

 protected:
    /* Threadpool to execute task functions */
    fds_threadpool &threadpool_;

    /* Lock around task maps */
    fds_mutex lock_;

    /**
     * Depending upon the requirements of the task as determined by
     * the task builder, either the templateKeyTaskMap_ will be used
     * to serialize tasks or the hashKeyTaskMap_ will be used. The
     * default is the templateKeyTaskMap_.
     *
     * If you would rather not serialize your tasks or incur the hit
     * for task serialization infrustructure, your SOL for now.
     */
    /* Map of task qs based on KeyT values. */
    std::unordered_map<KeyT, std::deque<TaskT>> templateKeyTaskMap_;
    /* Map of task qs based on hash values. */
    std::unordered_map<size_t, std::deque<TaskT>> hashKeyTaskMap_;
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
scheduleOnTemplateKey(const KeyT &k, const SynchronizedTaskExecutor::TaskT &task)
{
    threadpool_.schedule(&SynchronizedTaskExecutor<KeyT>::runTemplateKey_, this, k, task);
}

template <class KeyT>
void SynchronizedTaskExecutor<KeyT>::
runTemplateKey_(const KeyT &k, const SynchronizedTaskExecutor::TaskT &task)
{
    lock_.lock();
    auto itr = templateKeyTaskMap_.find(k);
    if (itr == templateKeyTaskMap_.end()) {
        /* No oustanding tasks with same id.  Start a new queue for incoming tasks with same id */
        const SynchronizedTaskExecutor::TaskT *taskP = &task;
        auto insRes = templateKeyTaskMap_.insert(std::make_pair(k, std::deque<SynchronizedTaskExecutor::TaskT>()));
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
                templateKeyTaskMap_.erase(k);
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

template <class KeyT>
void SynchronizedTaskExecutor<KeyT>::
scheduleOnHashKey(const size_t &k, const SynchronizedTaskExecutor::TaskT &task)
{
    threadpool_.schedule(&SynchronizedTaskExecutor<KeyT>::runHashKey_, this, k, task);
}

template <class KeyT>
void SynchronizedTaskExecutor<KeyT>::
runHashKey_(const size_t &k, const SynchronizedTaskExecutor::TaskT &task)
{
    lock_.lock();
    auto itr = hashKeyTaskMap_.find(k);
    if (itr == hashKeyTaskMap_.end()) {
        /* No oustanding tasks with same hash.  Start a new queue for incoming tasks with same hash. */
        const SynchronizedTaskExecutor::TaskT *taskP = &task;
        auto insRes = hashKeyTaskMap_.insert(std::make_pair(k, std::deque<SynchronizedTaskExecutor::TaskT>()));
        auto &sameHashTasks = insRes.first->second;
        sameHashTasks.push_back(task);
        lock_.unlock();

        /* Execute tasks with same id in this loop */
        while (true) {

            (*taskP)();

            fds_scoped_lock l(lock_);
            sameHashTasks.pop_front();
            if (sameHashTasks.size() == 0) {
                /* No more tasks with same id left.  Exit */
                hashKeyTaskMap_.erase(k);
                break;
            } else {
                /* More tasks were added with same id while we were executing this task.
                 * Move onto the next task
                 */
                taskP = &(sameHashTasks.front());
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

