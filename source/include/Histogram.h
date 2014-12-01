/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#ifndef SOURCE_INCLUDE_HISTOGRAM_H_
#define SOURCE_INCLUDE_HISTOGRAM_H_

#include <cstring>
#include <inttypes.h>
#include <ostream>
#include <string>
#include <util/Log.h>
#include <fds_assert.h>


/**
 * This is a simple histogram class.
 *
 * Currently, it only supports uint64_t data type, but this should really be
 * templatized to support both uint64_t and double.  No need to support other
 * base types.
 */
namespace fds {

/**
 * Maximum number of bins.  Statically sized. See comments below.
 */
const uint32_t histogramMaxNumRange = 128;

class Histogram {
  public:
    explicit Histogram(std::string& histogramName, size_t numBins);
    ~Histogram();

    bool histogramSetRanges(const uint64_t range[], size_t rangeSize);
    bool histogramSetRangesUniform(uint64_t rangeMin, uint64_t rangeMax);

    void histogramIncrement(uint64_t val);
    void histogramAccumulate(uint64_t val, uint64_t weight);

    void histogramIncrementAtomic(uint64_t val);
    void histogramAccumulateAtomic(uint64_t val, uint64_t weight);

    uint64_t inline histogramGetIth(size_t i)
    {
        fds_verify(i < histoNumBins);
        return histoBins[i];
    }

    void histogramReset();

    /**
     * output to stdout
     */
    friend std::ostream& operator<< (std::ostream &out,
                                     const Histogram& histogram);

    /**
     * output to log
     */
    friend boost::log::formatting_ostream& operator<< (boost::log::formatting_ostream& out,
                                                       const Histogram& histogram);

    /**
     * There are two states with the histogram:
     * 1) HISTOGRAM_UNINITIALIZED => histogram is instantiated, but has not bee initialized with range.
     * 2) HISTOGRAM_INITIALIZED => range is initialized.  Now can be used.
     */
    typedef enum {
        HISTOGRAM_UNINITIALIZED = 0,
        HISTOGRAM_INITIALIZED = 1
    } HistogramState;


  private:
    /* Intentionally making a static allocation.  We do not want to dynamically allocate, since
     * the memory is not guaranteed to co-locate.  This can potentially be a performance issue, since
     * it may require at least 2 page faults.
     *
     * The alignment is currently set to 8 bytes.  This is done to support atomic increment
     * and accumulate.  This is to ensure that the the bins are properly aligned at 8 bytes.
     *
     * TODO(Sean):
     * NOTE: Althouh multiple threads might update different bins, they may be updating
     *       the same cache line.  This can cacuse cache thrashing. If that's the case, then
     *       each bin shoulde be aligned to the system cache line size.
     *       Or,
     *       Increase the number of bins to reduce the chance of thrashing on the same
     *       cache line.
     *       Former is preferred.
     */
    alignas(8) uint64_t histoBins[histogramMaxNumRange];

    /**
     * The maximum number of range.  This is set once during the second step of
     * initialization.
     *
     * Number of range is alway +1 of number of bins.
     */
    alignas(8) uint64_t histoRange[histogramMaxNumRange];


    /**
     * The value is out of range
     */
    uint64_t histoOutOfRangeCnt;

    /**
     * Name of the histogram.
     */
    std::string histoName;

    /**
     * Number of bins in the histogram.  This should always be one less than
     * the number of ranges.
     */
    size_t histoNumBins;

    size_t histoMin;
    size_t histoMax;

    /**
     * Current state of histogram.  See the enumeration definition above.
     */
    HistogramState histoState;

    /**
     * The range is uniform distribution.  This flag is used to help quickly
     * index into the bin.  If the histogram has uniform distribution,
     * update to the bin is O(1).
     * However, if the distribution is non-uniform, it's O(n) search to get
     * the bin.
     */
    bool isUniformDistribution;

    bool inline histogramFindIndex(uint64_t val, size_t& idx);
    bool inline histogramFindIndexQuick(uint64_t val, size_t& linearIdx);
};


}  // namespace fds

#endif  // SOURCE_INCLUDE_HISTOGRAM_H_
