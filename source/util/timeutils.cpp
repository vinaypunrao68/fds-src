/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#include <util/timeutils.h>
#include <unistd.h>

namespace fds {
namespace util {

TimeStamp getTimeStampNanos() {
    timespec t;
    clock_gettime(CLOCK_REALTIME, &t);
    return (TimeStamp)t.tv_sec * 1000000000ull + (TimeStamp)t.tv_nsec;
}

TimeStamp getTimeStampMicros() {
    timespec t;
    clock_gettime(CLOCK_REALTIME, &t);
    return (TimeStamp)t.tv_sec * 1000000ull + (TimeStamp)t.tv_nsec/1000;
}

TimeStamp getTimeStampMillis() {
    timespec t;
    clock_gettime(CLOCK_REALTIME, &t);
    return (TimeStamp)t.tv_sec * 1000ull + (TimeStamp)t.tv_nsec/1000000;
}

TimeStamp StopWatch::start() {
    startTime = getTimeStampNanos();
    return startTime;
}

void StopWatch::reset() {
    startTime = getTimeStampNanos();
}

TimeStamp StopWatch::getElapsedNanos() {
    return getTimeStampNanos() - startTime;
}

void TimeTracker::start() {
    stopWatch.start();
}

TimeStamp TimeTracker::mark(const std::string& name) {
    TimeStamp elapsed = stopWatch.getElapsedNanos();
    vecTimes.push_back(make_pair(name, elapsed));
    return elapsed;
}

void  TimeTracker::done() {
    mark("--end--");
}

TimeStamp TimeTracker::getNanosAt(const std::string& name) {
    for (const auto& item : vecTimes) {
        if (name == item.first) return item.second;
    }

    return 0;
}

TimeStamp TimeTracker::totalNanos() {
    if (vecTimes.empty()) return 0;
    return vecTimes[vecTimes.size()-1].second;
}

std::ostream& operator<<(std::ostream& oss, const fds::util::TimeTracker& tracker) {
    oss << "Times:\n";
    auto iter = tracker.vecTimes.begin();
    fds::util::TimeStamp previous;

    if (iter != tracker.vecTimes.end()) {
        oss << iter->first << ":" << iter->second << "\n";
        previous = iter->second;
        ++iter;
    }

    while (iter != tracker.vecTimes.end()) {
        oss << iter->first << ":" << iter->second - previous << ":" << iter->second << "\n";
        previous = iter->second;
        ++iter;
    }
    return oss;
}

}  // namespace util
}  // namespace fds
