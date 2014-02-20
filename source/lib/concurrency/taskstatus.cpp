/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <concurrency/taskstatus.h>
namespace atc = apache::thrift::concurrency;

namespace fds { namespace concurrency {

TaskStatus::TaskStatus(uint numTasks) : numTasks(numTasks), monitor(&mutex) {
}

TaskStatus::~TaskStatus() {
    atc::Synchronized s(monitor);
    monitor.notifyAll();
}

void TaskStatus::await() {
    atc::Synchronized s(monitor);
    if ( 0 == numTasks ) {
        return;
    }
    monitor.wait();
}

bool TaskStatus::await(ulong timeout) {
    atc::Synchronized s(monitor);
    if ( 0 == numTasks ) {
        return true;
    }
    monitor.wait(timeout);
    return 0 == numTasks;
}

void TaskStatus::done() {
    atc::Synchronized s(monitor);
    if ( 0 == numTasks ) {
        return;
    }
    --numTasks;
    if ( 0 == numTasks ) {
        monitor.notifyAll();
    }
}

int TaskStatus::getNumTasks() const {
    return numTasks;
}

}  // namespace concurrency
}  // namespace fds
