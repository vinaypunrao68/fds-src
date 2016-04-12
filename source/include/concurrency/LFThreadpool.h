/*
 * Copyright 2015 Formation Data Systems, Inc.
 */
#ifndef INCLUDE_CONCURRENCY_LFTHREADPOOL_H_
#define INCLUDE_CONCURRENCY_LFTHREADPOOL_H_

#include <vector>
#include <queue>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <stdexcept>
#include <atomic>
#include <boost/lockfree/queue.hpp>
#include <pthread.h>
#include <climits>

#include <unistd.h>
#include <sys/syscall.h>
#include <linux/futex.h>
#include <sys/time.h>

#include <util/timeutils.h>
#include <fds_assert.h>

#include "EclipseWorkarounds.h"

namespace fds {
typedef std::function<void()> LockFreeTask;
struct LFMQThreadpool;


// futex provides a kernel facitility to wait for a value at a given address to
// change.  This essentially manages the blocking queue in the kernel.
// For some reason c++11 doesn't allow calls to libc wrapper futex().  So,
// need to create this a macro wrapper.
// futex() has return value, but the return value is not applicable to the
// how it is used.
#define my_futex(_addr, _op, _val, _to, _addr2, _val3) \
        syscall(SYS_futex, _addr, _op, _val, _to, _addr2, _val3)

// macro to pause the running logical processor.  On non-HT, this is a no-op.
// On HT enabled systems, this will yield the processor resource to the
// sibling logical processor.
#define cpu_relax() asm volatile("pause\n": : :"memory")

// How often we should pause the logical processor.
#define CPU_RELAX_FREQ  0x10   // 16

struct LockfreeWorker {
    enum State {
        INIT,
        STARTED,
        ABORTING,
        ABORTED
    };
    LockfreeWorker(LFMQThreadpool *parent,
                   const std::string &threadpoolId,
                   bool steal, std::vector<LockfreeWorker*> &peers);
    void start();
    void finish();
    void enqueue(LockFreeTask *t);
    void workLoop();
    std::string logString() const;

    // This should be cache line size aligned to avoid false sharing.
    alignas(64) int                         queueCnt = 0;

    /* Combination of threadpool id and thread id */
    LFMQThreadpool                          *parent_; 
    std::string                             id_;
    bool                                    steal_;
    std::vector<LockfreeWorker*>            &peers_;
    State                                   state_;
    // the task queue
    boost::lockfree::queue<LockFreeTask*>   tasks;

    std::thread*                            worker;
    /* Counters */
    uint64_t                                completedCntr;
    /* = -1 means thread is idle; >= 0 indicates thread is busy and the value
     * held is couner value when last thread check was run
     */
    int64_t                                 threadCheckCntr;
};

/**
* @brief Lockfree threadpool implementation that uses multiple queues, single queue
* per thread.
*/
struct LFMQThreadpool {
    LFMQThreadpool(const std::string &id, uint32_t sz, bool steal = false);
    LFMQThreadpool(uint32_t sz, bool steal = false);
    ~LFMQThreadpool();
    void stop();
    template <class F, class... Args>
    void schedule(F&& f, Args&&... args)
    {
        int i = idx++ % workers.size();
        auto b = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
        workers[i]->enqueue(new LockFreeTask(b));
    }
    template <class F, class... Args>
    void scheduleWithAffinity(uint64_t affin, F&& f, Args&&... args)
    {
        uint32_t idx = affin % workers.size();
        workers[idx]->enqueue(
                new LockFreeTask(std::bind(std::forward<F>(f), std::forward<Args>(args)...)));
    }
    /* Use this function to catch cases where long running/blocking
     * task is blocking thread in the threadpool
     * NOTE: Don't run this function on this threadpool, it's better to run on a
     * separate thread.
     */
    void threadpoolCheck();

    /**
    * @brief Returns thread id responsible for tasks with provided affinity
    * @param affinity
    */
    std::thread::id getThreadId(uint64_t affinity) const
    {
        return workers[affinity % workers.size()]->worker->get_id();
    }


    std::vector<LockfreeWorker*>            workers;
    int                                     idx;
    int64_t                                 threadCheckCntr;
};

typedef std::thread LFSQWorker;

/**
* @brief Lockfree threadpool implementation that uses single queue and all threads consume
* off of that single queue.
*/
struct LFSQThreadpool {
    LFSQThreadpool(uint32_t sz);
    ~LFSQThreadpool();

    template <class F, class... Args>
    void schedule(F&& f, Args&&... args)
    {
        bool ret = tasks.push(new LockFreeTask(
                    std::bind(std::forward<F>(f), std::forward<Args>(args)...)));
        fds_verify(ret == true);

        // After enqueueing to task queue, signal a single waiter.
        __sync_fetch_and_add(&queueCnt, 1);
        my_futex(&queueCnt, FUTEX_WAKE_PRIVATE, 1, NULL, NULL, 0);
    }

    void work();

    // This should be cache line size aligned to avoid false sharing.
    alignas(64) int queueCnt = 0;
    // workers
    std::vector<LFSQWorker*> workers;
    // the task queue
    boost::lockfree::queue<LockFreeTask*> tasks;
    // synchronization
    std::atomic<bool> workLoopTerminate;
};
}  // namespace fds

#endif  // INCLUDE_CONCURRENCY_LFTHREADPOOL_H_
