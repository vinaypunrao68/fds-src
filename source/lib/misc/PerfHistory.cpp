/**
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <limits>
#include <vector>
#include <PerfHistory.h>

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
        if (rhs.total() > total_) {
            total_ = rhs.total();
        }
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

void StatSlot::add(FdsStatType stat_type,
                   const GenericCounter& counter) {
    if (stat_map.count(stat_type) == 0) {
        stat_map[stat_type] = counter;
    } else {
        stat_map[stat_type] += counter;
    }
}

void StatSlot::add(FdsStatType stat_type,
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
                    case STAT_SM_CUR_DEDUP_BYTES:
                    case STAT_DM_CUR_TOTAL_BYTES:
                    case STAT_DM_CUR_TOTAL_OBJECTS:
                    case STAT_DM_CUR_TOTAL_BLOBS:
                        // these counters count total number so far; so
                        // adding them actually means taking the highest
                        // (which is also most recent) total
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
double StatSlot::getEventsPerSec(FdsStatType type) const {
    fds_verify(interval_sec > 0);
    if (stat_map.count(type) > 0) {
        return stat_map.at(type).countPerSec(interval_sec);
    }
    return 0;
}

fds_uint64_t StatSlot::getTotal(FdsStatType type) const {
    if (stat_map.count(type) > 0) {
        return stat_map.at(type).total();
    }
    return 0;
}

fds_uint64_t StatSlot::getCount(FdsStatType type) const {
    if (stat_map.count(type) > 0) {
        return stat_map.at(type).count();
    }
    return 0;
}

//
// returns total / count for event 'type'
//
double StatSlot::getAverage(FdsStatType type) const {
    if (stat_map.count(type) > 0) {
        return stat_map.at(type).average();
    }
    return 0;
}

double StatSlot::getMin(FdsStatType type) const {
    if (stat_map.count(type) > 0) {
        return stat_map.at(type).min();
    }
    return 0;
}

double StatSlot::getMax(FdsStatType type) const {
    if (stat_map.count(type) > 0) {
        return stat_map.at(type).max();
    }
    return 0;
}

void StatSlot::getCounter(FdsStatType type,
                          GenericCounter* out_counter) const {
    fds_verify(out_counter != NULL);
    GenericCounter counter;
    if (stat_map.count(type) > 0) {
        counter += stat_map.at(type);
    }
    *out_counter = counter;
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
        FdsStatType type;
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
          start_nano_(start_ts),
          last_slot_generation_(0) {
    fds_verify(max_slot_generations_ > 1);
    stat_slots_ = new(std::nothrow) StatSlot[max_slot_generations_];
    fds_verify(stat_slots_);
    for (fds_uint32_t i = 0; i < max_slot_generations_; ++i) {
        stat_slots_[i].reset(0, slot_interval_sec_);
    }
}

VolumePerfHistory::~VolumePerfHistory() {
    if (stat_slots_) {
        delete [] stat_slots_;
    }
}

void VolumePerfHistory::recordPerfCounter(fds_uint64_t ts,
                                          FdsStatType stat_type,
                                          const GenericCounter& counter) {
    fds_uint64_t rel_seconds = tsToRelativeSec(ts);
    fds_uint32_t index;
    write_synchronized(stat_lock_)
    {
        index = useSlotLockHeld(rel_seconds);
        if (index < max_slot_generations_) {
            stat_slots_[index].add(stat_type, counter);
        }
    }

    LOGTRACE << " Recorded stat type <" << stat_type
    << "> with rel seconds <" << rel_seconds
    << "> giving slot index <" << index
    << "> using counter <" << counter << ">.";
}

void VolumePerfHistory::recordEvent(fds_uint64_t ts,
                                    FdsStatType stat_type,
                                    fds_uint64_t value) {
    fds_uint64_t rel_seconds = tsToRelativeSec(ts);
    fds_uint32_t index;
    write_synchronized(stat_lock_) {
        index = useSlotLockHeld(rel_seconds);

        /**
         * An out-of-bounds index means the stat is too old.
         */
        if (index < max_slot_generations_) {
            stat_slots_[index].add(stat_type, value);
        }
    }

    if (index < max_slot_generations_) {
        LOGTRACE << " Recorded stat type <" << stat_type
                 << "> with rel seconds <" << rel_seconds
                 << "> giving slot index <" << index
                 << "> using value <" << value << ">.";
    } else {
        LOGTRACE << " Can't recorded stat type <" << stat_type
                 << "> with rel seconds <" << rel_seconds
                 << "> giving slot index <" << index
                 << "> using value <" << value << ">. Stat is too old.";
    }
}

