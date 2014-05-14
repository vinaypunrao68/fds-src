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

    typedef boost::interprocess::allocator<T,
            boost::interprocess::managed_shared_memory::segment_manager>
            BShmAlloc;
    
    typedef boost::lockfree::queue<T,
            boost::lockfree::capacity<1024>, // TODO(brian): Figure out better limit
            //boost::lockfree::fixed_sized<true>>,
            boost::lockfree::allocator<BShmAlloc>> BShmQueue;
  public:
    FdsShmQueue(char *name);
    virtual ~FdsShmQueue();
    
    void shmq_alloc(size_t capacity);
    void shmq_dealloc();
    void shmq_connect();
    void shmq_disconnect();
    bool shmq_enqueue(T item);
    bool shmq_dequeue(T &item);
    bool empty();
   
  protected:
    FdsShmem shm_mgd_segment;
    BShmQueue *shm_queue;
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
    // Create empty shared memory segment
    boost::interprocess::managed_shared_memory *mgr =
            shm_mgd_segment.shm_create_empty(capacity);

    // Put a queue in it
    shm_queue = mgr->construct<BShmQueue>("shm_queue")(mgr->get_segment_manager());
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
    std::pair<BShmQueue *, std::size_t> ret;
    
    // Connect to the shm segment first
    shm_mgd_segment.shm_attach();

    // Get the manager from the shm segment
    boost::interprocess::managed_shared_memory::segment_manager
            *seg_mgr = shm_mgd_segment.shm_get_mgr();

    // Request the queue from the manager
    ret = seg_mgr->find_no_lock<BShmQueue>("shm_queue");

    if (ret.first == 0) {
        shm_queue = nullptr;
        exit(-1);
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
bool FdsShmQueue<T>::shmq_dequeue(T &item)
{
    return shm_queue->pop(item);
}

template <class T>
bool FdsShmQueue<T>::empty()
{
    return shm_queue->empty();
}

}  // namespace fds
#endif  // SOURCE_INCLUDE_PLATFORM_FDS_SHM_QUEUE_H_
