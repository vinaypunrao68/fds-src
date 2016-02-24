/**
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <limits>
#include <vector>
#include <PerfHistory.h>
#include <err.h>

namespace fds {

/******** GenericCounter implementation ********/

GenericCounter::GenericCounter()
        : total_(0),
          count_(0),
          min_(std::numeric_limits<fds_uint64_t>::max()),
          max_(0) {
}

GenericCounter::GenericCounter(const FdsBaseCounter& c) {
}

GenericCounter::GenericCounter(const GenericCounter& c)
        : total_(c.total()),
          count_(c.count()),
          min_(c.min()),
          max_(c.max()) {
}

GenericCounter::~GenericCounter() {
}

void GenericCounter::reset() {
    total_ = 0;
    count_ = 0;
    min_ = std::numeric_limits<fds_uint64_t>::max();
    max_ = 0;
}

void GenericCounter::add(fds_uint64_t val) {
    if (val < min_) {
        min_ = val;
    }
    if (val > max_) {
        max_ = val;
    }
    total_ += val;
    ++count_;
}

GenericCounter& GenericCounter::operator +=(const GenericCounter & rhs) {
    if (&rhs != this) {
        if (rhs.min() < min_) {
            min_ = rhs.min();
        }
        if (rhs.max() > max_) {
            max_ = rhs.max();
        }
        total_ += rhs.total();
        count_ += rhs.count();
    }
    return *this;
}

GenericCounter & GenericCounter::operator +=(const fds_uint64_t & val) {
    if (val < min_) {
        min_ = val;
    }
    if (val > max_) {
        max_ = val;
    }
    total_ += val;
    ++count_;
    return *this;
}

void GenericCounter::updateTotal(const GenericCounter & rhs) {
    if (&rhs != this) {
        if (rhs.min() < min_) {
            min_ = rhs.min();
        }
        if (rhs.max() > max_) {
            max_ = rhs.max();
        }
        total_ = rhs.total();
        count_ += rhs.count();
    }
}

GenericCounter& GenericCounter::operator =(const GenericCounter & rhs) {
    if (&rhs != this) {
        min_ = rhs.min();
        max_ = rhs.max();
        total_ = rhs.total();
        count_ = rhs.count();
    }
    return *this;
}

GenericCounter& GenericCounter::operator =(const fds_uint64_t & val) {
    min_ = val;
    max_ = val;
    total_ = val;
    count_ = 1;
    return *this;
}

uint32_t GenericCounter::write(serialize::Serializer* s) const {
    uint32_t bytes = 0;
    bytes += s->writeI64(total_);
    bytes += s->writeI64(count_);
    bytes += s->writeI64(min_);
    bytes += s->writeI64(max_);
    return bytes;
}

uint32_t GenericCounter::read(serialize::Deserializer* d) {
    uint32_t bytes = 0;
    bytes += d->readI64(total_);
    bytes += d->readI64(count_);
    bytes += d->readI64(min_);
    bytes += d->readI64(max_);
    return bytes;
}

std::ostream& operator<< (std::ostream &out,
                          const GenericCounter& counter) {
    out << "[total " << counter.total()
        << " count " << counter.count()
        << " min " << counter.min()
        << " max " << counter.max() << "]";
    return out;
}

/********* StatSlot implementation **************/

StatSlot::StatSlot()
        : rel_sec(0),
          interval_sec(1) {
}

StatSlot::StatSlot(const StatSlot& slot)
        : rel_sec(slot.rel_sec),
          interval_sec(slot.interval_sec) {
    for (counter_map_t::const_iterator cit = slot.stat_map.cbegin();
         cit != slot.stat_map.cend();
         ++cit) {
        stat_map[cit->first] = cit->second;
    }
}

StatSlot::~StatSlot() {
}

void StatSlot::reset(fds_uint64_t rel_sec,
                     fds_uint32_t interval_sec /*= 0 */) {
    stat_map.clear();
    this->rel_sec = rel_sec;
    if (interval_sec > 0) {
        this->interval_sec = interval_sec;
    }
}

fds_bool_t StatSlot::isEmpty() const {
    return (stat_map.size() == 0);
}

void StatSlot::add(FdsVolStatType stat_type,
                   const GenericCounter& counter) {
    if (stat_map.count(stat_type) == 0) {
        stat_map[stat_type] = counter;
    } else {
        stat_map[stat_type] += counter;
    }
}

void StatSlot::add(FdsVolStatType stat_type,
                   fds_uint64_t value) {
    if (stat_map.count(stat_type) == 0) {
        stat_map[stat_type] = value;
    } else {
        stat_map[stat_type] += value;
    }
}

