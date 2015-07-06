#ifndef LFTHREAD_POOL_H
#define LFTHREAD_POOL_H

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

#include "EclipseWorkarounds.h"

namespace fds {
typedef std::function<void()> LockFreeTask;


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
    LockfreeWorker(int id, bool steal, std::vector<LockfreeWorker*> &peers)
    :  queueCnt(0),
    id_(id),
    steal_(steal),
    peers_(peers),
    tasks(1500),
    workLoopTerminate(false),
    completedCntr(0)
    {
    }

    void start() {
        worker = new std::thread(&LockfreeWorker::workLoop, this);
    }

    void finish()
    {
        workLoopTerminate = true;
        my_futex(&queueCnt, FUTEX_WAKE_PRIVATE, 1, NULL, NULL, 0);
        worker->join();
        delete worker;
    }

    void enqueue(LockFreeTask *t)
    {
        // enqueue a task.
        // The order of operation is important.
        // 1) enqueue
        // 2) increment the queue count
        // 3) signal a waiter.
        bool ret = tasks.push(t);

        if (ret) {
            // increment the queue count.
            __sync_fetch_and_add(&queueCnt, 1);

            // Wake up one.
            //
            // TODO(Sean)
            // This is doing a supurious wakeup.  Will need a way to prevent sending
            // signal if possible.  Will need some state/hand shake between
            // enqueue and workLoop.  Not sure how to do it yet.
            my_futex(&queueCnt, FUTEX_WAKE_PRIVATE, 1, NULL, NULL, 0);
        }
    }

    void workLoop() {
#if 0
        int policy;
        struct sched_param param;
        pthread_getschedparam(pthread_self(), &policy, &param);
        param.sched_priority = sched_get_priority_max(policy); // 20;
        pthread_setschedparam(pthread_self(), SCHED_RR, &param);
#endif

        for (;;)
        {
            LockFreeTask *task;
            int old_queueCnt, new_queueCnt;

            // TODO (Sean)
            // This block of code and a work() block of code is the same.  Need
            // to refactor them.  But for now, leaving them as is.
            //
            // The block of code in work() is better commented, so refer
            // to it for better explanation of the code.
            while (true) {
                bool dequeued = false;

                task = NULL;
                if (queueCnt == 0) {
                    // block if the queueCnt is 0.  The actual value of queueCnt and
                    // expected value is atomically checked.
                    // Although we intially entered this scope due to queueCnt == 0,
                    // by the time we get here, the state can change where we don't
                    // block here.
                    my_futex(&queueCnt, FUTEX_WAIT_PRIVATE, 0, NULL, NULL, 0);

                    if (workLoopTerminate.load(std::memory_order_relaxed)) {
                        return;
                    }

                    // TODO(Sean)
                    // This cmpxchg loop and the the one in the next block is the same.
                    // For some reason, during the testing, when re-factored the code, it
                    // wasn't working.  Could be a bug in  how I re-factored it, but for
                    // now leave it and revisit re-factoring later.
                    //
                    // See if we  can dequeue.  The steps are (important to maintain the order):
                    // 1) try to decrement atomically in cmpxchg loop.
                    //      1.1) if successful, thene we can dequeue a task.
                    //      1.2) if unsuccessful (i.e. queue is already empty), do nothing.
                    dequeued = true;
                    uint64_t cmpxchgMissed = 0;
                    do {
                        // This is a minor optimization to pause/yield current running logical
                        // processor to the other logical processor on the core.
                        // This will kick in if cmpxchg fails too many times, which implies heavy
                        // contention on the queueCnt.
                        if (cmpxchgMissed & CPU_RELAX_FREQ) {
                            cpu_relax();
                        } else {
                            ++cmpxchgMissed;
                        }

                        old_queueCnt = queueCnt;
                        fds_assert(old_queueCnt >= 0);

                        if (old_queueCnt == 0) {
                            // If the count is 0, then there is nothing in the queue.
                            // Do nothing and restart the outer loop.
                            dequeued = false;
                            break;
                        }

                        new_queueCnt = old_queueCnt - 1;


                    } while (!__sync_bool_compare_and_swap(&queueCnt, old_queueCnt, new_queueCnt));

                    // the count says we found something.  Now, grab a task.
                    if (dequeued) {
                        fds_verify(tasks.pop(task));
                    }
                } else {
                    dequeued = true;
                    uint64_t cmpxchgMissed = 0;
                    do {

                        // This is a minor optimization to pause/yield current running logical
                        // processor to the other logical processor on the core.
                        // This will kick in if cmpxchg fails too many times, which implies heavy
                        // contention on the queueCnt.
                        if (cmpxchgMissed & CPU_RELAX_FREQ) {
                            if (workLoopTerminate.load(std::memory_order_relaxed)) {
                                return;
                            }

                            cpu_relax();
                        } else {
                            ++cmpxchgMissed;
                        }

                        old_queueCnt = queueCnt;
                        fds_assert(old_queueCnt >= 0);

                        if (old_queueCnt == 0) {
                            dequeued = false;
                            break;
                        }

                        new_queueCnt = old_queueCnt - 1;

                    } while (!__sync_bool_compare_and_swap(&queueCnt, old_queueCnt, new_queueCnt));
                    if (dequeued) {
                        fds_verify(tasks.pop(task));
                    }
                }

                // If something is dequeued, then call the function.
                if (dequeued) {
                    fds_verify(NULL != task);
                    try {
                    task->operator()();
                    } catch (std::bad_alloc const& e) {
                      fds_panic("Failed allocation of memory: %s\n", e.what());
                    } catch (std::exception const& e) {
                      fds_panic("std::exception : %s\n", e.what());
                    } catch (...) {
                      fds_panic("unknown exception!");
                    }
                    delete task;
                } else {
                    fds_assert(NULL == task);
                }

            }  // while (true)


        }  // for (;;)
    }
    // This should be cache line size aligned to avoid false sharing.
    alignas(64) int queueCnt = 0;

