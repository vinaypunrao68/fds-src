/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#ifndef SOURCE_UTIL_CONCURRENCY_SYNCHRONIZATION_H_
#define SOURCE_UTIL_CONCURRENCY_SYNCHRONIZATION_H_

#include <boost/noncopyable.hpp>
#include <boost/thread/condition.hpp>
#include <boost/thread/locks.hpp>

#include "lib/fds_types.h"
#include "util/concurrency/Mutex.h"

namespace fds {

  /*
   * Class to provide notification to a single
   * waiter.
   * This provide a condition variable like
   * interface while hiding the boost condition
   * semantics from the user.
   */
  class fds_notification : boost::noncopyable {
 private:
    fds_mutex _mutex;
    fds_uint64_t cur_ok_waits;
    fds_uint64_t cur_notifies;
    boost::condition _condition;

 public:
    fds_notification();

    /*
     * Caller blocks until notified
     */
    void wait_for_notification();

    /*
     * Caller blocks until notified or
     * timeout, whichever happens first.
     */
    bool timed_wait_for_notification(int millis);

    /*
     * Notifies the waiter.
     */
    void notify();
  };

  /*
   * Class to provide notification to a many
   * waiters.
   * This provide a condition variable like
   * interface while hiding the boost condition
   * semantics from the user.
   */
  class fds_notify_all : boost::noncopyable {
 public:
    typedef fds_uint64_t when;

 private:
    fds_mutex _mutex;
    boost::condition _condition;
    when _last_done;
    when _last_returned;
    fds_uint32_t _num_waiting;

 public:
    /*
     * Notify every thread waiting.
     */
    fds_notify_all();

    /*
     * Returns an epoch like value that
     * is larger than the previously
     * notification epoch.
     */
    when now();

    /*
     * Wait for a specific notification epoch.
     */
    void wait_for(when);

    /*
     * Wait for the next notification epoch.
     */
    void await_beyond_now();

    /*
     * Notify all waiters up to and including
     * this epoch.
     */
    void notify_all(when);

    fds_uint32_t num_waiting() const {
      return _num_waiting;
    }
  };
}  // namespace fds

#endif  // SOURCE_UTIL_CONCURRENCY_SYNCHRONIZATION_H_
