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

fds_bool_t StatSlot::isEmpty() const {
    return (stat_map.size() == 0);
}

void StatSlot::add(FdsStatType stat_type,
                   const GenericCounter& counter) {
    stat_map[stat_type] += counter;
}

void StatSlot::add(FdsStatType stat_type,
                   fds_uint64_t value) {
    stat_map[stat_type] += value;
}

StatSlot& StatSlot::operator +=(const StatSlot & rhs) {
    if (&rhs != this) {
        fds_verify(rhs.interval_sec == interval_sec);
        for (counter_map_t::const_iterator cit = rhs.stat_map.cbegin();
             cit != rhs.stat_map.cend();
             ++cit) {
            stat_map[cit->first] += cit->second;
        }
    }
    return *this;
}

StatSlot& StatSlot::operator =(const StatSlot & rhs) {
    if (&rhs != this) {
        interval_sec = rhs.interval_sec;
        rel_ts_sec = rhs.rel_ts_sec;
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

//
// returns total / count for event 'type'
//
double StatSlot::getAverage(FdsStatType type) const {
    if (stat_map.count(type) > 0) {
        return stat_map.at(type).average();
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
    out << "Slot ts " << slot.rel_ts_sec
        << "/" << slot.interval_sec << " ";
    if (slot.stat_map.size() == 0) {
        out << "[empty]";
        return out;
    }

    for (counter_map_t::const_iterator cit = slot.stat_map.cbegin();
         cit != slot.stat_map.cend();
         ++cit) {
        out << "<" << cit->first << " : " << cit->second << " : Evt/sec "
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
                                          FdsStatType stat_type,
                                          const GenericCounter& counter) {
    fds_uint64_t rel_seconds = tsToRelativeSec(ts);
    fds_uint32_t index = useSlot(rel_seconds);
    if ((index >= 0) && (index < nslots_)) {
        stat_slots_[index].add(stat_type, counter);
    }
}

void VolumePerfHistory::recordEvent(fds_uint64_t ts,
                                    FdsStatType stat_type,
                                    fds_uint64_t value) {
    fds_uint64_t rel_seconds = tsToRelativeSec(ts);
    fds_uint32_t index = useSlot(rel_seconds);
    if ((index >= 0) && (index < nslots_)) {
        stat_slots_[index].add(stat_type, value);
    }
}

//
// returns local timestamp that corresponds to remote timestamp
//
fds_uint64_t VolumePerfHistory::getLocalRelativeSec(fds_uint64_t remote_rel_sec,
                                                    fds_uint64_t remote_start_ts) {
    fds_uint64_t local_ts = timestamp(start_nano_, remote_rel_sec);
    if (start_nano_ > remote_start_ts) {
        // remote_start_time ... start_time ......... local_ts
        fds_uint64_t diff_nano = start_nano_ - remote_start_ts;
        local_ts -= diff_nano;
    } else {
        // start_time ... remote_start_time ........ local_ts
        fds_uint64_t diff_nano = remote_start_ts - start_nano_;
        local_ts += diff_nano;
    }
    return tsToRelativeSec(local_ts);
}

//
// add slots from fdsp message
//
Error VolumePerfHistory::mergeSlots(const fpi::VolStatList& fdsp_volstats,
                                    fds_uint64_t fdsp_start_ts) {
    Error err(ERR_OK);
    fds_verify(fdsp_volstats.volume_id == volid_);
    for (fds_uint32_t i = 0; i < fdsp_volstats.statlist.size(); ++i) {
        fds_uint64_t rel_seconds = getLocalRelativeSec(fdsp_volstats.statlist[i].rel_seconds,
                                                       fdsp_start_ts);
        fds_uint32_t index = useSlot(rel_seconds);
        if ((index >= 0) && (index < nslots_)) {
            StatSlot remote_slot;
            err = remote_slot.loadSerialized(fdsp_volstats.statlist[i].slot_data);
            if (!err.ok()) {
                return err;
            }
            stat_slots_[index] += remote_slot;
        }
    }
    return err;
}

double VolumePerfHistory::getSMA(FdsStatType stat_type,
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
            sma += stat_slots_[ix].getEventsPerSec(stat_type);
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

fds_uint64_t VolumePerfHistory::toFdspPayload(fpi::VolStatList& fdsp_volstat,
                                              fds_uint64_t last_rel_sec) const {
    fds_uint32_t endix = last_slot_num_ % nslots_;
    fds_uint32_t startix = (endix + 1) % nslots_;  // index of oldest slot
    fds_uint32_t ix = startix;
    fds_uint64_t latest_ts = 0;
    fds_uint64_t last_added_ts = 0;

    fdsp_volstat.statlist.clear();
    fdsp_volstat.volume_id = volid_;
    while (ix != endix) {
        fds_uint64_t ts = stat_slots_[ix].getTimestamp();
        if (ts > last_rel_sec) {
            fpi::VolStatSlot fdsp_slot;
            stat_slots_[ix].getSerialized(fdsp_slot.slot_data);
            fdsp_slot.rel_seconds = ts;
            fdsp_volstat.statlist.push_back(fdsp_slot);

            if (latest_ts < ts) {
                latest_ts = ts;
            }
        }
        ix = (ix + 1) % nslots_;
    }

    if (latest_ts != 0) {
        last_added_ts = latest_ts;
    }

    return last_added_ts;
}

//
// print IOPS to file
//
fds_uint64_t VolumePerfHistory::print(std::ofstream& dumpFile,
                                      fds_uint64_t cur_ts,
                                      fds_uint64_t last_rel_sec) const {
    fds_uint32_t endix = last_slot_num_ % nslots_;
    fds_uint32_t startix = (endix + 1) % nslots_;  // ind of oldest slot
    fds_uint32_t ix = startix;
    fds_uint64_t latest_ts = 0;
    fds_uint64_t last_printed_ts = 0;

    // we will skip the last timeslot, because it may not be fully filled in
    while (ix != endix) {
        fds_uint64_t ts = stat_slots_[ix].getTimestamp();
        if (ts > last_rel_sec) {
            // Aggregate PUT_IO and GET_IO counters
            GenericCounter put_counter, io_counter;
            fds_uint32_t slot_len = stat_slots_[ix].slotLengthSec();
            stat_slots_[ix].getCounter(STAT_AM_PUT_OBJ, &put_counter);
            stat_slots_[ix].getCounter(STAT_AM_GET_OBJ, &io_counter);
            io_counter += put_counter;

            dumpFile << "[" << cur_ts << "],"
                     << volid_ << ","
                     << stat_slots_[ix].getTimestamp() << ","
                     << io_counter.countPerSec(slot_len) << ","   // iops
                     << io_counter.average() << ","               // ave latency
                     << io_counter.min() << ","                   // min latency
                     << io_counter.max() << ","                  // max latency
                     << stat_slots_[ix].getEventsPerSec(STAT_AM_GET_OBJ) << ","
                     << stat_slots_[ix].getEventsPerSec(STAT_AM_PUT_OBJ) << std::endl;

            if (latest_ts < ts) {
                latest_ts = ts;
            }
        }
        ix = (ix + 1) % nslots_;
    }

    if (latest_ts != 0) {
        last_printed_ts = latest_ts;
    }

    return last_printed_ts;
}

VolumePerfHistory::ptr VolumePerfHistory::getSnapshot() const {
    VolumePerfHistory::ptr snap(new VolumePerfHistory(volid_, start_nano_,
                                                      nslots_, slotsec_));
    snap->last_slot_num_ = last_slot_num_;
    for (fds_uint32_t ix = 0; ix < nslots_; ++ix) {
        snap->stat_slots_[ix] = stat_slots_[ix];
    }
    return snap;
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
