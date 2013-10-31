/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#ifndef STORMGR_INCLUDE_TIER_POLICY_H_
#define STORMGR_INCLUDE_TIER_POLICY_H_

#include <cpplist.h>
#include <fds_types.h>
#include <fdsp/orch_proto.h>
#include <concurrency/Mutex.h>
#include <concurrency/ThreadPool.h>
#include <net-proxies/vol_policy.h>

/*
 * --------------------------------------------
 * Tier Policy Executing and Auditing Framework
 * --------------------------------------------
 *
 * Theory of Operation
 *     The tier policy accepts policy settings, executes these policies and
 * audits them.  The following are main elements of the frame work and their
 * roles:
 * 1) TierTimeSpec: time specification for a policy to be effective.  Its
 * main design goals are:
 *    - Shareable among policy settings.  One time spec can be used by
 *      many policies.
 *    - Have unique string identifier (e.g. weekly-9to5, quarterly-reports...)
 *    - Hookup with the main scheduler to trigger corresponding actions when
 *      the current time meets its specification.
 * 2) TierPolAction: pure virtual interface for this framework to drive actions
 * to execute the policy.  Its main design goals are:
 *    - One for each volume/bucket.
 *    - Implement tier actions so that the framework and drive the tier policy
 *      according to TierTimeSpec schedule.  These actions are:
 *      o Choose media types (can have multiple tiers) and percentage of volume
 *        capacity used for each media.
 *      o Action to warm up data to the tier within the given duration and
 *        priority.
 *      o Action to activate the given tier with percentage of volume capacity.
 *      o Action to adjust current tier space consumption.
 *      o Action to cool down the tier, live in dormal state.
 *      o Action to deactive the tier policy completely.
 *      o Action to accept a generic TierStat object to enable the audit module
 *        to audit the effectiveness of the tier policy.  The derrived class
 *        must hookup with the given TierStat object in its data path to
 *        collect the stat for auditing.
 * 3) TierPolFuncSpec: the manager of TierTimeSpec and TierPolAction classes
 * to coordiate the schedule within the constrain of tier resources.  Its main
 * design goals are:
 *    - Stitch TierPolActions to a TierTimeSpec to carry on the policy at the
 *      right time at the current supply of tier resources.
 *    - Enable other components to alter tier policies to adapt changes to the
 *      current supply of tier resources.
 *    - Enable other component to pause/resume tier policy for a volume.
 *    - Remove current policy with a volume.
 * 4) TierPolicyExe: the supreme boss of TierPolFuncSpec.  Its main desgin
 * goals are:
 *    - Support multi-tenancy within a box if required.
 *    - Control the supply side of tier resources to each TierPolFuncSpec.
 *    - Work with the tier audit module to evaluate the effectiveness of
 *      different tier policies.
 * 5) TierStat/TierPolAudit: collect tier stats analyze these stats to audit
 * tier policies at system level.  Their desgin goals are:
 *    - Very low overhead to capture the histogram of IOPs and latency during
 *      the duration of tier policy.
 *    - Audit all policy settings and compare their effectiveness.
 */
namespace fds {

typedef orch_ProtocolInterface::tier_media_type_e fds_tier_media_e;
class TierPolAction;
class TierPolicyAudit;
class TierPolFuncSpec;

// TierStat
// --------
//
class TierStat
{
  public:
    TierStat(fds_uint64_t vol_uuid);
    ~TierStat();

    void tier_stat_rec_iop(fds_tier_type_e media, bool rd);
    void tier_stat_rec_latency(fds_tier_type_e media, fds_uint32_t microsec);

    void tier_stat_report(char const *const file);
  protected:
    friend class TierPolicyAudit;

    void tier_stat_begin();
    void tier_stat_end();
};

// TierTimeSpec
// ------------
//
class TierTimeSpec
{
  public:
    TierTimeSpec(char const *const name);
    ~TierTimeSpec();

