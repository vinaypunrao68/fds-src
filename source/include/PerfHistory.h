/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#ifndef SOURCE_INCLUDE_PERFHISTORY_H_
#define SOURCE_INCLUDE_PERFHISTORY_H_

#include <string>
#include <vector>
#include <unordered_map>

#include <fds_types.h>
#include <serialize.h>
#include <fds_error.h>
#include <fds_typedefs.h>
#include <concurrency/RwLock.h>
#include <fdsp/dm_api_types.h>
#include <StatTypes.h>

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
     * Use this instead of "operator +=" when the counter
     * is monitoring rather than counting. For example,
     * when we observe PUTs, we count them. (How many
     * took place within our sample period?) On the other
     * hand when we observe volume LBYTEs, we monitor
     * them. (What is the current value at this point in
     * our sample period?)
     */
    void updateTotal(const GenericCounter & rhs);

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
    inline double countPerSec(fds_uint32_t interval_sec) const {
        fds_verify(interval_sec > 0);
        return static_cast<double>(count_) / interval_sec;
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

typedef std::unordered_map<FdsVolStatType, GenericCounter, FdsStatHash> counter_map_t;

/**
 * This class describes one slot in the volume performance history.
 * A "slot" is a point-in-time bucket in which we collect all
 * counters related to the given volume as they appear at that time.
 */
class StatSlot: public serialize::Serializable {
  public:
    StatSlot();
    StatSlot(const StatSlot& slot);
    ~StatSlot();

    /**
     * Call this method before filling in the stat; resets to
     * record stats for timestamp 'rel_sec'
     * @param[in] rel_sec relative time in seconds
     * @param[in] stat_size is time interval for the stat; if
     * stat_size is given 0, then will use time interval that is
     * already (previously) set
     */
    void reset(fds_uint64_t rel_sec, fds_uint32_t interval_sec = 0);

    /**
     * Add counter of type 'counter_type' to this slot
     */
    void add(FdsVolStatType stat_type,
             const GenericCounter& counter);
    void add(FdsVolStatType stat_type,
             fds_uint64_t value);

    /**
     * slot length must match, but ignores the timestamp of rhs
     */
    StatSlot& operator +=(const StatSlot & rhs);
    StatSlot& operator =(const StatSlot & rhs);

    inline fds_uint64_t getRelSeconds() const { return rel_sec; }
    inline fds_uint32_t slotLengthSec() const { return interval_sec; }
    fds_bool_t isEmpty() const;
    /**
     * count / time interval of this slot in seconds
     */
    double getEventsPerSec(FdsVolStatType type) const;
    /**
     * total value of event 'type'
     */
    fds_uint64_t getTotal(FdsVolStatType type) const;
    fds_uint64_t getCount(FdsVolStatType type) const;
    /**
     * total / count for event 'type'
     */
    double getAverage(FdsVolStatType type) const;
    double getMin(FdsVolStatType type) const;
    double getMax(FdsVolStatType type) const;
    /**
     * Get counter of a given type
     */
    void getCounter(FdsVolStatType type,
                    GenericCounter* out_counter) const;

    /**
     * How many counters are recorded in our stat_map.
     */
    std::size_t getCounterCount() const;

    /*
     * For serializing and de-serializing
     */
    uint32_t virtual write(serialize::Serializer* s) const;
    uint32_t virtual read(serialize::Deserializer* d);

    friend std::ostream& operator<< (std::ostream &out,
                                     const StatSlot& slot);

  private:
    counter_map_t stat_map;

    /**
     * Timestamp in seconds, relative to start of collection.
     * Set to MAX to start with to aid identification of rel_sec 0 stats.
     */
    fds_uint64_t rel_sec = std::numeric_limits<fds_uint64_t>::max();
    fds_uint32_t interval_sec;  /**< time interval between slots of this type. */
};

/**
 * Recent volume's history of aggregated performance counters, where each
 * time slot in history is of the same time interval and time interval is
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
                           FdsVolStatType stat_type,
                           const GenericCounter& counter);

    /**
     * Record one event into the history
     * @param[in] ts (not relative!) timestamp in nanos of the counter
     * @param[in] stat_type type of the counter
     * @param[in] value is the value of the stat
     */
    void recordEvent(fds_uint64_t ts,
                     FdsVolStatType stat_type,
                     fds_uint64_t value);

    /**
     * Returns simple moving average of a given stat type
     * The last timeslot is not counted in the moving average, because
     * it may not be fully filled in
     * @param stat_type stat for which we calculate SMA
     * @param end_ts last timestamp to consider for the SMA, 0 if consider
     * the timestamp of the last fully filled timeslot
     * @param interval_sec interval in seconds for which calculate SMA, 0
     * if calculate for the whole history
     */
    double getSMA(FdsVolStatType stat_type,
                  fds_uint64_t end_ts = 0,
                  fds_uint32_t interval_sec = 0);

    /**
     * Add slots from fdsp message to this history
     * It is up to the caller that we don't add the same slot more than once
     */
    Error mergeSlots(const fpi::VolStatList& fdsp_volstats, fds_uint64_t fdsp_start_ts);
    void mergeSlots(const std::vector<StatSlot>& stat_list);

    /**
     * Copies history into FDSP volume stat list. Only slots with relative
     * seconds that are greater than last_rel_sec are copied
     * @param[out] fdsp_volstats Collected volume statistics.
     * @param[in] last_rel_sec is a relative timestamp in seconds. std::numeric_limits<fds_uint64_t>::max()
     *            indicates the initial call so that we can pick up rel_sec 0 stats.
     * @return last timestamp copied into FDSP volume stat list
     */
    fds_uint64_t toFdspPayload(fpi::VolStatList& fdsp_volstats,
                               fds_uint64_t last_rel_sec);

    /**
     * Copies the history in the StatSlot array where index 0 contains the
     * earliest timestamp copied. Only slots with relative seconds that are greater than
     * last_rel_sec are copied
     * @param[in] last_rel_sec is a relative timestamp in seconds. std::numeric_limits<fds_uint64_t>::max()
     *            indicates the initial call so that we can pick up rel_sec 0 stats.
     * @param[in] last_seconds_to_ignore number of most recent seconds
     * that will not be copied into the list; 0 means copy all most recent
     * history
     * @return last timestamp copied into stat_list
     */
    fds_uint64_t toSlotList(std::vector<StatSlot>& stat_list,
                            fds_uint64_t last_rel_sec,
                            fds_uint32_t last_seconds_to_ignore = 0);

    friend std::ostream& operator<< (std::ostream &out,
                                     const VolumePerfHistory& hist);

    /**
     * For now to maintain same file format when printing fine-grain qos stats
     * so that our gnuplot printing scripts works
     * Format:
     * [curts],volid,seconds_since_beginning_of_history,iops,ave_lat,min_lat,max_lat
     * @param[in] cur_ts timestamp that will be printed
     * @param[in] last_rel_sec will print relative timestamps in seconds > last_rel_sec. std::numeric_limits<fds_uint64_t>::max()
     *            indicates the initial call so that we can pick up rel_sec 0 stats.
     * @param[in] last_seconds_to_ignore number of most recent seconds
     * that will not be printed; 0 means print all most recent
     * history
     * @return last timestamp printed
     */
    fds_uint64_t print(std::ofstream& dumpFile,
                       boost::posix_time::ptime curts,
                       fds_uint64_t last_rel_sec,
                       fds_uint32_t last_seconds_to_ignore = 0);

    VolumePerfHistory::ptr getSnapshot();
    inline fds_uint64_t getTimestamp(fds_uint64_t rel_seconds) const {
        return (local_start_ts_ + rel_seconds * NANOS_IN_SECOND);
    }
    inline fds_uint64_t getStartTime() const {
      return local_start_ts_;
    }
    inline fds_uint32_t secondsInSlot() const {
        return slot_interval_sec_;
    }
    inline fds_uint32_t numberOfSlots() const {
        return max_slot_generations_;
    }

  private:  /* methods */
    fds_uint32_t relSecToStatHistIndex_LockHeld(fds_uint64_t rel_seconds);
    inline fds_uint64_t tsToRelativeSec(fds_uint64_t tsnano) const {
        if (tsnano < local_start_ts_) {
            return 0;
        }
        return (tsnano - local_start_ts_) / NANOS_IN_SECOND;
    }
    inline fds_uint64_t relativeSecToTS(fds_uint64_t start_nano,
                                        fds_uint64_t rel_sec) const {
        return (start_nano + rel_sec * NANOS_IN_SECOND);
    }
    fds_uint64_t getLocalRelativeSec(fds_uint64_t remote_rel_sec,
                                     fds_uint64_t remote_start_ts);

  private:
    /**
     * Configurable parameters
     */
    fds_volid_t volid_;       /**< volume id  */
    fds_uint32_t max_slot_generations_;     /**< Maximum number of slot generations kept in the historical record beyond which we wrap. */
    fds_uint32_t slot_interval_sec_;    /**< time interval in seconds between slot generations */

    fds_uint64_t local_start_ts_;  /**< "relative seconds" in history are timestamps relative to this timestamp. */

    /**
     * Array of time slots with stats. Once we fill in the last slot
     * in the array, we will circle around and re-use the first slot for the
     * next time interval and so on.
     */
    StatSlot* stat_slots_;
    fds_uint64_t last_slot_generation_;  // The last generation (in historical line of descent) of slots since local_start_ts_. One generation every slot_interval_sec_ seconds.
    fds_rwlock stat_lock_;  // protects stat_slots_ and last_slot_generation_
};

/**
 * TODO(Greg): Replace with macro DEFINE_OUTPUT_FUNCS from .../source/umod-lib/common/fdsp_utils.cpp.
 *
 * A "helper" class so that we can override Thrift's operator<< definition
 * for fpi::VolStatList.
 */
class VolStatListWrapper {
    public:
        VolStatListWrapper() = delete;
        explicit VolStatListWrapper(fpi::VolStatList& volStatList) : volStatList_(volStatList) {};

        fpi::VolStatList getVolStatList() const {
            return volStatList_;
        }

    private:
        fpi::VolStatList& volStatList_;
};

std::ostream& operator<< (std::ostream &out,
                          const VolStatListWrapper& volStatListWrapper);

}  // namespace fds

#endif  // SOURCE_INCLUDE_PERFHISTORY_H_
