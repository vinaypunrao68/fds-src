#ifndef INCLUDE_CONCURRENCY_SEMAPHORE_H_
#define INCLUDE_CONCURRENCY_SEMAPHORE_H_

#include <thrift/concurrency/Monitor.h>
#include <util/timeutils.h>
/**
 * Simple Semaphore. Will maintain N permits
 * which can be acquired and released.
 */
namespace fds {
    namespace concurrency {
        struct Semaphore {
            Semaphore(uint permits = 1);
            virtual ~Semaphore();

            virtual void acquire(uint permits = 1);
            virtual bool tryAcquire(uint permits = 1, util::TimeStamp timeout = 0);

            virtual void release(uint permits=1);

            virtual int getAvailablePermits() const ;
  
          protected:
            uint numPermits = 1;
            uint maxPermits = 1;
            apache::thrift::concurrency::Monitor monitor;
        };
    } // namespace concurrency
} // namespace fds
#endif
