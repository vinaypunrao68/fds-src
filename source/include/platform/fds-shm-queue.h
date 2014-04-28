/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_PLATFORM_FDS_SHM_QUEUE_H_
#define SOURCE_INCLUDE_PLATFORM_FDS_SH_QUEUE_H_

#include <platform/fds-shmem.h>

namespace fds {
    
template<typename T>
class FdsShmQueue
{
  public:
    FdsShmQueue(const *char name, size_t size);
    virtual ~FdsShmQueue();

    void enqueue(T item);
    T dequeue();
    T peek();

  protected:
    T * head;
    T * tail;
    size_t size;

  private:
    FdsShmem segment;

};

}  // namespace fds
#endif  // SOURCE_INCLUDE_PLATFORM_FDS_SHMEM_H_
