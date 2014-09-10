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
    template<typename F, typename A>
    thpool_req(F f, A a, int sched_tck) : Task(boost::bind(f, a)) {
        thp_sched_tck = sched_tck;
        dlist_obj_init(&thp_link, this);
    }
    template<typename F, typename A, typename B>
    thpool_req(F f, A a, B b, int sched_tck) : Task(boost::bind(f, a, b)) {
        thp_sched_tck = sched_tck;
        dlist_obj_init(&thp_link, this);
    }
    template<typename F, typename A, typename B, typename C>
    thpool_req(F f, A a, B b, C c, int sched_tck) : Task(boost::bind(f, a, b, c)) {
        thp_sched_tck = sched_tck;
        dlist_obj_init(&thp_link, this);
    }
    template<typename F, typename A, typename B, typename C, typename D>
    thpool_req(F f, A a, B b, C c, D d, int tck) : Task(boost::bind(f, a, b, c, d)) {
        thp_sched_tck = tck;
        dlist_obj_init(&thp_link, this);
    }
    template<typename F, typename A, typename B, typename C, typename D, typename E>
    thpool_req(F f, A a, B b, C c, D d, E e, int tck)
        : Task(boost::bind(f, a, b, c, d, e)) {
        thp_sched_tck = tck;
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
    int                 thp_sched_tck;
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
    dlist_t             thp_wk_term;       /* FIFO list of terminate wks */
    dlist_t             thp_tasks;         /* FIFO list of tasks. */
    int                 thp_max_tasks;     /* max pending tasks. */
    int                 thp_act_threads;
    int                 thp_num_threads;
    int                 thp_max_threads;
    int                 thp_thres_spawn;
    int                 thp_idle_sec;
    int                 thp_barrier_wait;
    int                 thp_spawning;
    int                 thp_tasks_pend;
    int                 thp_cur_tck;

    /* Thread pool stats. */
    fds_uint32_t        thp_total_tasks;
    fds_uint32_t        thp_exec_direct;

    /* Called by the worker thread to dequeue or put itself to idle state. */
    thpool_req *thp_dequeue_task_or_idle(thpool_worker *worker);

    /* Worker notifies the pool owner when its thread exits. */
    void thp_worker_exits(thpool_worker *worker);

    /* Threadpool house keeping jobs. */
    void thp_house_keeping();

  public:
    ~fds_threadpool();
    /*
     * Create the threadpool with min, request threshold to spawn new thread,
     * max, and the time in seconds to destroy an indle thread.
     */
    explicit fds_threadpool(int min_thr = 10);
    fds_threadpool(int max_tsk, int spawn_thres,
                   int idle_sec, int min_thr = 10, int max_thr = 10);

    /* Block until all preceding pending tasks are completed. */
    void thp_barrier();
    void thp_periodic_work();

    /* Scheduling functions. */
    void schedule(thpool_req *task);

    template<typename F, typename A>
    void schedule(F f, A a, int tck = 0) {
       schedule(new thpool_req(f, a, tck));
    }
    template<typename F, typename A, typename B>
    void schedule(F f, A a, B b, int tck = 0) {
       schedule(new thpool_req(f, a, b, tck));
    }
    template<typename F, typename A, typename B, typename C>
    void schedule(F f, A a, B b, C c, int tck = 0) {
       schedule(new thpool_req(f, a, b, c, tck));
    }
    template<typename F, typename A, typename B, typename C, typename D>
    void schedule(F f, A a, B b, C c, D d, int tck = 0) {
       schedule(new thpool_req(f, a, b, c, d, tck));
    }
    template<typename F, typename A, typename B, typename C, typename D, typename E>
    void schedule(F f, A a, B b, C c, D d, E e, int tck = 0) {
       schedule(new thpool_req(f, a, b, c, d, e, tck));
    }
    template<typename F, typename A, typename B,
             typename C, typename D, typename E, typename G>
    void schedule(F f, A a, B b, C c, D d, E e, G g, int tck = 0) {
       schedule(new thpool_req(f, a, b, c, d, e, g, tck));
    }
};

typedef boost::shared_ptr<fds_threadpool> fds_threadpoolPtr;

}  // namespace fds

#endif  // SOURCE_UTIL_CONCURRENCY_THREADPOOL_H_
