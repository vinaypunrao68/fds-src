#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

#include "PerfStats.h"

namespace fds {

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

    om_client = NULL;
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

  void PerfStats::registerOmClient(OMgrClient* _om_client)
  {
    om_client = _om_client;
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

  void PerfStats::recordIO(fds_uint32_t     class_id,
                           long             microlat,
                           diskio::DataTier tier,
                           fds_io_op_t      opType) {
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
    hist->addIo(elapsed.total_seconds(), microlat, tier, opType);

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

  void PerfStats::pushToOM()
  {
    /* check if we registered OM client */
    if (!om_client) return;

    /* we are only pushing stats if they are enabled */
    if ( !isEnabled()) return;

    om_client->pushPerfstatsToOM();
  }

  /* timer taks to print all perf stats */
  void StatTimerTask::runTimerTask()
  {
     stats->print();
     stats->pushToOM();
  }

} /* namespace fds*/

