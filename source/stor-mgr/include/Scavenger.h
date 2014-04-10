/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_SCAVENGER_H_
#define SOURCE_INCLUDE_SCAVENGER_H_
#include <fdsp/FDSP_types.h>
#include <fds_volume.h>
#include <fds_err.h>
#include <fds_types.h>
#include <odb.h>
#include <unistd.h>
#include <assert.h>
#include <iostream>
#include <util/Log.h>
#include <persistent_layer/dm_service.h>
#include <persistent_layer/dm_io.h>
#include <fds_migration.h>
#include <string>
#include <StorMgr.h>
#include <TokenCompactor.h>

#include <fds_qos.h>
#include <qos_ctrl.h>
#include <fds_obj_cache.h>
#include <fds_assert.h>
#include <fds_config.hpp>
#include <fds_stopwatch.h>

#include <utility>
#include <atomic>
#include <unordered_map>
#include <ObjStats.h>
#include <ObjMeta.h>

namespace fds { 
  class DiskScavenger {
   public :
      DiskScavenger(fds_log *log, fds_uint32_t disk_id, fds_uint32_t proc_max_tokens);
      ~DiskScavenger();
  
      fds_log *scav_log;
      fds_uint32_t    disk_idx;
      std::string     fs_mount_path;
      fds_uint64_t    dsk_avail_size;
      fds_uint64_t    dsk_tot_size;
      fds_uint32_t    dsk_reclaim_size;  // Size of reclaimable space of the garbage objects

      fds_token_id    last_token;
      fds_uint32_t    max_tokens_in_proc;     // Number of Disks/Tokens in compaction at a time
      fds_mutex  *disk_scav_mutex;
      std::unordered_map<fds_token_id, TokenCompactor *> tokenCompactorDb;
      void startScavenge();
      void stopScavenge();
  };

  typedef enum { 
    SCAV_POLICY_DISK_THRESHOLD,
    SCAV_POLICY_TIMER_BASED
  } ScavPolicyType;
  
  class ScavControl { 
   public:
     ScavControl(fds_log *log, fds_uint32_t num_thrds);
     ~ScavControl();
     ScavPolicyType  scavPolicy;
     fds_log *scav_log;
     fds_uint32_t num_disks;
     fds_mutex  *scav_mutex;
     std::unordered_map<fds_uint32_t, DiskScavenger *> diskScavTbl;
     fds_uint32_t  max_disks_compacting;
    
     void startScavengeProcess();
     void stopScavengeProcess();
     fds::Error addTokenCompactor(fds_token_id tok_id, fds_uint32_t disk_idx);
     fds::Error deleteTokenCompactor(fds_token_id tok_id, fds_uint32_t disk_idx);
     void startScavenger(fds_uint32_t disk_idx);
     void stopScavenger(fds_uint32_t disk_idx);
  
  };

}
#endif  // SOURCE_INCLUDE_SCAVENGER_H_
