/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

#include <fds_process.h>
#include <PerfStats.h>

namespace fds {

PerfStats::PerfStats(const std::string prefix, int slots, int slot_len_sec)
        : sec_in_slot(slot_len_sec),
          num_slots(slots),
          statTimer(new FdsTimer()),
          statTimerTask(new StatTimerTask(*statTimer, this))
{
    start_time = boost::posix_time::second_clock::universal_time();

    /* name of file where we dump stats should contain current local time */
    boost::posix_time::ptime now = boost::posix_time::second_clock::local_time();

    const FdsRootDir *root = g_fdsprocess->proc_fdsroot();
    std::string dirname(root->dir_fds_var_stats() + "stats-data");
    root->fds_mkdir(dirname.c_str());

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
    std::string fname = dirname + std::string("//") + prefix +
            std::string("_") + ts_str + std::string(".stat");
    statfile.open(fname.c_str(), std::ios::out | std::ios::app);

    /* we will init histories when we see new class_ids (in recordIo() method) */

    /* for now, stats are disbled by default */
    b_enabled = ATOMIC_VAR_INIT(false);

    om_client = NULL;
}

PerfStats::~PerfStats()
{
    statTimer->destroy();
    for (std::unordered_map<fds_volid_t, StatHistory*>::iterator it = histmap.begin();
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
        bool ret = statTimer->scheduleRepeated(statTimerTask, 
                                std::chrono::seconds(sec_in_slot * (num_slots - 2)));
        if (!ret) {
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

/* returns appropriate stat history, or creates one if it does not exist */
StatHistory* PerfStats::getHistoryWithReadLockHeld(fds_volid_t class_id)
{
    StatHistory* hist = NULL;

    std::unordered_map<fds_volid_t, StatHistory*>::iterator iter = histmap.find(class_id);
    if (histmap.end() != iter) {
        hist = iter->second;
        fds_assert(hist);
    } else {
      /* we see this class_id for the first time, create a history for it */
      map_rwlock.read_unlock();
      hist = new(std::nothrow) StatHistory(class_id, num_slots, sec_in_slot);
      if (!hist) return NULL;

      map_rwlock.write_lock();
      histmap[class_id] = hist;
      map_rwlock.write_unlock();

      map_rwlock.read_lock();
    }

    return hist;
}

void PerfStats::recordIO(fds_volid_t     class_id,
                         long             microlat,
                         diskio::DataTier tier,
                         fds_io_op_t      opType) {
    if ( !isEnabled()) return;
    
    StatHistory* hist = NULL;
    boost::posix_time::time_duration elapsed = boost::posix_time::microsec_clock::universal_time() - start_time;

    map_rwlock.read_lock();
    hist = getHistoryWithReadLockHeld(class_id);

    /* stat history should handle its own lock */
    if (hist)
      hist->addIo(elapsed.total_seconds(), microlat, tier, opType);

    map_rwlock.read_unlock();
}

void PerfStats::print()
{
    if ( !isEnabled()) return;

    map_rwlock.read_lock();
    boost::posix_time::ptime now_local = boost::posix_time::microsec_clock::local_time();
    for (std::unordered_map<fds_volid_t, StatHistory*>::iterator it = histmap.begin();
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

    /* For now, slot length of stats should be the same on SH/SM and OM 
     * so if you want to modify slot length, change FDS_STAT_DEFAULT_SLOT_LENGTH
     * Asserting here if sec_in_slot is not that value; 
     * TODO: either synchronize among SHs/ SMs about slot length or somehow 
     * handle different slot lengths on OM      */
    fds_verify(sec_in_slot == FDS_STAT_DEFAULT_SLOT_LENGTH);

    FDS_ProtocolInterface::FDSP_VolPerfHistListType hist_list;
    map_rwlock.read_lock();
    for (std::unordered_map<fds_volid_t, StatHistory*>::iterator it = histmap.begin();
	 it != histmap.end();
	 ++it)
    {
        FDS_ProtocolInterface::FDSP_VolPerfHistType vol_hist;
	vol_hist.vol_uuid = it->first;
	it->second->getStats(&vol_hist.stat_list);
	if ( (vol_hist.stat_list).size() > 0) {
            /* we only want to send stats of non-idle vols */
            hist_list.push_back(vol_hist);
	}
      }
    map_rwlock.read_unlock();

    /* if all vols are idle, we don't want to send stats to OM */
    if (hist_list.size() > 0) {
      om_client->pushPerfstatsToOM(to_iso_string(start_time), sec_in_slot, hist_list);
    }
  }

void PerfStats::setStatFromFdsp(fds_volid_t class_id, 
                                const std::string& start_timestamp,
                                const FDS_ProtocolInterface::FDSP_PerfStatType& stat_msg)
{
    StatHistory* hist = NULL;
    if ( !isEnabled()) return;
    
    /* rel_seconds in 'stat_msg' is relative to 'start_timestamp', so we need to 
     * calculate timestamp in seconds relative to start_time of this history */
    boost::posix_time::ptime remote_start_ts = boost::posix_time::from_iso_string(start_timestamp);
    long loc_rel_sec = stat_msg.rel_seconds;
    if (start_time > remote_start_ts) {
      boost::posix_time::time_duration diff = start_time - remote_start_ts;
      // remote_start_time ... start_time ......... rel_seconds
      loc_rel_sec -= diff.total_seconds();
    }
    else {
      boost::posix_time::time_duration diff = remote_start_ts - start_time;
      // start_time ... remote_start_time ........ rel_seconds
      loc_rel_sec += diff.total_seconds();
    }

    map_rwlock.read_lock();
    hist = getHistoryWithReadLockHeld(class_id);

    /* stat history should handle its own lock */
    if (hist)
      hist->addStat(loc_rel_sec, stat_msg);

    map_rwlock.read_unlock();
  }

long PerfStats::getAverageIOPS(fds_volid_t class_id,
                               const boost::posix_time::ptime end_ts,
                               int interval_sec)
{
    long ret_iops = 0;
 
    if ( !isEnabled()) return ret_iops;
    
    /* if we asked IOPS too early, IOPS is 0 (=not recorded) */
    if (end_ts < start_time) 
      return ret_iops;

    StatHistory* hist = NULL;
    boost::posix_time::time_duration elapsed = end_ts - start_time;

    map_rwlock.read_lock();
    hist = getHistoryWithReadLockHeld(class_id);

    /* stat history should handle its own lock */
    if (hist)
      ret_iops = hist->getAverageIOPS(elapsed.total_seconds(), interval_sec);

    map_rwlock.read_unlock();
    return ret_iops;
  }

/* timer taks to print all perf stats */
void StatTimerTask::runTimerTask()
{
    stats->print();
    stats->pushToOM();
}

}  // namespace fds