StatSlot& StatSlot::operator +=(const StatSlot & rhs) {
    if (&rhs != this) {
        // fds_verify(rhs.interval_sec == interval_sec);
        for (counter_map_t::const_iterator cit = rhs.stat_map.cbegin();
             cit != rhs.stat_map.cend();
             ++cit) {
            if (stat_map.count(cit->first) == 0) {
                stat_map[cit->first] = cit->second;
            } else {
                switch (cit->first) {
                    case STAT_SM_CUR_DOMAIN_DEDUP_BYTES_FRAC:
                    case STAT_SM_CUR_DEDUP_BYTES:
                    case STAT_DM_CUR_LBYTES:
                    case STAT_DM_CUR_PBYTES:
                    case STAT_DM_CUR_LOBJECTS:
                    case STAT_DM_CUR_POBJECTS:
                    case STAT_DM_CUR_BLOBS:
                        // These "counters" record latest values. They
                        // "monitor" rather than "count". So
                        // "adding" them actually means taking the
                        // most recent.
                        stat_map[cit->first].updateTotal(cit->second);
                        break;
                    default:
                        // remaining type of stats are simple addition
                        stat_map[cit->first] += cit->second;
                };
            }
        }
    }
    return *this;
}

StatSlot& StatSlot::operator =(const StatSlot & rhs) {
    if (&rhs != this) {
        interval_sec = rhs.interval_sec;
        rel_sec = rhs.rel_sec;
        stat_map.clear();
        for (counter_map_t::const_iterator cit = rhs.stat_map.cbegin();
             cit != rhs.stat_map.cend();
             ++cit) {
            stat_map[cit->first] = cit->second;
        }
    }
    return *this;
}

//
// returns count / time interval of this slot for event 'type'
//
double StatSlot::getEventsPerSec(FdsVolStatType type) const {
    fds_verify(interval_sec > 0);
    if (stat_map.count(type) > 0) {
        return stat_map.at(type).countPerSec(interval_sec);
    }
    return 0;
}

fds_uint64_t StatSlot::getTotal(FdsVolStatType type) const {
    if (stat_map.count(type) > 0) {
        return stat_map.at(type).total();
    }
    return 0;
}

fds_uint64_t StatSlot::getCount(FdsVolStatType type) const {
    if (stat_map.count(type) > 0) {
        return stat_map.at(type).count();
    }
    return 0;
}

//
// returns total / count for event 'type'
//
double StatSlot::getAverage(FdsVolStatType type) const {
    if (stat_map.count(type) > 0) {
        return stat_map.at(type).average();
    }
    return 0;
}

double StatSlot::getMin(FdsVolStatType type) const {
    if (stat_map.count(type) > 0) {
        return stat_map.at(type).min();
    }
    return 0;
}

double StatSlot::getMax(FdsVolStatType type) const {
    if (stat_map.count(type) > 0) {
        return stat_map.at(type).max();
    }
    return 0;
}

void StatSlot::getCounter(FdsVolStatType type,
                          GenericCounter* out_counter) const {
    fds_verify(out_counter != NULL);
    GenericCounter counter;
    if (stat_map.count(type) > 0) {
        counter += stat_map.at(type);
    }
    *out_counter = counter;
}

std::size_t StatSlot::getCounterCount() const {
    return stat_map.size();
}

uint32_t StatSlot::write(serialize::Serializer* s) const {
    uint32_t bytes = 0;
    uint32_t size = stat_map.size();
    bytes += s->writeI64(rel_sec);
    bytes += s->writeI32(interval_sec);
    bytes += s->writeI32(size);
    for (counter_map_t::const_iterator cit = stat_map.cbegin();
         cit != stat_map.cend();
         ++cit) {
        bytes += s->writeI32(cit->first);
        bytes += (cit->second).write(s);
    }
    return bytes;
}

uint32_t StatSlot::read(serialize::Deserializer* d) {
    uint32_t bytes = 0;
    uint32_t size = 0;

    stat_map.clear();
    bytes += d->readI64(rel_sec);
    bytes += d->readI32(interval_sec);
    bytes += d->readI32(size);
    for (fds_uint32_t i = 0; i < size; ++i) {
        FdsVolStatType type;
        GenericCounter counter;
        bytes += d->readI32(reinterpret_cast<int32_t&>(type));
        bytes += counter.read(d);
        stat_map[type] = counter;
    }

    return bytes;
}

std::ostream& operator<< (std::ostream &out,
                          const StatSlot& slot) {
    out << "Slot rel secs/sample interval " << slot.rel_sec
        << "/" << slot.interval_sec << " ";
    if (slot.stat_map.size() == 0) {
        out << "[empty]";
        return out;
    }

    for (counter_map_t::const_iterator cit = slot.stat_map.cbegin();
         cit != slot.stat_map.cend();
         ++cit) {
        out << "<" << cit->first << " : " << cit->second << " : Evts/sec "
            << slot.getEventsPerSec(cit->first) << "> ";
    }
    return out;
}

/*************** VolumePerfHistory ***********************/

VolumePerfHistory::VolumePerfHistory(fds_volid_t volid,
                                     fds_uint64_t start_ts,
                                     fds_uint32_t slots,
                                     fds_uint32_t slot_len_sec)
        : volid_(volid),
          max_slot_generations_(slots),
          slot_interval_sec_(slot_len_sec),
          local_start_ts_(start_ts),
          last_slot_generation_(0) {
    fds_verify(max_slot_generations_ > 1);
    stat_slots_ = new(std::nothrow) StatSlot[max_slot_generations_];
    fds_verify(stat_slots_);
    for (fds_uint32_t i = 0; i < max_slot_generations_; ++i) {
        stat_slots_[i].reset(std::numeric_limits<fds_uint64_t>::max() /* Indicates unused */,
                             slot_interval_sec_);
    }
}

