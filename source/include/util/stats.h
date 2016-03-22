/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_UTIL_STATS_H_
#define SOURCE_INCLUDE_UTIL_STATS_H_
#include <vector>
#include <string>
#include <ostream>
namespace fds {
namespace util {
struct Stats {
    std::string name;
    explicit Stats(std::string name);
    Stats();

    void add(long double num);
    void calculate();
    void reset();

    void setDropPercent(uint64_t percent);
    uint64_t getCount() const;
    long double getAverage() const;
    long double getStdDev() const;
    long double getMedian() const;
    long double getMax() const;
    long double getMin() const;
    friend std::ostream& operator<< (std::ostream &os, const Stats& stats);
  protected:
    uint64_t dropPercent = 5;
    uint64_t count;
    long double max, min;
    long double avg;
    long double stddev;
    long double median;
    std::vector<long double> numbers;
};
std::ostream& operator<< (std::ostream &os, const fds::util::Stats& stats);
}  // namespace util
}  // namespace fds
#endif  // SOURCE_INCLUDE_UTIL_STATS_H_
