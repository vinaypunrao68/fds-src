#ifndef UTIL_PERF_STAT_H_
#define UTIL_PERF_STAT_H_

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <unordered_map>
#include <fstream>
#include <atomic>
#include <IceUtil/IceUtil.h>
#include <boost/date_time/posix_time/posix_time.hpp>

#include "fds_volume.h"
#include "concurrency/RwLock.h"

#define FDS_STAT_DEFAULT_HIST_SLOTS    7
#define FDS_STAT_DEFAULT_SLOT_LENGTH   1    /* in sec */
#define FDS_STAT_MAX_HIST_SLOTS        50

namespace fds {

  class Stat
  {
  public:
    long nsamples;
    long min_lat;
    long max_lat;
    double ave_lat;

    Stat();
    ~Stat();

    void reset();

    void add(long lat);
    void add(const Stat& stat);
  };


  /* Wrapper to Stat class so that we will be able
   * to differentiate stats for reads/writes, possibly different 
   * I/O sizes, etc.
   * For now, not differentiating IOs */
class IoStat 
{
 public:
  IoStat();
  ~IoStat();

  /* is stat_size_sec==0, do not change time interval */
  void reset(long ts_sec, int stat_size_sec=0);
  void add(long microlat);

  /* stats accessors */
  long getTimestamp() const;
  long getIops() const;
  long getAveLatency() const;
  long getMinLatency() const;
  long getMaxLatency() const;

 private:
  Stat stat;
  long rel_ts_sec; /* timestamp in seconds since the start of stats gathering */
  int interval_sec; /* time interval of this stat */
};


/* 
 * Recent history of IO stats, with configurable:
 * number of time slots that hold recent history
 * time slot length in seconds 
 */
class StatHistory
{
 public:
  StatHistory(fds_uint32_t _id, 
	      int slots=FDS_STAT_DEFAULT_HIST_SLOTS,
	      int slot_len_sec=FDS_STAT_DEFAULT_SLOT_LENGTH);

  ~StatHistory();

  void addIo(long rel_seconds, long microlat);
   
  int getStatsCopy(IoStat** stat_ary, int* len);

  /* Each stat is printed in format: 
   * [curts],volid,seconds_since_beginning_of_history,iops,ave_lat,min_lat,max_lat  */
  void print(std::ofstream& dumpFile, boost::posix_time::ptime curts);

 private:
  /* identifyer of a IO stream/volume/qos class/etc for which we are collecting this history */
  fds_uint32_t id;

  /* array of time slots with stats, once we fill in 
  * the last slot in the array, we will circulate and re-use the 
  * first slot for the next time interval, and so on */
  IoStat* stat_slots;
  int nslots;
  int sec_in_slot;

  long last_slot_num;

  /* lock protecting this history */
  fds_rwlock lock;

  /* keep track of timestamps we print, so that we
   * don't output same stat more than once */
  long last_printed_ts; 
};


/* Handles all recording and printing of stats.
 * Keeps separate histories of stats perclass_id
 * we will normally use volume id as a statclass_id 
 * but it could be anything else. The stats will be 
 * printed into one file, with second column = statclass_id
 * Configurable params:
 *    stat slot length in seconds 
 * */
class PerfStats
{
 public: 
  PerfStats(const std::string prefix, int slot_len_sec = FDS_STAT_DEFAULT_SLOT_LENGTH);
  ~PerfStats();

  /* explicitly enable or desable stats; disabled stats mean that recordIO
   * and printing functions are noop. Once stats are enabled again, we will
   * continue to print stats to the same output file (so file will have a gap
   * in stats when they were disabled  */
  Error enable();
  void disable(); 
  inline bool isEnabled() const {
    return std::atomic_load(&b_enabled); 
  }

  /* Record IO in appropriate stats history. If we see statclass_id for the first time
   * will start a stat history for this statclass_id  */
  void recordIO(fds_uint32_t class_id, long microlat);

  /* print stats to file */
  void print();

 private:
  std::atomic<bool> b_enabled;

  /* number of seconds in one stat slot, configurable */
  int sec_in_slot;
  int num_slots;

  /* class_id to stat history map */
  std::unordered_map<fds_uint32_t, StatHistory*> histmap;
  /* read/write lock protecting histmap */
  fds_rwlock map_rwlock;

  boost::posix_time::ptime start_time;
  std::ofstream statfile;

  IceUtil::TimerPtr statTimer;
  IceUtil::TimerTaskPtr statTimerTask;
};


 using namespace IceUtil;
 class StatTimerTask:public IceUtil::TimerTask {
 public:
   PerfStats* stats;

   StatTimerTask(PerfStats* _stats) {
     stats = _stats;
   };
   ~StatTimerTask() {}

   void runTimerTask();
 };


} /* namespace fds */


#endif /* UTIL_PERF_STAT_H_ */