    // XXX: simple API for now.
    // Return true if the current time matches with the timespec and the effective
    // duration in unit of seconds.
    //
    bool tier_time_effect(fds_uint32_t &duration_sec);

  protected:
    // XXX: simple start/duration(stop), no periodic/range of various sec/min/
    // hour/day/week/month/quarter/year...
    //
    fds_uint32_t             tier_duration_sec;
    char const *const        tier_tspec_name;
};

// TierPolAction
// -------------
// Define pure virtual class to enable plugins to carry on tier policy actions.
//
// Control path:
// warm_up -> activate -> adjust_consumption -> cool_down -> deactivate
//     ^               ^                     |      |
//     |               +---------------------+      |
//     +--------------------------------------------+
// Note: the different between "cool_down" and "deactivate" is the framework
// still collects stats for auditing.
//
// Admin path:
// - Accept TierStat from the audit module.
// - Accept thread pool to schedule threads and thread quota...
// - Given information about tier media and percentage of capacity that it
//   can use.
//
class TierPolAction
{
  public:
    TierPolAction(fds_uint64_t vol_uuid);
    ~TierPolAction();

    // Control path actions.
    //
    virtual void
    tier_warm_up(fds_uint32_t duration_sec, int tier_pct, int priority) = 0;

    virtual void tier_activate(fds_tier_type_e media, int tier_pct) = 0;
    virtual void tier_adjust_consumption(fds_tier_type_e media, int tier_pct) = 0;
    virtual void tier_cool_down(fds_uint32_t duration_sec) = 0;
    virtual void tier_deactivate() = 0;

    // Admin path functions.
    //
    TierStat &tier_get_stat() const { return *tier_stat; }
    void tier_assign_stat(TierStat *stat) { tier_stat = stat; }
    void tier_assign_thrpool(fds_threadpool *pool) { tier_thrpool = pool; }

  protected:
    TierStat                  *tier_stat;
    TierTimeSpec const        *tier_time_spec;
    fds_threadpool            *tier_thrpool;

    fds_uint64_t              tier_vol_uuid;
    fds_uint32_t              tier_cur_duration;
    fds_tier_type_e           tier_cur_media[TIER_MAX_MEDIA_TYPES];
    int                       tier_med_pct_consume[TIER_MAX_MEDIA_TYPES];
};

// TierPolFuncSpec
// ---------------
//
class TierPolFuncSpec
{
  public:
    TierPolFuncSpec(char const *const fn_name, fds_threadpool *pool);
    ~TierPolFuncSpec();

    // Given a timespec and action, carry on the tier policy schedule at the
    // right time.
    //
    virtual void
    tier_sched_spec(TierTimeSpec const &spec, TierPolAction &action);

    // Alter the tiering schedule for the given volume.
    //
    virtual void
    tier_sched_alter(TierTimeSpec const &spec, fds_uint64_t vol_uuid);

    // Pause/resume the current tier policy for the given volume.
    //
    virtual void tier_sched_pause(fds_uint64_t vol_uuid);
    virtual void tier_sched_resume(fds_uint64_t vol_uuid);

    // Remove current tier policy out of the given volume uuid and return
    // back the original TierPolAction to the caller.
    //
    virtual TierPolAction *
    tier_sched_remove(fds_uint64_t vol_uuid);

  protected:
    friend class TierPolicyAudit;

    char const *const        tier_fn_name;
    ChainList                tier_time_spec;
    ChainList                tier_act_policy;
    ChainList                tier_stat;
    fds_threadpool           *tier_pool;
    fds_mutex                tier_fn_mtx;

    void tier_sched_run_jobs();
};

// TierPolicyExe
// -------------
// TBD
//
class TierPolicyExe
{
  public:
    TierPolicyExe() {}
    ~TierPolicyExe() {}

  protected:
};

// TierPolicyAudit
// ---------------
//
class TierPolicyAudit
{
  public:
    TierPolicyAudit() {}
    ~TierPolicyAudit() {}

  private:
};

} // namespace fds

#endif /* STORMGR_INCLUDE_TIER_POLICY_H_ */
