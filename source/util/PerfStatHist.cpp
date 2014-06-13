#include <limits>
#include <util/PerfStatHist.h>

namespace fds {

  Stat::Stat()
    : nsamples(0), min_lat(std::numeric_limits<unsigned long>::max()), max_lat(0), ave_lat(0) 
  {
  }

  Stat::~Stat() 
  {
  }

  void Stat::reset()
  {
    nsamples = 0;
    ave_lat = 0;
    min_lat = std::numeric_limits<unsigned long>::max(); /* real latency should be smaller */
    max_lat = 0; 
  }

  void Stat::add(unsigned long lat) 
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
    :rel_ts_sec(0), interval_sec(FDS_STAT_DEFAULT_SLOT_LENGTH)
  {
  }

  IoStat::~IoStat()
  {
  }

  void IoStat::add(unsigned long microlat, diskio::DataTier tier, fds_io_op_t opType)
  {
    int index = statIndex(tier, opType);
    fds_verify(index < PERF_STAT_TYPES);
    stat[index].add(microlat);
    stat[PERF_STAT_TYPES-1].add(microlat);
  }

  void IoStat::reset(long ts_sec, int stat_size_sec)
  {
    for (int i = 0; i < PERF_STAT_TYPES; ++i) {
      stat[i].reset();
    }
    rel_ts_sec = ts_sec;
    if (stat_size_sec > 0)
      interval_sec = stat_size_sec;
  }

  long IoStat::getTimestamp() const
  {
    return rel_ts_sec;
  }

  /* returns total iops */
  unsigned long IoStat::getIops() const 
  {
    return (unsigned long)((double)stat[PERF_STAT_TYPES-1].nsamples / (double) interval_sec);
  }

  /* returns iops of specific type */
  unsigned long IoStat::getIops(diskio::DataTier tier, fds_io_op_t opType) const 
  {
    int index = statIndex(tier, opType);
    fds_verify(index < PERF_STAT_TYPES)
    return (unsigned long)((double)stat[index].nsamples / (double) interval_sec);
  }

  /* ave latency over all ops */
  unsigned long IoStat::getAveLatency() const
  {
    return (long)stat[PERF_STAT_TYPES-1].ave_lat;
  }

  /* ave latency over ops of specific type */
  unsigned long IoStat::getAveLatency(diskio::DataTier tier, fds_io_op_t opType) const
  {
    int index = statIndex(tier, opType);
    fds_verify(index < PERF_STAT_TYPES);
    return (unsigned long)stat[index].ave_lat;
  }

  unsigned long IoStat::getMinLatency() const
  {
  if (stat[PERF_STAT_TYPES-1].nsamples == 0)
    return 0;
  else
    return stat[PERF_STAT_TYPES-1].min_lat;
  }

  unsigned long IoStat::getMaxLatency() const
  {
    return stat[PERF_STAT_TYPES-1].max_lat;
  }

  void IoStat::addFromFdspMsg(const FDS_ProtocolInterface::FDSP_PerfStatType& fdsp_stat)
  {
    int index = fdsp_stat.stat_type;
    fds_verify((index >= 0) && (index < PERF_STAT_TYPES));

    Stat stat_to_add;
    stat_to_add.reset();
    stat_to_add.nsamples = fdsp_stat.nios;
    stat_to_add.min_lat = fdsp_stat.min_lat;
    stat_to_add.max_lat = fdsp_stat.max_lat;
    stat_to_add.ave_lat = fdsp_stat.ave_lat; 

    stat[index].add(stat_to_add); 
  }

