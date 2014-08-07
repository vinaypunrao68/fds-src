/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#ifndef SOURCE_INCLUDE_PERFHISTORY_H_
#define SOURCE_INCLUDE_PERFHISTORY_H_

#include <string>
#include <unordered_map>

#include <fds_types.h>
#include <serialize.h>
#include <PerfTypes.h>

namespace fds {

class GenericCounter: public serialize::Serializable {
  public:
    GenericCounter();
    explicit GenericCounter(const FdsBaseCounter& c);
    explicit GenericCounter(const GenericCounter& c);
    ~GenericCounter();

    void reset();
    void add(fds_uint64_t val);
    GenericCounter & operator +=(const GenericCounter & rhs);
    GenericCounter & operator +=(const fds_uint64_t & val);
    GenericCounter & operator =(const GenericCounter & rhs);
    GenericCounter & operator =(const fds_uint64_t & val);

    /**
     * Getters
     */
    inline fds_uint64_t min() const { return min_; }
    inline fds_uint64_t max() const { return max_; }
    inline fds_uint64_t count() const { return count_; }
    inline fds_uint64_t total() const { return total_; }
    inline double average() const {
        if (count_ == 0) return 0;
        return static_cast<double>(total_) / count_;
    }

    /*
     * For serializing and de-serializing
     */
    uint32_t virtual write(serialize::Serializer* s) const;
    uint32_t virtual read(serialize::Deserializer* d);

    friend std::ostream& operator<< (std::ostream &out,
                                     const GenericCounter& counter);

  private:
    fds_uint64_t total_;
    fds_uint64_t count_;
    fds_uint64_t min_;
    fds_uint64_t max_;
};

typedef std::unordered_map<PerfEventType, GenericCounter, PerfEventHash> counter_map_t;

/**
 * This class describes one slot in the volume performance history
 */
class StatSlot: public serialize::Serializable {
  public:
    StatSlot();
    ~StatSlot();

    /**
     * Call this method before filling in the stat; resets to
     * record stats for timestamp 'rel_sec'
     * @param[in] rel_sec relative time in seconds
     * @param[in] stat_size is time interval for the stat; if
     * stat_size is given 0, then will use time interval that is
     * already (previously) set
     */
    void reset(fds_uint64_t rel_sec, fds_uint32_t stat_size = 0);

    /**
     * Add counter of type 'counter_type' to this slot
     */
    void add(PerfEventType counter_type,
             const GenericCounter& counter);
    void add(PerfEventType counter_type,
             fds_uint64_t value);

    inline fds_uint64_t getTimestamp() const { return rel_ts_sec; }
    /**
     * count / time interval of this slot in seconds
     */
    double getEventsPerSec(PerfEventType type) const;
    /**
     * total value of event 'type'
     */
    fds_uint64_t getTotal(PerfEventType type) const;
    /**
     * total / count for event 'type'
     */
    double getAverage(PerfEventType type) const;

    /*
     * For serializing and de-serializing
     */
    uint32_t virtual write(serialize::Serializer* s) const;
    uint32_t virtual read(serialize::Deserializer* d);

    friend std::ostream& operator<< (std::ostream &out,
                                     const StatSlot& slot);

  private:
    counter_map_t stat_map;
    fds_uint64_t rel_ts_sec;  /**< timestamp in seconds, relative */
    fds_uint32_t interval_sec;  /**< time interval of this slot */
};

/**
 * Recent volume's history of aggregated performance counters, where each
 * time slot in history is same time interval and time interval is
 * configurable. History is implemented as circular array.
 */
class VolumePerfHistory {
  public:
    VolumePerfHistory(fds_volid_t volid,
                      fds_uint64_t start_ts,
                      fds_uint32_t slots,
                      fds_uint32_t slot_len_sec);
    ~VolumePerfHistory();

    typedef boost::shared_ptr<VolumePerfHistory> ptr;
    typedef boost::shared_ptr<const VolumePerfHistory> const_ptr;

    /**
     * Record aggregated Perf counter into the history
     * @param[in] ts (not relative!) timestamp in nanos of the counter
     * @param[in] counter_type type of the counter
     * @param[in] counter is the counter to record
     */
    void recordPerfCounter(fds_uint64_t ts,
                           PerfEventType counter_type,
                           const GenericCounter& counter);

    /**
     * Record one event into the history
     * @param[in] ts (not relative!) timestamp in nanos of the counter
     * @param[in] counter_type type of the counter
     * @param[in] value is the value of the event
     */
    void recordEvent(fds_uint64_t ts,
                     PerfEventType counter_type,
                     fds_uint64_t value);

    /**
     * Returns simple moving average of a given perf event type
     * The last timeslot is not counted in the moving average, because
     * it may not be fully filled in
     * @param event_type perf event for which we calculate SMA
     * @param end_ts last timestamp to consider for the SMA, 0 if consider
     * the timestamp of the last fully filled timeslot
     * @param interval_sec interval in seconds for which calculate SMA, 0
     * if calculate for the whole history
     */
    double getSMA(PerfEventType event_type,
                  fds_uint64_t end_ts = 0,
                  fds_uint32_t interval_sec = 0);

    /**
     * Merge history 'rsh' with this history; if some timestamps are outside
     * of range of this history, these timestamps are ignored.
     * Given history 'rhs' must have the same volume id. Slot lenth of 'rhs'
     * must be equal or smaller than slot length of this history.
     * The caller must make sure that we merge any perf counter once (eg. calling
     * merge with same history twice will resut in 2x counters). Use begin_ts
     * to tell the merge method to ignore slots in history that were already merged.
     * @param[in] rhs volume performance history that needs to be merged with
     * this history
     * @param[in] begin_ts only merge timestamps from 'rhs' that are >= begin_ts
     * If begin_ts is 0, then consider all timestamps in 'rhs'
     */
    void merge(const VolumePerfHistory& rhs,
               fds_uint64_t begin_ts);


    friend std::ostream& operator<< (std::ostream &out,
                                     const VolumePerfHistory& hist);

  private:  /* methods */
    fds_uint32_t useSlot(fds_uint64_t rel_seconds);
    inline fds_uint64_t tsToRelativeSec(fds_uint64_t tsnano) const {
        fds_verify(tsnano >= start_nano_);
        return (tsnano - start_nano_) / 1000000000;
    }

  private:
    /**
     * Configurable parameters
     */
    fds_volid_t volid_;       /**< volume id  */
    fds_uint32_t nslots_;     /**< number of slots in history */
    fds_uint32_t slotsec_;    /**< slot length is seconds */

    fds_uint64_t start_nano_;  /**< ts in history are relative to this ts */

    /**
     * Array of time slots with stats; once we fill in the last slot
     * in the array, we will circulate and re-use the first slot for the
     * next time interval and so on
     */
    StatSlot* stat_slots_;
    fds_uint64_t last_slot_num_;  /**< sequence number of the last slot */
};

}  // namespace fds

#endif  // SOURCE_INCLUDE_PERFHISTORY_H_
