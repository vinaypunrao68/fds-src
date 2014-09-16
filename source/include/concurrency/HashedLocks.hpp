#ifndef INCLUDE_CONCURRENCY_HASHEDLOCKS_H_
#define INCLUDE_CONCURRENCY_HASHEDLOCKS_H_

#include <vector>
#include <functional>
#include <boost/noncopyable.hpp>
#include <concurrency/Mutex.h>

namespace fds {

/**
* @brief Provides synchronization primitive (lock) based on hash code of the object being locked.
* Access to objects that map to same bucket based on the hash is serialized.  Access to objects
* that fall in different buckets is parallelized.
*
* @tparam T object type that needs locking
* @tparam HashFunctorT functor that defines operator(T&)
*/
template <class T, class HashFunctorT = std::hash<T>>
class HashedLocks : boost::noncopyable
{
 public:
    explicit HashedLocks(const uint32_t &tblSize)
        : locks_(tblSize)
    {
    }

    /**
    * @brief for locking
    *
    * @param objToLock
    */
    void lock(T *objToLock) {
        size_t lockIdx = (hashFunctor_(*objToLock) % locks_.size());
        locks_[lockIdx].lock.lock();
        locks_[lockIdx].lockHolderObj = objToLock;
    }

    /**
    * @brief For unlocking.  Make sure the same object that locked is being unlocked.
    *
    * @param objToUnlock
    */
    void unlock(T *objToUnlock) {
        size_t lockIdx = (hashFunctor_(*objToUnlock) % locks_.size());
        fds_verify(locks_[lockIdx].lockHolderObj == objToUnlock);
        locks_[lockIdx].lockHolderObj = nullptr;
        locks_[lockIdx].lock.unlock();
    }

 protected:
    struct LockEntry {
        /* lock */
        fds_mutex lock;
        /* Reference to locked object */
        T *lockHolderObj;
    };
 
    HashFunctorT hashFunctor_;
    /* Lock around taskMap_ */    
    std::vector<LockEntry> locks_;
};

template <class T, class HashedLocksT>
class ScopedHashedLock : boost::noncopyable
{
 public:
    ScopedHashedLock(HashedLocksT &hl, T &obj)
        : hashedLocks_(hl),
        obj_(obj)
    {
        hashedLocks_.lock(&obj_);
    }
    ~ScopedHashedLock() {
        hashedLocks_.unlock(&obj_);
    }

 protected:
    HashedLocksT &hashedLocks_;
    T &obj_;
};
}  // namespace fds
#endif   // INCLUDE_CONCURRENCY_SPINLOCK_H_
