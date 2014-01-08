/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#include <boost/date_time/posix_time/posix_time.hpp>

#include <concurrency/Synchronization.h>

namespace fds {

fds_notification::fds_notification()
    : _mutex("notification mutex"),
      cur_ok_waits(1),
      cur_notifies(0) {
}

void fds_notification::wait_for_notification()
{
    /*
     * Note scoped_lock allocation implicitly
     * acquires the lock. The lock is also implicitly
     * released when the lock's scope ends.
     */
    fds_scoped_lock _slock(_mutex);

    /*
     * The while loop ensures that we only have a single
     * outstanding notification to a waiting thread at a time.
     */
    while (cur_ok_waits != cur_notifies) {
        /* Scoped lock is released while waiting. */
        _condition.wait(_slock.boost());
    }
    cur_ok_waits++;
}

bool fds_notification::timed_wait_for_notification(int millis)
{
    fds_scoped_lock _slock(_mutex);
    if (cur_ok_waits != cur_notifies) {
        _condition.timed_wait(_slock.boost(),
            boost::posix_time::milliseconds(millis));
    }
    /*
     * If we timed out, then we weren't notified
     * and we return false to indicate timeout.
     */
    if (cur_ok_waits != cur_notifies) {
        return false;
    }
    /*
     * We were notified, so return true to reflect.
     */
    cur_ok_waits++;
    return true;
}

/*
 * Notifies a single waiter.
 */
void fds_notification::notify()
{
    fds_scoped_lock _slock(_mutex);
    assert(cur_notifies != cur_ok_waits);
    cur_notifies++;
    _condition.notify_one();
}

fds_notify_all::fds_notify_all()
    : _mutex("notify_all mutex"),
      _last_done(0),
      _last_returned(0),
      _num_waiting(0) {
}

/*
 * Incs and returns the counter.
 */
fds_notify_all::when fds_notify_all::now()
{
    fds_scoped_lock _slock(_mutex);
    return ++_last_returned;
}

/*
 * Waits for a specific notification epoch.
 */
void fds_notify_all::wait_for(when e)
{
    fds_scoped_lock _slock(_mutex);
    ++_num_waiting;
    /*
     * Keep waiting until at least 'e'
     * was done.
     */
    while (_last_done < e) {
        _condition.wait(_slock.boost());
    }
}

void fds_notify_all::await_beyond_now()
{
    fds_scoped_lock _slock(_mutex);
    ++_num_waiting;
    when e = ++_last_returned;
    /*
     * Wait for the next notification
     * that's after now().
     */
    while (_last_done <= e) {
        _condition.wait(_slock.boost());
    }
}

void fds_notify_all::notify_all(when e)
{
    fds_scoped_lock _slock(_mutex);
    _last_done   = e;
    _num_waiting = 0;
    _condition.notify_all();
}

}  // namespace fds
