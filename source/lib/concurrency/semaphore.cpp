/*

 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <fds_assert.h>
#include <concurrency/semaphore.h>
#include <thrift/concurrency/Util.h>
#include <fds_error.h>
namespace atc = apache::thrift::concurrency;

namespace fds { namespace concurrency {

Semaphore::Semaphore(uint permits) : maxPermits(permits), numPermits(permits) {
}

Semaphore::~Semaphore() {
    atc::Synchronized s(monitor);
    monitor.notifyAll();
}

void Semaphore::acquire(uint permits) {
    atc::Synchronized s(monitor);
    if (permits + numPermits > maxPermits) throw fds::Exception("invalid number of permits");
    while (numPermits < permits) {
        monitor.wait();
    }
    numPermits -= permits;
}

bool Semaphore::tryAcquire(uint permits, util::TimeStamp timeout) {
    atc::Synchronized s(monitor);
    if (permits + numPermits > maxPermits) return false;
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
    if (permits + numPermits > maxPermits) throw fds::Exception("releasing more permits than max allowed");

    numPermits += permits;
    monitor.notifyAll();
}

int Semaphore::getAvailablePermits() const {
    atc::Synchronized s(monitor);
    return numPermits;
}

}  // namespace concurrency
}  // namespace fds
