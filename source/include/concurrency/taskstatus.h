#ifndef INCLUDE_CONCURRENCY_TASKSTATUS_H_
#define INCLUDE_CONCURRENCY_TASKSTATUS_H_

#include <thrift/concurrency/Monitor.h>
#include <fds_error.h>
/**
 * TaskStatus: Wait for a task to be done
 * Eg:
 *   Say you want to do socket connect only after some init functions.
 *   class variable : TaskStatus initTask;
 *   thread 1:
 *       initTask.await(); // this will wait till the initTask is done.
 *       socket.connect();
 *       ...
 *   thread 2:
 *       initData();
 *       initTask.done(); //signal the other threads that this task is done
 */

namespace fds {
    namespace concurrency {
        struct TaskStatus {
            // for more than 1 task pass it in ctor
            TaskStatus(uint _numTasks=1);
            void reset(uint numTasks);
            virtual ~TaskStatus();

            // wait for all tasks to be done w/o timeout
            virtual void await();
            virtual bool await(ulong timeout);

            // mark one task as complete
            virtual void done();

            // get the no.of tasks remaining
            virtual int getNumTasks() const ;
            Error error;
          protected:
            uint numTasks = 0;
            apache::thrift::concurrency::Monitor monitor;
            apache::thrift::concurrency::Mutex mutex;

        };
    } // namespace concurrency
} // namespace fds
#endif
