/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#include <assert.h>
#include <iostream>
#include <concurrency/ThreadPool.h>

using namespace std;
namespace fds {

class thpool_worker
{
  private:
    friend class fds_threadpool;
    bool wk_spawn_thread(void);

    int                   wk_pool_idx;
    dlist_t               wk_link;        // protected by owner's lock.
    thp_state_e           wk_curr_state;  // the thread's private state.
    thp_state_e           wk_prev_state;  // the thread's private state.
    boost::condition      wk_condition;   // synchronize this thread alone.
    fds_threadpool       *wk_owner;
    boost::thread        *wk_thread;      // private to the worker thread.
    thpool_req           *wk_act_task;    // owner set, pick up by this thread.

    /** \wk_verify_obj
     * ---------------
     * Do consistency check on the thread worker obj.
     */
    inline void wk_verify_obj(void)
    {
        assert(wk_pool_idx < wk_owner->thp_num_threads);
        assert(this == wk_owner->thp_workers[wk_pool_idx]);
    }
    /** \wk_exec_task
     * --------------
     * Execute the task and destroy it.
     */
    inline void wk_exec_task(thpool_req *task)
    {
        assert(task->thp_empty_chain_link() == true);
        wk_prev_state = wk_curr_state;
        wk_curr_state = RUNNING;
        (*task)();
        delete task;
    }
    /** \wk_make_idle
     * --------------
     * Make the worker thread idle.
     */
    inline void wk_make_idle(void)
    {
        /* TODO: assert that the lock is owned. */
        assert(wk_curr_state == RUNNING);
        assert(dlist_empty(&wk_link));

        wk_prev_state = wk_curr_state;
        wk_curr_state = IDLE;
        dlist_add_front(&wk_owner->thp_wk_idle, &wk_link);
    }
    /** \wk_wakeup_with_task
     * ---------------------
     * Wake up the worker thread on direct job submitted to it.
     */
    inline void wk_wakeup_with_task(thpool_req *task)
    {
        /* TODO: assert that the lock is owned. */
        assert(wk_curr_state == IDLE);

        wk_act_task   = task;
        wk_prev_state = wk_curr_state;
        wk_curr_state = WAKING_UP;
        dlist_init(&wk_link);
        wk_condition.notify_one();
    }
    /** \wk_wakeup
     * -----------
     * Wake up the worker thread when the threadpool wants to exit.  The
     * worker thread still drains all pending tasks until it's done.  Don't
     * care if the signal gets lost because the worker is running.
     */
    inline void wk_wakeup(void)
    {
        /* TODO: assert that the lock is owned. */
        assert(wk_owner->thp_state == EXITING);
        wk_condition.notify_one();
    }
  public:
    thpool_worker(fds_threadpool *owner, int idx);
    ~thpool_worker();