VolumePerfHistory::~VolumePerfHistory() {
    if (stat_slots_) {
        delete [] stat_slots_;
    }
}

void VolumePerfHistory::recordPerfCounter(fds_uint64_t ts,
                                          FdsVolStatType stat_type,
                                          const GenericCounter& counter) {
    fds_uint64_t rel_seconds = tsToRelativeSec(ts);
    fds_uint32_t index;
    write_synchronized(stat_lock_)
    {
        index = relSecToStatHistIndex_LockHeld(rel_seconds);

        /**
         * An out-of-bounds index means the stat is too old.
         */
        if (index < max_slot_generations_) {
            stat_slots_[index].add(stat_type, counter);
        }
    }

    //LOGTRACE << " Recorded stat type <" << stat_type
    //<< "> with rel seconds <" << rel_seconds
    //<< "> giving slot index <" << index
    //<< "> using counter <" << counter << ">.";
}

void VolumePerfHistory::recordEvent(fds_uint64_t ts,
                                    FdsVolStatType stat_type,
                                    fds_uint64_t value) {
    fds_uint64_t rel_seconds = tsToRelativeSec(ts);
    fds_uint32_t index;
    write_synchronized(stat_lock_) {
        index = relSecToStatHistIndex_LockHeld(rel_seconds);

        /**
         * An out-of-bounds index means the stat is too old.
         */
        if (index < max_slot_generations_) {
            stat_slots_[index].add(stat_type, value);
        }
    }

    //if (index < max_slot_generations_) {
    //    LOGTRACE << "For volume <" << volid_
    //             << "> recorded stat type <" << stat_type
    //             << "> with rel seconds <" << rel_seconds
    //             << "> giving slot index <" << index
    //             << "> using value <" << value << ">.";
    //} else {
    //    LOGTRACE << "For volume <" << volid_
    //             << "> can't record stat type <" << stat_type
    //             << "> with rel seconds <" << rel_seconds
    //             << "> giving slot index <" << index
    //             << "> using value <" << value << ">. Stat is too old.";
    //}
}

//
// Returns local "relative seconds" that corresponds to the remote "relative seconds".
// Assumes both systems are collecting stats over the same interval of time. Also
// assumes that while collection start time nanos may be vastly different, the clocks
// on the two machines are roughly in sync' - at least within the range of a collection
// interval.
//
// If we determine that the remote relative seconds are earlier than our local start
// time, we return std::numeric_limits<fds_uint64_t>::max().
//
fds_uint64_t VolumePerfHistory::getLocalRelativeSec(fds_uint64_t remote_rel_sec,
                                                    fds_uint64_t remote_start_ts) {
    //LOGTRACE << "remote_rel_sec = <" << remote_rel_sec
    //         << ">, remote_start_ts = <" << remote_start_ts
    //         << ">, local_start_ts_ = <" << local_start_ts_ << ">.";
    /**
     * Check if we're the local machine, or possibly (though
     * improbably) on the same timestamp as the local machine.
     */
    if (remote_start_ts == local_start_ts_) {
        return remote_rel_sec;
    }

    /**
     * To do this conversion, we need to know in what generation of stats with respect
     * to our local machine the remote machine started collecting stats. For example,
     * assuming we collect stats on both machines every 60 seconds and if
     * remote_start_ts == 530000000000 and local_start_ts_ == 210000000000, then stats started collecting
     * locally earlier than remotely. And in fact, the remote machine's collection start
     * time corresponds to a local "relative seconds" of ((530000000000 - 210000000000)/NANOS_IN_SECOND) = 320
     * or a local stat generation of 320 DIV 60 = 5. So in terms of local stat collection,
     * a remote_rel_sec of, say, 420 needs to be adjusted up by 320 seconds to get the corresponding
     * local "relative seconds". Thus in this case we would have a local "relative seconds" of
     * 420 + 320 = 740 or a local stat generation of 740 DIV 60 = 12. Note that at the remote
     * machine, this was stat generation 420 DIV 60 = 7.
     *
     * If the situation were reversed with remote_start_ts == 210000000000 and local_start_ts_ == 530000000000,
     * then stats started collecting remotely earlier than locally and we should adjust
     * remote_rel_sec such that we ignore any generation of stats that were collected before the
     * local machine started its collection. That is, we need to ignore the first
     * ((530000000000 - 210000000000)/NANOS_IN_SECOND) DIV 60 = 5 generations of remote stats.
     * Whence the local "relative seconds" would be
     * 420 - 320 = 100 or a local stat generation of 100 DIV 60 = 1. Note that at the remote
     * machine, this was stat generation 420 DIV 60 = 7.
     *
     */
    auto local_rel_sec = remote_rel_sec;
    auto diff_rel_sec = tsToRelativeSec(remote_start_ts);

    /**
     * tsToRelativeSec gives us an absolute value relative to
     * local_start_ts. We have to determine the relationship
     * of the remote timestamp to the local timestamp to see
     * whether that absolute value should be added or substracted.
     */
    if (local_start_ts_ < remote_start_ts) {
        local_rel_sec += diff_rel_sec;
    } else {
        local_rel_sec = (local_rel_sec >= diff_rel_sec) ? local_rel_sec - diff_rel_sec :
                                                          std::numeric_limits<fds_uint64_t>::max();
    }

    return local_rel_sec;
}

