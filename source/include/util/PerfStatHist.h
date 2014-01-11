#ifndef UTIL_PERF_STAT_H_
#define UTIL_PERF_STAT_H_

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <unordered_map>
#include <fstream>
#include <atomic>
#include <boost/date_time/posix_time/posix_time.hpp>

#include "fds_volume.h"
#include "concurrency/RwLock.h"

#include <persistent_layer/dm_io.h>

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
#define PERF_STAT_TYPES 6
class IoStat 
{
 public:
  IoStat();
  ~IoStat();

  /* is stat_size_sec==0, do not change time interval */
  void reset(long ts_sec, int stat_size_sec=0);
  void add(long microlat,
	   diskio::DataTier tier = diskio::maxTier,  /* Defaults to invalid tier */
	   fds_io_op_t opType = FDS_OP_INVALID);     /* Defaults to invalid op */

  /* stats accessors */
  long getTimestamp() const;
  long getIops(diskio::DataTier tier, fds_io_op_t opType) const;
  long getAveLatency(diskio::DataTier tier, fds_io_op_t opType) const;

  /* these are total for all op types */
  long getIops() const;
  long getAveLatency() const;
  long getMinLatency() const;
  long getMaxLatency() const;

  /* copy stat to fdsp struct if the stat contains at least one sample / IO */
  bool copyIfNotZero(FDS_ProtocolInterface::FDSP_PerfStatType& fdsp_stat,
                     int stat_type);

  /* add stats to 'stat' of appropriate type from fdsp msg */
  void addFromFdspMsg(const FDS_ProtocolInterface::FDSP_PerfStatType& fdsp_stat);

 private: /* methods */
  inline int statIndex(diskio::DataTier tier, fds_io_op_t opType) const {
    if ((tier == diskio::diskTier) && (opType == FDS_IO_READ))
      return 0;
    if ((tier == diskio::diskTier) && (opType == FDS_IO_WRITE))
      return 1;
    if ((tier == diskio::flashTier) && (opType == FDS_IO_READ))
      return 2;
    if ((tier == diskio::flashTier) && (opType == FDS_IO_WRITE))
      return 3;
    return 4;
  }

 private:
  Stat stat[PERF_STAT_TYPES]; /* read/disk, write/disk, read/flash, write/flash, other, total */
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

  void addIo(long rel_seconds, 
	     long microlat,
	     diskio::DataTier tier = diskio::maxTier,  /* Defaults to invalid tier */
	     fds_io_op_t opType = FDS_OP_INVALID);     /* Defaults to invalid op */
   

  /* used to send stats history to OM. To save amount of data we send over to OM
   * we remember last stats getStats returns and start filling in the history 
   * from tha last timestamp. This means that calling getStats back to back will 
   * result in first call to getStats returning stat history and second call to 
   * getStats will not return anything (if called right away), because new time slot
   * did not fill yet. Thus it works similarly to 'print' method, where we get perf 
   * of a particular time slot only once.  */
  void getStats(FDS_ProtocolInterface::FDSP_PerfStatListType* perf_list);
  void addStat(long rel_seconds,
               const FDS_ProtocolInterface::FDSP_PerfStatType& fdsp_stat);

  /* get average IOPS for interval [rel_end_seconds - interval_sec, rel_end_seconds) */
  long getAverageIOPS(long rel_end_seconds, 
		      int interval_sec);


  /* Each stat is printed in format: 
   * [curts],volid,seconds_since_beginning_of_history,iops,ave_lat,min_lat,max_lat  */
  void print(std::ofstream& dumpFile, boost::posix_time::ptime curts);

 private: /* methods */
  int prepareSlotWithWriteLockHeld(long rel_seconds); 

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

  /* keep track of timestamps we return with 'getStats', so 
   * that we don't return same stat more than once; We could
   * re-use 'last_printed_ts', but that would mean interleaving 
   * code in 'print' and 'getStats', so to separate these two 
   * functionalities we keeping track fo those timestamps separately  */
  long last_returned_ts;
};


} /* namespace fds */


#endif /* UTIL_PERF_STAT_H_ */
