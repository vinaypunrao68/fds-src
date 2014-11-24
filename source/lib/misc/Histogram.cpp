/**
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <limits>
#include <string>
#include <fds_assert.h>

#include <Histogram.h>
#include <util/Log.h>

namespace fds {

Histogram::Histogram(std::string& histogramName, size_t numBins)
{
    /**
     * number of bins is 1 less than range.
     */
    fds_verify(numBins < histogramMaxNumRange);

    histoNumBins = numBins;
    histoState = HISTOGRAM_UNINITIALIZED;

    histoName = histogramName;

    histoOutOfRangeCnt = 0;

    /**
     * Initialize both bins and range table.
     */
    memset(histoBins, 0, sizeof(uint64_t) * histogramMaxNumRange);
    memset(histoRange, 0, sizeof(uint64_t) * histogramMaxNumRange);
}

Histogram::~Histogram()
{
}


/**
 * Lookup histogram index corresponding to value.  This is used only when
 * histogram is uniformly distributed.
 */
bool inline
Histogram::histogramFindIndexQuick(uint64_t val, size_t& quickIdx)
{
    fds_assert(isUniformDistribution);

    if ((val < histoRange[0]) ||  (val > histoRange[histoNumBins])) {
        return false;
    }

    uint64_t stride = (histoMax - histoMin) / histoNumBins;
    uint64_t adjustedVal = (val - histoRange[0]);
    size_t adjustedIdx = adjustedVal / stride;

    if ((val >= histoRange[adjustedIdx]) && (val < histoRange[adjustedIdx + 1])) {
        quickIdx = adjustedIdx;
        return true;
    } else {
        return false;
    }
}

/**
 * Way to find index corresponding to value.
 * There are 2 ways for histogram index lookup.
 * 1) if the histogram distribution is uniform, then use a quick lookup,
 *    which is O(1) complexity.
 * 2) if the histogram distribution is non-uniform, then use binary
 *    search, which is O(logN) complexity.
 */
bool inline
Histogram::histogramFindIndex(uint64_t val, size_t& idx)
{
    bool found;

    /**
     * If uniform distribution, then we can do a quick index lookup.
     */
    if (isUniformDistribution) {
        found = histogramFindIndexQuick(val, idx);
        return found;
    }

    /**
     * Non-uniform distribution.  So, do binary search for index.
     */
    size_t upper = histoNumBins;
    size_t lower = 0;
    size_t mid = 0;

    while (upper - lower > 1) {
        mid = (upper + lower) / 2;
        if (val >= histoRange[mid]) {
            lower = mid;
        } else {
            upper = mid;
        }
    }

    if ((val >= histoRange[lower]) && (val < histoRange[lower + 1])) {
        idx = lower;
        return true;
    }  else {
        return false;
    }
}

/**
 * This function sets the range of the existing histgram that has not been initialized.
 *
 * The distribution can be anything, but lookup is O(n);
 *
 * Here is the example of the range specifier:
 *      [1,10) [10,100) [100,1000) [1000, 10000) ...
 *
 *      histp = new Histogram(5);
 *
 *      bin[0] -- 0 <= x < 10
 *      bin[1] -- 10 <= x < 100
 *      bin[2] -- 100 <= x < 1000
 *      bin[3] -- 1000 <= x < 10000
 *      bin[4] -- 10000 <= x
 *
 *      range[6] = {0, 10, 100, 1000, 10000, MAXUINT64};
 *
 *      histp->histogramSetRanges(range, 6);
 */
bool
Histogram::histogramSetRanges(const uint64_t range[], size_t rangeSize)
{
    fds_verify(HISTOGRAM_UNINITIALIZED == histoState);

    if (rangeSize != (histoNumBins + 1)) {
        return false;
    }

    isUniformDistribution = false;

    for (size_t i = 0; i <= histoNumBins; ++i) {
        histoRange[i] = range[i];
        if (i != 0) {
            fds_verify(histoRange[i] > histoRange[i - 1]);
        }
    }

    for (size_t i = 0; i < histoNumBins; ++i) {
        histoBins[i] = 0UL;
    }

    histoState = HISTOGRAM_INITIALIZED;

    return true;
}