//
// add slots from fdsp message
//
Error VolumePerfHistory::mergeSlots(const fpi::VolStatList& fdsp_volstats,
                                    fds_uint64_t fdsp_start_ts) {
    fds_verify(static_cast<uint64_t>(fdsp_volstats.volume_id) == volid_.get());
    Error err(ERR_OK);

    for (fds_uint32_t i = 0; i < fdsp_volstats.statlist.size(); ++i) {
        StatSlot remote_slot;
        err = remote_slot.loadSerialized(fdsp_volstats.statlist[i].slot_data);
        if (!err.ok()) {
            return err;
        }

        /**
         * Convert the "remote relative seconds" to a more reasonable "local relative seconds" figure.
         * This is necessary since, not only are the machines expected to *not* be working with the
         * same timestamps, but their collection start times may be different as well.
         */
        fds_uint64_t rel_seconds = getLocalRelativeSec(fdsp_volstats.statlist[i].rel_seconds, fdsp_start_ts);
        if (rel_seconds == std::numeric_limits<fds_uint64_t>::max()) {
            /**
             * getLocalRelativeSec() has determined that these stats from the remote
             * were collected for a generation (slot) prior to the start of our
             * aggregation of stats. We have nowhere to record these.
             */
            LOGDEBUG << "Slot from remote service collected prior to our start and therefore too old to merge: " << remote_slot;
        } else {
            fds_uint32_t index;
            write_synchronized(stat_lock_) {
                index = relSecToStatHistIndex_LockHeld(rel_seconds);

                /**
                 * An out-of-bounds index means the stat is too old.
                 */
                if (index < max_slot_generations_) {
                    //LOGTRACE << "Merging slot from remote service: " << remote_slot;
                    stat_slots_[index] += remote_slot;
                } else {
                    LOGDEBUG << "Slot from remote service too old to merge: " << remote_slot;
                }
            }
        }
    }
    return err;
}

//
// add slots from list of slots
//
void VolumePerfHistory::mergeSlots(const std::vector<StatSlot>& stat_list) {
    for (fds_uint32_t i = 0; i < stat_list.size(); ++i) {
        fds_uint32_t index;
        write_synchronized(stat_lock_) {
            index = relSecToStatHistIndex_LockHeld(stat_list[i].getRelSeconds());

            /**
             * An out-of-bounds index indicates that this slot is too
             * old to record.
             */
            if (index < max_slot_generations_) {
                //LOGTRACE << "Merging local slot: " << stat_list[i];
                stat_slots_[index] += stat_list[i];
            } else {
                LOGDEBUG << "Local slot too old to merge: " << stat_list[i];
            }
        }
    }
}

double VolumePerfHistory::getSMA(FdsVolStatType stat_type,
                                 fds_uint64_t end_ts,
                                 fds_uint32_t interval_sec) {
    double sma = 0;
    fds_uint32_t endix = last_slot_generation_ % max_slot_generations_;
    auto startix = (last_slot_generation_ >= max_slot_generations_) ?  // We don't start wrapping until this point.
                                       (endix + 1) % max_slot_generations_ :
                                       0;  // The oldest slot. Copy from and including this bound.
    fds_uint32_t ix = startix;

    fds_uint32_t slot_count = 0;
    fds_uint64_t rel_end_sec = (end_ts == 0) ?
                               last_slot_generation_ * slot_interval_sec_ : tsToRelativeSec(end_ts);
    fds_uint64_t rel_start_sec = rel_end_sec - interval_sec;

    // we will skip the last timeslot, because it is most likely
    // not filled in
    while (ix != endix) {
        fds_uint64_t ts = stat_slots_[ix].getRelSeconds();
        if ((ts >= rel_start_sec) && (ts < rel_end_sec)) {
            sma += stat_slots_[ix].getEventsPerSec(stat_type);
            ++slot_count;
        }
        ix = (ix + 1) % max_slot_generations_;
    }

    // get the average
    if (slot_count > 0) {
        sma = sma / slot_count;
    }

    return sma;
}

/**
 * Finds and returns an index to a slot that should hold the
 * stats for the given rel_seconds. If we need to start
 * a new slot, an old slot an any older will be cleared.
 *
 * Assumes the caller holds the stat_lock_ lock.
 */
