/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_UTIL_MATH_UTIL_H_
#define SOURCE_INCLUDE_UTIL_MATH_UTIL_H_

#include <vector>

/*
 * Some useful math methods
 */
namespace fds {
namespace util {

/**
 * Calculates weigted moving average over an array of
 * values, where value at index 0 is the oldest value
 * and value at index (vals.size() - 1) is the most recent value.
 * Moving weighted average gives more weight to more recent
 * values
 */
double fds_get_wma(const std::vector<double> vals,
                   double* weight_sum);

/**
 * Calculates standard deviation from the weighted average
 * where the value with index 0 in 'vals' array has the
 * lowest weight and value with index vals.size - 1 has the
 * highest weight.
 */
double fds_get_wstdev(const std::vector<double> vals);

}  // namespace util
}  // namespace fds

#endif  // SOURCE_INCLUDE_UTIL_MATH_UTIL_H_
