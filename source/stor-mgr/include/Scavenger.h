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

#define SCAV_DEFAULT_PROC_MAX_TOKENS       1
#define SCAV_DEFAULT_DSK_AVAIL_THRESHOLD_1 70   // see DiskScavPolicyInternal
#define SCAV_DEFAULT_DSK_AVAIL_THRESHOLD_2 50   // see DiskScavPolicyInternal
#define SCAV_DEFAULT_TOK_RECLAIM_THRESHOLD 51   // see DiskScavPolicyInternal

  /**
   * Scavenger policy per DiskScavenger
   * This is internal policy to which we translate user-set policy
   */
  class DiskScavPolicyInternal {
  public:
    /**
     * Thresholds for percent (from 0 to 100) of available disk space.
     * Start compaction when actual percent < dsk_avail_threshold_1
     * If actual avail space is in between two thresholds, then we only
     * compact tokens that have percent of reclaimable space vs. total
     * used space > tok_reclaim_threshold. In other words, we only
     * compact tokens that are worth compacting = have enough reclaimable
     * space.
     * If actual avail space < dsk_avail_threshold_2, then we will compact
     * all tokens on this disk
     */
    fds_uint32_t dsk_avail_threshold_1;
    fds_uint32_t dsk_avail_threshold_2;

    /**
     * Threshold for percent (from 0 to 100) of reclaimable space vs.
     * total space occupied by a particular SM token. This threshold is
     * used only if actual percent of available disk space is in between
     * dsk_avail_threshold_1 and dsk_avail_threshold_2
     */
    fds_uint32_t tok_reclaim_threshold;
    /**
     * Maximum number of tokens we process per disk at the same time
     */
    fds_uint32_t proc_max_tokens;

    DiskScavPolicyInternal();  // uses SCAV_DEFAULT_* defaults
    DiskScavPolicyInternal(const DiskScavPolicyInternal& policy);
    DiskScavPolicyInternal& operator=(const DiskScavPolicyInternal& other);
  };

  class DiskScavenger {
   public :
      DiskScavenger(fds_uint16_t _disk_id,
                    diskio::DataTier _tier);
      ~DiskScavenger();

      void startScavenge(fds_uint32_t token_reclaim_threshold=0);
      void stopScavenge();

      /**
       * Setters for scavenger policies
       */
      void setPolicy(const DiskScavPolicyInternal& policy);
      void setProcMaxTokens(fds_uint32_t max_tokens);

      /**
       * @return true if this disk scavenger is for given tier
       */
      inline fds_bool_t isTier(diskio::DataTier _tier) const {
          return _tier == tier;
      }
      inline fds_uint16_t diskId() const {
	return disk_id;
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
       * Only includes those tokens whose percent of reclaimable space is
       * >= token_reclaim_threshold
       */
      void findTokensToCompact(fds_uint32_t token_reclaim_threshold);

    private:
      std::atomic<fds_bool_t> in_progress;  // protects from starting multiple scavenge cycles

      /**
       * non-configurable params (set in constructor
       */
      diskio::DataTier tier;
      fds_uint16_t    disk_id;

      /**
       * configurable disk scavenger policy
       */
      DiskScavPolicyInternal scav_policy;

      std::vector<TokenCompactorPtr> tok_compactor_vec;

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
      fds_token_id next_token;
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