//
// add slots from fdsp message
//
Error VolumePerfHistory::mergeSlots(const fpi::VolStatList& fdsp_volstats) {
    Error err(ERR_OK);
    fds_verify(static_cast<uint64_t>(fdsp_volstats.volume_id) == volid_.get());
    for (fds_uint32_t i = 0; i < fdsp_volstats.statlist.size(); ++i) {
        StatSlot remote_slot;
        err = remote_slot.loadSerialized(fdsp_volstats.statlist[i].slot_data);
        if (!err.ok()) {
            return err;
        }

        /**
         * Regardless of the relationship between the clocks of the machine that
         * generated this stat_list and this machine merging them, relative seconds
         * between the two machines do not require any transformation since in both
         * places they represent the difference between the start_nano_ and current
         * timestamp of their respective machines converted to seconds.
         *
         * It's a difference, not an absolute.
         */
        fds_uint64_t rel_seconds = fdsp_volstats.statlist[i].rel_seconds;
        fds_uint32_t index;
        write_synchronized(stat_lock_) {
            index = useSlotLockHeld(rel_seconds);
            if (index < max_slot_generations_) {
                stat_slots_[index] += remote_slot;
            }
        }

        LOGTRACE << "For volume <" << fdsp_volstats.volume_id << "> using slot index <" << index
        << "> with relative seconds <" << rel_seconds << "> to merge stats.";
    }
    return err;
}

//
// add slots from list of slots
//
void VolumePerfHistory::mergeSlots(const std::vector<StatSlot>& stat_list) {
    for (fds_uint32_t i = 0; i < stat_list.size(); ++i) {
        /**
         * Regardless of the relationship between the clocks of the machine that
         * generated this stat_list and this machine merging them, relative seconds
         * between the two machines do not require any transformation since in both
         * places they represent the difference between the start_nano_ and current
         * timestamp of their respective machines converted to seconds.
         *
         * It's a difference, not an absolute.
         */
        auto rel_seconds = stat_list[i].getRelSeconds();
        write_synchronized(stat_lock_) {
            auto index = useSlotLockHeld(rel_seconds);

            /**
             * An out-of-bounds index indicates that this slot is too
             * old to record.
             */
            if (index < max_slot_generations_) {
                stat_slots_[index] += stat_list[i];
            }
        }
    }
}

