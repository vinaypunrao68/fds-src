#ifndef UTIL_PERF_STAT_H_
#define UTIL_PERF_STAT_H_

#include <stdio.h>
#include <stdlib.h>
#include <boost/date_time/posix_time/posix_time.hpp>

namespace fds {

#define FDS_STAT_DEFAULT_HIST_SLOTS  10
#define FDS_STAT_DEFAULT_SLOT_LENGTH 1    /* in sec */

  class Stat
  {
  public:
    int nsamples;
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

  void add(long microlat);
  void reset(long ts_sec);

  /* stats accessors */
  long getTimestamp() const;
  int getIos() const;
  double getAveLatency() const;

 private:
  Stat stat;
  long rel_ts_sec; /* timestamp in seconds since the start of stats gathering */
};


/* 
 * Recent history of IO stats, with configurable:
 * number of time slots that hold recent history
 * time slot length in seconds 
 */
class StatHistory
{
 public:
  StatHistory();
  StatHistory(int slots, int slot_len_sec);
  ~StatHistory();

  void addIo(boost::posix_time::ptime timestamp, long microlat);
 
  int getStatsCopy(IoStat** stat_ary, int* len);

 private:
  long ts2slotnum(boost::posix_time::ptime timestamp); 

 private:
  /* array of time slots with stats, once we fill in 
  * the last slot in the array, we will circulate and re-use the 
  * first slot for the next time interval, and so on */
  IoStat* stat_slots;
  int nslots;
  int sec_in_slot;

  boost::posix_time::ptime start_time;
  long last_slot_num;
};


} /* namespace fds */


#endif /* UTIL_PERF_STAT_H_ */
