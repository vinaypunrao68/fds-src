/**
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <limits>
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
        : rel_ts_sec(0),
          interval_sec(1) {
}

StatSlot::~StatSlot() {
}

void StatSlot::reset(fds_uint64_t rel_sec,
                     fds_uint32_t stat_size /*= 0 */) {
    stat_map.clear();
    rel_ts_sec = rel_sec;
    if (stat_size > 0) {
        interval_sec = stat_size;
    }
}

void StatSlot::add(PerfEventType counter_type,
                   const GenericCounter& counter) {
    stat_map[counter_type] += counter;
}

void StatSlot::add(PerfEventType counter_type,
                   fds_uint64_t value) {
    stat_map[counter_type] += value;
}

//
// returns count / time interval of this slot for event 'type'
//
double StatSlot::getEventsPerSec(PerfEventType type) const {
    fds_verify(interval_sec > 0);
    if (stat_map.count(type) > 0) {
        double events = stat_map.at(type).count();
        return events / interval_sec;
    }
    return 0;
}

fds_uint64_t StatSlot::getTotal(PerfEventType type) const {
    if (stat_map.count(type) > 0) {
        return stat_map.at(type).total();
    }
    return 0;
}

//
// returns total / count for event 'type'
//
double StatSlot::getAverage(PerfEventType type) const {
    if (stat_map.count(type) > 0) {
        return stat_map.at(type).average();
    }
    return 0;
}

uint32_t StatSlot::write(serialize::Serializer* s) const {
    uint32_t bytes = 0;
    uint32_t size = stat_map.size();
    bytes += s->writeI64(rel_ts_sec);
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
    bytes += d->readI64(rel_ts_sec);
    bytes += d->readI32(interval_sec);
    bytes += d->readI32(size);
    for (fds_uint32_t i = 0; i < size; ++i) {
        PerfEventType type;
        GenericCounter counter;
        bytes += d->readI32(reinterpret_cast<int32_t&>(type));
        bytes += counter.read(d);
    }

    return bytes;
}

std::ostream& operator<< (std::ostream &out,
                          const StatSlot& slot) {
    out << "Slot ts " << slot.rel_ts_sec
        << "/" << slot.interval_sec << " ";
    if (slot.stat_map.size() == 0) {
        out << "[empty]";
        return out;
    }

    for (counter_map_t::const_iterator cit = slot.stat_map.cbegin();
         cit != slot.stat_map.cend();
         ++cit) {
        out << "<" << cit->first << " : " << cit->second << "> ";
    }
    return out;
}

/*************** VolumePerfHistory ***********************/

VolumePerfHistory::VolumePerfHistory(fds_volid_t volid,
                                     fds_uint64_t start_ts,
                                     fds_uint32_t slots,
                                     fds_uint32_t slot_len_sec)
        : volid_(volid),
          nslots_(slots),
          slotsec_(slot_len_sec),
          start_nano_(start_ts),
          last_slot_num_(0) {
    stat_slots_ = new(std::nothrow) StatSlot[nslots_];
    fds_verify(stat_slots_);
    for (fds_uint32_t i = 0; i < nslots_; ++i) {
        stat_slots_[i].reset(0, slotsec_);
    }
}

VolumePerfHistory::~VolumePerfHistory() {
    if (stat_slots_) {
        delete [] stat_slots_;
    }
}

void VolumePerfHistory::recordPerfCounter(fds_uint64_t ts,
                                          PerfEventType counter_type,
                                          const GenericCounter& counter) {
    fds_uint64_t rel_seconds = tsToRelativeSec(ts);
    fds_uint32_t index = useSlot(rel_seconds);
    if ((index >= 0) && (index < nslots_)) {
        stat_slots_[index].add(counter_type, counter);
    }
}

void VolumePerfHistory::recordEvent(fds_uint64_t ts,
                                    PerfEventType counter_type,
                                    fds_uint64_t value) {
    fds_uint64_t rel_seconds = tsToRelativeSec(ts);
    fds_uint32_t index = useSlot(rel_seconds);
    if ((index >= 0) && (index < nslots_)) {
        stat_slots_[index].add(counter_type, value);
    }
}

double VolumePerfHistory::getSMA(PerfEventType event_type,
                                 fds_uint64_t end_ts,
                                 fds_uint32_t interval_sec) {
    double sma = 0;
    fds_uint32_t endix = last_slot_num_ % nslots_;
    fds_uint32_t startix = (endix + 1) % nslots_;  // ind of oldest slot
    fds_uint32_t ix = startix;

    fds_uint32_t slot_count = 0;
    fds_uint64_t rel_end_sec = (end_ts == 0) ?
            last_slot_num_ * slotsec_ : tsToRelativeSec(end_ts);
    fds_uint64_t rel_start_sec = rel_end_sec - interval_sec;

    // we will skip the last timeslot, because it is most likely
    // not filled in
    while (ix != endix) {
        fds_uint64_t ts = stat_slots_[ix].getTimestamp();
        if ((ts >= rel_start_sec) && (ts < rel_end_sec)) {
            sma += stat_slots_[ix].getEventsPerSec(event_type);
            ++slot_count;
        }
        ix = (ix + 1) % nslots_;
    }

    // get the average
    if (slot_count > 0) {
        sma = sma / slot_count;
    }

    return sma;
}

//
// Finds a slot that should hold rel_seconds timestamp; if we need to start
// a new slot, the old slot will be cleared and also unfilled slots in between
//
fds_uint32_t VolumePerfHistory::useSlot(fds_uint64_t rel_seconds) {
    fds_uint64_t slot_num = rel_seconds / slotsec_;
    fds_uint64_t slot_start_sec = rel_seconds;
    fds_uint32_t index = slot_num % nslots_;

    // common case -- we will use already existing slot

    // check if we already started to fill in this slot
    if (stat_slots_[index].getTimestamp() == slot_start_sec) {
        // we already started filling in this slot, will add stats to it
        return index;
    }

    // we need to start a new time slot
    if (slot_num <= last_slot_num_) {
        // the new time slot is in the past -- too old
        return (nslots_ + 1);  // out of range index
    }

    // start filling a new time slot
    stat_slots_[index].reset(slot_start_sec);

    // if there are unfilled slots in between, clear them and
    // set appropriate timestamps
    if (slot_num == 0) {
        last_slot_num_ = slot_num;
        return index;
    }

    fds_uint64_t cur_slot_num = slot_num - 1;
    while (cur_slot_num > last_slot_num_) {
        fds_uint32_t ind = cur_slot_num % nslots_;

        // make sure we did not do full circle back to 'index'
        if (ind == index) break;

        // check if we haven't started filling in this timeslot
        // this matters if stats may come out of order
        if (stat_slots_[ind].getTimestamp() != (cur_slot_num * slotsec_)) {
            stat_slots_[ind].reset(cur_slot_num * slotsec_);
        }

        cur_slot_num--;
    }

    // update last slot num
    last_slot_num_ = slot_num;
    return index;
}

std::ostream& operator<< (std::ostream &out,
                          const VolumePerfHistory& hist) {
    fds_uint32_t endix = hist.last_slot_num_ % hist.nslots_;
    fds_uint32_t startix = (endix + 1) % hist.nslots_;  // ind of oldest slot
    fds_uint32_t ix = startix;

    out << "Perf History: volume " << std::hex << hist.volid_
        << std::dec << "\n";
    while (ix != endix) {
        out << hist.stat_slots_[ix] << "\n";
        ix = (ix + 1) % hist.nslots_;
    }
    return out;
}

}  // namespace fds