fds_uint32_t VolumePerfHistory::relSecToStatHistIndex_LockHeld(fds_uint64_t rel_seconds) {
    auto slot_generation = rel_seconds / slot_interval_sec_;  // The generation (in historical line of descent) of slots since local_start_ts_ with respect to rel_seconds. Could be more than max_slot_generations_ and if so we wrap.
    auto slot_generation_rel_sec = slot_generation * slot_interval_sec_;  // Number of seconds since local_start_ts_ that gets us to slot_generation.
    auto slot_generation_index = slot_generation % max_slot_generations_;  // Index into stat_slots_ that corresponds to this slot_generation.

    //LOGTRACE << "slot_interval_sec_ = " << slot_interval_sec_
    //         << ", max_slot_generations_ = " << max_slot_generations_
    //         << ", rel_seconds = " << rel_seconds
    //         << ", slot_generation = " << slot_generation
    //         << ", slot_generation_rel_sec = " << slot_generation_rel_sec
    //         << ", slot_generation_index = " << slot_generation_index
    //         << ", last_slot_generation_ = " << last_slot_generation_;

    // Check if we already started to fill or are ready to fill in this slot.
    if (stat_slots_[slot_generation_index].getRelSeconds() == slot_generation_rel_sec) {
        //LOGTRACE << "Already recording stats for this slot_generation.";

        /**
         * Be sure to bump our latest generation if necessary.
         */
        if (slot_generation > last_slot_generation_) {
            last_slot_generation_ = slot_generation;
        }

        return static_cast<fds_uint32_t>(slot_generation_index);
    }

    /**
     * If this generation of slot based on rel_seconds is earlier than our
     * recorded history to-date, we'll just toss it. We don't have a mechanism
     * to record it and it's too old to be useful with respect to the history
     * we currently have anyway.
     *
     * For example, if we can record up to 4 generations of slots (max_slot_generations_ == 4) and
     * if the latest generation of slots (last_slot_generation_) is 6, then at this point we can't
     * record stats for slot_generations 3 or earlier. We can only record for generations
     * 4, 5, 6, and the now new 7.
     *
     * (Remember: slot_generation, and therefore last_slot_generation_, are 0-based.)
     */
    if ((last_slot_generation_ >= max_slot_generations_) && // We don't start wrapping until this point.
        (slot_generation <= ((last_slot_generation_ + 1) - max_slot_generations_))) {
         return (max_slot_generations_ + 1);  // Return an out of range slot_generation_index to indicate we can't record stats for the given rel_seconds.
    }

    /**
     * Clear a spot in our slot generation array for this slot generation. If it was being used, that slot
     * generation now becomes too old to maintain further.
     */
    //LOGTRACE << "Reusing slot_generation_index and setting last_slot_generation_ to slot_generation.";

    stat_slots_[slot_generation_index].reset(slot_generation_rel_sec);
    last_slot_generation_ = slot_generation;

    return static_cast<fds_uint32_t>(slot_generation_index);
}

fds_uint64_t VolumePerfHistory::toSlotList(std::vector<StatSlot>& stat_list,
                                           fds_uint64_t last_rel_sec,
                                           fds_uint32_t last_seconds_to_ignore) {

    SCOPEDREAD(stat_lock_);
    auto slot_copy_upper_bound_index = last_slot_generation_ % max_slot_generations_;  // Copy up to but not including this bound, the last generation of stats we have. We assume the upper bound is not complete yet.
    auto slot_copy_lower_bound_index = (last_slot_generation_ >= max_slot_generations_) ?  // We don't start wrapping until this point.
                                       (slot_copy_upper_bound_index + 1) % max_slot_generations_ :
                                       0;  // The oldest slot. Copy from and including this bound.

    /**
     * Adjust upper bound according to the given number of recent seconds to ignore.
     *
     * By default we would have not copied the upper bound. But the caller may be
     * even more pessimistic about other services being behind in getting their
     * stats to us. In that case, we adjust the upper bound back in history.
     */
    fds_uint32_t num_latest_slots_to_ignore;
    if (last_seconds_to_ignore == 0) {
        num_latest_slots_to_ignore = 0;
    } else {
        /**
         * As stated, we are already going to ignore the upper bound. In addition,
         * we will ignore these last few slot generations based on requested seconds
         * to ignore.
         */
        num_latest_slots_to_ignore = last_seconds_to_ignore / slot_interval_sec_;
    }
    fds_verify(num_latest_slots_to_ignore <= max_slot_generations_);

    for (fds_uint32_t i = 0; i < num_latest_slots_to_ignore; ++i) {
        if (slot_copy_upper_bound_index == 0) {
            slot_copy_upper_bound_index = (last_slot_generation_ >= max_slot_generations_) ?  // We don't start wrapping until this point.
                                          max_slot_generations_ - 1 : 0;
        } else {
            --slot_copy_upper_bound_index;
        }
    }

    //LOGTRACE << "last_rel_sec = <" << last_rel_sec
    //         << ">, last_seconds_to_ignore = <" << last_seconds_to_ignore
    //         << ">, num_latest_slots_to_ignore = <" << num_latest_slots_to_ignore
    //         << ">, last_slot_generation_ = <" << last_slot_generation_
    //         << ">, max_slot_generations_ = <" << max_slot_generations_
    //         << ">, slot_copy_lower_bound_index = <" << slot_copy_lower_bound_index
    //         << ">, slot_copy_upper_bound_index = <" << slot_copy_upper_bound_index << ">.";

    auto slot_index = slot_copy_lower_bound_index;
    fds_uint32_t slots_copied = 0;

    stat_list.clear();
    while (slot_index != slot_copy_upper_bound_index) {
        auto slot_rel_seconds = stat_slots_[slot_index].getRelSeconds();

        /**
         * Ensure we don't copy any slots from the given last_rel_sec
         * entry or earlier, mindful of copying the first generation
         * of slots.
         */
        if ((slot_rel_seconds != std::numeric_limits<fds_uint64_t>::max()) &&  // If this slot *has* been used to capture stats and
            ((slot_rel_seconds > last_rel_sec) ||                              // (this slot has not been copied yet or
             (last_rel_sec == std::numeric_limits<fds_uint64_t>::max()))) {    //  this is our first copy effort and so all slot_rel_seconds (except max's) are wanted.)
            stat_list.push_back(stat_slots_[slot_index]);
            ++slots_copied;
        }

        slot_index = (slot_index + 1) % max_slot_generations_;  // Go to next slot, mindful to wrap.
    }

    /**
     * Return the relative seconds of the latest generation of slot we copied.
     * It was up to but not including last_slot_generation_ and the number of
     * slots we were asked to ignore.
     */
    fds_uint64_t last_added_rel_sec;
    if (slots_copied == 0) {
        last_added_rel_sec = last_rel_sec;
    } else {
        auto last_added_generation = last_slot_generation_ - (num_latest_slots_to_ignore + 1);
        last_added_rel_sec = last_added_generation * slot_interval_sec_;
    }

    return last_added_rel_sec;
}

