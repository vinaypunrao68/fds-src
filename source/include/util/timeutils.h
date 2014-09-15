/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#ifndef SOURCE_UTIL_TIMEUTILS_H_
#define SOURCE_UTIL_TIMEUTILS_H_

#include <shared/fds_types.h>
#include <string>
#include <vector>
#include <map>
#include <ostream>
namespace fds {
namespace util {

using TimeStamp = fds_uint64_t;

TimeStamp getTimeStampNanos();
TimeStamp getTimeStampMicros();
TimeStamp getTimeStampMillis();
TimeStamp getTimeStampSeconds();

extern fds_uint64_t CYCLES_PER_SECOND;

// use with care .. will be called on load and will
// initialize CYCLES_PER_SECOND
fds_uint64_t getCpuSpeedHz();

fds_uint64_t rdtsc();
fds_uint64_t rdtsc_barrier();

// same as rdtsc
fds_uint64_t getClockTicks();

// convert clock ticks to nanos
TimeStamp getNanosFromTicks(fds_uint64_t ticks);

/**
 * To track time interval once
 */
struct StopWatch {
    TimeStamp start();
    void reset();
    TimeStamp getElapsedNanos();
  protected:
    TimeStamp startTime = 0;
};

/**
 * To track time intervals in multiple stages..
 * Maintains a named vector to store
 * different points in time..
 * print the tracker to get the individual times.
 */

struct TimeTracker {
    void start();
    TimeStamp mark(const std::string& name);
    void done();
    TimeStamp getNanosAt(const std::string& name);
    TimeStamp totalNanos();

    friend std::ostream& operator<<(std::ostream& oss, const TimeTracker& tracker);
  protected:
    std::vector< std::pair<std::string, TimeStamp> > vecTimes;
    StopWatch stopWatch;
};
std::ostream& operator<<(std::ostream& oss, const fds::util::TimeTracker& tracker);

} // namespace util
} // namespace fds

#endif // SOURCE_UTIL_TIMEUTILS_H_
