/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_UTIL_STATS_H_
#define SOURCE_INCLUDE_UTIL_STATS_H_
#include <vector>
#include <string>
namespace fds {
namespace util {
struct Stats {
    std::string name;
    explicit Stats(std::string name);
    Stats();

    void add(long double num);
    void calculate();

    void setDropPercent(uint64_t percent);
    uint64_t getCount();
    long double getAverage();
    long double getStdDev();
    long double getMedian();
    long double getMax();
    long double getMin();

  protected:
    uint64_t dropPercent = 5;
    uint64_t count;
    long double max, min;
    long double avg;
    long double stddev;
    long double median;
    std::vector<long double> numbers;
};

}  // namespace util
}  // namespace fds

#endif  // SOURCE_INCLUDE_UTIL_STATS_H_
