/*
 * Copyright 2013 Formation Data Systems, Inc.
 *
 *  Hierarchical token bucket algorithm
 * 
 */
#ifndef SOURCE_UTIL_COUNTER_HISTORY_H_
#define SOURCE_UTIL_COUNTER_HISTORY_H_

#include <boost/date_time/posix_time/posix_time.hpp>

#define COUNTER_HIST_MAX_LENGTH 20

namespace fds {

 /* A circular array of counters (e.g. to track access times or IOPS) 
  * where each array entry represents a time slot (configurable length,
  * the precision if 1 second). */
class CounterHist8bit {
 public:
  boost::posix_time::ptime last_access_ts;
  fds_uint8_t counters[nslots];

  static boost::posix_time::ptime getFirstSlotTimestamp() {
    return boost::posix_time::second_clock::universal_time();
  }

 static const nslots = 8;
 static const max_value = 127;

 /* Increment counter. Will increment counter at timeslot representing current time */
 inline void increment(const boost::posix_time::ptime& first_ts, fds_uint32_t slot_len_seconds)
 {
  assert(cur_ts > last_access_ts);
  boost::posix_time::ptime now = boost::posix_time::microsec_clock::universal_time();

  int last_slot_num = getSlotNum(first_ts, last_access_ts, slot_len_seconds);
  int next_slot_num = getSlotNum(first_ts, now, slot_len_seconds);
  int slot_ix = last_slot_num % nslots;

  if (next_slot_num > last_slot_num) {
    /* will start filling the next slot, possibly skipping idle */
    do {
      ++last_slot_num;
      slot_ix = (slot_ix + 1) % nslots;
      counters[slot_ix] = 0;
    } while (next_slot_num < last_slot_num);
  }

  /* increment value in current slot */
  if (counters[slots_ix] < (max_value-1))
    counters[slot_ix]++;

  /* update last_access_ts */
  last_access_ts = now;
}

 /* Add another counter history to this one */
 inline void add(const CounterHist8bit& counter_hist, const boost::posix_time::ptime& first_ts, fds_uint32_t slot_len_seconds)
 {
   //TBD
 }

 /* This is an array of powers of 0.9 */
 static const double kweights[COUNTER_HIST_MAX_LENGTH] =
  {1.0000, 0.9000, 0.8100, 0.7290, 0.6561, 0.5905, 0.5314, 0.4783, 0.4305, 0.3874, 
   0.3487, 0.3138, 0.2824, 0.2541, 0.2288, 0.2059, 0.1853, 0.1668, 0.1501, 0.1351};

 /* Returns weighted counter (the counters of most recent slots get higher weight) */
 inline fds_uint32_t getWeightedCount(const boost::posix_time::ptime& first_ts, fds_uint32_t slot_len_seconds)
 {
   /* calculating weighted average over recent history using 
    * formula from Zygaria paper [RTAS 2006] */
   double wma = 0.0;
   int slot_ix = getSlotNum(first_ts, last_access_ts, slot_len_seconds) % nslots;

   int ix = (slot_ix + 1) % nslots; // oldest history slot  
   for (int i = (nslots-1); i >= 0; --i)
     {
       wma += (counters[ix] * kweights[i]);
       ix = (ix + 1) % nslots;
    }
   return (fds_uint32_t)wma;
 }

 private: /* methods */

 inline int getSlotNum(const boost::posix_time::ptime& first_ts, 
		       const boost::posix_time::ptime& ts,
		       fds_uint32_t slot_len_seconds)
 {
   boost::posix_time::duration delta = ts - first_ts;
   return delta.seconds() / slot_len_seconds;
 }

};

} // namespace 

#endif /* SOURCE_UTIL_COUNTER_HISTORY_H */
