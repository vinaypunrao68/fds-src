/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

#ifndef SOURCE_INCLUDE_CONCURRENCY_SPINLOCK_H_
#define SOURCE_INCLUDE_CONCURRENCY_SPINLOCK_H_

#include <atomic>
#include <string>

namespace fds
{

struct fds_spin_lock {
    explicit fds_spin_lock(const std::string& name = "none") :
        _name(name)
        { _lock.clear(); }

    fds_spin_lock(fds_spin_lock const& rhs) = delete;
    fds_spin_lock& operator=(fds_spin_lock const& rhs) = delete;
    virtual ~fds_spin_lock() = default;

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
        fds_spin_lock& _lock;

        /*
         * The constructor creates a boost scoped lock
         * using the mutex provided.
         */
        explicit scoped_lock(fds_spin_lock &l)  // NOLINT(*)
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

typedef fds_spin_lock::scoped_lock fds_scoped_spin_lock;

}  // namespace fds


#endif  // SOURCE_INCLUDE_CONCURRENCY_SPINLOCK_H_
