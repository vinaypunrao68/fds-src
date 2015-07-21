/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

#ifndef SOURCE_INCLUDE_CONCURRENCY_SPINLOCK_H_
#define SOURCE_INCLUDE_CONCURRENCY_SPINLOCK_H_

#include <atomic>
#include <string>

namespace fds
{

struct fds_spinlock {
    explicit fds_spinlock(const std::string& name = "none") :
        _name(name)
        { _lock.clear(); }

    fds_spinlock(fds_spinlock const& rhs) = delete;
    fds_spinlock& operator=(fds_spinlock const& rhs) = delete;
    virtual ~fds_spinlock() = default;

    std::string get_name() const
        { return _name; }

    void lock() {
        while(!try_lock()) ;  // spin
    }

    void unlock() {
        _lock.clear(std::memory_order_release);  // release lock
    }

    bool try_lock() {
        return !_lock.test_and_set(std::memory_order_acquire);  // acquire lock
    }

    struct scoped_lock {
      public:
        fds_spinlock& _lock;

        /*
         * The constructor creates a boost scoped lock
         * using the mutex provided.
         */
        explicit scoped_lock(fds_spinlock &l)  // NOLINT(*)
                : _lock(l) {
            _lock.lock();
        }

        ~scoped_lock() {
            _lock.unlock();
        }
    };

 private:
    std::string _name;
    std::atomic_flag _lock;
};

typedef fds_spinlock::scoped_lock fds_scoped_spinlock;

}  // namespace fds


#endif  // SOURCE_INCLUDE_CONCURRENCY_SPINLOCK_H_
