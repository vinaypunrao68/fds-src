#ifndef UTIL_PERF_STAT_H_
#define UTIL_PERF_STAT_H_

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <boost/date_time/posix_time/posix_time.hpp>

#include "include/fds_volume.h"

#define FDS_STAT_DEFAULT_HIST_SLOTS    10
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
  int getIops() const;
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
  StatHistory(fds_volid_t volume_id, 
	      int slots=FDS_STAT_DEFAULT_HIST_SLOTS,
	      int slot_len_sec=FDS_STAT_DEFAULT_SLOT_LENGTH);

  ~StatHistory();

  void addIo(boost::posix_time::ptime timestamp, long microlat);
   
  int getStatsCopy(IoStat** stat_ary, int* len);

  /* Each stat is printed in format: 
   * [curts],volid,seconds_since_beginning_of_history,iops,ave_lat,min_lat,max_lat  */
  void print(std::ofstream& dumpFile, boost::posix_time::ptime curts);

 private:
  long ts2slotnum(boost::posix_time::ptime timestamp); 

 private:
  /* volume id for which we are collecting this history */
  fds_volid_t volid;

  /* array of time slots with stats, once we fill in 
  * the last slot in the array, we will circulate and re-use the 
  * first slot for the next time interval, and so on */
  IoStat* stat_slots;
  int nslots;
  int sec_in_slot;

  boost::posix_time::ptime start_time;
  long last_slot_num;

  /* keep track of timestamps we print, so that we
   * don't output same stat more than once */
  long last_printed_ts; 
};



} /* namespace fds */


#endif /* UTIL_PERF_STAT_H_ */
