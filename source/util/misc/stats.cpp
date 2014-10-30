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

void Stats::add(long double num) {
    numbers.push_back(num);
}

void Stats::setDropPercent(uint64_t percent) {
    dropPercent = percent;
}

uint64_t Stats::getCount() {
    return count;
}

long double Stats::getAverage() {
    return avg;
}

long double Stats::getStdDev() {
    return stddev;
}

long double Stats::getMedian() {
    return median;
}

long double Stats::getMax() {
    return max;
}

long double Stats::getMin() {
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


}  // namespace util
}  // namespace fds