    int id_;
    bool steal_;
    std::vector<LockfreeWorker*> &peers_;
    // the task queue
    boost::lockfree::queue<LockFreeTask*> tasks;

    // synchronization
    std::atomic<bool> workLoopTerminate;
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
            delete w;
        }
    }
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
    workLoopTerminate(false)
    {
        for (uint32_t i = 0; i < workers.size(); i++) {
            workers[i] = new LFSQWorker(std::bind(&LFSQThreadpool::work, this));
        }
    }

    ~LFSQThreadpool()
    {
        // Set the termination state.
        workLoopTerminate = true;

        // Broadcast to all waiters.
        my_futex(&queueCnt, FUTEX_WAKE_PRIVATE, INT_MAX, NULL, NULL, 0);

        // Wait for all workers to exit.
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

        // After enqueueing to task queue, signal a single waiter.
        __sync_fetch_and_add(&queueCnt, 1);
        my_futex(&queueCnt, FUTEX_WAKE_PRIVATE, 1, NULL, NULL, 0);
    }

    void work() {

        for (;;)
        {
            LockFreeTask *task;
            int old_queueCnt, new_queueCnt;

            while (true) {
                bool dequeued = false;
                task = NULL;

                if (queueCnt == 0) {
                    // block if the queueCnt is 0.  The actual value of queueCnt and
                    // expected value is atomically checked.
                    // Although we intially entered this scope due to queueCnt == 0,
                    // by the time we get here, the state can change where we don't
                    // block here.
                    my_futex(&queueCnt, FUTEX_WAIT_PRIVATE, 0, NULL, NULL, 0);

                    if (workLoopTerminate.load(std::memory_order_relaxed)) {
                        return;
                    }

                    // TODO(Sean)
                    // This cmpxchg loop and the the one in the next block is the same.
                    // For some reason, during the testing, when re-factored the code, it
                    // wasn't working.  Could be a bug in  how I re-factored it, but for
                    // now leave it and revisit re-factoring later.
                    //
                    // See if we  can dequeue.  The steps are (important to maintain the order):
                    // 1) try to decrement atomically in cmpxchg loop.
                    //      1.1) if successful, thene we can dequeue a task.
                    //      1.2) if unsuccessful (i.e. queue is already empty), do nothing.
                    dequeued = true;
                    uint64_t cmpxchgMissed = 0;
                    do {

                        // This is a minor optimization to pause/yield current running logical
                        // processor to the other logical processor on the core.
                        // This will kick in if cmpxchg fails too many times, which implies heavy
                        // contention on the queueCnt.
                        if (cmpxchgMissed & CPU_RELAX_FREQ) {
                            cpu_relax();
                        } else {
                            ++cmpxchgMissed;
                        }

                        old_queueCnt = queueCnt;
                        fds_assert(old_queueCnt >= 0);

                        if (old_queueCnt == 0) {
                            // If the count is 0, then there is nothing in the queue.
                            // Do nothing and restart the outer loop.
                            dequeued = false;
                            break;
                        }

                        // Decrement the count.
                        new_queueCnt = old_queueCnt - 1;

                    } while (!__sync_bool_compare_and_swap(&queueCnt, old_queueCnt, new_queueCnt));

                    // the count says we found something.  Now, grab a task.
                    if (dequeued) {
                        fds_verify(tasks.pop(task));
                    }
                } else {
                    // See if we  can dequeue.  The steps are
                    // 1) try to decrement atomically in cmpxchg loop.
                    //      1.1) if successful, then we can dequeue a task.
                    //      1.2) if unsuccessful (i.e. queue is already empty), do nothing.
                    dequeued = true;
                    uint64_t cmpxchgMissed = 0;
                    do {

                        // This is a minor optimization to pause/yield current running logical
                        // processor to the other logical processor on the core.
                        // This will kick in if cmpxchg fails too many times, which implies heavy
                        // contention on the queueCnt.
                        if (cmpxchgMissed & CPU_RELAX_FREQ) {
                            if (workLoopTerminate.load(std::memory_order_relaxed)) {
                                return;
                            }

                            cpu_relax();
                        } else {
                            ++cmpxchgMissed;
                        }

                        old_queueCnt = queueCnt;
                        fds_assert(old_queueCnt >= 0);

                        if (old_queueCnt == 0) {
                            // If the count is 0, then there is nothing in the queue.
                            // Do nothing and restart the outer loop.
                            dequeued = false;
                            break;
                        }

                        // Decrement the count.
                        new_queueCnt = old_queueCnt - 1;

                    } while (!__sync_bool_compare_and_swap(&queueCnt, old_queueCnt, new_queueCnt));

                    // the count says we found something.  Now, grab a task.
                    if (dequeued) {
                        fds_verify(tasks.pop(task));
                    }
                }

                // If something is dequeued, then call the function.
                if (dequeued) {
                    fds_assert(NULL != task);
                    task->operator()();
                    delete task;
                } else {
                    fds_assert(NULL == task);
                }
            }  // while (true)
        }  // for (;;)
    }

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

#endif  // LFTHREAD_POOL_H