  bool IoStat::copyIfNotZero(FDS_ProtocolInterface::FDSP_PerfStatType& fdsp_stat,
                             int stat_type)
  {
    int index = stat_type; /* we use index for stat type, 
                            * make sure only IoStat interprets stat_type */
    if (index >= PERF_STAT_TYPES) 
      return false;

    /* to reduce amount of data we transfer, OM for now only cares about total, disk/flash read/write 
     * stat_type. This means ignoring index > 3 && index != PERF_STAT_TYPE-1*/
    if ((index > 3) && (index != (PERF_STAT_TYPES-1)))
	return false;

    long iops = (long)((double)stat[index].nsamples / (double) interval_sec);
    if (iops <= 0)
      return false; /* no iops recorded */

    fdsp_stat.stat_type = stat_type;
    fdsp_stat.rel_seconds = rel_ts_sec;
    fdsp_stat.nios = iops;
    fdsp_stat.min_lat = stat[index].min_lat;
    fdsp_stat.max_lat = stat[index].max_lat;
    fdsp_stat.ave_lat = (long)stat[index].ave_lat;
    return true;
  }

  StatHistory::StatHistory(fds_uint32_t _id, int slots, int slot_len_sec)
    : id(_id), nslots(slots), sec_in_slot(slot_len_sec), last_printed_ts(-1), last_returned_ts(-1)
  {
    /* cap nslots to max */
    if (nslots > FDS_STAT_MAX_HIST_SLOTS)
      nslots = FDS_STAT_MAX_HIST_SLOTS;
    
    /* create array that will hold stats history */
    stat_slots = new IoStat[nslots];
    for (int i = 0; i < nslots; ++i)
      {
	stat_slots[i].reset(0, sec_in_slot);
      }

    last_slot_num = 0;
  }

  StatHistory::~StatHistory()
  {
    delete [] stat_slots;
  }

  void StatHistory::addIo(long rel_seconds, unsigned long microlat, diskio::DataTier tier, fds_io_op_t opType)
  {
    int index = 0;
    lock.write_lock();

    index = prepareSlotWithWriteLockHeld(rel_seconds);
    if ((index >= 0) && (index < nslots))
      stat_slots[index].add(microlat, tier, opType); 

    lock.write_unlock();
  }

  void StatHistory::addStat(long rel_seconds, 
			    const FDS_ProtocolInterface::FDSP_PerfStatType& fdsp_stat)
  {
    int index = 0;
    lock.write_lock();

    index = prepareSlotWithWriteLockHeld(rel_seconds);
    if ((index >= 0) && (index < nslots)) {
      stat_slots[index].addFromFdspMsg(fdsp_stat);
    }

    lock.write_unlock();
  }


  /* \return index of slot that we need to fill in, -1 means do not fill in  */
  int StatHistory::prepareSlotWithWriteLockHeld(long rel_seconds)
  {
    long slot_num = rel_seconds / sec_in_slot;
    long slot_start_sec = rel_seconds;
    int index = slot_num % nslots;

    /* common case -- we will add IO to already existing slot */

    /* check if we already started to fill in this slot */
    if (stat_slots[index].getTimestamp() == slot_start_sec)
      {
	/* the slot indexed with this timestamp already has I/Os of the same time interval */
	return index;
      }
     
    if (slot_num <= last_slot_num) {
      /* we need a new slot, but it's it the past, so we got timestamp that is too old */
      /* maybe we should return error, since we should configure long enough history so that does not happen */
      return -1;
    }

    /* we need to start a new time slot */

    /* start filling a new timeslot */
    stat_slots[index].reset(slot_start_sec);

    /* if there are unfilled slots in between, clear then and set appropriate timestamps */
    long cur_slot_num = slot_num - 1;
    while (cur_slot_num > last_slot_num) {
      int ind = cur_slot_num % nslots;

      /* make sure we did not do full circle back to the index of timestamp of this IO */
      if (ind == index) break;

      /* we still need to check that we haven't started filling in this timeslot
       * does not matter in SH/SM where timestamps come in order, but important 
       * when OM filling in the stats that may come out of order */
      if (stat_slots[ind].getTimestamp() != (cur_slot_num * sec_in_slot))
	stat_slots[ind].reset(cur_slot_num * sec_in_slot);

      cur_slot_num--;
    }

    /* update last slot num */
    last_slot_num = slot_num;

    return index;
  }

