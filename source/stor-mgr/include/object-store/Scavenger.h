/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#ifndef SOURCE_STOR_MGR_INCLUDE_OBJECT_STORE_SCAVENGER_H_
#define SOURCE_STOR_MGR_INCLUDE_OBJECT_STORE_SCAVENGER_H_

#include <atomic>
#include <set>
#include <list>
#include <string>
#include <unordered_map>
#include <vector>
#include <functional>

#include <fds_module.h>
#include "fds_error.h"
#include "fds_types.h"
#include <persistent-layer/dm_io.h>
#include <object-store/SmDiskMap.h>
#include <object-store/TokenCompactor.h>

namespace fds {

// forward declaration
class SmPersistStoreHandler;

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

    friend std::ostream& operator<< (std::ostream &out,
                                     const DiskScavPolicyInternal& policy);
};

/**
 * Callback type to notify that compaction process is finished for this token
 */
typedef std::function<void (fds_uint16_t diskId,
                            const Error& error)> disk_compaction_done_handler_t;

class DiskScavenger {
  public :
    DiskScavenger(fds_uint16_t _disk_id,
                  diskio::DataTier _tier,
                  SmIoReqHandler *data_store,
                  SmPersistStoreHandler* persist_store,
                  const SmDiskMap::const_ptr& diskMap,
                  fds_bool_t noPersistStateScavStats);
    ~DiskScavenger();

    enum ScavState {
        SCAV_STATE_IDLE,     // not doing any compaction
        SCAV_STATE_INPROG,   // compaction in progress
        SCAV_STATE_STOPPING  // stop was called, finishing compaction
    };

    Error startScavenge(fds_bool_t verify,
                        disk_compaction_done_handler_t done_hdlr,
                        fds_uint32_t token_reclaim_threshold = 0);
    void stopScavenge();

    /**
     * Setters for scavenger policies
     */
    void setPolicy(const DiskScavPolicyInternal& policy);
    void setProcMaxTokens(fds_uint32_t max_tokens);

    /**
     * Returns current scavenger policy
     */
    DiskScavPolicyInternal getPolicy() const;

    /**
     * Returns number of tokens that are in the process
     * of being compacted, and number of tokens that this
     * disk scavenger already compacted
     * If scavenger is not in progress, returns 0 for both
     * values
     */
    void getProgress(fds_uint32_t *toksCompacting,
                     fds_uint32_t *toksFinished);

    ScavState getState() const {
        return atomic_load(&state);
    }

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
    fds_bool_t updateDiskStats(fds_bool_t verify_data,
                               disk_compaction_done_handler_t done_hdlr);

    fds_bool_t isTokenCompacted(const fds_token_id& tok_id);

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

    Error getDiskStats(diskio::DiskStat* retStat);

  private:
    // state of compaction progress
    std::atomic<ScavState> state;
    fds_bool_t noPersistScavStats;

    /**
     * non-configurable params (set in constructor)
     */
    diskio::DataTier tier;
    fds_uint16_t    disk_id;

    // TODO(Anna) we are piggybacking data verification here
    // we should consider making it a more generic maintenance
    // taks, or abstract the generic part into a base class
    std::atomic<fds_bool_t> verifyData;

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

    /**
     * callback for disk compaction done method which is set every time
     * startScavenge() is called. When state is SCAV_STATE_IDLE, this cb
     * is undefined.
     */
    disk_compaction_done_handler_t done_evt_handler;

    // keeps copy of SM disk map
    SmDiskMap::const_ptr smDiskMap;
    SmPersistStoreHandler* persistStoreGcHandler;
};

typedef enum {
    SCAV_POLICY_DISK_THRESHOLD,
    SCAV_POLICY_TIMER_BASED
} ScavPolicyType;

/**
 * disk id to DiscScavenger object map
 */
typedef std::unordered_map<fds_uint16_t, DiskScavenger *> DiskScavTblType;

class ScavControl : public Module {
  public:
    explicit ScavControl(const std::string &modName,
                         SmIoReqHandler *data_store,
                         SmPersistStoreHandler *persist_store);
    ~ScavControl();

    typedef std::unique_ptr<ScavControl> unique_ptr;
    typedef std::shared_ptr<ScavControl> ptr;
    typedef std::shared_ptr<const ScavControl> const_ptr;

