/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#include <limits.h>
#include <iostream>
#include <boost/date_time/posix_time/posix_time_types.hpp>

#include <fds_assert.h>
#include <concurrency/ThreadPool.h>

namespace fds {

class thpool_worker
{
  private:
    friend class fds_threadpool;
    bool wk_spawn_thread(void);

    int                   wk_pool_idx;
    int                   wk_idle_sec;
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
        fds_assert(wk_pool_idx < wk_owner->thp_max_threads);
        fds_assert(this == wk_owner->thp_workers[wk_pool_idx]);
    }
    /** \wk_exec_task
     * --------------
     * Execute the task and destroy it.
     */
    inline void wk_exec_task(thpool_req *task)
    {
        fds_assert(task->thp_empty_chain_link() == true);
        wk_prev_state = wk_curr_state;
        wk_curr_state = RUNNING;

        /* TODO(Vy): implement timewait queue! */
        if (task->thp_sched_tck != 0) {
            sleep(task->thp_sched_tck);
        }
        (*task)();
        delete task;
    }
    /** \wk_make_idle
     * --------------
     * Make the worker thread idle.
     */
    inline void wk_make_idle(void)
    {
        /* TODO: fds_assert that the lock is owned. */
        fds_assert(wk_curr_state == RUNNING);
        fds_assert(dlist_empty(&wk_link));

        wk_prev_state = wk_curr_state;
        wk_curr_state = IDLE;
        dlist_add_front(&wk_owner->thp_wk_idle, &wk_link);
    }
    /** \wk_make_term
     * --------------
     * Make the worker thread terminate but don't free this obj.
     */
    inline void wk_make_term(void)
    {
        fds_assert(dlist_empty(&wk_link));

        wk_prev_state = wk_curr_state;
        wk_curr_state = TERM;
        dlist_add_front(&wk_owner->thp_wk_term, &wk_link);
    }
    /** \wk_wakeup_with_task
     * ---------------------
     * Wake up the worker thread on direct job submitted to it.
     */
    inline void wk_wakeup_with_task(thpool_req *task)
    {
        /* TODO: fds_assert that the lock is owned. */
        fds_assert(wk_curr_state == IDLE);

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
        /* TODO: fds_assert that the lock is owned. */
        fds_assert(wk_owner->thp_state == EXITING);
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
    fds_assert(wk_act_task == nullptr);
    fds_assert(dlist_empty(&wk_link));
}

/** \thpool_worker::wk_spawn_thread
 * --------------------------------
 */
bool
thpool_worker::wk_spawn_thread(void)
{
    wk_verify_obj();
    fds_assert((wk_curr_state == INIT) || (wk_curr_state == TERM));

    wk_owner->thp_mutex.lock();
    dlist_rm_init(&wk_link);
    wk_owner->thp_mutex.unlock();

    fds_assert(dlist_empty(&wk_link));
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
    fds_assert(dlist_empty(&wk_link));
    fds_assert(wk_curr_state == SPAWNING);

    wk_owner->thp_mutex.lock();
    if (wk_owner->thp_spawning > 0) {
        wk_owner->thp_spawning--;
        std::cout << "Spawning thr " << wk_pool_idx << "\n";
    }
    wk_owner->thp_mutex.unlock();

    task = nullptr;
    wk_curr_state = RUNNING;
    for (run = true; run == true; ) {
        if (task != nullptr) {
            wk_owner->thp_house_keeping();
            wk_exec_task(task);
        }
        /* We are done with our direct task, exec any pending ones. */
        while ((task = wk_owner->thp_dequeue_task_or_idle(this)) != 0) {
            wk_owner->thp_house_keeping();
            wk_exec_task(task);
        }
        wk_owner->thp_mutex.lock();
        while ((wk_act_task == nullptr) && (wk_owner->thp_state != EXITING)) {
            fds_assert(wk_curr_state == IDLE);
            fds_assert(!dlist_empty(&wk_link));

            if (wk_idle_sec > 0) {
                if (!wk_condition.timed_wait(wk_owner->thp_mutex,
                        boost::posix_time::seconds(wk_idle_sec))) {
                    /* Timeout, exit the idle thread */
                    run = false;
                    dlist_rm_init(&wk_link);
                    std::cout << "Thr " << wk_pool_idx << " idle, exit...\n";
                    break;
                }
            } else {
                wk_condition.wait(wk_owner->thp_mutex);
            }
        }
        if (wk_owner->thp_state == EXITING) {
            dlist_rm_init(&wk_link);
        }
        wk_owner->thp_mutex.unlock();

        fds_assert(dlist_empty(&wk_link));
        task = wk_act_task;
        if (task != nullptr) {
            wk_act_task = nullptr;
            fds_assert((wk_curr_state == WAKING_UP) ||
                       (wk_curr_state == EXITING));
        } else {
            run = false;
        }
    }
    wk_owner->thp_house_keeping();
    wk_owner->thp_worker_exits(this);
}

/** \fds_threadpool constructor
 * ----------------------------
 */
fds_threadpool::fds_threadpool(int min_thr)
    : fds_threadpool(-1, -1, -1, min_thr, min_thr) {}

fds_threadpool::fds_threadpool(int max_tsk, int spawn_thres,
                               int idle_sec, int min_thr, int max_thr)
    : thp_mutex("thpool mtx"),
      thp_state(RUNNING),
      thp_total_tasks(0),
      thp_exec_direct(0),
      thp_act_threads(0),
      thp_max_tasks(max_tsk),
      thp_thres_spawn(spawn_thres),
      thp_idle_sec(idle_sec),
      thp_max_threads(max_thr),
      thp_num_threads(min_thr),
      thp_tasks_pend(0),
      thp_spawning(0)
{
    int            i;
    // dlist_t       *iter;
    thpool_worker *worker;

    fds_assert(min_thr <= max_thr);
    fds_assert((min_thr > 0) && (max_thr > 0));
    if (thp_max_tasks <= 0) {
        thp_max_tasks = INT_MAX;
    }
    if (thp_thres_spawn <= 0) {
        thp_thres_spawn = INT_MAX;
    }
    dlist_init(&thp_wk_idle);
    dlist_init(&thp_wk_term);
    dlist_init(&thp_tasks);

    thp_workers = new thpool_worker * [max_thr];
    for (i = 0; i < min_thr; i++) {
        thp_workers[i] = new thpool_worker(this, i);

        /* We never terminate these threads. */
        thp_workers[i]->wk_idle_sec = -1;
        dlist_add_back(&thp_wk_term, &thp_workers[i]->wk_link);
    }
    for (; i < max_thr; i++) {
        thp_workers[i] = new thpool_worker(this, i);
        thp_workers[i]->wk_idle_sec = thp_idle_sec;
        dlist_add_back(&thp_wk_term, &thp_workers[i]->wk_link);
    }
    /* Spawn threads when all objects have been constructed. */
    for (i = 0; i < min_thr; i++) {
        worker = thp_workers[i];
        if (worker->wk_spawn_thread()) {
            thp_act_threads++;
        }
    }
}

/** \fds_threadpool destructor
 * ---------------------------
 */
fds_threadpool::~fds_threadpool()
{
    int            i;
    dlist_t       *ptr;
    thpool_worker *worker;

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
    fds_assert(dlist_empty(&thp_tasks));
    fds_assert(dlist_empty(&thp_wk_idle));

    thp_state = TERM;
    for (i = 0; !dlist_empty(&thp_wk_term); i++) {
        ptr    = dlist_rm_front(&thp_wk_term);
        worker = fds_object_of(thpool_worker, wk_link, ptr);

        fds_assert(worker->wk_owner == this);
        fds_assert(worker->wk_pool_idx < thp_max_threads);
        fds_assert(thp_workers[worker->wk_pool_idx] == worker);
        delete worker;
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

/** \thp_house_keeping
 * -------------------
 * Do common house keeping jobs to start new thread if number of pending req
 * exceeds the threshold.
 */
void
fds_threadpool::thp_house_keeping()
{
    dlist_t       *ptr;
    thpool_worker *worker;

    worker = nullptr;
    if (thp_tasks_pend < thp_thres_spawn) {
        return;
    }
    thp_mutex.lock();
    if (thp_spawning == 0) {
        ptr = dlist_rm_front(&thp_wk_term);
        if (ptr != nullptr) {
            thp_spawning++;
            worker = fds_object_of(thpool_worker, wk_link, ptr);
        }
    }
    thp_mutex.unlock();
    if (worker != nullptr) {
        worker->wk_spawn_thread();
    }
}

/** \fds_threadpool::thp_worker_exits
 * ----------------------------------
 * Worker thread notifies the owner when it exits the main loop, terminate
 * the thread.
 */
void
fds_threadpool::thp_worker_exits(thpool_worker *worker)
{
    fds_assert(worker->wk_act_task == nullptr);
    fds_assert(dlist_empty(&worker->wk_link));

    thp_mutex.lock();
    thp_act_threads--;
    if (thp_act_threads == 0) {
        thp_condition.notify_one();
    }
    worker->wk_make_term();
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

    fds_assert(thp_state != TERM);
    thp_mutex.lock();
    thp_total_tasks++;
    if (!dlist_empty(&thp_wk_idle)) {
        ptr    = dlist_rm_front(&thp_wk_idle);
        worker = fds_object_of(thpool_worker, wk_link, ptr);

        worker->wk_verify_obj();
        fds_assert(worker->wk_curr_state == IDLE);
        fds_assert(task->thp_empty_chain_link() == true);

        thp_exec_direct++;
        worker->wk_wakeup_with_task(task);
    } else {
        thp_tasks_pend++;
        task->thp_chain_task(&thp_tasks);
    }
    thp_mutex.unlock();
    thp_house_keeping();
}

// thp_periodic_work
// -----------------
// Implement timer service here.
//
void
fds_threadpool::thp_periodic_work()
{
    while (1) {
        thp_cur_tck++;
        sleep(1);
    }
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

    fds_assert(dlist_empty(&worker->wk_link));
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
