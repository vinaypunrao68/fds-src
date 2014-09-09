/*
 * Copyright 2013-2014 Formation Data Systems, Inc.
 */
#include "policy_tier.h"
#include "fds_assert.h"

namespace fds {

TierPolicyModule   gl_tierPolicy("Tier Policy Module");

// ----------------------------------------------------------------------------

TierPolicyModule::TierPolicyModule(char const *const name)
    : Module(name), tier_pool(nullptr), tier_fn(nullptr)
{
    tier_exe   = new TierPolicyExe();
    tier_audit = new TierPolicyAudit();
}

TierPolicyModule::~TierPolicyModule()
{
    delete tier_exe;
    delete tier_audit;
    delete tier_fn;
    delete tier_pool;
}

// \mod_init
// ---------
//
int
TierPolicyModule::mod_init(SysParams const *const param)
{
    // XXX: should take threadpool size from param.
    //
    tier_pool = new fds_threadpool(2);
    tier_fn   = new TierPolFuncSpec("simple tier policy", tier_pool);
    return 0;
}

// \mod_startup
// ------------
//
void
TierPolicyModule::mod_startup()
{
    fds_verify(tier_fn != nullptr);
    fds_verify(tier_pool != nullptr);

    tier_pool->schedule(TierPolFuncSpec::tier_sched_run_jobs, tier_fn);
}

// \mod_shutdown
// -------------
//
void
TierPolicyModule::mod_shutdown()
{
}

// ----------------------------------------------------------------------------

// TierTimeSpec
// ------------
//
TierTimeSpec::TierTimeSpec(char const *const name)
    : tier_tspec_name(name), tier_refcnt(0)
{
}

TierTimeSpec::~TierTimeSpec()
{
}

bool
TierTimeSpec::tier_time_effect(fds_uint32_t &duration_sec)
{
    duration_sec = 0xffff;
    return true;
}

// ----------------------------------------------------------------------------

// TierPolAction
// -------------
//
TierPolAction::TierPolAction(fds_uint64_t tier_uuid)
    : tier_vol_uuid(tier_uuid), tier_stat(nullptr), tier_time_spec(nullptr),
      tier_thrpool(nullptr), tier_cur_duration(0), tier_link(this),
      tier_refcnt(0), tier_phase(TIER_ACT_IDLE), tier_prev_phase(TIER_ACT_IDLE)
{
    int i;

    for (i = 0; i < TIER_MAX_MEDIA_TYPES; i++) {
        tier_cur_media[i] = FDS_ProtocolInterface::TIER_MEIDA_NO_VAL;
        tier_med_pct_consume[i] = 0;
    }
}

TierPolAction::~TierPolAction()
{
    fds_verify(tier_phase == TIER_ACT_DEACTIVATE);
    fds_verify(tier_link.chain_empty() == true);
    delete tier_stat;
}

// tier_do_action_thr
// ------------------
//
void
TierPolAction::tier_do_action_thr(TierPolAction *act)
{
    act->tier_do_action();
}

// tier_do_action
// --------------
// The main handler running in its own thread to carry out the tier policy.
// It depends on virtual methods to do the actual work.
//
void
TierPolAction::tier_do_action()
{
    fds_uint32_t duration;

    if (tier_time_spec->tier_time_effect(duration) == true) {
        // XXX: simple activate call for now.
        if (tier_phase != TierPolAction::TIER_ACT_ACTIVE_NORM) {
            tier_activate(FDS_ProtocolInterface::TIER_MEDIA_SSD, tier_med_pct_consume[0]);
        } else {
            tier_deactivate();
        }
    }
}

// ----------------------------------------------------------------------------

// TierPolFuncSpec
// ---------------
//
TierPolFuncSpec::TierPolFuncSpec(char const *const fn_name, fds_threadpool *p)
    : tier_fn_name(fn_name), tier_pool(p), tier_fn_mtx("tierfn mtx")
{
}

TierPolFuncSpec::~TierPolFuncSpec()
{
    // TODO(vy): must walk throught all tables and destroy objects in there.
}

// tier_sched_spec
// ---------------
//
void
TierPolFuncSpec::tier_sched_spec(TierTimeSpec::pointer spec,
                                 TierPolAction::pointer act)
{
    bool         init;
    fds_uint64_t vol_uuid;

    vol_uuid = act->tier_get_vol_uuid();
    tier_fn_mtx.lock();
    if (tier_policy.count(vol_uuid) == 0) {
        init = true;
        fds_verify(act->tier_link.chain_empty() == true);
        tier_policy[act->tier_get_vol_uuid()] = TierPolAction::pointer(act);
        tier_pol_active.chain_add_back(&act->tier_link);
    } else {
        init = false;
        fds_verify(act->tier_link.chain_empty() == false);
    }
    if (tier_time_spec.count(spec->tier_tspec_name) == 0) {
        tier_time_spec[spec->tier_tspec_name] = TierTimeSpec::pointer(spec);
    }
    tier_fn_mtx.unlock();

    if (init == true) {
        act->tier_assign_stat(new TierStat(vol_uuid));
        act->tier_prev_phase = act->tier_phase;
        act->tier_phase      = TierPolAction::TIER_ACT_IDLE;
        act->tier_thrpool    = tier_pool;
        act->tier_time_spec  = spec;
    }
    tier_pool->schedule(TierPolFuncSpec::tier_sched_run_jobs, this);
}

// tier_sched_alter
// ----------------
//
void
TierPolFuncSpec::tier_sched_alter(TierTimeSpec::pointer spec,
                                  fds_uint64_t uuid)
{
}

// tier_sched_pause
// ----------------
//
void
TierPolFuncSpec::tier_sched_pause(fds_uint64_t vol_uuid)
{
    TierPolAction::pointer act;

    tier_fn_mtx.lock();
    if (tier_policy.count(vol_uuid) == 0) {
        act = tier_policy[vol_uuid];
        fds_verify(act != nullptr);

        act->tier_link.chain_rm_init();
        act->tier_prev_phase = act->tier_phase;
        act->tier_phase = TierPolAction::TIER_ACT_PAUSE;
    } else {
        act = nullptr;
    }
    tier_fn_mtx.unlock();

    if (act != nullptr) {
        // XXX: more stuffs needed here.
        fds_verify(act->tier_link.chain_empty() == true);
    }
}

// tier_sched_resume
// -----------------
//
void
TierPolFuncSpec::tier_sched_resume(fds_uint64_t vol_uuid)
{
    TierPolAction::pointer act;

    tier_fn_mtx.lock();
    if (tier_policy.count(vol_uuid) == 0) {
        act = tier_policy[vol_uuid];
        fds_verify(act != nullptr);

        act->tier_phase = act->tier_prev_phase;
        if (act->tier_link.chain_empty() == true) {
            tier_pol_active.chain_add_back(&act->tier_link);
        }
    } else {
        act = nullptr;
    }
    tier_fn_mtx.unlock();
    tier_pool->schedule(TierPolFuncSpec::tier_sched_run_jobs, this);
}

// tier_sched_remove
// -----------------
//
TierPolAction::pointer
TierPolFuncSpec::tier_sched_remove(fds_uint64_t vol_uuid)
{
    TierPolAction::pointer act;

    tier_fn_mtx.lock();
    if (tier_policy.count(vol_uuid) == 0) {
        act = tier_policy[vol_uuid];
        fds_verify(act != nullptr);

        tier_policy.erase(vol_uuid);
        act->tier_link.chain_rm_init();
        act->tier_prev_phase = act->tier_phase;
        act->tier_phase = TierPolAction::TIER_ACT_DEACTIVATE;
    } else {
        act = nullptr;
    }
    tier_fn_mtx.unlock();

    if (act != nullptr) {
        fds_verify(act->tier_link.chain_empty() == true);
        act->tier_thrpool = nullptr;
        act->tier_deactivate();
    }
    tier_pool->schedule(TierPolFuncSpec::tier_sched_run_jobs, this);
    return act;
}

// tier_sched_run_jobs
// -------------------
//
void
TierPolFuncSpec::tier_sched_run_jobs(TierPolFuncSpec *spec)
{
    spec->tier_sched_run();
}

// tier_sched_run
// --------------
//
void
TierPolFuncSpec::tier_sched_run()
{
    ChainList      local, save;
    fds_uint32_t   duration;
    TierPolAction *act;

    tier_fn_mtx.lock();
    local.chain_transfer(&tier_pol_active);
    tier_fn_mtx.unlock();

    while ((act = local.chain_rm_front<TierPolAction>()) != nullptr) {
        save.chain_add_back(&act->tier_link);
        if ((act->tier_phase == TierPolAction::TIER_ACT_PAUSE) ||
            (act->tier_phase == TierPolAction::TIER_ACT_DEACTIVATE)) {
            continue;
        }
        if (act->tier_time_spec->tier_time_effect(duration) == true) {
            tier_pool->schedule(TierPolAction::tier_do_action_thr, act);
        }
    }
    tier_fn_mtx.lock();
    tier_pol_active.chain_transfer(&save);
    tier_fn_mtx.unlock();
}

// ----------------------------------------------------------------------------

// TierStat
// --------
//
TierStat::TierStat(fds_uint64_t vol_uuid)
{
}

TierStat::~TierStat()
{
}

// tier_stat_rec_iop
// -----------------
//
void
TierStat::tier_stat_rec_iop(FDS_ProtocolInterface::tier_media_type_e media, bool rd)
{
}

// tier_stat_rec_latency
// ---------------------
//
void
TierStat::tier_stat_rec_latency(FDS_ProtocolInterface::tier_media_type_e media, fds_uint32_t usec)
{
}

// tier_stat_report
// ----------------
//
void
TierStat::tier_stat_report(char const *const file)
{
}

// tier_stat_begin
// ---------------
//
void
TierStat::tier_stat_begin()
{
}

// tier_stat_end
// -------------
//
void
TierStat::tier_stat_end()
{
}

}  // namespace fds
