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
    FdsShmQueue(char *name);
    virtual ~FdsShmQueue();
    
    void shmq_connect();
    void shmq_alloc(size_t capacity);
    void shmq_dealloc();
    void shmq_disconnect();
    bool shmq_enqueue(T item);
    T shmq_dequeue();
    bool empty();
   
  protected:
    FdsShmem shm_mgd_segment;
    boost::lockfree::queue<T,
        boost::lockfree::fixed_sized<true>> *shm_queue;
};

template <class T>
FdsShmQueue<T>::FdsShmQueue(char *name)
        : shm_mgd_segment(name), shm_queue(nullptr) {}

template <class T>
FdsShmQueue<T>::~FdsShmQueue()
{}

template <class T>
void FdsShmQueue<T>::shmq_alloc(size_t capacity)
{
    // TODO(brian): Figure out how we want to handle capacity
    // should it be in terms of <T> or in raw bytes?

    // Create empty shared memory segment
    boost::interprocess::managed_shared_memory *mgr =
            shm_mgd_segment.shm_create_empty(capacity + 1024);
    // Put a queue in it
    shm_queue = mgr->construct<boost::lockfree::queue<T,
        boost::lockfree::fixed_sized<true>>>("shm_queue")(capacity);
}

template <class T>
void FdsShmQueue<T>::shmq_dealloc()
{
    // Make sure we clean up the shared memory segment
    shm_mgd_segment.shm_remove();
}

template <class T>
void FdsShmQueue<T>::shmq_connect()
{
    // Create the pair to get from the find operation
    std::pair<boost::lockfree::queue<T,
            boost::lockfree::fixed_sized<true>> *, std::size_t> ret;
    
    // Connect to the shm segment first
    shm_mgd_segment.shm_attach();

    // Get the manager from the shm segment
    boost::interprocess::managed_shared_memory::segment_manager
            *seg_mgr = shm_mgd_segment.shm_get_mgr();

    // Request the queue from the manager
    ret = seg_mgr->find_no_lock<boost::lockfree::queue<T,
            boost::lockfree::fixed_sized<true>>>("shm_queue");

    if (ret.first == 0) {
        shm_queue = nullptr;
    }

    // Set the queue ptr
    shm_queue = ret.first;
}

template <class T>
void FdsShmQueue<T>::shmq_disconnect()
{
    shm_queue = nullptr;
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
