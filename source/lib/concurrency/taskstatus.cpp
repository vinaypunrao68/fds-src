/*

 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <fds_assert.h>
#include <concurrency/taskstatus.h>
#include <thrift/concurrency/Util.h>

namespace atc = apache::thrift::concurrency;

namespace fds { namespace concurrency {

TaskStatus::TaskStatus(uint nTasks) : monitor(&mutex), numTasks(nTasks) {
}

void TaskStatus::reset(uint nTasks) {
    atc::Synchronized s(monitor);
    fds_verify(this->numTasks == 0);
    this->numTasks = nTasks;
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
    while ( numTasks > 0 ) {
        monitor.wait();
    }
}

bool TaskStatus::await(ulong timeout) {
    atc::Synchronized s(monitor);
    if ( 0 == numTasks ) {
        return true;
    }
    try {
        struct timespec abstime;
        atc::Util::toTimespec(abstime, atc::Util::currentTime() + timeout);

        while ( numTasks > 0 ) {
            monitor.waitForTime(&abstime);
        }
    } catch(const atc::TimedOutException& e) {
        return false;
    }

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