    void wk_loop(void);
};

/** \thpool_worker constructor
 * ---------------------------
 * Create a default object without any thread
 *
 * @param owner (i) Reference to owning threadpool
 * @param index (i) Index of the worker in owner's pool.
 */
thpool_worker::thpool_worker(fds_threadpool *owner, int index)
    : wk_curr_state(INIT),
      wk_prev_state(INIT),
      wk_owner(owner),
      wk_pool_idx(index),
      wk_thread(nullptr),
      wk_act_task(nullptr),
      wk_condition()
{
    dlist_init(&wk_link);
}

/** \thpool_worker destructor
 * --------------------------
 */
thpool_worker::~thpool_worker()
{
    /* TODO: not sure what to do with wk_thread */
    wk_verify_obj();
    assert(wk_act_task == nullptr);
    assert(dlist_empty(&wk_link));
}

/** \thpool_worker::wk_spawn_thread
 * --------------------------------
 */
bool
thpool_worker::wk_spawn_thread(void)
{
    wk_verify_obj();
    assert(wk_curr_state == INIT);
    assert(dlist_empty(&wk_link));

    wk_curr_state = SPAWNING;
    wk_thread = new boost::thread(boost::bind(&thpool_worker::wk_loop, this));
    return true;
}

/** \thpool_worker::wk_loop
 * -------------------------
 */
void
thpool_worker::wk_loop(void)
{
    bool         run;
    thpool_req  *task;;

    wk_verify_obj();
    assert(dlist_empty(&wk_link));
    assert(wk_curr_state == SPAWNING);

    task = nullptr;
    wk_curr_state = RUNNING;
    for (run = true; run == true; ) {
        if (task != nullptr) {
            wk_exec_task(task);
        }
        /* We are done with our direct task, exec any pending ones. */
        while ((task = wk_owner->thp_dequeue_task_or_idle(this)) != 0) {
            wk_exec_task(task);
        }
        wk_owner->thp_mutex.lock();
        while ((wk_act_task == nullptr) && (wk_owner->thp_state != EXITING)) {
            assert(wk_curr_state == IDLE);
            assert(!dlist_empty(&wk_link));
            wk_condition.wait(wk_owner->thp_mutex);
        }
        if (wk_owner->thp_state == EXITING) {
            dlist_rm_init(&wk_link);
        }
        wk_owner->thp_mutex.unlock();

        assert(dlist_empty(&wk_link));
        task = wk_act_task;
        if (task != nullptr) {
            wk_act_task = nullptr;
            assert((wk_curr_state == WAKING_UP) || (wk_curr_state == EXITING));
        } else {
            run = false;
        }
    }
    wk_owner->thp_worker_exits(this);
}

/** \fds_threadpool constructor
 * ----------------------------
 */
fds_threadpool::fds_threadpool(fds_uint32_t num_threads)
    : thp_mutex("thpool mtx"),
      thp_state(RUNNING),
      thp_tasks_pend(0),
      thp_total_tasks(0),
      thp_exec_direct(0),
      thp_act_threads(0),
      thp_num_threads(num_threads)
{
    int            i;
    dlist_t       *iter;
    thpool_worker *worker;

    dlist_init(&thp_wk_idle);
    dlist_init(&thp_tasks);

    /* First version, spawn all threads, no dynamic scale up/down. */
    thp_workers = new thpool_worker * [num_threads];
    for (i = 0; i < num_threads; i++) {
        thp_workers[i] = new thpool_worker(this, i);
    }
    /* Spawn threads when all objects have been constructed. */
    for (i = 0; i < num_threads; i++) {
        worker = thp_workers[i];
        if (worker->wk_spawn_thread()) {
            /* TODO: use atomic type. */
            thp_act_threads++;
        }
    }
}

/** \fds_threadpool destructor
 * ---------------------------
 */
fds_threadpool::~fds_threadpool()
{
    int i;

    thp_mutex.lock();
    thp_state = EXITING;

    /* Wake up all worker threads. */
    for (i = 0; i < thp_num_threads; i++) {
        thp_workers[i]->wk_wakeup();
    }
    while (thp_act_threads > 0) {
        thp_condition.wait(thp_mutex);
    }
    /* all threads exited now. */
    assert(dlist_empty(&thp_tasks));
    assert(dlist_empty(&thp_wk_idle));

    thp_state = TERM;
    for (i = 0; i < thp_num_threads; i++) {
        assert(thp_workers[i] != nullptr);
        delete thp_workers[i];
    }
    thp_mutex.unlock();

    delete [] thp_workers;
}

/** \fds_threadpool::thp_barrier
 * -----------------------------
 */
void
fds_threadpool::thp_barrier()
{
}

/** \fds_threadpool::thp_worker_exits
 * ----------------------------------
 * Worker thread notifies the owner when it exits the main loop, terminate
 * the thread.
 */
void
fds_threadpool::thp_worker_exits(thpool_worker *worker)
{
    assert(worker->wk_act_task == nullptr);
    assert(dlist_empty(&worker->wk_link));

    thp_mutex.lock();
    thp_act_threads--;
    if (thp_act_threads == 0) {
        thp_condition.notify_one();
    }
    thp_mutex.unlock();
}

/** \fds_threadpool::schedule
 * --------------------------
 * Schedules a task to be run.
 *
 * @param task (i) Nullary pointer to task function
 */
void
fds_threadpool::schedule(thpool_req *task)
{
    dlist_t       *ptr;
    thpool_worker *worker;

    assert(thp_state != TERM);
    thp_mutex.lock();
    thp_total_tasks++;
    if (!dlist_empty(&thp_wk_idle)) {
        ptr    = dlist_rm_front(&thp_wk_idle);
        worker = fds_object_of(thpool_worker, wk_link, ptr);

        worker->wk_verify_obj();
        assert(worker->wk_curr_state == IDLE);
        assert(task->thp_empty_chain_link() == true);

        thp_exec_direct++;
        worker->wk_wakeup_with_task(task);
    } else {
        /* TODO: QoS or enforce quota, don't let pending grows > max. */
        thp_tasks_pend++;
        task->thp_chain_task(&thp_tasks);
    }
    thp_mutex.unlock();
}

/** \fds_threadpool::thp_dequeue_task_or_idle
 * ------------------------------------------
 * Dequeue the first eligible task out of the common queue and submit it to the
 * worker's request.  If the queue is empty, queue the worker to the idle list.
 */
thpool_req *
fds_threadpool::thp_dequeue_task_or_idle(thpool_worker *worker)
{
    dlist_t    *ptr;
    thpool_req *task;

    assert(dlist_empty(&worker->wk_link));
    thp_mutex.lock();
    if (!dlist_empty(&thp_tasks)) {
        thp_tasks_pend--;
        ptr  = dlist_rm_front(&thp_tasks);
        task = thpool_req::thp_task_from_dlist(ptr);
        task->thp_init_chain_link();
    } else {
        worker->wk_make_idle();
        task = nullptr;
    }
    thp_mutex.unlock();
    return task;
}

}  // namespace fds
