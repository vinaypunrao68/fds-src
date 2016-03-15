/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#include <string>
#include <algorithm>
#include <cmath>
#include <util/stats.h>
namespace fds {
namespace util {

Stats::Stats(std::string name) : name(name) {
    numbers.reserve(256);
}

Stats::Stats() : Stats("") {
}

void Stats::reset() {
    numbers.clear();
    count = avg = stddev = min = max = median = 0;
}

void Stats::add(long double num) {
    numbers.push_back(num);
}

void Stats::setDropPercent(uint64_t percent) {
    dropPercent = percent;
}

uint64_t Stats::getCount() const {
    return count;
}

long double Stats::getAverage() const {
    return avg;
}

long double Stats::getStdDev() const {
    return stddev;
}

long double Stats::getMedian() const {
    return median;
}

long double Stats::getMax() const {
    return max;
}

long double Stats::getMin() const {
    return min;
}

void Stats::calculate() {
    std::sort(numbers.begin(), numbers.end());

    uint64_t drop = std::round(numbers.size() * dropPercent / 100.0);
    count = numbers.size() - (2 * drop);

    uint64_t begin = drop;
    uint64_t end = numbers.size() - drop;

    min = numbers[begin];
    max = numbers[end-1];

    long double sum = 0;

    for (uint64_t i = begin ; i < end ; i++) {
        sum += numbers[i];
    }

    // average
    avg = sum / (1.0 * count);

    // median
    if (1 == count) {
        median = numbers[begin];
    } else if (0 == count%2) {
        median = numbers[begin + count/2 - 1];
    } else {
        median = (numbers[begin + count/2 - 1] + numbers[begin + count/2]) / 2.0;
    }

    // std dev
    sum = 0;
    for (uint64_t i = begin ; i < end ; i++) {
        sum += std::pow(numbers[i] - avg, 2);
    }
    stddev = std::sqrt(sum/count);
}

std::ostream& operator<< (std::ostream &os, const fds::util::Stats& stats) {
    return os << "["
              << " count:" << stats.getCount()
              << " median:" << stats.getMedian()
              << " avg:" << stats.getAverage()
              << " stddev:" << stats.getStdDev()
              << " min:" << stats.getMin()
              << " max:" << stats.getMax()
              << " ]";
}

}  // namespace util
}  // namespace fds
