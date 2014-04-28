/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_SCAVENGER_H_
#define SOURCE_INCLUDE_SCAVENGER_H_

#include <atomic>
#include <unordered_map>
#include <string>

#include <fds_error.h>
#include <fds_types.h>
#include <TokenCompactor.h>

namespace fds { 
  class DiskScavenger {
   public :
      DiskScavenger(fds_uint16_t _disk_id,
                    diskio::DataTier _tier,
                    fds_uint32_t proc_max_tokens);
      ~DiskScavenger();

      fds_uint16_t    disk_id;
      std::string     fs_mount_path;
      fds_uint64_t    dsk_avail_size;
      fds_uint64_t    dsk_tot_size;
      fds_uint32_t    dsk_reclaim_size;  // Size of reclaimable space of the garbage objects

      void startScavenge();
      void stopScavenge();

      /**
       * @return true if this disk scavenger is for given tier
       */
      inline fds_bool_t isTier(diskio::DataTier _tier) const {
          return _tier == tier;
      }

      /**
       * Callback from token compactor that compaction for a token is done
       */
      void compactionDoneCb(fds_uint32_t token_id, const Error& error);

      /**
       * Query and update disk stats (available size ,etc)
       * If GC is in progress, this method is noop
       */
      void updateDiskStats();

    private:
      /**
       * Return token that needs to be compacted next
       * @return true if token is found, false if no more tokens to compact
       */
      fds_bool_t getNextCompactToken(fds_token_id* tok_id);

      /**
       * Re-builds tokenDb with a set of tokens we need to compact
       * TODO(xxx) add param to tell policy to filter tokens (or include
       * all tokens on the tier/disk)
       */
      void findTokensToCompact();

    private:
      std::atomic<fds_bool_t> in_progress;  // protects from starting multiple scavenge cycles
      fds_token_id next_token;
      fds_uint32_t max_tokens_in_proc;      // Number of Disks/Tokens in compaction at a time
      std::vector<TokenCompactorPtr> tok_compactor_vec;
      diskio::DataTier tier;

      /**
       * protects mainly next_token, and tokenDb when we get a next token
       */
      fds_mutex disk_scav_lock;
      /**
       * A set of token ids that scavenger is currently compacting
       * Undefined, if compacting is not in progress
       * If need to know list of tokens that are held by this disk,
       * get it from the persistent layer
       */
      std::set<fds_token_id> tokenDb;
  };

  typedef enum { 
    SCAV_POLICY_DISK_THRESHOLD,
    SCAV_POLICY_TIMER_BASED
  } ScavPolicyType;

  /**
   * disk id to DiscScavenger object map
   */
  typedef std::unordered_map<fds_uint16_t, DiskScavenger *> DiskScavTblType;
  
  class ScavControl { 
   public:
     ScavControl(fds_uint32_t num_thrds);
     ~ScavControl();
    
     /**
      * @param b_all == true, then will scavenge both ssd and hdd
      * @apram if b_all == false, then will scavenge tgt_tier tier
      */
     void startScavengeProcess(fds_bool_t b_all, diskio::DataTier tgt_tier);
     void stopScavengeProcess();

     void startScavenger(fds_uint16_t disk_id);
     void stopScavenger(fds_uint16_t disk_id);

     /**
      * Called on timer to query & update disk/token file state
      */
     void updateDiskStats();

    private:
     ScavPolicyType  scavPolicy;

     fds_uint32_t num_hdd;  // number of hdd devices
     fds_uint32_t num_ssd;  // number of ssd devices

     // disk idx -> DiskScavenger pointer
     // holds disk scavengers for both HDDs and SSDs
     DiskScavTblType diskScavTbl;

     // lock protecting diskScavTbl
     fds_mutex  scav_lock;

     fds_uint32_t  max_disks_compacting;

     /**
      *  Timer to query & update disk/token file stats
      */
     FdsTimerPtr scav_timer;
     FdsTimerTaskPtr scav_timer_task;
  };

  class ScavTimerTask: public FdsTimerTask {
    public:
      ScavControl* scavenger;

      ScavTimerTask(FdsTimer& timer, ScavControl* sc)  //NOLINT
              : FdsTimerTask(timer) {
          scavenger = sc;
      }
      ~ScavTimerTask() {}

      void runTimerTask();
  };

}
#endif  // SOURCE_INCLUDE_SCAVENGER_H_
