/*
 * Copyright 2015 Formation Data Systems, Inc.
 */
#include <util/Log.h>
#include <concurrency/LFThreadpool.h>

namespace fds {

LockfreeWorker::LockfreeWorker(LFMQThreadpool *parent,
                               const std::string &threadpoolId, bool steal,
                               std::vector<LockfreeWorker*> &peers)
:  queueCnt(0),
parent_(parent),
id_(threadpoolId),
steal_(steal),
peers_(peers),
state_(INIT),
tasks(1500),
completedCntr(0),
threadCheckCntr(-1)
{
}

void LockfreeWorker::start() {
    state_ = STARTED;
    worker = new std::thread(&LockfreeWorker::workLoop, this);
}

void LockfreeWorker::finish()
{
    if (worker == nullptr) {
        return;
    }
    state_ = ABORTING;
    while (state_ == ABORTING) {
        /* We loop here because it's possible in workLoop() to miss a wake up.
         * when we are aborting we loop until worker thread wakes up and exits
         * cleanly
         */
        my_futex(&queueCnt, FUTEX_WAKE_PRIVATE, 1, NULL, NULL, 0);
        std::this_thread::sleep_for(std::chrono::microseconds(500));
    }
    fds_assert(state_ == ABORTED);
    worker->join();
    delete worker;
    worker = nullptr;
}

void LockfreeWorker::enqueue(LockFreeTask *t)
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

void LockfreeWorker::workLoop() {
    /* Set id to be combination of threadpool id and this thread id */
    std::stringstream ss;
    ss << id_ << ":" << std::hex<< std::this_thread::get_id() << std::dec;
    id_ = ss.str();

    GLOGNOTIFY << "Starting LFThread worker id: " << id_;

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

                if (state_ == ABORTING) {
                    state_ = ABORTED;
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
                        if (state_ == ABORTING) {
                            state_ = ABORTED;
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
                fds_verify(task);
                try {
                    DBG(auto preTaskTimeMs = util::getTimeStampMillis());
                    threadCheckCntr = parent_->threadCheckCntr;

                    task->operator()();

#ifdef DEBUG
                    auto postTaskTimeMs = util::getTimeStampMillis();
                    if (postTaskTimeMs - preTaskTimeMs > 5000) { \
                        GLOGWARN << logString() <<  " last task execution took: "
                            << postTaskTimeMs - preTaskTimeMs
                            << "ms. Consider breaking up the task";
                    }
#endif
                } catch (std::bad_alloc const& e) {
                    fds_panic("Failed allocation of memory: %s : calling %s\n", e.what(), task->target_type().name());
                } catch (std::invalid_argument const& e) {
                    fds_panic("invalid_argument exception : %s : calling %s\n", e.what(), task->target_type().name());
                } catch (std::exception const& e) {
                    fds_panic("std::exception : %s : calling %s\n", e.what(), task->target_type().name());
                } catch (...) {
                    fds_panic("unknown exception : calling %s !\n", task->target_type().name());
                }
                threadCheckCntr = -1;  // indicates thread is idle
                delete task;
            } else {
                fds_assert(NULL == task);
            }

        }  // while (true)


    }  // for (;;)
}

std::string LockfreeWorker::logString() const
{
    return id_;
}

LFMQThreadpool::LFMQThreadpool(const std::string &id, uint32_t sz, bool steal)
: workers(sz),
    idx(0),
    threadCheckCntr(0)
{
    for (uint32_t i = 0; i < workers.size(); i++) {
        workers[i] = new LockfreeWorker(this, id, steal, workers);
    }
    for (uint32_t i = 0; i < workers.size(); i++) {
        workers[i]->start();
    }
}

LFMQThreadpool::LFMQThreadpool(uint32_t sz, bool steal)
: LFMQThreadpool("UnnamedThreadpool", sz, steal)
{
}

LFMQThreadpool::~LFMQThreadpool()
{
    for (auto &w : workers) {
        w->finish();
        delete w;
    }
}

void LFMQThreadpool::threadpoolCheck() {
    for (const auto& worker : workers) {
        if (worker->threadCheckCntr >= 0 &&
            threadCheckCntr - worker->threadCheckCntr > 1) {
            GLOGWARN << worker->logString()
                <<  " thread seems blocked for thread check cycles: "
                << (threadCheckCntr - worker->threadCheckCntr);
        }
    }
    ++threadCheckCntr;
}

void LFMQThreadpool::stop()
{
    for (auto &w : workers) {
        w->finish();
    }
}

LFSQThreadpool::LFSQThreadpool(uint32_t sz)
: workers(sz),
    tasks(1500),
    workLoopTerminate(false)
{
    for (uint32_t i = 0; i < workers.size(); i++) {
        workers[i] = new LFSQWorker(std::bind(&LFSQThreadpool::work, this));
    }
}

LFSQThreadpool::~LFSQThreadpool()
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

void LFSQThreadpool::work() {

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

}  // namespace fds
