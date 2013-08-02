/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#ifndef SOURCE_UTIL_CONCURRENCY_THREADPOOL_H_
#define SOURCE_UTIL_CONCURRENCY_THREADPOOL_H_


#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/thread/condition.hpp>

#include <list>

#include "lib/fds_types.h"
#include "util/concurrency/Mutex.h"
#include "util/concurrency/Thread.h"

namespace fds {

  /*
   * A nullary function or functor to represent a task.
   * Will generally be the result of a boost::bind.
   */
  typedef boost::function<void(void)> Task;

  class fds_threadpool;

  class Worker : boost::noncopyable {
 private:
    fds_threadpool* _owner;
    bool _is_done;
    boost::thread _thread;
    fds_mutex _list_mutex;
    std::list<Task> _task;

    void loop();
 public:
    explicit Worker(fds_threadpool* owner);

    ~Worker();

    void set_task(const Task& func);
  };

  class fds_threadpool : boost::noncopyable {
 private:
    fds_mutex _mutex;
    boost::condition _condition;

    /*
     * A LIFO stack of worker threads
     */
    std::list<Worker*> _free_workers;

    /*
     * A FIFO queue of jobs
     */
    std::list<Task> _tasks;

    fds_uint32_t _tasks_remaining;
    fds_uint32_t _num_threads;

    /*
     * Should be called from a worker thread.
     */
    void task_done(Worker* worker);
    friend class Worker;

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
     * completed. New tasks can still be scheduled
     * and this funciton will continue to wait for
     * those. Tasks can still be scheduled after
     * this returns.
     */
    void join();

    /*
     * Scheduling functions. These are all basically
     * wrappers around boost::bind for varying parameter
     * combinations to make the interface a bit cleaner.
     */
    void schedule(Task task);
    template<typename F, typename A>
        void schedule(F f, A a) {
      schedule(boost::bind(f, a));
    }
    template<typename F, typename A, typename B>
        void schedule(F f, A a, B b) {
      schedule(boost::bind(f, a, b));
    }
    template<typename F, typename A, typename B, typename C>
        void schedule(F f, A a, B b, C c) {
      schedule(boost::bind(f, a, b, c));
    }
    template<typename F, typename A, typename B, typename C, typename D>
        void schedule(F f, A a, B b, C c, D d) {
      schedule(boost::bind(f, a, b, c, d));
    }
    template<typename F,
        typename A,
        typename B,
        typename C,
        typename D,
        typename E>
        void schedule(F f, A a, B b, C c, D d, E e) {
      schedule(boost::bind(f, a, b, c, d, e));
    }
  };
}  // namespace fds

#endif  // SOURCE_UTIL_CONCURRENCY_THREADPOOL_H_
