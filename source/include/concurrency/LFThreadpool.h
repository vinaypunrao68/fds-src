#ifndef LFTHREAD_POOL_H
#define LFTHREAD_POOL_H

#include <vector>
#include <queue>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <stdexcept>
#include <concurrency/spinlock.h>
#include <chrono>
#include <atomic>
#include <boost/lockfree/queue.hpp>
#include <pthread.h>

namespace fds {
typedef std::function<void()> LockFreeTask;

struct LockfreeWorker {
    LockfreeWorker(int id, bool steal, std::vector<LockfreeWorker*> &peers)
    : id_(id),
    steal_(steal),
    peers_(peers),
    tasks(1500),
    stop(false),
    completedCntr(0)
    {
    }

    void start() {
        worker = new std::thread(&LockfreeWorker::workLoop, this);
    }

    void finish()
    {
        stop = true;
        condition.notify_one();
        worker->join();
        delete worker;
    }

    void enqueue(LockFreeTask *t)
    {
        bool ret = tasks.push(t);
        fds_verify(ret == true);

        condition.notify_one();
    }

    void workLoop() {
#if 0
        int policy;
        struct sched_param param;
        pthread_getschedparam(pthread_self(), &policy, &param);
        param.sched_priority = sched_get_priority_max(policy); // 20;
        pthread_setschedparam(pthread_self(), SCHED_RR, &param);
#endif
#if 0
        int core_id = id_+1;
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(core_id, &cpuset);
        if (pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset) < 0)
        {
            std::cerr << "Failed to set priority\n";
            return;
        }
#endif

#ifdef OLD_WORK_LOOP
        for(;;)
        {
            LockFreeTask *task;
            int spinCnt = 0;
            while (true) {
                if (stop) {
                    return;
                }
                /* Pop work */
                if (tasks.pop(task)) {
                    break;
                }
                /* No work. Try and steal */
                if (steal_) {
                    bool found = false;
                    for (uint32_t i = 0; i < peers_.size(); i++) {
                        auto idx = (id_ + i + 1) % peers_.size();
                        if (peers_[idx]->tasks.pop(task)) {
                            found = true;
                            break;
                        }
                    }
                    if (found) {
                        break;
                    }
                    spinCnt = 1000;
                }

                ++spinCnt;
                if (spinCnt > 100000) {
                    std::unique_lock<std::mutex> lock(this->queue_mutex);
                    condition.wait_for(lock, std::chrono::milliseconds(5));
                    // TODO(Rao): Implement backoff
                }
            }
            task->operator()();
            delete task;
            completedCntr++;
        }
#endif  // OLD_WORK_LOOP


        for (;;)
        {
            LockFreeTask *task;
            uint64_t empty_cnt = 0;
            uint64_t wait_durations[3] = { 5, 10, 20};
            uint32_t duration_idx = 0;
            uint32_t task_miss_steps = 0;

            while (true) {
                if (stop) {
                    return;
                }
                // not thread-safe, but no problem...
                while (tasks.empty()) {
                    ++empty_cnt;
                    if (empty_cnt > 1000) {
                        std::unique_lock<std::mutex> lock(this->queue_mutex);
                        condition.wait_for(lock, std::chrono::milliseconds(wait_durations[duration_idx]));
                        empty_cnt = 0;
                        break;
                    }
                }  // tasks.empty()

                if (tasks.pop(task)) {
                    duration_idx = 0;
                    break;
                }
                if (duration_idx < 2) {
                    ++duration_idx;
                }
                fds_assert(duration_idx <= 2);
            }  // while (true)

            task->operator()();
            delete task;

        }  // for (;;)
    }
    int id_;
    bool steal_;
    std::vector<LockfreeWorker*> &peers_;
    // the task queue
    boost::lockfree::queue<LockFreeTask*> tasks;

    // synchronization
    std::mutex queue_mutex;
    std::condition_variable condition;
    std::atomic<bool> stop;
    std::thread* worker;
    /* Counters */
    uint64_t completedCntr;
};

