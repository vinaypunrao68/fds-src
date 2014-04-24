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

    private:
      /**
       * Return token that needs to be compacted next
       * @return true if token is found, false if no more tokens to compact
       */
      fds_bool_t getNextCompactToken(fds_token_id* tok_id);

      /**
       * Resets tokenDB with the current set of tokens assigned to this disk_id
       * The tokens are retrived from persistence layer
       */
      void updateTokenDb();

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
      std::set<fds_token_id> tokenDb;  // tokens held by this disk
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
  };

}
#endif  // SOURCE_INCLUDE_SCAVENGER_H_
