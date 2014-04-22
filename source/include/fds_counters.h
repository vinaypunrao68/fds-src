/* Copyright 2013 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_FDS_COUNTERS_H_
#define SOURCE_INCLUDE_FDS_COUNTERS_H_

#include <string>
#include <iostream>
#include <atomic>
#include <vector>
#include <ostream>
#include <boost/noncopyable.hpp>
#include <fds_assert.h>
#include <concurrency/Mutex.h>

namespace fds {
/* Forward declarations */
class FdsCounters;
class FdsBaseCounter;
class FdsCountersMgr;

/**
 * @brief Counter manager.  Mananges the job of exporting registered
 * FdsCounters class objects in various different formats.
 * Supported format are:
 * -Graphite
 */
class FdsCountersMgr : public boost::noncopyable {
public:
    FdsCountersMgr(const std::string &id);
    void add_for_export(FdsCounters *counters);
    void remove_from_export(FdsCounters *counters);

    FdsCounters* get_counters(const std::string &id);

    std::string export_as_graphite();

    void export_to_ostream(std::ostream &stream);

protected:
    std::string id_;
    /* Counter objects that are exported out */
    std::vector<FdsCounters*> exp_counters_;
    /* Lock for this object */
    fds_mutex lock_;
};

/**
 * @brief Base counters class.  Any module that has a set of counters
 * should derive from this class
 */
class FdsCounters : public boost::noncopyable { 
public:
    FdsCounters(const std::string &id, FdsCountersMgr *mgr);
    /* Exposed for mock testing */
    FdsCounters();
    virtual ~FdsCounters();

    std::string id() const;

    std::string toString();

    void toMap(std::map<std::string, int64_t>& m) const;

protected:
    void add_for_export(FdsBaseCounter* cp);

    void remove_from_export(FdsBaseCounter* cp);

protected:
    /* Exported counters */
    std::vector<FdsBaseCounter*> exp_counters_;
    /* Id of this counter set */
    std::string id_;

    friend class FdsBaseCounter;
    friend class FdsCountersMgr;
};

/**
 * @brief Base class for counters that are to be exported.
 */
class FdsBaseCounter : public boost::noncopyable {
public:
    FdsBaseCounter(const std::string &id, FdsCounters *export_parent);
    /* Exposed for testing */
    FdsBaseCounter();
    virtual ~FdsBaseCounter();

    virtual uint64_t value() const = 0;
    virtual std::string id() const;

private:
    std::string id_;
};


/**
 * @brief Numeric counter
 */
class NumericCounter : public FdsBaseCounter
{
public:
    NumericCounter(const std::string &id, FdsCounters *export_parent);
    /* Exposed for testing */
    NumericCounter();

    virtual uint64_t value() const override;

    inline void incr();
    inline void incr(const uint64_t v);

private:
    std::atomic<uint64_t> val_;
};

/**
 * @brief Latency Counter
 */
class LatencyCounter : public FdsBaseCounter
{
public:
    LatencyCounter(const std::string &id, FdsCounters *export_parent);

    /* Exposed for testing */
    LatencyCounter();

    virtual uint64_t value() const override;

    inline void update(const uint64_t &latency);

private:
    std::atomic<uint64_t> total_latency_;
    std::atomic<uint64_t> cnt_;
};

}  // namespace fds

#endif  // SOURCE_INCLUDE_FDS_COUNTERS_H_
