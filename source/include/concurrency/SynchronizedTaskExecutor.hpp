/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_SYNCHRONIZED_TASK_EXECUTOR_H_
#define SOURCE_INCLUDE_SYNCHRONIZED_TASK_EXECUTOR_H_

#include <condition_variable>
#include <unordered_map>
#include <deque>
#include <mutex>
#include <utility>
#include <concurrency/ThreadPool.h>

namespace fds {

template <class KeyT>
class SynchronizedTaskExecutor
{
 public:
    typedef std::function<void ()>TaskT;
    SynchronizedTaskExecutor(fds_threadpool &tp);
    SynchronizedTaskExecutor(fds_threadpool &tp, bool useAffinity);
    ~SynchronizedTaskExecutor();

    void scheduleOnTemplateKey(const KeyT &k, const TaskT &task);
    void scheduleOnHashKey(const size_t &k, const TaskT &task);
    void scheduleOnHashKeys(const size_t &k1, const size_t &k2, const TaskT &task);

    void runTemplateKey_(const KeyT &k, const TaskT &task);
    void runHashKey_(const size_t &k, const TaskT &task);
    void runHashKeys_(const size_t &k1, const size_t &k2, const TaskT &task);

 protected:
    /* Threadpool to execute task functions */
    fds_threadpool &threadpool_;

    /* When set, each task is given a thread affinity.  The task is only run by 
     * a thread with particular id
     */
    bool useAffinity_;

    /* Lock around task maps */
    std::mutex lock_;

    /* Notification around finishing a queue */
    std::condition_variable queue_finished_;

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

 private:
    void runLoop_(const size_t &k, const SynchronizedTaskExecutor::TaskT &task);
};

template <class KeyT>
SynchronizedTaskExecutor<KeyT>::SynchronizedTaskExecutor(fds_threadpool &tp)
: SynchronizedTaskExecutor<KeyT>(tp, true)
{
}

template <class KeyT>
SynchronizedTaskExecutor<KeyT>::SynchronizedTaskExecutor(
    fds_threadpool &tp, bool useAffinity)
: threadpool_(tp),
    useAffinity_(useAffinity)
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
    if (useAffinity_) {
        std::hash<KeyT> kHash;
        threadpool_.scheduleWithAffinity(kHash(), task);
    } else {
        threadpool_.schedule(&SynchronizedTaskExecutor<KeyT>::runTemplateKey_, this, k, task);
    }
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

            std::lock_guard<std::mutex> g(lock_);
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
    if (useAffinity_) {
        threadpool_.scheduleWithAffinity(k, task);
    } else {
        threadpool_.schedule(&SynchronizedTaskExecutor<KeyT>::runHashKey_, this, k, task);
    }
}

template <class KeyT>
void SynchronizedTaskExecutor<KeyT>::
runHashKey_(const size_t &k, const SynchronizedTaskExecutor::TaskT &task)
{
    lock_.lock();
    auto itr = hashKeyTaskMap_.find(k);
    if (itr == hashKeyTaskMap_.end()) {
        runLoop_(k, task);
    } else {
        /* We have outstanding tasks with same id.  Add this task to that queue */
        itr->second.push_back(task);
        lock_.unlock();
    }
}

template <class KeyT>
void SynchronizedTaskExecutor<KeyT>::
scheduleOnHashKeys(const size_t &k1, const size_t &k2, const SynchronizedTaskExecutor::TaskT &task)
{
    threadpool_.schedule(&SynchronizedTaskExecutor<KeyT>::runHashKeys_, this, k1, k2, task);
}

template <class KeyT>
void SynchronizedTaskExecutor<KeyT>::
runHashKeys_(const size_t &k1, const size_t &k2, const SynchronizedTaskExecutor::TaskT &task)
{
    runHashKey_(k1,
                [this, &k1, &k2, &task] () mutable -> void {
                    auto lock = std::unique_lock<std::mutex>(lock_);
                    queue_finished_.wait(lock,
                                         [this, k2] () -> bool {
                                            return hashKeyTaskMap_.end() == hashKeyTaskMap_.find(k2);
                                         });
                    runLoop_(k2, task);
                });
}

template <class KeyT>
void SynchronizedTaskExecutor<KeyT>::
runLoop_(const size_t &k, const SynchronizedTaskExecutor::TaskT &task) {
    /* No oustanding tasks with same hash.  Start a new queue for incoming tasks with same hash. */
    const SynchronizedTaskExecutor::TaskT *taskP = &task;
    auto insRes = hashKeyTaskMap_.insert(std::make_pair(k, std::deque<SynchronizedTaskExecutor::TaskT>()));
    auto &sameHashTasks = insRes.first->second;
    sameHashTasks.push_back(task);
    lock_.unlock();

    /* Execute tasks with same id in this loop */
    while (true) {

        (*taskP)();

        std::lock_guard<std::mutex> g(lock_);
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
    queue_finished_.notify_all();
}

}  // namespace fds
#endif  // SOURCE_INCLUDE_SYNCHRONIZED_TASK_EXECUTOR_H_

