/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <concurrency/eventtracker.h>
#include <thrift/concurrency/Util.h>
#include <map>
#include <string>

namespace atc = apache::thrift::concurrency;

namespace fds { namespace concurrency {

NumberedEventTracker::NumberedEventTracker() : monitor(&mutex), eventCounter(0) {
}

bool NumberedEventTracker::await(fds_uint64_t event, ulong timeout) {
    atc::Synchronized s(monitor);
    auto iter = mapEvents.find(event);
    if (iter != mapEvents.end()) {
        mapEvents.erase(iter);
        return true;
    }

    if (timeout > 0) {
        try {
            struct timespec abstime;
            atc::Util::toTimespec(abstime, atc::Util::currentTime() + timeout);

            while (iter != mapEvents.end()) {
                monitor.waitForTime(&abstime);
                iter = mapEvents.find(event);
            }
        } catch(const atc::TimedOutException& e) {
            return false;
        }
    } else {
        while (iter != mapEvents.end()) {
            monitor.waitForever();
            iter = mapEvents.find(event);
        }
    }

    if (iter != mapEvents.end()) {
        mapEvents.erase(iter);
        return true;
    }
    return false;
}

// mark one task as complete
void NumberedEventTracker::generate(fds_uint64_t event) {
    atc::Synchronized s(monitor);
    auto iter = mapEvents.find(event);

    if ( iter != mapEvents.end() ) {
        monitor.notifyAll();
    }
}

fds_uint64_t NumberedEventTracker::getUniqueEventId() {
    return ++eventCounter;
}

//=============================================================================

NamedEventTracker::NamedEventTracker() : monitor(&mutex) {
}

bool NamedEventTracker::await(const std::string& event, ulong timeout) {
    atc::Synchronized s(monitor);
    auto iter = mapEvents.find(event);
    if (iter != mapEvents.end()) {
        mapEvents.erase(iter);
        return true;
    }

    if (timeout > 0) {
        try {
            struct timespec abstime;
            atc::Util::toTimespec(abstime, atc::Util::currentTime() + timeout);

            while (iter != mapEvents.end()) {
                monitor.waitForTime(&abstime);
                iter = mapEvents.find(event);
            }
        } catch(const atc::TimedOutException& e) {
            return false;
        }
    } else {
        while (iter != mapEvents.end()) {
            monitor.waitForever();
            iter = mapEvents.find(event);
        }
    }

    if (iter != mapEvents.end()) {
        mapEvents.erase(iter);
        return true;
    }
    return false;
}

// mark one task as complete
void NamedEventTracker::generate(const std::string& event) {
    atc::Synchronized s(monitor);
    auto iter = mapEvents.find(event);

    if ( iter != mapEvents.end() ) {
        monitor.notifyAll();
    }
}


}  // namespace concurrency
}  // namespace fds

