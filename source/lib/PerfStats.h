/*
 * Copyright 2013 Formation Data Systems, Inc.
 *
 */
#ifndef LIB_PERF_STATS_H_
#define LIB_PERF_STATS_H_

#include <IceUtil/IceUtil.h>

#include <util/PerfStatHist.h>
#include <lib/OMgrClient.h>

namespace fds {

/* Handles all recording and printing of stats.
 * Keeps separate histories of stats per class_id.
 * We will normally use volume id as a statclass_id 
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
  void recordIO(fds_uint32_t class_id,
                long microlat,
                diskio::DataTier tier = diskio::maxTier,  /* Defaults to invalid tier */
                fds_io_op_t opType = FDS_OP_INVALID);     /* Defaults to invalid op */

  /* print stats to file */
  void print();

  /* for sending stats to OM */
  void registerOmClient(OMgrClient* _om_client);

  /* called on timer event to send stats to OM */
  void pushToOM();

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

  /* does not own, get passed via registerOmClient */
  OMgrClient* om_client;
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


#endif /* LIB_PERF_STATS_H_ */
