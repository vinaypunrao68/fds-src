/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#ifndef __STOR_HV_VOLS_H_
#define __STOR_HV_VOLS_H_

#include <lib/Ice-3.5.0/cpp/include/Ice/Ice.h>
#include <lib/Ice-3.5.0/cpp/include/IceUtil/IceUtil.h>
#include <boost/thread/thread.hpp>
#include <boost/lockfree/queue.hpp>
#include <iostream>
#include <boost/atomic.hpp>
#include "fds-include/ThreadPool.h"

#include <unordered_map>

#include "fdsp/FDSP.h"
#include "include/fds_err.h"
#include "include/fds_types.h"
#include "include/fds_volume.h"
#include "util/Log.h"
#include "util/PerfStat.h"
#include "util/concurrency/RwLock.h"
#include "VolumeCatalogCache.h"
#include "StorHvJournal.h"


/* defaults */
#define FDS_DEFAULT_VOL_UUID 1


/*
 * Forward declaration of SH control class
 */
class StorHvCtrl;

namespace fds {

class StorHvVolume : public FDS_Volume
{
public:
  StorHvVolume(const VolumeDesc& vdesc, StorHvCtrl *sh_ctrl, fds_log *parent_log);
  ~StorHvVolume();

  /* safe destruction, after this, volume object is not valid */
  void destroy();
  
  bool isValidLocked() const;

  /* read locks. Journal table and volume catalog cache have their own locks, 
   * so exposing read lock to protect against volume being destroyed 
   */
  void readLock();
  void readUnlock();

public: /* data*/

  StorHvJournal *journal_tbl;
  VolumeCatalogCache *vol_catalog_cache;
  int blkdev_minor;
  /* Reference to parent SH instance */
  StorHvCtrl *parent_sh;

  /* Volume's perf stat history */
  StatHistory *stat_history;
 /*
   * per volume queue
   */
  boost::lockfree::queue<fbd_request_t*>  *volQueue;

private: /* data */

  /* lock to prevent volume destruction while accessing volume data */
  fds_rwlock rwlock; 
  bool is_valid;
};

class StorHvVolumeLock 
{
 public:
  /* Ok if vol == NULL, will not do anything */
  StorHvVolumeLock(StorHvVolume *vol);
  ~StorHvVolumeLock();
    
 private:
  StorHvVolume *shvol;
};

class StorHvVolumeTable
{
 public:
  /* A logger is created if not passed in */
  StorHvVolumeTable(StorHvCtrl *sh_ctrl);
  /* Use logger that passed in to the constructor */
  StorHvVolumeTable(StorHvCtrl *sh_ctrl, fds_log *parent_log);
  ~StorHvVolumeTable();

  Error registerVolume(const VolumeDesc& vdesc); 
  Error removeVolume(fds_volid_t vol_uuid);

  /* 
   * Returns the locked volume object. Guarantees that the
   * returned volume object is valid (i.e., can safely access
   * journal table and volume catalog cache) 
   * Must call StorHvVolume::readUnlock on returned volume object 
   * Returns NULL if volume does not exist
   */
  StorHvVolume* getLockedVolume(fds_volid_t vol_uuid);

  /* 
   * Returns volume but not thread-safe 
   * Use StorHvVolumeLock to lock the volume and check if volume
   * object is still valid via StorHvVolume::isValidLocked() 
   * before using the volume object 
   * Returns NULL is volume does not exist
   */
  StorHvVolume* getVolume(fds_volid_t vol_uuid);

  fds_threadpool  *tp;
  /* per volume queue scheduler */
  void schedulePerVolIO();
  void killMainThread();

  /* Dumping per-volume performance stats */
  void startPerfStats();
  void stopPerfStats();
  void printPerfStats(std::ofstream& dumpFile);

 private: /* methods */ 

  /* handler for volume-related control message from OM */
  static void volumeEventHandler(fds_volid_t vol_uuid, 
                                 VolumeDesc *vdb,
                                 fds_vol_notify_t vol_action);
 
  /* print volume map, other detailed state to log */
  void dump();
    
 private: /* data */

  /* volume uuid -> StorHvVolume map */
  std::unordered_map<fds_volid_t, StorHvVolume*> volume_map;   

  /* Protects volume_map */
  fds_rwlock map_rwlock;
  
  /* Reference to parent SH instance */
  StorHvCtrl *parent_sh;

  /*
   * Pointer to logger to use 
   */
  fds_log *vt_log;
  fds_bool_t created_log;

  /* timer to dump perf stats  */
  IceUtil::TimerPtr statTimer;
  IceUtil::TimerTaskPtr statTimerTask;  
};

 using namespace IceUtil;
 class StorHvStatTimerTask : public IceUtil::TimerTask {
 public:
   StorHvVolumeTable* volTable;
   std::ofstream statfile;

   StorHvStatTimerTask(StorHvVolumeTable* voltab);
   ~StorHvStatTimerTask() {
     if (statfile.is_open()) {
       statfile.close();
     }
   }

   void runTimerTask();
 };

void scheduleIO(StorHvVolumeTable *tPtr);
} // namespace fds

#endif // __STOR_HV_VOLS_H_