struct LFMQThreadpool {
    LFMQThreadpool(uint32_t sz, bool steal = false)
    : workers(sz),
      idx(0)
    {
        for (uint32_t i = 0; i < workers.size(); i++) {
            workers[i] = new LockfreeWorker(i, steal, workers);
        }
        for (uint32_t i = 0; i < workers.size(); i++) {
            workers[i]->start();
        }
    }
    ~LFMQThreadpool()
    {
        for (auto &w : workers) {
            w->finish();
        }
        for (uint32_t i = 0; i < workers.size(); i++) {
            delete workers[i];
        }
    }
    template <class F, class... Args>
    void schedule(F&& f, Args&&... args)
    {
        int i = idx++ % workers.size();
        workers[i]->enqueue(
                new LockFreeTask(std::bind(std::forward<F>(f), std::forward<Args>(args)...)));
    }
    template <class F, class... Args>
    void scheduleWithAffinity(uint64_t affin, F&& f, Args&&... args)
    {
        uint32_t idx = affin % workers.size();
        workers[idx]->enqueue(
                new LockFreeTask(std::bind(std::forward<F>(f), std::forward<Args>(args)...)));
    }
    // std::vector<IThreadpoolWorker> workers;
    std::vector<LockfreeWorker*> workers;
    // std::atomic<int> idx;
    int idx;
};

typedef std::thread LFSQWorker;

struct LFSQThreadpool {
    LFSQThreadpool(uint32_t sz)
    : workers(sz),
    tasks(1500),
    stop(false)
    {
        for (uint32_t i = 0; i < workers.size(); i++) {
            workers[i] = new LFSQWorker(std::bind(&LFSQThreadpool::work, this));
        }
    }

    ~LFSQThreadpool()
    {
        stop = true;
        condition.notify_all();
        for (uint32_t i = 0; i < workers.size(); i++) {
            workers[i]->join();
            delete workers[i];
        }
    }

    template <class F, class... Args>
    void schedule(F&& f, Args&&... args)
    {
        bool ret = tasks.push(new LockFreeTask(
                    std::bind(std::forward<F>(f), std::forward<Args>(args)...)));
        fds_verify(ret == true);

        condition.notify_one();
    }

    void work() {
#ifdef OLD_WORK_LOOP
        for(;;)
        {
            LockFreeTask *task;
            int spinCnt = 0;
            while (true) {
                if (stop) {
                    return;
                }
                if (tasks.pop(task)) {
                    break;
                }
                ++spinCnt;
                if (spinCnt > 100000) {
                    std::unique_lock<std::mutex> lock(this->queue_mutex);
                    condition.wait_for(lock, std::chrono::milliseconds(5));
                }
            }
            task->operator()();
            delete task;
        }  // for (;;)
#endif  // OLD_WORK_LOOP;

        for (;;)
        {
            LockFreeTask *task;
            uint64_t empty_cnt = 0;
            uint64_t wait_durations[3] = { 5, 10, 20};
            uint32_t duration_idx = 0;
            uint32_t task_miss_steps = 0;

            while (true) {
                if (stop) {
                    return;
                }
                // not thread-safe, but no problem...
                while (tasks.empty()) {
                    ++empty_cnt;
                    if (empty_cnt > 1000) {
                        std::unique_lock<std::mutex> lock(this->queue_mutex);
                        condition.wait_for(lock, std::chrono::milliseconds(wait_durations[duration_idx]));
                        empty_cnt = 0;
                        break;
                    }
                }  // tasks.empty()

                if (tasks.pop(task)) {
                    duration_idx = 0;
                    break;
                }
                if (duration_idx < 2) {
                    ++duration_idx;
                }
                fds_assert(duration_idx <= 2);
            }  // while (true)

            task->operator()();
            delete task;
        }  // for (;;)

    }

    // workers
    std::vector<LFSQWorker*> workers;
    // the task queue
    boost::lockfree::queue<LockFreeTask*> tasks;

    // synchronization
    std::mutex queue_mutex;
    std::condition_variable condition;
    bool stop;
};
}  // namespace fds

#endif  // LFTHREAD_POOL_H
