#include "PerfStat.H"

namespace fds {

  Stat::Stat()
    : nsamples(0), min_lat(98765432), max_lat(0), ave_lat(0) 
  {
  }

  Stat::~Stat() 
  {
  }

  void Stat::reset()
  {
    nsamples = 0;
    ave_lat = 0;
    min_lat = 98765432; /* real latency should be smaller */
    max_lat = 0; 
  }

  void Stat::add(long lat) 
  {
    double n = nsamples;
    ++nsamples;

    /* accumulate average latency */
    ave_lat = (n*ave_lat + (double)lat)/(double)nsamples;

    if (lat < min_lat)
      min_lat = lat;

    if (lat > max_lat)
      max_lat = lat;
  }

  void Stat::add(const Stat& stat)
  {
    double n1 = nsamples;
    double n2 = stat.nsamples;
    nsamples += stat.nsamples;

    ave_lat = (n1*ave_lat + n2*stat.ave_lat)/(double)nsamples;

    if (min_lat > stat.min_lat)
      min_lat = stat.min_lat;

    if (max_lat < stat.max_lat)
      max_lat = stat.max_lat;
  }

  IoStat::IoStat()
    :stat(), rel_ts_sec(0), interval_sec(FDS_STAT_DEFAULT_SLOT_LENGTH)
  {
  }

  IoStat::~IoStat()
  {
  }

  void IoStat::add(long microlat)
  {
    stat.add(microlat);
  }

  void IoStat::add(const IoStat& iostat)
  {
    stat.add(iostat.stat);
  }

  void IoStat::reset(long ts_sec, int stat_size_sec)
  {
    stat.reset();
    rel_ts_sec = ts_sec;
    if (stat_size_sec > 0)
      interval_sec = stat_size_sec;
  }

  long IoStat::getTimestamp() const
  {
    return rel_ts_sec;
  }

  int IoStat::getIops() const 
  {
    return (int)((double)stat.nsamples / (double) interval_sec);
  }

  long IoStat::getAveLatency() const
  {
    return (long)stat.ave_lat;
  }

  long IoStat::getMinLatency() const
  {
  if (stat.nsamples == 0)
    return 0;
  else
    return stat.min_lat;
  }

  long IoStat::getMaxLatency() const
  {
    return stat.max_lat;
  }

  StatHistory::StatHistory(int _stream_id, int slots, int slot_len_sec)
    : stream_id(_stream_id), nslots(slots), sec_in_slot(slot_len_sec), last_printed_ts(-1)
  {
    /* cap nslots to max */
    if (nslots > FDS_STAT_MAX_HIST_SLOTS)
      nslots = FDS_STAT_MAX_HIST_SLOTS;
    
    /* create array that will hold stats history */
    stat_slots = new IoStat[nslots];
    if (!stat_slots) {
      nslots = 0;
      return;
    }
    for (int i = 0; i < nslots; ++i)
      {
	stat_slots[i].reset(0, sec_in_slot);
      }

    start_time = 0.0;
    last_slot_num = 0;
  }

  StatHistory::~StatHistory()
  {
    delete [] stat_slots;
  }

  void StatHistory::addStat(const IoStat& stat)
  {
    long slot_num = stat.getTimestamp() / sec_in_slot;
    long slot_start_sec = stat.getTimestamp();
    int index = slot_num % nslots;

    /* check if we already started to fill in this slot */
    if (stat_slots[index].getTimestamp() == slot_start_sec)
      {
	/* the slot indexed with this timestamp already has I/Os of the same time interval */
	stat_slots[index].add(stat);
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
    stat_slots[index].add(stat);

    /* update last slot num */
    last_slot_num = slot_num;
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

  void StatHistory::print(FILE* dumpFile, double curts)
  {
    int endix = last_slot_num % nslots;
    int startix = (endix + 1) % nslots; /* index of oldest stat */
    int ix = startix;
    long latest_ts = 0;

    if (nslots == 0) return;

    /* we will skip the last timeslot, because it is most likely not filled in */
    while (ix != endix)
      {
	long ts = stat_slots[ix].getTimestamp();
	if (ts > last_printed_ts)
	  {
            fprintf(dumpFile, "-- %.2f,%d,%ld,%d,%ld,%ld,%ld\n",
		    curts, 
		    stream_id,
		    (long)start_time + stat_slots[ix].getTimestamp(),
		    stat_slots[ix].getIops(), 
		    stat_slots[ix].getAveLatency(),
		    stat_slots[ix].getMinLatency(),
		    stat_slots[ix].getMaxLatency());

	    /*
	    dumpFile << "[" << to_simple_string(curts) << "]," 
		     << volid << ","
		     << stat_slots[ix].getTimestamp() << ","
		     << stat_slots[ix].getIops() << ","
		     << stat_slots[ix].getAveLatency() << ","
		     << stat_slots[ix].getMinLatency() << ","
		     << stat_slots[ix].getMaxLatency() << std::endl;

	    */
	    if (latest_ts < ts)
	      latest_ts = ts;

	  }
	ix = (ix + 1) % nslots;
      }

    if (latest_ts != 0)
      last_printed_ts = latest_ts;
  }

} /* namespace fds*/
