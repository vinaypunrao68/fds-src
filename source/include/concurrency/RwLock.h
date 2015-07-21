/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#ifndef SOURCE_UTIL_CONCURRENCY_RWLOCK_H_
#define SOURCE_UTIL_CONCURRENCY_RWLOCK_H_

#include <boost/thread/shared_mutex.hpp>

namespace fds {
    /*
     * Basic wrapper for boost shared_mutex.
     * The boost shared_mutex is an upgradable
     * mutex and provides provisions for
     * conditional writers.
     */
    class SharedMutex {
  private:
        boost::shared_mutex _m;

  public:
        SharedMutex() = default;
        virtual ~SharedMutex() = default;
        SharedMutex(SharedMutex const& rhs) = delete;
        SharedMutex& operator=(SharedMutex const& rhs) = delete;
        SharedMutex(SharedMutex&& rhs) = default;
        SharedMutex& operator=(SharedMutex&& rhs) = default;

  protected:
        /*
         * Blocks unitl exclusive ownership of the mutex
         * can be acquired.
         */
        void lock() {
            _m.lock();
        }

        /*
         * Releases exclusive ownership.
         */
        void unlock() {
            _m.unlock();
        }

        /*
         * Blocks until upgrade ownership of the mutex
         * can be acquired.
         */
        void lock_upgradable() {
            _m.lock_upgrade();
        }

        /*
         * Release upgradable ownership.
         */
        void unlock_upgradable() {
            _m.unlock_upgrade();
        }

        /*
         * Upgrades upgradables ownership to
         * exclusive ownership. Blocks until
         * exclusive ownership can be acquired.
         */
        void upgrade() {
            _m.unlock_upgrade_and_lock();
        }

        /*
         * Acquires shared ownership.
         */
        void lock_shared() {
            _m.lock_shared();
        }

        /*
         * Releases shared ownership.
         */
        void unlock_shared() {
            _m.unlock_shared();
        }

        /*
         * Attempts to acquire shared ownership
         * without blocking.
         */
        bool lock_shared_try() {
            /*
             * Currently don't set any wait timer. Just
             * give up immediately
             */
            int timer_millis = 0;
            return _m.timed_lock_shared(
                boost::posix_time::milliseconds(timer_millis));
        }

        /*
         * Attempts to acquire exclusive ownership
         * without blocking.
         */
        bool lock_try() {
            /*
             * Currently don't set any wait timer. Just
             * give up immediately
             */
            int timer_millis = 0;
            return _m.timed_lock_shared(
                boost::posix_time::milliseconds(timer_millis));
        }
    };

    /*
     * Main fds_rwlock class that will be used.
     * Extends the upgradable shared_mutex to provide
     * a simpler rwlock interface.
     */
    class fds_rwlock : public SharedMutex {
  public:
        fds_rwlock() = default;
        ~fds_rwlock() override = default;

        void write_lock() {
            SharedMutex::lock();
        }

        void write_unlock() {
            SharedMutex::unlock();
        }

        void read_lock() {
            SharedMutex::lock_shared();
        }

        void read_unlock() {
            SharedMutex::unlock_shared();
        }

        /*
         * Acquires a conditional write lock.
         * This does NOT provide exclusive access
         * but also does NOT block if there are
         * existing readers. Once a conditional
         * write lock is held, other attempts to
         * acquire a conditional write lock will
         * block.
         */
        void cond_write_lock() {
            SharedMutex::lock_upgradable();
        }

        /*
         * Release a conditional write lock.
         */
        void cond_write_unlock() {
            SharedMutex::unlock_upgradable();
        }

        /*
         * Upgrades conditional ownership to exclusive
         * ownership. Will block until exclusive
         * access is acquired.
         * Precondition: Conditional ownership is held.
         * The caller must now use unlock() to release
         * as exclusive access will be held.
         */
        void upgrade() {
            SharedMutex::upgrade();
        }
    };


    /**
     * Helper Guards for Read and Write Locks
     * These are scoped locks, that will lock at ctor & unlock at dtor
     */
    class ReadGuard {
        fds_rwlock& lock;

  public :
        ReadGuard (fds_rwlock& l) : lock(l) {
            lock.read_lock();
        }

        ReadGuard (fds_rwlock* l) : lock(*l) {
            lock.read_lock();
        }

        ~ReadGuard() {
            lock.read_unlock();
        }
    };

    class WriteGuard {
        fds_rwlock& lock;
  public :
        WriteGuard (fds_rwlock& l) : lock(l) {
            lock.write_lock();
        }

        WriteGuard (fds_rwlock* l) : lock(*l) {
            lock.write_lock();
        }

        ~WriteGuard() {
            lock.write_unlock();
        }
    };

    struct sync_write_helper {
        WriteGuard wg;
        bool s = true;
      sync_write_helper(fds_rwlock& l)  : wg(l) {}
      sync_write_helper(fds_rwlock* l)  : wg(l) {}
    };

    struct sync_read_helper {
        ReadGuard rg;
        bool s = true;
      sync_read_helper(fds_rwlock& l)  : rg(l) {}
      sync_read_helper(fds_rwlock* l)  : rg(l) {}
    };

#define SCOPEDREAD(l) ReadGuard __rg__(l)
#define SCOPEDWRITE(l) WriteGuard __wg__(l)
#define read_synchronized(l)  for (sync_read_helper  _sync_(l); _sync_.s ; _sync_.s=false)
#define write_synchronized(l) for (sync_write_helper _sync_(l) ; _sync_.s ; _sync_.s=false)

}  // namespace fds

#endif  // SOURCE_UTIL_CONCURRENCY_RWLOCK_H_