/**
 * This function set of range on uniform distribution of the existing histogram that
 * has been instantiated, but not yet initialized.
 */
bool
Histogram::histogramSetRangesUniform(uint64_t Min, uint64_t Max)
{
    fds_verify(HISTOGRAM_UNINITIALIZED == histoState);
    fds_verify(Max >= histoNumBins);
    fds_verify(((Max - Min) % histoNumBins) == 0);

    isUniformDistribution = true;

    histoMin = Min;
    histoMax = Max;

    uint64_t stride = (Max - Min) / histoNumBins;
    uint64_t numRange = (Max / stride) + 1;

    histoRange[0] = Min;
    for (uint64_t i = 1; i <= histoNumBins; ++i) {
        histoRange[i] = histoRange[i - 1] + stride;
    }

    for (size_t i = 0; i < histoNumBins; ++i) {
        histoBins[i] = 0UL;
    }

    histoState = HISTOGRAM_INITIALIZED;
    return true;
}

/**
 * Non-thread safe version of histogram increment.
 */
void
Histogram::histogramIncrement(uint64_t val)
{
    fds_assert(HISTOGRAM_INITIALIZED == histoState);

    histogramAccumulate(val, 1);
}

/**
 * Non-thread safe version of histogram accumulator.
 */
void
Histogram::histogramAccumulate(uint64_t val, uint64_t weight)
{
    fds_assert(HISTOGRAM_INITIALIZED == histoState);
    size_t idx;

    bool found = histogramFindIndex(val, idx);
    if (found) {
        histoBins[idx] += weight;
    } else {
        ++histoOutOfRangeCnt;
    }
}

/**
 * Thread safe version of histogram increment.
 */
void
Histogram::histogramIncrementAtomic(uint64_t val)
{
    fds_assert(HISTOGRAM_INITIALIZED == histoState);

    histogramAccumulateAtomic(val, 1);
}

/**
 * Thread safe version of histogram accumulator.
 */
void
Histogram::histogramAccumulateAtomic(uint64_t val, uint64_t weight)
{
    fds_assert(HISTOGRAM_INITIALIZED == histoState);
    size_t idx;

    bool found = histogramFindIndex(val, idx);
    if (found) {
        __sync_add_and_fetch(&histoBins[idx], weight);
    } else {
        __sync_add_and_fetch(&histoOutOfRangeCnt, 1);
    }
}

/**
 * This interface resets the histogram.  This just resets the total and per bin counters.
 * This interface does not modifi the existing ranges.
 *
 * NOTE:  This interface is not thread safe.  It's up to the caller to synchronize
 *        access to the histogram data structure while it's being reset.
 */
void
Histogram::histogramReset()
{
    fds_assert(HISTOGRAM_INITIALIZED == histoState);

    memset(histoBins, 0, sizeof(uint64_t) * histoNumBins);
}


/**
 * prints out histogram data to stdout.
 */
std::ostream& operator<< (std::ostream &out,
                          const Histogram& histogram)
{
    out << "Histogram Name: " << histogram.histoName << std::endl;
    for (size_t i = 0; i < histogram.histoNumBins; ++i) {
        out << "[" << histogram.histoRange[i] << ",";
        out << histogram.histoRange[i + 1];
        out << ") => " << histogram.histoBins[i] << std::endl;
    }
    out << "Out of range count = " << histogram.histoOutOfRangeCnt;
    out << std::endl;
    return out;
}

/**
 * prints out histogram data to log.
 */
boost::log::formatting_ostream& operator<< (boost::log::formatting_ostream& out,
                                            const Histogram& histogram)
{
    out << "Histogram Name: " << histogram.histoName << std::endl;
    for (size_t i = 0; i < histogram.histoNumBins; ++i) {
        out << "[" << histogram.histoRange[i] << ",";
        out << histogram.histoRange[i + 1];
        out << ") => " << histogram.histoBins[i] << std::endl;
    }
    out << "Out of range count = " << histogram.histoOutOfRangeCnt;
    out << std::endl;

    return out;
}

}  // namespace fds
