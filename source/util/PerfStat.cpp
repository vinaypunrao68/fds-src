#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

#include <util/PerfStat.h>

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

  long IoStat::getIops() const 
  {
    return (long)((double)stat.nsamples / (double) interval_sec);
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

  StatHistory::StatHistory(fds_uint32_t _id, int slots, int slot_len_sec)
    : id(_id), nslots(slots), sec_in_slot(slot_len_sec), last_printed_ts(-1)
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

    last_slot_num = 0;
  }

  StatHistory::~StatHistory()
  {
    delete [] stat_slots;
  }

  void StatHistory::addIo(long rel_seconds, long microlat)
  {
    long slot_num = rel_seconds / sec_in_slot;
    long slot_start_sec = rel_seconds;
    int index = slot_num % nslots;

    /* common case -- we will add IO to already existing slot */
    lock.write_lock();

    /* check if we already started to fill in this slot */
    if (stat_slots[index].getTimestamp() == slot_start_sec)
      {
	/* the slot indexed with this timestamp already has I/Os of the same time interval */
	stat_slots[index].add(microlat);
        lock.write_unlock();
	return;
      }
     
    if (slot_num <= last_slot_num) {
      /* we need a new slot, but it's it the past, so we got timestamp that is too old */
      /* maybe we should return error, since we should configure long enough history so that does not happen */
      lock.write_unlock();
      return;
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

      stat_slots[ind].reset(cur_slot_num * sec_in_slot);
      cur_slot_num--;
    }

    /* actually add IO stat  */
    stat_slots[index].add(microlat);

    /* update last slot num */
    last_slot_num = slot_num;

    lock.write_unlock();
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

  void StatHistory::print(std::ofstream& dumpFile, boost::posix_time::ptime curts)
  {
    lock.read_lock();
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
	    dumpFile << "[" << to_simple_string(curts) << "]," 
		     << id << ","
		     << stat_slots[ix].getTimestamp() << ","
		     << stat_slots[ix].getIops() << ","
		     << stat_slots[ix].getAveLatency() << ","
		     << stat_slots[ix].getMinLatency() << ","
		     << stat_slots[ix].getMaxLatency() << std::endl;

	    if (latest_ts < ts)
	      latest_ts = ts;

	  }
	ix = (ix + 1) % nslots;
      }

    if (latest_ts != 0)
      last_printed_ts = latest_ts;

    lock.read_unlock();
  }

  /******** PerfStats class implementation *******/

  PerfStats::PerfStats(const std::string prefix, int slot_len_sec)
    : sec_in_slot(slot_len_sec),
      num_slots(FDS_STAT_DEFAULT_HIST_SLOTS),
      statTimer(new IceUtil::Timer),
      statTimerTask(new StatTimerTask(this))
  {
    start_time = boost::posix_time::second_clock::universal_time();

    /* name of file where we dump stats should contain current local time */
    boost::posix_time::ptime now = boost::posix_time::second_clock::local_time();

    std::string dirname("stats");
    std::string nowstr = to_simple_string(now);
    std::size_t i = nowstr.find_first_of(" ");
    std::size_t k = nowstr.find_first_of(".");
    std::string ts_str("");
    if (i != std::string::npos) {
      ts_str = nowstr.substr(0, i);
      if (k == std::string::npos) {
	ts_str.append("-");
	ts_str.append(nowstr.substr(i+1));
      }	     	    
    }

    /*make sure stats directory exists */
    if ( !boost::filesystem::exists(dirname.c_str()) )
      {
	boost::filesystem::create_directory(dirname.c_str());
      }

    /* use timestamp in the filename */
    std::string fname =dirname + std::string("//") + prefix + std::string("_") + ts_str + std::string(".stat");
    statfile.open(fname.c_str(), std::ios::out | std::ios::app );

    /* we will init histories when we see new class_ids (in recordIo() method) */

    /* for now, stats are disbled by default */
    b_enabled = ATOMIC_VAR_INIT(false);
  }

  PerfStats::~PerfStats()
  {
    statTimer->destroy();
    for (std::unordered_map<fds_uint32_t, StatHistory*>::iterator it = histmap.begin();
	 it != histmap.end();
	 ++it)
      {
	StatHistory* hist = it->second;
	delete hist;
      }
    histmap.clear();

    if (statfile.is_open()){
      statfile.close();
    }
  }

  Error PerfStats::enable()
  {
    Error err(ERR_OK);
    bool was_enabled = atomic_exchange(&b_enabled, true);
    if (!was_enabled) {
      IceUtil::Time interval = IceUtil::Time::seconds(sec_in_slot * (num_slots - 2));
      try {
	statTimer->scheduleRepeated(statTimerTask, interval);
      } catch (IceUtil::IllegalArgumentException&) {
	/* ok, already dumping stats, ignore this exception */
      } catch (...) {
	err = ERR_MAX; 
      }
    }
    return err;
  }

  void PerfStats::disable()
  {
    bool was_enabled = atomic_exchange(&b_enabled, false);
    if (was_enabled) {
       statTimer->cancel(statTimerTask);
       /* print stats we gathered but have not yet printed */
       print();
    }
  }

  void PerfStats::recordIO(fds_uint32_t class_id, long microlat)
  {
    if ( !isEnabled()) return;

    StatHistory* hist = NULL;
    boost::posix_time::time_duration elapsed = boost::posix_time::microsec_clock::universal_time() - start_time;

    map_rwlock.read_lock();
    if (histmap.count(class_id) > 0) {
      hist = histmap[class_id];
      assert(hist);
    }
    else{
      /* we see this class_id for the first time, create a history for it */
      map_rwlock.read_unlock();
      hist = new StatHistory(class_id, num_slots, sec_in_slot);
      if (!hist) return;

      map_rwlock.write_lock();
      histmap[class_id] = hist;
      map_rwlock.write_unlock();
 
      map_rwlock.read_lock();
    }

    /* stat history should handle its own lock */
    hist->addIo(elapsed.total_seconds(), microlat);

    map_rwlock.read_unlock();
  }

  void PerfStats::print()
  {
    if ( !isEnabled()) return;

    map_rwlock.read_lock();
    boost::posix_time::ptime now_local = boost::posix_time::microsec_clock::local_time();
    for (std::unordered_map<fds_uint32_t, StatHistory*>::iterator it = histmap.begin();
	 it != histmap.end();
	 ++it)
      {
	it->second->print(statfile, now_local);
      }
    map_rwlock.read_unlock();
  }

  /* timer taks to print all perf stats */
  void StatTimerTask::runTimerTask()
  {
     stats->print();
  }

} /* namespace fds*/
