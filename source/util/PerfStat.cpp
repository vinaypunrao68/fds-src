#include "PerfStat.h"

namespace fds {

  Stat::Stat()
    : nsamples(0), ave_lat(0) 
  {
  }

  Stat::~Stat() 
  {
  }

  void Stat::reset()
  {
    nsamples = 0;
    ave_lat = 0;
  }

  void Stat::add(long lat) 
  {
    double n = nsamples;
    ++nsamples;

    /* accumulate average latency */
    ave_lat = (n*ave_lat + (double)lat)/(double)nsamples;
  }

  void Stat::add(const Stat& stat)
  {
    double n1 = nsamples;
    double n2 = stat.nsamples;
    nsamples += stat.nsamples;

    ave_lat = (n1*ave_lat + n2*stat.ave_lat)/(double)nsamples;
  }

  IoStat::IoStat()
    :stat()
  {
  }

  IoStat::~IoStat()
  {
  }

  void IoStat::add(long microlat)
  {
    stat.add(microlat);
  }

  void IoStat::reset(long ts_sec)
  {
    stat.reset();
    rel_ts_sec = ts_sec;
  }

  long IoStat::getTimestamp() const
  {
    return rel_ts_sec;
  }

  int IoStat::getIos() const 
  {
    return stat.nsamples;
  }

  double IoStat::getAveLatency() const
  {
    return stat.ave_lat;
  }

  StatHistory::StatHistory(int slots, int slot_len_sec)
  {
    nslots = slots;
    sec_in_slot = slot_len_sec;
    
    /* create array that will hold stats history */
    stat_slots = new IoStat[nslots];
    if (!stat_slots) {
      nslots = 0;
      return;
    }

    start_time = boost::posix_time::second_clock::universal_time();
    last_slot_num = 0;
  }

  StatHistory::~StatHistory()
  {
    delete [] stat_slots;
  }

  void StatHistory::addIo(boost::posix_time::ptime timestamp, long microlat)
  {
    long slot_num = ts2slotnum(timestamp);
    long slot_start_sec = slot_num * sec_in_slot;
    int index = slot_num % nslots;

    /* check if we already started to fill in this slot */
    if (stat_slots[index].getTimestamp() == slot_start_sec)
      {
	/* the slot indexed with this timestamp already has I/Os of the same time interval */
	stat_slots[index].add(microlat);
	return;
      }

    /* we need to start a new time slot */

    if (slot_num <= last_slot_num) {
      /* we need a new slot, but it's it the past, so we got timestamp that is too old */
      /* maybe we should return error, since we should configure long enough history so that does not happen */
      return;
    }

    /* start filling a new timeslot */
    stat_slots[index].reset(slot_start_sec);

    /* if there are unfilled slots in between, clear then and set appropriate timestamps */
    long cur_slot_num = slot_num - 1;
    while (cur_slot_num > last_slot_num) {
      int ind = cur_slot_num % nslots;

      /* make sure we did not do full circle back to the index of timestamp of this IO */
      if (ind == index) break;

      stat_slots[ind].reset(cur_slot_num * sec_in_slot);
      cur_slot_num--;
    }

    /* actually add IO stat  */
    stat_slots[index].add(microlat);

    /* update last slot num */
    last_slot_num = slot_num;
  }

  long StatHistory::ts2slotnum(boost::posix_time::ptime timestamp)
  {
    boost::posix_time::time_duration elapsed = timestamp - start_time;
    return (elapsed.total_seconds() / sec_in_slot);
  }

  int StatHistory::getStatsCopy(IoStat** stat_ary, int* len)
  {
    IoStat *stats = NULL;
    int length = nslots - 1; /* we most likely will not have last slot completely filled in*/

    int statix = ((last_slot_num % nslots) + 1) % nslots;  /* index of the oldest stat in stat_slots */

    /* if we just started and have uninitialized slots, 
     * it's ok, we will just get first set of 0s, we can
     * throw away first history, if not, it will still look ok on graph */

    /* create stats array */
    stats = new IoStat[length];
    if (!stats) {
      return -1;
    }

    /* fill in stats array, index = 0 will have the oldest stat */
    for (int i = 0; i < length; ++i)
      {
	stats[i] = stat_slots[statix];
	statix = (statix + 1) % nslots;
      }

    /* return */
    *stat_ary = stats;
    *len = length;

    return 0;
  }

} /* namespace fds*/
