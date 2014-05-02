/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_PLATFORM_FDS_SHM_QUEUE_H_
#define SOURCE_INCLUDE_PLATFORM_FDS_SHM_QUEUE_H_

#include <platform/fds-shmem.h>
#include <boost/lockfree/queue.hpp>


namespace fds {
    
template<typename T>
class FdsShmQueue
{
  public:
    FdsShmQueue(char *name, size_t capacity);
    virtual ~FdsShmQueue();

    bool shmq_enqueue(T item);
    T shmq_dequeue();
    bool empty();

  protected:
    FdsShmem shm_mgd_segment;
    boost::lockfree::queue<T,
            boost::lockfree::fixed_sized<true>> *shm_queue;
};

template <class T>
FdsShmQueue<T>::FdsShmQueue(char *name, size_t capacity)
        : shm_mgd_segment(name), shm_queue(nullptr)

{
    // Create empty shared memory segment
    boost::interprocess::managed_shared_memory *mgr =
            shm_mgd_segment.shm_create_empty(1024);
    // Put a queue in it
    shm_queue = mgr->construct<boost::lockfree::queue
            <T, boost::lockfree::fixed_sized<true>>>("shm_queue")(capacity);
}

template <class T>
FdsShmQueue<T>::~FdsShmQueue()
{
    // Make sure we clean up the shared memory segment
    shm_mgd_segment.shm_remove();
}

template <class T>
bool FdsShmQueue<T>::shmq_enqueue(T item)
{
    return shm_queue->bounded_push(item);
}

template <class T>
T FdsShmQueue<T>::shmq_dequeue()
{
    T item;
    if (shm_queue->pop(item)) {
        return item;
    }
    return (T)NULL;
}

template <class T>
bool FdsShmQueue<T>::empty()
{
    return shm_queue->empty();
}


}  // namespace fds
#endif  // SOURCE_INCLUDE_PLATFORM_FDS_SHM_QUEUE_H_
