/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#ifndef FDS_INCLUDE_THREADPOOL_H_
#define FDS_INCLUDE_THREADPOOL_H_

#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/thread/condition.hpp>

#include <shared/dlist.h>
#include "include/fds_types.h"
#include "util/concurrency/Mutex.h"

namespace fds {
class thpool_req;
class thpool_worker;
class fds_threadpool;
enum thp_state_e { IDLE, RUNNING, SPAWNING, EXITING };

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
        dlist_init(&_thp_link);
    }
    template<typename F, typename A, typename B>
    thpool_req(F f, A a, B b) : Task(boost::bind(f, a, b)) {
        dlist_init(&_thp_link);
    }
    template<typename F, typename A, typename B, typename C>
    thpool_req(F f, A a, B b, C c) : Task(boost::bind(f, a, b, c)) {
        dlist_init(&_thp_link);
    }
    template<typename F, typename A, typename B, typename C, typename D>
    thpool_req(F f, A a, B b, C c, D d) : Task(boost::bind(f, a, b, c, d)) {
        dlist_init(&_thp_link);
    }
    template<
        typename F, typename A, typename B,
        typename C, typename D, typename E
    >
    thpool_req(F f, A a, B b, C c, D d, E e)
        : Task(boost::bind(f, a, b, c, d, e)) {
        dlist_init(&_thp_link);
    }

  private:
    thpool_req() {}
    friend class thpool_worker;
    friend class fds_threadpool;

    dlist_t             _thp_link;
};

class fds_threadpool : boost::noncopyable
{
  private:
    fds_mutex           _thp_mutex;
    boost::condition    _thp_condition;
    dlist_t             _thp_idle;       /* FIFO list of idle workers. */
    dlist_t             _thp_busy;       /* keep track of active workers. */
    dlist_t             _thp_tasks;      /* FIFO list of tasks. */
    fds_uint32_t        _thp_tasks_pend;
    fds_uint32_t        _thp_num_threads;

    friend class thpool_worker;

  public:
    explicit fds_threadpool(fds_uint32_t num_threads = 10);

    /*
     * Meant to block until all tasks are completed.
     * As a result, things like schedule() should not
     * called within the destructor.
     */
    ~fds_threadpool();

    /*
     * Blocks until all of the worker tasks are
     * completed. New tasks can still be ndscheduled
     * and this funciton will continue to wait for
     * those. Tasks can still be scheduled after
     * this returns.
     */
    void thp_join();

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
