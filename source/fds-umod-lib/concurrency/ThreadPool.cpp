/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#include <ThreadPool.h>
#include "util/concurrency/Thread.h"

namespace fds {

class thpool_worker : boost::noncopyable
{
  private:
    fds_threadpool     *_thp_owner;
    // boost::thread::id   _thp_id;
    thp_state_e         _thp_state;
    boost::thread       _thp_worker;
    thpool_req         *_thp_act;

    void thp_work_loop(void);
  public:
    explicit thpool_worker(fds_threadpool *owner);
    ~thpool_worker();
};

/**
 * Worker constructor
 *
 * @param owner (i) Reference to owning threadpool
 *
 * Creating a new worker creates a thread with
 * no initial job to do;
 */
thpool_worker::thpool_worker(fds_threadpool *owner)
{
}

/**
 * Worker destructor
 *
 * Acts as a join waiting for the
 * current operation is complete.
 */
thpool_worker::~thpool_worker()
{
}

/*
 * The worker's work loop. A task is pulled
 * from the _last list and performed.
 */
void
thpool_worker::thp_work_loop()
{
}

fds_threadpool::fds_threadpool(fds_uint32_t num_threads)
    : _thp_num_threads(num_threads),
      _thp_mutex("thpool mtx")
{
    dlist_init(&_thp_idle);
    dlist_init(&_thp_busy);
    dlist_init(&_thp_tasks);
}

/*
 * Thread pool destructor. This waits
 * for any existing tasks to be completed.
 * Though new jobs can still be scheduled
 * this should be avoided as it will block
 * the destruction of the pool indefinitely.
 */
fds_threadpool::~fds_threadpool()
{
}

/*
 * Waits for all tasks to complete. Can
 * be thought of as a joinall(). However,
 * new jobs can still be scheduled while
 * join() is wating and it will then wait
 * for these new jobs as well.
 */
void
fds_threadpool::thp_join()
{
}

/*
 * Schedules a task to be run.
 *
 * @param task (i) Nullary pointer to task function
 *
 * The base way to schedule jobs in the thread pool.
 * Jobs that exceed the current size of the pool are
 * queued for work.
 */
void
fds_threadpool::schedule(thpool_req *task)
{
    dlist_add_back(&_thp_tasks, &task->_thp_link);
}

}  // namespace fds
