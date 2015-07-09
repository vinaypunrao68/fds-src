/*

 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <fds_assert.h>
#include <concurrency/semaphore.h>
#include <thrift/concurrency/Util.h>

namespace atc = apache::thrift::concurrency;

namespace fds { namespace concurrency {

Semaphore::Semaphore(uint permits) : numPermits(permits) {
}

Semaphore::~Semaphore() {
    atc::Synchronized s(monitor);
    monitor.notifyAll();
}

void Semaphore::acquire(uint permits) {
    atc::Synchronized s(monitor);
 
    while (numPermits < permits) {
        monitor.wait();
    }
    numPermits -= permits;
}

bool Semaphore::tryAcquire(uint permits, util::TimeStamp timeout) {
    atc::Synchronized s(monitor);
    if (numPermits > permits) {
        numPermits -= permits;
        return true;
    }

    if (0 == timeout) return false;

    try {
        struct timespec abstime;
        atc::Util::toTimespec(abstime, atc::Util::currentTime() + timeout);

        while (numPermits < permits) {
            monitor.waitForTime(&abstime);
        }
        numPermits -= permits;
        return true;
    } catch(const atc::TimedOutException& e) {
        return false;
    }
}

void Semaphore::release(uint permits) {
    atc::Synchronized s(monitor);
    if ( 0 == numPermits ) {
        return;
    }
    numPermits += permits;
    if (numPermits > 0) {
        monitor.notifyAll();
    }
}

int Semaphore::getAvailablePermits() const {
    atc::Synchronized s(monitor);
    return numPermits;
}

}  // namespace concurrency
}  // namespace fds
