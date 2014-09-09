/*
 * Copyright 2013-2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_STOR_MGR_INCLUDE_POLICY_TIER_H_
#define SOURCE_STOR_MGR_INCLUDE_POLICY_TIER_H_

#include <boost/atomic.hpp>
#include <boost/intrusive_ptr.hpp>
#include <boost/unordered_map.hpp>

#include "cpplist.h"
#include "fds_types.h"
#include "fds_module.h"
#include "fdsp/FDSP_types.h"
#include "concurrency/Mutex.h"
#include "concurrency/ThreadPool.h"
#include "net-proxies/vol_policy.h"

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

class TierPolAction;
class TierPolicyAudit;
class TierPolFuncSpec;
class TierPolicyModule;

// TierStat
// --------
//
class TierStat {
    public:
     explicit TierStat(fds_uint64_t vol_uuid);
     ~TierStat();

     void tier_stat_rec_iop(FDS_ProtocolInterface::tier_media_type_e media, bool rd);
     void tier_stat_rec_latency(FDS_ProtocolInterface::tier_media_type_e media,
                                fds_uint32_t microsec);
     void tier_stat_report(char const *const file);

    protected:
     friend class TierPolicyAudit;

     void tier_stat_begin();
     void tier_stat_end();
};

// TierTimeSpec
// ------------
//
class TierTimeSpec {
    public:
     typedef boost::intrusive_ptr<TierTimeSpec> pointer;
     explicit TierTimeSpec(char const *const name);
     ~TierTimeSpec();

     // XXX: simple API for now.
     // Return true if the current time matches with the timespec and the effective
     // duration in unit of seconds.
     //
     bool tier_time_effect(fds_uint32_t &duration_sec);

     char const *const        tier_tspec_name;

    private:
     mutable boost::atomic<int> tier_refcnt;
     friend void intrusive_ptr_add_ref(const TierTimeSpec *x) {
         x->tier_refcnt.fetch_add(1, boost::memory_order_relaxed);
     }
     friend void intrusive_ptr_release(const TierTimeSpec *x) {
         if (x->tier_refcnt.fetch_sub(1, boost::memory_order_release) == 1) {
             boost::atomic_thread_fence(boost::memory_order_acquire);
             delete x;
         }
     }

    protected:
     // XXX: simple start/duration(stop), no periodic/range of various sec/min/
     // hour/day/week/month/quarter/year...
     //
     fds_uint32_t             tier_duration_sec;
};

// TierPolAction
// -------------
// Define pure virtual class to enable plugins to carry on tier policy actions.
//
// Control path:
// idle -> warm_up -> activate -> adjust_consumption -> cool_down -> deactivate
//      ^               ^                     |      |
//      |               +---------------------+      |
//      +-------------------------------------------+
// Note: the different between "cool_down" and "deactivate" is the framework
// still collects stats for auditing.
//
// Admin path:
// - Accept TierStat from the audit module.
// - Accept thread pool to schedule threads and thread quota...
// - Given information about tier media and percentage of capacity that it
//   can use.
//
class TierPolAction {
    public:
     typedef boost::intrusive_ptr<TierPolAction> pointer;
     typedef enum {
         TIER_ACT_IDLE = 0,
         TIER_ACT_WARMUP,
         TIER_ACT_ACTIVE_NORM,
         TIER_ACT_ACTIVE_LOW,
         TIER_ACT_ACTIVE_HIGH,
         TIER_ACT_PAUSE,
         TIER_ACT_RESUME,
         TIER_ACT_COOLDOWN,
         TIER_ACT_DEACTIVATE
     } tier_phase_e;

     explicit TierPolAction(fds_uint64_t vol_uuid);
     virtual ~TierPolAction();

     // Control path actions.
     //
     virtual void
         tier_warm_up(fds_uint32_t duration_sec, int tier_pct, int priority) = 0;

     virtual void tier_activate(FDS_ProtocolInterface::tier_media_type_e media, int tier_pct) = 0;
     virtual void tier_adjust_consumption(FDS_ProtocolInterface::tier_media_type_e media,
                                          int tier_pct) = 0;
     virtual void tier_cool_down(fds_uint32_t duration_sec) = 0;
     virtual void tier_deactivate() = 0;

     // Admin path functions.
     //
     TierStat       &tier_get_stat() const { return *tier_stat; }
     fds_threadpool *tier_get_thrpool() const { return tier_thrpool; }
     fds_uint64_t    tier_get_vol_uuid() const { return tier_vol_uuid; }

    private:
     ChainLink                  tier_link;
     tier_phase_e               tier_phase;
     tier_phase_e               tier_prev_phase;
     mutable boost::atomic<int> tier_refcnt;

     friend class TierPolFuncSpec;
     friend void intrusive_ptr_add_ref(const TierPolAction *x) {
         x->tier_refcnt.fetch_add(1, boost::memory_order_relaxed);
     }
     friend void intrusive_ptr_release(const TierPolAction *x) {
         if (x->tier_refcnt.fetch_sub(1, boost::memory_order_release) == 1) {
             boost::atomic_thread_fence(boost::memory_order_acquire);
             delete x;
         }
     }
     void tier_assign_thrpool(fds_threadpool *pool) { tier_thrpool = pool; }
     void tier_assign_stat(TierStat *stat) { tier_stat = stat; }

     // Wrapper for threadpool's schedule function.
     static void tier_do_action_thr(TierPolAction *act);
     void tier_do_action();

    protected:
     TierStat                 *tier_stat;
     fds_threadpool           *tier_thrpool;
     TierTimeSpec::pointer    tier_time_spec;

     fds_uint64_t             tier_vol_uuid;
     fds_uint32_t             tier_cur_duration;
     FDS_ProtocolInterface::tier_media_type_e tier_cur_media[TIER_MAX_MEDIA_TYPES];
     int                      tier_med_pct_consume[TIER_MAX_MEDIA_TYPES];
};

// TierPolFuncSpec
// ---------------
//
class TierPolFuncSpec {
    public:
     TierPolFuncSpec(char const *const fn_name, fds_threadpool *pool);
     virtual ~TierPolFuncSpec();

     // Given a timespec and action, carry on the tier policy schedule at the
     // right time.
     //
     virtual void
         tier_sched_spec(TierTimeSpec::pointer spec, TierPolAction::pointer act);

     // Alter the tiering schedule for the given volume.
     //
     virtual void
         tier_sched_alter(TierTimeSpec::pointer spec, fds_uint64_t vol_uuid);

     // Pause/resume the current tier policy for the given volume.
     //
     virtual void tier_sched_pause(fds_uint64_t vol_uuid);
     virtual void tier_sched_resume(fds_uint64_t vol_uuid);

     // Remove current tier policy out of the given volume uuid and return
     // back the original TierPolAction to the caller.
     //
     virtual TierPolAction::pointer tier_sched_remove(fds_uint64_t vol_uuid);

    protected:
     friend class TierPolicyAudit;
     friend class TierPolicyModule;
     typedef boost::unordered_map
         <char const *const, TierTimeSpec::pointer> TimeSpecMap;
     typedef boost::unordered_map
         <fds_uint64_t, TierPolAction::pointer> TierActionMap;

     char const *const        tier_fn_name;
     TimeSpecMap              tier_time_spec;
     TierActionMap            tier_policy;

     // Policies in the active list
     ChainList                tier_pol_active;
     fds_threadpool           *tier_pool;
     fds_mutex                tier_fn_mtx;

     // Wrapper for threadpool's schedule function.
     static void tier_sched_run_jobs(TierPolFuncSpec *fn);
     void tier_sched_run();
};

// TierPolicyExe
// -------------
// TBD
//
class TierPolicyExe {
    public:
     TierPolicyExe() {}
     ~TierPolicyExe() {}

    protected:
};

// TierPolicyAudit
// ---------------
//
class TierPolicyAudit {
    public:
     TierPolicyAudit() {}
     ~TierPolicyAudit() {}

    private:
};

class TierPolicyModule : public virtual Module {
    public:
     explicit TierPolicyModule(char const *const name);
     ~TierPolicyModule();

     virtual int  mod_init(SysParams const *const param);
     virtual void mod_startup();
     virtual void mod_shutdown();

     TierPolFuncSpec &mod_TierFuncSpec(char const *const name) {
         return *tier_fn;
     }

    private:
     TierPolicyExe            *tier_exe;
     TierPolicyAudit          *tier_audit;
     fds_threadpool           *tier_pool;

     // XXX: for now, make a single policy function spec.
     //
     TierPolFuncSpec          *tier_fn;
};

extern TierPolicyModule  gl_tierPolicy;

}  // namespace fds

#endif  // SOURCE_STOR_MGR_INCLUDE_POLICY_TIER_H_