StatSlot* VolumePerfHistory::getPreviousSlot(const StatSlot& current_slot) {
    SCOPEDREAD(stat_lock_);
    auto slot_search_upper_bound_index = last_slot_generation_ % max_slot_generations_;
    auto slot_search_lower_bound_index = (last_slot_generation_ >= max_slot_generations_) ?  // We don't start wrapping until this point.
                                             (slot_search_upper_bound_index + 1) % max_slot_generations_ : 0;  // The "oldest" slot.

    StatSlot* prev_slot = nullptr;

    /**
     * Locate our current slot.
     */
    auto slot_index = slot_search_lower_bound_index;
    bool current_slot_found = false;
    while ((slot_index != slot_search_upper_bound_index) && !current_slot_found) {
        if (current_slot.getRelSeconds() == stat_slots_[slot_index].getRelSeconds()) {
            current_slot_found = true;
            break;
        }

        slot_index = (slot_index + 1) % max_slot_generations_;  // Go to next slot, mindful to wrap.
    }

    /**
     * If we found the current_slot, see if we can find the
     * one previous to it.
     */
    if (current_slot_found) {
        /**
         * Make sure the current slot is not the slot at our lower
         * bound. If it is, there is no slot previous to it.
         */
        if (slot_index != slot_search_lower_bound_index) {
            /**
             * Go to the previous slot mindful to wrap.
             */
            slot_index = (slot_index == 0) ? max_slot_generations_ - 1 : slot_index - 1;
            prev_slot = &(stat_slots_[slot_index]);

            /**
             * If the previous slot is uninitialized, we have no previous slot data.
             */
            if (prev_slot->getRelSeconds() == std::numeric_limits<fds_uint64_t>::max()) {
                prev_slot = nullptr;
            } else {
                /**
                 * If we don't recognize the previous slot's timestamp, then we can't use it.
                 */
                if (prev_slot->getRelSeconds() != current_slot.getRelSeconds() - slot_interval_sec_) {
                    prev_slot = nullptr;
                }
            }
        }
    }

    return prev_slot;
}

/**
 * Here we are generating a list of Volume Stats for a specific volume
 * ready to be sent to the service that will aggregate them. In this
 * case, we want to capture all the slots or generations of stats that
 * have been collected since we last collected.
 */
