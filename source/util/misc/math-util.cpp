/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#include <fds_types.h>
#include <util/math-util.h>

namespace fds {
namespace util {

//
// Using formula from Zygaria paper [RTAS 2006], same we did in qos_htb and counter
// TODO(xxx) use this method to calculate wma in htb and counter
//
double fds_get_wma(const std::vector<double> vals,
                   double* weight_sum) {
    double wma = 0.0;
    double weight = 1.0;
    double wsum = 0;
    if (vals.size() == 0) {
        *weight_sum = 0;
        return wma;
    }

    // use highest weight for most recent value; most recent
    // value is the last value in the list
    for (fds_int32_t i = (vals.size() - 1); i >= 0; --i) {
        wma += weight * vals[i];
        wsum += weight;
        weight = weight * 0.9;  // powers of 0.9
    }
    wma = wma / wsum;
    if (weight_sum) {
        *weight_sum = wsum;
    }
    return wma;
}

//
// returns standard deviation from the weighted average
//
double fds_get_wstdev(const std::vector<double> vals) {
    double stdev = 0;
    double wsum = 0;
    double wma = fds_get_wma(vals, &wsum);
    for (fds_uint32_t i = 0; i < vals.size(); ++i) {
        stdev += (vals[i] - wma) * (vals[i] - wma);
    }
    stdev = (vals.size() > 0) ? std::sqrt(stdev / wsum) : 0.0;
    return stdev;
}

}  // namespace util
}  // namespace fds
