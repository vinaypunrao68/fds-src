/*
 * Copyright 2013 Formation Data Systems, Inc.
 *
 * A 'decaying' counter class that is implemented
 * as a circular array of counters. Useful for 
 * getting weighted average or sum based on how
 * the recency of the counter
 * 
 */
#ifndef SOURCE_UTIL_COUNTER_HISTORY_H_
#define SOURCE_UTIL_COUNTER_HISTORY_H_

#include <boost/date_time/posix_time/posix_time.hpp>
#include <assert.h>
#include "fds_types.h"

#define COUNTER_HIST_MAX_LENGTH 20

namespace fds {

 /* A circular array of counters (e.g. to track access times or IOPS) 
  * where each array entry represents a time slot (configurable length,
  * the precision if 1 second). 
  * 
  * Usage:
  * Every function expects 'first_ts' timestamp which the 
  * caller can get by first calling (one time):
  * CounterHist8bit::getFirstSlotTimestamp() and then pass it
  * to every function that is called later 
  * Every function also expected 'slot_len_seconds' which must
  * be the same all the time (otherwise the behavior is undefined),
  * this is the time length in second of a slot. First 'slot_len_seconds'
  * the first entry of the array gets updates, then the next entry and so
  * on. When we fill the whole array, we start erasing oldest slot (set it to 0)
  * and start incrementing it, and so on.
  *
  * This class cares about the space usage, so only keeps timestamp of the last 
  * access and array of 8 (could change the nslots constant) of 8-bit entries. So
  * with current configuration, the class takes 2*64 bit space. 
  *
  * */
class CounterHist8bit {
 public:
  CounterHist8bit() {
    last_access_ts = boost::posix_time::second_clock::universal_time();
    memset(counters, 0, nslots * sizeof(fds_uint8_t));
  }
  ~CounterHist8bit() {}

  /* last time we updated the counter by calling increment() function */
  boost::posix_time::ptime last_access_ts;

  static const int nslots = 8;
  static const int max_value = 127;

  /* use this array directly ONLY if need to save it to leveldb or other persistent
  * store. If need to get the current counter, call getWeighedCounter that returns
  * the weigted sum of counters */
  fds_uint8_t counters[nslots];

  /* call this function first, but once is enough for all objects we need to keep counters for */
  static boost::posix_time::ptime getFirstSlotTimestamp() {
    return boost::posix_time::second_clock::universal_time();
  }

 /* Increment counter. Will increment counter at timeslot representing current time
  * It will also update last_access_ts with the current time
  * So if you are re-using last_access_ts for 'last read timestamp', this function will 
  * automatically update it  
  * \return true if we move on to the next time slot, otherwise false */
 inline fds_bool_t increment(const boost::posix_time::ptime& first_ts, fds_uint32_t slot_len_seconds)
 {
  assert(first_ts <= last_access_ts);
  fds_bool_t bnext_slot = false;
  boost::posix_time::ptime now = boost::posix_time::second_clock::universal_time();

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
    bnext_slot = true;
  }

  /* increment value in current slot */
  if (counters[slot_ix] < (max_value-1))
    counters[slot_ix]++;

  /* update last_access_ts */
  last_access_ts = now;

  return bnext_slot;
}

 /* Add another counter history to this one */
 inline void add(const CounterHist8bit& counter_hist, const boost::posix_time::ptime& first_ts, fds_uint32_t slot_len_seconds)
 {
   //TBD
 }

 /* Returns weighted counter (the counters of most recent slots get higher weight) 
 * Call this method to get one value representing the count, the most common usage of this class */
 inline fds_uint32_t getWeightedCount(const boost::posix_time::ptime& first_ts, fds_uint32_t slot_len_seconds)
 {

   /* This is an array of powers of 0.9 */
   const double kweights[COUNTER_HIST_MAX_LENGTH] =
     {1.0000, 0.9000, 0.8100, 0.7290, 0.6561, 0.5905, 0.5314, 0.4783, 0.4305, 0.3874, 
     0.3487, 0.3138, 0.2824, 0.2541, 0.2288, 0.2059, 0.1853, 0.1668, 0.1501, 0.1351};

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
   boost::posix_time::time_duration delta = ts - first_ts;
   return delta.seconds() / slot_len_seconds;
 }

};

} // namespace 

#endif /* SOURCE_UTIL_COUNTER_HISTORY_H */