fds_uint64_t VolumePerfHistory::toFdspPayload(fpi::VolStatList& fdsp_volstat,
                                              fds_uint64_t last_rel_sec) {

    SCOPEDREAD(stat_lock_);
    auto slot_copy_upper_bound_index = last_slot_generation_ % max_slot_generations_;  // Copy up to *and* including this bound. In this case we're sending all the collected stats.
    auto slot_copy_lower_bound_index = (last_slot_generation_ >= max_slot_generations_) ?  // We don't start wrapping until this point.
                                            (slot_copy_upper_bound_index + 1) % max_slot_generations_ :
                                            0;  // The oldest slot. Copy from and including this bound.

    //LOGTRACE << "last_rel_sec = <" << last_rel_sec
    //         << ">, last_slot_generation_ = <" << last_slot_generation_
    //         << ">, max_slot_generations_ = <" << max_slot_generations_
    //         << ">, slot_copy_lower_bound_index = <" << slot_copy_lower_bound_index
    //         << ">, slot_copy_upper_bound_index = <" << slot_copy_upper_bound_index << ">.";

    fdsp_volstat.statlist.clear();
    fdsp_volstat.volume_id = volid_.get();

    fds_uint32_t slots_copied = 0;
    auto first_time = true;
    for (auto slot_index = slot_copy_lower_bound_index;

            // Stop if we're past the last generation. Determining which slot holds the last generation
            // is different depending upon whether we've wrapped or not.
            (last_slot_generation_ < max_slot_generations_) ?
                                            ((slot_index == 0) ? first_time :
                                                                 (slot_index <= last_slot_generation_)) :
                                            ((slot_index == slot_copy_lower_bound_index) ? first_time :
                                                                                           true);

              // Go to next slot, mindful to wrap.
              slot_index = (slot_index + 1) % max_slot_generations_, first_time = false) {

        auto slot_rel_seconds = stat_slots_[slot_index].getRelSeconds();

        /**
         * Ensure that we don't copy any unused slots and that
         * we don't copy any slots from the given last_rel_sec
         * entry or earlier.
         */
        if ((slot_rel_seconds != std::numeric_limits<fds_uint64_t>::max()) &&  // If this slot *has* been used to capture stats and
            ((slot_rel_seconds > last_rel_sec) ||                              // (this slot has not been copied yet or
             (last_rel_sec == std::numeric_limits<fds_uint64_t>::max()))) {    //  this is our first copy effort and so all slot_rel_seconds (except max's) are wanted.)
            fpi::VolStatSlot fdsp_slot;
            stat_slots_[slot_index].getSerialized(fdsp_slot.slot_data);
            fdsp_slot.rel_seconds = slot_rel_seconds;
            fdsp_volstat.statlist.push_back(fdsp_slot);

            ++slots_copied;
        }
    }

    /**
     * Return the relative seconds of the latest generation of slot we serialized.
     * It was up to and including last_slot_generation_.
     */
    fds_uint64_t last_added_rel_sec;
    if (slots_copied == 0) {
        last_added_rel_sec = last_rel_sec;
    } else {
        last_added_rel_sec = last_slot_generation_ * slot_interval_sec_;
    }

    return last_added_rel_sec;
}

//
// print IOPS to file
//
fds_uint64_t VolumePerfHistory::print(std::ofstream& dumpFile,
                                      boost::posix_time::ptime curts,
                                      fds_uint64_t last_rel_sec,
                                      fds_uint32_t last_seconds_to_ignore) {

    SCOPEDREAD(stat_lock_);
    auto slot_print_upper_bound_index = last_slot_generation_ % max_slot_generations_;  // Print up to but not including this bound. We assume the upper bound is not compelte yet.
    auto slot_print_lower_bound_index = (last_slot_generation_ >= max_slot_generations_) ?  // We don't start wrapping until this point.
                                        (slot_print_upper_bound_index + 1) % max_slot_generations_ :
                                        0;  // The oldest slot. Print from and including this bound.

    /**
     * Adjust upper bound according to the given number of recent seconds to ignore.
     *
     * By default we would have not copied the upper bound. But the caller may be
     * even more pessimistic about other services being behind in getting their
     * stats to us. In that case, we adjust the upper bound back in history.
     */
    fds_uint32_t num_latest_slots_to_ignore;
    if (last_seconds_to_ignore == 0) {
        num_latest_slots_to_ignore = 0;
    } else {
        /**
         * We subtract 1 from the number of slots to ignore based on the last seconds
         * to ignore since, as stated, we are already going to ignore the upper bound.
         */
        num_latest_slots_to_ignore = (last_seconds_to_ignore / slot_interval_sec_) - 1;
    }

    fds_verify(num_latest_slots_to_ignore <= (max_slot_generations_ - 1));

    for (fds_uint32_t i = 0; i < num_latest_slots_to_ignore; ++i) {
        if (slot_print_upper_bound_index == 0) {
            slot_print_upper_bound_index = max_slot_generations_ - 1;
        } else {
            --slot_print_upper_bound_index;
        }
    }

    auto slot_index = slot_print_lower_bound_index;

    fds_uint32_t slots_printed = 0;
    while (slot_index != slot_print_upper_bound_index) {
        auto slot_rel_seconds = stat_slots_[slot_index].getRelSeconds();
        if ((slot_rel_seconds != std::numeric_limits<fds_uint64_t>::max()) &&  // If this slot *has* been used to capture stats and
            ((slot_rel_seconds > last_rel_sec) ||                              // (this slot has not been printed yet or
             (last_rel_sec == std::numeric_limits<fds_uint64_t>::max()))) {    //  this is our first print effort and so all slot_rel_seconds (except max's) are wanted.)
            // Aggregate PUT_IO and GET_IO counters
            GenericCounter put_counter, io_counter, tot_counter, m_counter;
            fds_uint32_t slot_len = stat_slots_[slot_index].slotLengthSec();
            stat_slots_[slot_index].getCounter(STAT_AM_PUT_OBJ, &put_counter);
            stat_slots_[slot_index].getCounter(STAT_AM_GET_OBJ, &io_counter);
            io_counter += put_counter;

            stat_slots_[slot_index].getCounter(STAT_AM_GET_BMETA, &tot_counter);
            stat_slots_[slot_index].getCounter(STAT_AM_PUT_BMETA, &m_counter);
            tot_counter += m_counter;
            tot_counter += io_counter;

            dumpFile << "[" << to_simple_string(curts) << "],"
            << volid_ << ","
            << stat_slots_[slot_index].getRelSeconds() << ","
            << static_cast<fds_uint32_t>(tot_counter.countPerSec(slot_len)) << ","
            << static_cast<fds_uint32_t>(io_counter.countPerSec(slot_len)) << ","  // iops
                     << static_cast<fds_uint32_t>(io_counter.average()) << ","    // ave latency
                     << io_counter.min() << ","                   // min latency
                     << io_counter.max() << ","                  // max latency
                     << stat_slots_[slot_index].getAverage(STAT_AM_QUEUE_WAIT) << ","  // ave wait time
                     << stat_slots_[slot_index].getMin(STAT_AM_QUEUE_WAIT) << ","      // min wait time
                     << stat_slots_[slot_index].getMax(STAT_AM_QUEUE_WAIT) << ","      // max wait time
                     << stat_slots_[slot_index].getAverage(STAT_AM_QUEUE_BACKLOG) << ","
                        << stat_slots_[slot_index].getMin(STAT_AM_QUEUE_BACKLOG) << ","
                        << stat_slots_[slot_index].getMax(STAT_AM_QUEUE_BACKLOG) << ","
                        << stat_slots_[slot_index].getEventsPerSec(STAT_AM_GET_OBJ) << ","
                        << stat_slots_[slot_index].getAverage(STAT_AM_GET_OBJ) << ","
                        << stat_slots_[slot_index].getEventsPerSec(STAT_AM_GET_BMETA) << ","
                        << stat_slots_[slot_index].getAverage(STAT_AM_GET_BMETA) << ","
                        << stat_slots_[slot_index].getEventsPerSec(STAT_AM_PUT_OBJ) << ","
                        << stat_slots_[slot_index].getAverage(STAT_AM_PUT_OBJ) << ","
                     << stat_slots_[slot_index].getEventsPerSec(STAT_AM_PUT_BMETA) << ","
                     << stat_slots_[slot_index].getAverage(STAT_AM_PUT_BMETA) << std::endl;
        }

        ++slots_printed;

        slot_index = (slot_index + 1) % max_slot_generations_;
    }

    /**
     * Return the relative seconds of the latest generation of slot we printed.
     * It was up to but not including last_slot_generation_ and the number of
     * slots we were asked to ignore.
     */
    fds_uint64_t last_printed_rel_sec;
    if (slots_printed == 0) {
        last_printed_rel_sec = last_rel_sec;
    } else {
        last_printed_rel_sec = last_slot_generation_ - (num_latest_slots_to_ignore + 1);
    }

    return last_printed_rel_sec;
}

