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

std::string getLocalTimeString(TimeStamp t = 0); // NOLINT
std::string getGMTimeString(TimeStamp t = 0); // NOLINT

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
 * Return seconds from human readable time
 * input string as: case-insensitive 
 * "12345",""12345s", "12345sec" = 12345
 * "1h", "1 hour" = 3600
 * "60m", "60mins" = 3600
 * "2d", "2 days" = 2*86400
 * "1w", "1 week" = 7 * 86400
 *  MM:SS = MM*60 + SS
 *  HH:MM:SS = HH*3600 + MM*60 + SS
 */
TimeStamp getSecondsFromHumanTime(const std::string& strTime);

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
