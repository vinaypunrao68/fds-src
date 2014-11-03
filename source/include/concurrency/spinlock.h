/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#ifndef INCLUDE_CONCURRENCY_SPINLOCK_H_
#define INCLUDE_CONCURRENCY_SPINLOCK_H_

#include <boost/atomic.hpp>
#include <fds_assert.h>

/**
 * Code from http://www.boost.org/doc/libs/1_54_0/doc/html/atomic/usage_examples.html
 */
namespace fds {
class Spinlock : boost::noncopyable {
 public:
    Spinlock() : state_(Unlocked) {}

    void lock()
    {
        while (state_.exchange(Locked, boost::memory_order_acquire) == Locked) {
            /* busy-wait */
        }
    }
    bool try_lock() {
        // TODO(Rao): Implement.  For now always returning false
        fds_verify(!"Not implemented");
        return false;
    }
    void unlock()
    {
        state_.store(Unlocked, boost::memory_order_release);
    }

 private:
    typedef enum {Locked, Unlocked} LockState;
    boost::atomic<LockState> state_;
};

class ScopedSpinlock : boost::noncopyable {
 public:
    /*
     * The constructor creates a boost scoped lock
     * using the mutex provided.
     */
    explicit ScopedSpinlock(Spinlock &l)
        : _l(l)
    {
        _l.lock();
    }
    ~ScopedSpinlock() {
        _l.unlock();
    }

 private:
    Spinlock &_l;
};

}  // namespace fds

#endif