VolumePerfHistory::ptr VolumePerfHistory::getSnapshot() {
    VolumePerfHistory::ptr snap(new VolumePerfHistory(volid_, local_start_ts_,
                                                      max_slot_generations_, slot_interval_sec_));
    SCOPEDREAD(stat_lock_);
    snap->last_slot_generation_ = last_slot_generation_;
    for (fds_uint32_t ix = 0; ix < max_slot_generations_; ++ix) {
        snap->stat_slots_[ix] = stat_slots_[ix];
    }
    return snap;
}

std::ostream& operator<< (std::ostream &out,
                          const VolumePerfHistory& hist) {
    auto slot_stream_upper_bound_index = hist.last_slot_generation_ % hist.max_slot_generations_;  // Stream up to but not including this bound.
    auto slot_stream_lower_bound_index = (hist.last_slot_generation_ >= hist.max_slot_generations_) ?  // We don't start wrapping until this point.
                                         (slot_stream_upper_bound_index + 1) % hist.max_slot_generations_ :
                                         0;  // The oldest slot. Stream from and including this bound.

    out << "Perf History: volume " << std::hex << hist.volid_
        << std::dec << "\n";

    auto first_time = true;
    for (auto slot_index = slot_stream_lower_bound_index;

        // Stop if we're past the last generation. Determining which slot holds the last generation
        // is different depending upon whether we've wrapped or not.
         (hist.last_slot_generation_ < hist.max_slot_generations_) ?
                                         ((slot_index == 0) ? first_time :
                                                              (slot_index <= hist.last_slot_generation_)) :
                                         ((slot_index == slot_stream_lower_bound_index) ? first_time :
                                                                                          true);

        // Go to next slot, mindful to wrap.
         slot_index = (slot_index + 1) % hist.max_slot_generations_, first_time = false) {

        out << hist.stat_slots_[slot_index] << "\n";
    }
    return out;
}

std::ostream& operator<< (std::ostream &out,
                          const VolStatListWrapper&volStatListWrapper) {
    out << "Stat List: volume " << std::hex << volStatListWrapper.getVolStatList().volume_id << std::dec << "\n";

    for (fds_uint32_t i = 0; i < volStatListWrapper.getVolStatList().statlist.size(); ++i) {
        StatSlot slot;

        auto err = slot.loadSerialized(volStatListWrapper.getVolStatList().statlist[i].slot_data);
        if (!err.ok()) {
            out << err;
            return out;
        }

        out << slot << "\n";
    }

    return out;
}
}  // namespace fds