    /**
     * Enable means that we start running automatic scavenging
     * for disks that are in a given disk map
     * Background task cannot enable scavenger if it was disabled
     * by the user.
     * TODO(Anna) allow enable scavenger only when all tasks that
     * needed to disable it, enable it again
     */
    Error enableScavenger(const SmDiskMap::const_ptr& disk_map,
                          SmCommandInitiator initiator);

    /**
     * Stop running automatic scavenging and do not allow to
     * start scavenger process until enableScavenger() is called
     * @param initiator who disables scavenger (user or background task)
     */
    void disableScavenger(SmCommandInitiator initiator);

    /**
    * Set whether the scrubber will run with scavenger or not
    */
    void setDataVerify(fds_bool_t verify) {
        verifyData = verify;
    }

    /**
    * Get status of scrubber (enabled=true, disabled=false)
    */
    void getDataVerify(const fpi::CtrlQueryScrubberStatusRespPtr& statusResp);

    /**
     * Start scavenging
     */
    void startScavengeProcess();
    /**
     * Stop scavenging if it is in progress
     */
    void stopScavengeProcess();

    /**
     * Changes scavenger policy for all existing disks
     * TODO(Anna) we may want to set policy on a particular disk
     */
    Error setScavengerPolicy(fds_uint32_t dsk_avail_threshold_1,
                             fds_uint32_t dsk_avail_threshold_2,
                             fds_uint32_t tok_reclaim_threshold,
                             fds_uint32_t proc_max_tokens);
    /**
     * Query current scavenger policy. Since for now all disks
     * have the same scavenger policy, we return the policy that is
     * set on all disks.
     * TODO(Anna) may want to get policy for a particular disk
     */
    Error getScavengerPolicy(const fpi::CtrlQueryScavengerPolicyRespPtr& policyResp);

    /**
     * Status getters
     */
    fds_bool_t isEnabled() const {
        return atomic_load(&enabled);
    }
    void getScavengerStatus(const fpi::CtrlQueryScavengerStatusRespPtr& statusResp);

    /**
     * Returns progress in terms of percent. Returns 100 if
     * scavenger is not running
     */
    fds_uint32_t getProgress();

    /**
     * Tells scavenger that SM came up from persistent state
     * Since we don't persist some stats needed by scavenger like
     * deleted bytes, reclaimable bytes, this tells scavenger to get the
     * stats (later during next scavenger cycle) from the object DB
     */
    void setPersistState();

    /**
     * Called on timer to query & update disk/token file state
     */
    void updateDiskStats();

    /**
     * Callback from DiskScavenger that compaction for a disk is done
     */
    void diskCompactionDoneCb(fds_uint16_t diskId, const Error& error);

    // FDS module control functions
    int  mod_init(SysParams const *const param);
    void mod_startup();
    void mod_shutdown();

  private:  // methods
    Error createDiskScavengers(const SmDiskMap::const_ptr& diskMap);
    Error getDiskStats(diskio::DiskStat* retStat);

  private:
    std::atomic<fds_bool_t> enabled;
    SmCommandInitiator whoDisabledMe;
    fds_bool_t noPersistScavStats;
    std::atomic<fds_bool_t> verifyData;

    ScavPolicyType  scavPolicy;

    // disk idx -> DiskScavenger pointer
    // holds disk scavengers for both HDDs and SSDs
    DiskScavTblType diskScavTbl;

    // lock protecting diskScavTbl
    fds_mutex  scav_lock;

    /// configurable parameters
    fds_uint32_t  max_disks_compacting;
    fds_uint32_t intervalSeconds;

    /// to enable compacting configurable number of disks
    /// at a time
    fds_uint16_t nextDiskToCompact;

    /**
     *  Timer to query & update disk/token file stats
     */
    FdsTimerPtr scav_timer;
    FdsTimerTaskPtr scav_timer_task;

    // keeps pointer to pass to disk scavengers -> token compactors
    // so they can enqueue messages to qos queues
    SmIoReqHandler *dataStoreReqHandler;
    SmPersistStoreHandler* persistStoreGcHandler;
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

}  // namespace fds
#endif  // SOURCE_STOR_MGR_INCLUDE_OBJECT_STORE_SCAVENGER_H_
