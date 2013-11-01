/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#include <policy_tier.h>

namespace fds {

// ----------------------------------------------------------------------------
// TierTimeSpec
// ------------
//
TierTimeSpec::TierTimeSpec(char const *const name)
    : tier_tspec_name(name)
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

// TierPolAction
// -------------
//
TierPolAction::TierPolAction(fds_uint64_t tier_uuid)
    : tier_vol_uuid(tier_uuid)
{
}

TierPolAction::~TierPolAction()
{
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
}

// tier_sched_spec
// ---------------
//
void
TierPolFuncSpec::tier_sched_spec(TierTimeSpec const &spec, TierPolAction &act)
{
}

// tier_sched_alter
// ----------------
//
void
TierPolFuncSpec::tier_sched_alter(TierTimeSpec const &spec, fds_uint64_t uuid)
{
}

// tier_sched_pause
// ----------------
//
void
TierPolFuncSpec::tier_sched_pause(fds_uint64_t vol_uuid)
{
}

// tier_sched_resume
// -----------------
//
void
TierPolFuncSpec::tier_sched_resume(fds_uint64_t vol_uuid)
{
}

// tier_sched_remove
// -----------------
//
TierPolAction *
TierPolFuncSpec::tier_sched_remove(fds_uint64_t vol_uuid)
{
    return nullptr;
}

// tier_sched_run_jobs
// -------------------
//
void
TierPolFuncSpec::tier_sched_run_jobs()
{
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
TierStat::tier_stat_rec_iop(fds_tier_type_e media, bool rd)
{
}

// tier_stat_rec_latency
// ---------------------
//
void
TierStat::tier_stat_rec_latency(fds_tier_type_e media, fds_uint32_t usec)
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

} // namespace fds
