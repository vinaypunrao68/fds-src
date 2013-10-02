/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#ifndef FDS_INCLUDE_THREADPOOL_H_
#define FDS_INCLUDE_THREADPOOL_H_

#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/thread/condition.hpp>

#include <fds-include/shared/dlist.h>
#include "include/fds_types.h"
#include "util/concurrency/Mutex.h"

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
    ~thpool_req() {} // assert the list is empty.

    /*
     * Thread pool request constructors.
     * These are all basically wrappers around boost::bind for varying
     * parameter combinations to make the interface a bit cleaner.
     */
    template<typename F, typename A>
    thpool_req(F f, A a) : Task(boost::bind(f, a)) {
        dlist_obj_init(&thp_link, this);
    }
    template<typename F, typename A, typename B>
    thpool_req(F f, A a, B b) : Task(boost::bind(f, a, b)) {
        dlist_obj_init(&thp_link, this);
    }
    template<typename F, typename A, typename B, typename C>
    thpool_req(F f, A a, B b, C c) : Task(boost::bind(f, a, b, c)) {
        dlist_obj_init(&thp_link, this);
    }
    template<typename F, typename A, typename B, typename C, typename D>
    thpool_req(F f, A a, B b, C c, D d) : Task(boost::bind(f, a, b, c, d)) {
        dlist_obj_init(&thp_link, this);
    }
    template<
        typename F, typename A, typename B,
        typename C, typename D, typename E
    >
    thpool_req(F f, A a, B b, C c, D d, E e)
        : Task(boost::bind(f, a, b, c, d, e)) {
        dlist_obj_init(&thp_link, this);
    }
    /** \thp_chain_task
     * ----------------
     * Chain this task to a list.
     */
    inline void
    thp_chain_task(dlist_t *list)
    {
        dlist_obj_chain_back(list, &thp_link);
    }
    /** \thp_init_chain_link
     * ---------------------
     * Initialize the chain link of this object.
     */
    inline void
    thp_init_chain_link(void)
    {
        dlist_init(&thp_link.dl_link);
    }
    /** \thp_empty_chain_link
     * ----------------------
     * Return boolean status if the request is chained elsewhere.
     */
    inline bool
    thp_empty_chain_link(void)
    {
        return dlist_empty(&thp_link.dl_link);
    }
    /** \thp_task_from_dlist
     * ---------------------
     * Return the owner task from the link ptr.
     */
    static inline thpool_req *
    thp_task_from_dlist(dlist_t *link)
    {
        return (thpool_req *)dlist_obj_from_link(link);
    }

  private:
    friend class thpool_worker;

    thpool_req() {}
    dlist_obj_t         thp_link;
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
    fds_uint32_t        thp_max_tasks;     /* max pending tasks. */
    fds_uint32_t        thp_tasks_pend;
    fds_uint32_t        thp_act_threads;
    fds_uint32_t        thp_num_threads;
    fds_uint32_t        thp_barrier_wait;

    /* Thread pool stats. */
    fds_uint32_t        thp_total_tasks;
    fds_uint32_t        thp_exec_direct;

    /* Called by the worker thread to dequeue or put itself to idle state. */
    thpool_req *thp_dequeue_task_or_idle(thpool_worker *worker);

    /* Worker notifies the pool owner when its thread exits. */
    void thp_worker_exits(thpool_worker *worker);

  public:
    explicit fds_threadpool(fds_uint32_t num_threads = 10);
    ~fds_threadpool();

    /* Block until all preceding pending tasks are completed. */
    void thp_barrier();

    /* Scheduling functions. */
    void schedule(thpool_req *task);

    template<typename F, typename A>
    void schedule(F f, A a)
    {
       schedule(new thpool_req(f, a));
    }
    template<typename F, typename A, typename B>
    void schedule(F f, A a, B b)
    {
       schedule(new thpool_req(f, a, b));
    }
    template<typename F, typename A, typename B, typename C>
    void schedule(F f, A a, B b, C c)
    {
       schedule(new thpool_req(f, a, b, c));
    }
    template<typename F, typename A, typename B, typename C, typename D>
    void schedule(F f, A a, B b, C c, D d)
    {
       schedule(new thpool_req(f, a, b, c, d));
    }
    template<
        typename F, typename A, typename B,
        typename C, typename D, typename E
    >
    void schedule(F f, A a, B b, C c, D d, E e)
    {
       schedule(new thpool_req(f, a, b, c, d, e));
    }
};
}  // namespace fds

#endif  // SOURCE_UTIL_CONCURRENCY_THREADPOOL_H_