  void StatHistory::print(std::ofstream& dumpFile, boost::posix_time::ptime curts)
  {
    if (nslots == 0) return;

    lock.read_lock();
    int endix = last_slot_num % nslots;
    int startix = (endix + 1) % nslots; /* index of oldest stat */
    int ix = startix;
    long latest_ts = 0;

    /* we will skip the last timeslot, because it is most likely not filled in */
    while (ix != endix)
      {
	long ts = stat_slots[ix].getTimestamp();
	if (ts > last_printed_ts)
	  {
	    dumpFile << "[" << to_simple_string(curts) << "]," 
		     << id << ","
		     << stat_slots[ix].getTimestamp() << ","
		     << stat_slots[ix].getIops() << ","
		     << stat_slots[ix].getAveLatency() << ","
		     << stat_slots[ix].getMinLatency() << ","
		     << stat_slots[ix].getMaxLatency() << ","
		     << stat_slots[ix].getIops(diskio::diskTier, FDS_IO_READ) << ","
		     << stat_slots[ix].getIops(diskio::diskTier, FDS_IO_WRITE) << ","
		     << stat_slots[ix].getIops(diskio::flashTier, FDS_IO_READ) << ","
		     << stat_slots[ix].getIops(diskio::flashTier, FDS_IO_WRITE) << std::endl;

	    if (latest_ts < ts)
	      latest_ts = ts;

	  }
	ix = (ix + 1) % nslots;
      }

    if (latest_ts != 0)
      last_printed_ts = latest_ts;

    lock.read_unlock();
  }

  unsigned long StatHistory::getAverageIOPS(long rel_end_seconds,
				   int interval_sec)
  {
    unsigned long ret_iops = 0;

    lock.read_lock();
    int endix = last_slot_num % nslots;
    int startix = (endix + 1) % nslots; /* index of oldest stat */
    int ix = startix;

    int slot_count = 0;
    long rel_start_seconds = rel_end_seconds - interval_sec;

    /* we will skip the last timeslot, because it is most likely not filled in */
    while (ix != endix)
      {
	long ts = stat_slots[ix].getTimestamp();
	if ((ts >= rel_start_seconds) && (ts < rel_end_seconds))
	  {
	    ret_iops += stat_slots[ix].getIops();
	    slot_count++;
	  }
	ix = (ix + 1) % nslots;
      }

    /* get the average */
    if (slot_count > 0)
      ret_iops = (unsigned long)( (double)ret_iops/(double)slot_count );

    lock.read_unlock();
    return ret_iops;
  }

void StatHistory::getStats(FDS_ProtocolInterface::FDSP_PerfStatListType* perf_list)
{
    if (nslots == 0) return;

    long latest_ts = 0;
    lock.read_lock();
    int endix = last_slot_num % nslots;
    int startix = (endix + 1) % nslots; /* index of oldest stat */
    int ix = startix;

    /* we will skip the last timeslot, because it is most likely not filled in */
    while (ix != endix)
      {
	long ts = stat_slots[ix].getTimestamp();
	if (ts > last_returned_ts)
	  {
	    {
                /* total */
                FDS_ProtocolInterface::FDSP_PerfStatType tot_stat;
                if (stat_slots[ix].copyIfNotZero(tot_stat, PERF_STAT_TYPES-1)) {
                    perf_list->push_back(tot_stat);
                }
	    }
	    for (int i = 0; i < (PERF_STAT_TYPES-1); ++i)
	      {
                  FDS_ProtocolInterface::FDSP_PerfStatType stat;
                  if (stat_slots[ix].copyIfNotZero(stat, i)) {
                      perf_list->push_back(stat);
                  }
	      }
	    if (latest_ts < ts)
	      latest_ts = ts;

	  }

	ix = (ix + 1) % nslots;
      }

    if (latest_ts != 0)
      last_returned_ts = latest_ts;

    lock.read_unlock();
}


} /* namespace fds*/
