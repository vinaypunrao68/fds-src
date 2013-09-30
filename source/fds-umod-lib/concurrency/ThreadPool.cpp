/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#include <assert.h>
#include <iostream>
#include <ThreadPool.h>
#include "util/concurrency/Thread.h"

using namespace std;
namespace fds {

class thpool_worker : boost::noncopyable
{
  private:
    friend class fds_threadpool;
    void wk_spawn_thread(void);

    int                 _wk_pool_idx;
    dlist_t             _wk_link;      // protected by owner's lock.
    thp_state_e         _wk_state;     // the thread's private state.
    fds_threadpool     *_wk_owner;     // read-only data.
    boost::thread      *_wk_thread;    // private to the worker thread.
    thpool_req         *_wk_act_task;  // owner set, pick up by this thread.

    /* Execute the task and destroy it. */
    inline void wk_exec_task(thpool_req *task)
    {
        assert(dlist_empty(&task->_thp_link));
        (*task)();
        delete task;
    }
    /* Do consistency check on the thread worker obj. */
    inline void wk_verify_obj(void)
    {
        fds_threadpool *pool = _wk_owner;

        assert(_wk_pool_idx < pool->_thp_num_threads);
        assert(this == pool->_thp_workers[_wk_pool_idx]);
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
    : _wk_state(INIT),
      _wk_owner(owner),
      _wk_pool_idx(index),
      _wk_thread(0),
      _wk_act_task(0)
{
    dlist_init(&_wk_link);
}

/** \thpool_worker destructor
 * --------------------------
 */
thpool_worker::~thpool_worker()
{
}

/** \thpool_worker::wk_spawn_thread
 * --------------------------------
 */
void
thpool_worker::wk_spawn_thread(void)
{
    wk_verify_obj();
    assert(_wk_state == INIT);
    assert(!dlist_empty(&_wk_link));

    _wk_state  = SPAWNING;
    _wk_thread = new boost::thread(boost::bind(&thpool_worker::wk_loop, this));
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
    assert(_wk_state == SPAWNING);
    assert(dlist_empty(&_wk_link));

    task = 0;
    _wk_state = RUNNING;
    for (run = true; run == true; ) {
        if (task != 0) {
            /* TODO: convert to atomic */
            _wk_state = RUNNING;
            _wk_owner->_thp_exec_direct++;
            wk_exec_task(task);
        }
        /* We are done with our direct task, exec any pending ones. */
        while ((task = _wk_owner->thp_dequeue_task_or_idle(this)) != 0) {
            wk_exec_task(task);
        }
        _wk_owner->_thp_mutex.lock();
        while ((_wk_act_task == 0) && (_wk_owner->_thp_state != EXITING)) {
            assert(_wk_state == IDLE);
            assert(!dlist_empty(&_wk_link));
            _wk_owner->_thp_cond.wait(_wk_owner->_thp_mutex);
        }
        task = _wk_act_task;
        if (task != 0) {
            _wk_act_task = 0;
            assert(_wk_state == WAKING_UP);
        } else {
            run = false;
        }
        _wk_owner->_thp_mutex.unlock();
        assert(dlist_empty(&_wk_link));
    }
}

/** \fds_threadpool constructor
 * ----------------------------
 */
fds_threadpool::fds_threadpool(fds_uint32_t num_threads)
    : _thp_mutex("thpool mtx"),
      _thp_state(RUNNING),
      _thp_tasks_pend(0),
      _thp_total_tasks(0),
      _thp_exec_direct(0),
      _thp_exec_dequeue(0),
      _thp_num_threads(num_threads)
{
    int            i;
    dlist_t       *iter;
    thpool_worker *worker;

    dlist_init(&_thp_wk_idle);
    dlist_init(&_thp_tasks);

    /* First version, spawn all threads, no dynamic scale up/down. */
    _thp_workers = new thpool_worker * [num_threads];
    for (i = 0; i < num_threads; i++) {
        _thp_workers[i] = new thpool_worker(this, i);
    }
    /* Spawn threads when all objects have been constructed. */
    for (i = 0; i < num_threads; i++) {
        worker = _thp_workers[i];
        // worker->wk_spawn_thread();
    }
    assert(i == num_threads);
}

/** \fds_threadpool destructor
 * ---------------------------
 */
fds_threadpool::~fds_threadpool()
{
}

/** \fds_threadpool::thp_join
 * --------------------------
 */
void
fds_threadpool::thp_join()
{
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
    dlist_add_back(&_thp_tasks, &task->_thp_link);
    (*task)();
}

/** \fds_threadpool::thp_dequeue_task_or_idle
 * ------------------------------------------
 */
thpool_req *
fds_threadpool::thp_dequeue_task_or_idle(thpool_worker *worker)
{
    return 0;
}

}  // namespace fds