double VolumePerfHistory::getSMA(FdsStatType stat_type,
                                 fds_uint64_t end_ts,
                                 fds_uint32_t interval_sec) {
    double sma = 0;
    fds_uint32_t endix = last_slot_generation_ % max_slot_generations_;
    fds_uint32_t startix = (endix + 1) % max_slot_generations_;  // ind of oldest slot
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
fds_uint32_t VolumePerfHistory::useSlotLockHeld(fds_uint64_t rel_seconds) {
    auto slot_generation = rel_seconds / slot_interval_sec_;  // The generation (in historical line of descent) of slots since start_nano_ with respect to rel_seconds. Could be more than max_slot_generations_ and if so we wrap.
    auto remainder = rel_seconds % slot_interval_sec_;
    auto slot_generation_rel_sec = rel_seconds - remainder;  // Number of seconds since start_nano_ that gets us to slot_generation.
    auto slot_generation_index = slot_generation % max_slot_generations_;  // Index into stat_slots_ that corresponds to this slot_generation.

    LOGTRACE << "slot_interval_sec_ = " << slot_interval_sec_
             << ", max_slot_generations_ = " << max_slot_generations_
             << ", rel_seconds = " << rel_seconds
             << ", slot_generation = " << slot_generation
             << ", slot_generation_rel_sec = " << slot_generation_rel_sec
             << ", slot_generation_index = " << slot_generation_index
             << ", last_slot_generation_ = " << last_slot_generation_;

    // Check if we already started to fill in this slot.
    if (stat_slots_[slot_generation_index].getRelSeconds() == slot_generation_rel_sec) {
        // We have! Go with it.
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
    stat_slots_[slot_generation_index].reset(slot_generation_rel_sec);
    last_slot_generation_ = slot_generation;

    /**
     * Based on this latest slot generation, we may find older generation slots in our array that
     * are no longer valid because we can't store that generation any more. (Consider
     * what happens, for example, if I/O for a volume stops for a period of time
     * rendering our history in the array obsolete given the latest generation to be
     * stored just determined.) Therefore, spin through the array searching for these old guys
     * and put them out of the misery, making way for the newer generations.
     *
     * The newer generations would be ahead of the current generation in the array, being mindful of wrapping.
     */
    auto last_slot_generation_index = slot_generation_index;
    slot_generation_index = (slot_generation_index + 1) % max_slot_generations_;
    slot_generation++;
    while (slot_generation_index != last_slot_generation_index) {
        /**
         * Look for slots whose relative seconds are less than the earliest relative seconds
         * we could store. (We either have to convert the latest unsupported generation to relative seconds
         * or relative seconds to generation. We'll do the former.)
         *
         * We'll take the opportunity to set the expected relative seconds for the newer generation slot which means
         * if we hit a match on expected relative seconds, we're done!
         */
        if (stat_slots_[slot_generation_index].getRelSeconds() <= ((last_slot_generation_ - max_slot_generations_) * slot_interval_sec_)) {
            stat_slots_[slot_generation_index].reset(slot_generation * slot_interval_sec_);
        } else if (stat_slots_[slot_generation_index].getRelSeconds() == slot_generation * slot_interval_sec_) {
            break;
        }

        /**
         * Wrap the index, not the slot generation.
         */
        slot_generation_index = (slot_generation_index + 1) % max_slot_generations_;
        slot_generation++;
    }

    return static_cast<fds_uint32_t>(last_slot_generation_index);
}

fds_uint64_t VolumePerfHistory::toSlotList(std::vector<StatSlot>& stat_list,
                                           fds_uint64_t last_rel_sec,
                                           fds_uint32_t max_slots,
                                           fds_uint32_t last_seconds_to_ignore) {
    /**
     * Ensure last_slot_generation_ is up to date with current time
     * (last_slot_generation_ managed by useSlotLockHeld()).
     * This is important because if IO stopped and history was not updated,
     * the last slot before idle period will be lost. Why? Because we will
     * assume that the last slot generation is not complete until we move
     * to the next slot generation.
     */
    auto rel_seconds = tsToRelativeSec(util::getTimeStampNanos());
    write_synchronized(stat_lock_) {
        useSlotLockHeld(rel_seconds);
    }

    SCOPEDREAD(stat_lock_);
    auto slot_copy_upper_bound_index = last_slot_generation_ % max_slot_generations_;  // Copy up to but not including this bound.
    auto slot_copy_lower_bound_index = (last_slot_generation_ + 1) % max_slot_generations_;  // Potentially the oldest slot. Copy from and including this bound.

    /**
     * Adjust lower bound according to the given number of recent seconds to ignore.
     */
    auto num_latest_slots_to_ignore = last_seconds_to_ignore / slot_interval_sec_;
    fds_verify(num_latest_slots_to_ignore < (max_slot_generations_ - 1));
    for (fds_uint32_t i = 0; i < num_latest_slots_to_ignore; ++i) {
        if (slot_copy_upper_bound_index == 0) {
            slot_copy_upper_bound_index = max_slot_generations_ - 1;
        } else {
            --slot_copy_upper_bound_index;
        }
    }

    auto slot_index = slot_copy_lower_bound_index;
    fds_uint64_t latest_rel_seconds = 0;
    fds_uint32_t slots_copied = 0;

    stat_list.clear();
    while (slot_index != slot_copy_upper_bound_index) {
        auto slot_rel_seconds = stat_slots_[slot_index].getRelSeconds();

        /**
         * Ensure we don't copy any slots from the given last_rel_sec
         * entry or earlier.
         */
        if (slot_rel_seconds > last_rel_sec) {
            stat_list.push_back(stat_slots_[slot_index]);
            ++slots_copied;

            if (latest_rel_seconds < slot_rel_seconds) {
                latest_rel_seconds = slot_rel_seconds;
            }
        }

        if ((max_slots > 0) && (slots_copied >= max_slots)) break;

        slot_index = (slot_index + 1) % max_slot_generations_;  // Go to next slot, mindful to wrap.
    }

    /**
     * Return the relative seconds of the latest slot we copied. If
     * we didn't copy anything, return the lastest relative seconds
     * we were told to start with.
     */
    auto last_added_rel_sec = last_rel_sec;
    if (latest_rel_seconds != 0) {
        last_added_rel_sec = latest_rel_seconds;
    }

    return last_added_rel_sec;
}

fds_uint64_t VolumePerfHistory::toFdspPayload(fpi::VolStatList& fdsp_volstat,
                                              fds_uint64_t last_rel_sec) {
    /**
     * Ensure last_slot_generation_ is up to date with current time
     * (last_slot_generation_ managed by useSlotLockHeld()).
     * This is important because if IO stopped and history was not updated,
     * the last slot before idle period will be lost. Why? Because we will
     * assume that the last slot generation is not complete until we move
     * to the next slot generation.
     */
    auto rel_seconds = tsToRelativeSec(util::getTimeStampNanos());
    write_synchronized(stat_lock_) {
        useSlotLockHeld(rel_seconds);
    }

    SCOPEDREAD(stat_lock_);
    auto slot_copy_upper_bound_index = last_slot_generation_ % max_slot_generations_;  // Copy up to but not including this bound.
    auto slot_copy_lower_bound_index = (slot_copy_upper_bound_index + 1) % max_slot_generations_;  // Potentially the oldest slot. Copy from and including this bound.

    auto slot_index = slot_copy_lower_bound_index;
    fds_uint64_t latest_rel_seconds = 0;

    fdsp_volstat.statlist.clear();
    fdsp_volstat.volume_id = volid_.get();
    while (slot_index != slot_copy_upper_bound_index) {
        auto slot_rel_seconds = stat_slots_[slot_index].getRelSeconds();

        /**
         * Ensure we don't copy any slots from the given last_rel_sec
         * entry or earlier.
         */
        if (slot_rel_seconds > last_rel_sec) {
            fpi::VolStatSlot fdsp_slot;
            stat_slots_[slot_index].getSerialized(fdsp_slot.slot_data);
            fdsp_slot.rel_seconds = slot_rel_seconds;
            fdsp_volstat.statlist.push_back(fdsp_slot);

            if (latest_rel_seconds < slot_rel_seconds) {
                latest_rel_seconds = slot_rel_seconds;
            }
        }

        slot_index = (slot_index + 1) % max_slot_generations_;  // Go to next slot, mindful to wrap.
    }

    /**
     * Return the relative seconds of the latest slot we copied. If
     * we didn't copy anything, return the lastest relative seconds
     * we were told to start with.
     */
    auto last_added_rel_sec = last_rel_sec;
    if (latest_rel_seconds != 0) {
        last_added_rel_sec = latest_rel_seconds;
    }

    return last_added_rel_sec;
}

//
// print IOPS to file
//
fds_uint64_t VolumePerfHistory::print(std::ofstream& dumpFile,
                                      boost::posix_time::ptime curts,
                                      fds_uint64_t last_rel_sec) {
    // ensure last_slot_generation_ is up to date
    // important because if IO stopped and history was not updated
    // the last slot before idle period will never printed
    fds_uint64_t rel_seconds = tsToRelativeSec(util::getTimeStampNanos());
    write_synchronized(stat_lock_) {
        fds_uint32_t index = useSlotLockHeld(rel_seconds);
    }

    fds_uint32_t endix = last_slot_generation_ % max_slot_generations_;
    fds_uint32_t startix = (endix + 1) % max_slot_generations_;  // ind of oldest slot
    fds_uint32_t ix = startix;
    fds_uint64_t latest_ts = 0;
    fds_uint64_t last_printed_ts = last_rel_sec;

    // we will skip the last timeslot, because it may not be fully filled in
    while (ix != endix) {
        fds_uint64_t ts = stat_slots_[ix].getRelSeconds();
        if (ts > last_rel_sec) {
            // Aggregate PUT_IO and GET_IO counters
            GenericCounter put_counter, io_counter, tot_counter, m_counter;
            fds_uint32_t slot_len = stat_slots_[ix].slotLengthSec();
            stat_slots_[ix].getCounter(STAT_AM_PUT_OBJ, &put_counter);
            stat_slots_[ix].getCounter(STAT_AM_GET_OBJ, &io_counter);
            io_counter += put_counter;

            stat_slots_[ix].getCounter(STAT_AM_GET_BMETA, &tot_counter);
            stat_slots_[ix].getCounter(STAT_AM_PUT_BMETA, &m_counter);
            tot_counter += m_counter;
            tot_counter += io_counter;

            dumpFile << "[" << to_simple_string(curts) << "],"
            << volid_ << ","
            << stat_slots_[ix].getRelSeconds() << ","
            << static_cast<fds_uint32_t>(tot_counter.countPerSec(slot_len)) << ","
            << static_cast<fds_uint32_t>(io_counter.countPerSec(slot_len)) << ","  // iops
                     << static_cast<fds_uint32_t>(io_counter.average()) << ","    // ave latency
                     << io_counter.min() << ","                   // min latency
                     << io_counter.max() << ","                  // max latency
                     << stat_slots_[ix].getAverage(STAT_AM_QUEUE_WAIT) << ","  // ave wait time
                     << stat_slots_[ix].getMin(STAT_AM_QUEUE_WAIT) << ","      // min wait time
                     << stat_slots_[ix].getMax(STAT_AM_QUEUE_WAIT) << ","      // max wait time
                     << stat_slots_[ix].getAverage(STAT_AM_QUEUE_FULL) << ","
                     << stat_slots_[ix].getMin(STAT_AM_QUEUE_FULL) << ","
                     << stat_slots_[ix].getMax(STAT_AM_QUEUE_FULL) << ","
                     << stat_slots_[ix].getEventsPerSec(STAT_AM_GET_OBJ) << ","
                     << stat_slots_[ix].getAverage(STAT_AM_GET_OBJ) << ","
                     << stat_slots_[ix].getEventsPerSec(STAT_AM_GET_BMETA) << ","
                     << stat_slots_[ix].getAverage(STAT_AM_GET_BMETA) << ","
                     << stat_slots_[ix].getEventsPerSec(STAT_AM_PUT_OBJ) << ","
                     << stat_slots_[ix].getAverage(STAT_AM_PUT_OBJ) << ","
                     << stat_slots_[ix].getEventsPerSec(STAT_AM_PUT_BMETA) << ","
                     << stat_slots_[ix].getAverage(STAT_AM_PUT_BMETA) << std::endl;

            if (latest_ts < ts) {
                latest_ts = ts;
            }
        }
        ix = (ix + 1) % max_slot_generations_;
    }

    if (latest_ts != 0) {
        last_printed_ts = latest_ts;
    }

    return last_printed_ts;
}

VolumePerfHistory::ptr VolumePerfHistory::getSnapshot() {
    VolumePerfHistory::ptr snap(new VolumePerfHistory(volid_, start_nano_,
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
    fds_uint32_t endix = hist.last_slot_generation_ % hist.max_slot_generations_;
    fds_uint32_t startix = (endix + 1) % hist.max_slot_generations_;  // ind of oldest slot
    fds_uint32_t ix = startix;

    out << "Perf History: volume " << std::hex << hist.volid_
        << std::dec << "\n";
    while (ix != endix) {
        out << hist.stat_slots_[ix] << "\n";
        ix = (ix + 1) % hist.max_slot_generations_;
    }
    return out;
}

}  // namespace fds
