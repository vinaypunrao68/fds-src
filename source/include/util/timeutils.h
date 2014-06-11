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

struct StopWatch {
    TimeStamp start();
    void reset();
    TimeStamp getElapsedNanos();
  protected:
    TimeStamp startTime = 0;
};

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
