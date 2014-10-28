#ifndef ITHREAD_POOL_H
#define ITHREAD_POOL_H

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

typedef std::function<void()> LockFreeTask;

struct LockfreeWorker {
    LockfreeWorker(int id)
    : id_(id),
    tasks(1500),
    stop(false),
    worker(&LockfreeWorker::workLoop, this)
    {
    }

    void finish()
    {
        stop = true;
        condition.notify_one();
        worker.join();
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
        }
    }
    int id_;
    // the task queue
    boost::lockfree::queue<LockFreeTask*> tasks;

    // synchronization
    std::mutex queue_mutex;
    std::condition_variable condition;
    std::atomic<bool> stop;
    std::thread worker;
};

struct IThreadpool {
    IThreadpool(uint32_t sz)
    : workers(sz),
      idx(0)
    {
        for (uint32_t i = 0; i < workers.size(); i++) {
            workers[i] = new LockfreeWorker(i);
        }
    }
    ~IThreadpool()
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
        }
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

#endif
