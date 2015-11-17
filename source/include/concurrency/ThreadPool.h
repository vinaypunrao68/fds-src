/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#ifndef INCLUDE_CONCURRENCY_THREADPOOL_H_
#define INCLUDE_CONCURRENCY_THREADPOOL_H_

#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
#include <boost/thread/condition.hpp>

#include <shared/dlist.h>
#include <fds_types.h>
#include <concurrency/Mutex.h>
#include <concurrency/Thread.h>
#include <concurrency/LFThreadpool.h>

namespace fds {
class thpool_req;
class thpool_worker;
class fds_threadpool;

enum thp_state_e { INIT, IDLE, TERM, WAKING_UP, RUNNING, SPAWNING, EXITING };

/*
 * A nullary function or functor to represent a task.
 * Will generally be the result of a boost::bind.
 */
typedef boost::function<void(void)> Task;

class thpool_req : public Task
{
  public:
    virtual ~thpool_req() {} // assert the list is empty.

    /*
     * Thread pool request constructors.
     * These are all basically wrappers around boost::bind for varying
     * parameter combinations to make the interface a bit cleaner.
     */
     template <class F, class... Args>
     thpool_req(F&& f, Args&&... args): Task(std::bind(std::forward<F>(f), std::forward<Args>(args)...))
     {
        dlist_obj_init(&thp_link, this);
     }

    /** \thp_chain_task
     * ----------------
     * Chain this task to a list.
     */
    inline void
    thp_chain_task(dlist_t *list) {
        dlist_obj_chain_back(list, &thp_link);
    }
    /** \thp_init_chain_link
     * ---------------------
     * Initialize the chain link of this object.
     */
    inline void
    thp_init_chain_link(void) {
        dlist_init(&thp_link.dl_link);
    }
    /** \thp_empty_chain_link
     * ----------------------
     * Return boolean status if the request is chained elsewhere.
     */
    inline bool
    thp_empty_chain_link(void) {
        return dlist_empty(&thp_link.dl_link);
    }
    /** \thp_task_from_dlist
     * ---------------------
     * Return the owner task from the link ptr.
     */
    static inline thpool_req *
    thp_task_from_dlist(dlist_t *link) {
        return (thpool_req *)dlist_obj_from_link(link);
    }

  private:
    friend class thpool_worker;
    dlist_obj_t         thp_link;

    thpool_req() {}
};

class fds_threadpool : boost::noncopyable
{
  private:
    friend class        thpool_worker;
    fds_mutex           thp_mutex;
    boost::condition    thp_condition;
    thp_state_e         thp_state;         /* state of the pool. */
    thpool_worker     **thp_workers;
    dlist_t             thp_wk_idle;       /* FIFO list of idle workers. */
    dlist_t             thp_tasks;         /* FIFO list of tasks. */
    int                 thp_num_threads;
    int                 thp_active_threads;
    int                 thp_tasks_pend;
    bool                use_lftp_instead;
    /* Thread pool stats. */
    fds_uint32_t        thp_total_tasks;
    fds_uint32_t        thp_exec_direct;

    /* Called by the worker thread to dequeue or put itself to idle state. */
    thpool_req *thp_dequeue_task_or_idle(thpool_worker *worker);

    LFMQThreadpool *lfthreadpool;

  public:
    ~fds_threadpool();

    /**
    * @brief Stops all the threads in the threadpool.  Only implmented for LFThreadpool
    */
    void stop();

    /*
     * Create the threadpool with specified number of thread.
     */
    fds_threadpool(int num_thr = 10, bool use_lftp = true);

    /* Scheduling functions. */
    template <class F, class... Args>
    void schedule(F&& f, Args&&... args);
    
    template <class F, class... Args>
    void scheduleWithAffinity(uint64_t affinity, F&& f, Args&&... args);

    void scheduleTask(thpool_req *task);

    /* Worker notifies the pool owner when its thread exits. */
    void thp_worker_exit(thpool_worker *worker);

};

/*
 * Schedules a task to be run. Either picks up lfthreadpool to schedule the task
 * if config file says use_lftp = true. If not, then a fds threadpool request is
 * created and is scheduled on fds_threadpool.
 */
template<class F, class... Args>
void fds_threadpool::schedule(F&& f, Args&&... args)
{
    if (use_lftp_instead) {
        lfthreadpool->schedule(std::forward<F>(f), std::forward<Args>(args)...);
    } else {
        thpool_req *task = new thpool_req(std::forward<F>(f), std::forward<Args>(args)...);
        scheduleTask(task);
    }
}

template <class F, class... Args>
void fds_threadpool::scheduleWithAffinity(uint64_t affinity, F&& f, Args&&... args)
{
    fds_verify(use_lftp_instead == true);
    lfthreadpool->scheduleWithAffinity(
        affinity,
        std::forward<F>(f), std::forward<Args>(args)...);
}

typedef boost::shared_ptr<fds_threadpool> fds_threadpoolPtr;

}  // namespace fds

#endif  // SOURCE_UTIL_CONCURRENCY_THREADPOOL_H_
