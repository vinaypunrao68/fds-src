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

namespace fds {

typedef orch_ProtocolInterface::tier_media_type_e fds_tier_type_e;
class TierPolAction;
class TierPolicyAudit;
class TierPolFuncSpec;

class TierTimeSpec
{
  public:
    TierTimeSpec(char const *const name);
    ~TierTimeSpec() {}

    // XXX: simple API for now.
    void tier_start(fds_uint32_t duration_sec, fds_tier_media_e type, int pct);

  protected:
    // XXX: simple start/duration(stop), no periodic/range of various sec/min/
    // hour/day/week/month/quarter/year...
    //
    fds_uint32_t             tier_duration_sec;
    fds_tier_media_e         tier_media_type;
    int                      tier_media_pct;
    char const *const        tier_tspec_name;
};

class TierStat
{
  public:
    TierStat(fds_uint64_t vol_uuid);
    ~TierStat();

    void tier_stat_begin();
    void tier_stat_inc_iop(fds_tier_type_e media, bool rd);
    void tier_stat_rec_latency(fds_tier_type_e media, fds_uint32_t microsec);
    void tier_stat_end();

    void tier_stat_report(char const *const file);
  protected:
};

class TierPolAction
{
  public:
    TierPolAction(fds_uint64_t vol_uuid);
    ~TierPolAction();

    virtual void
    tier_warm_up(fds_uint32_t duration_sec, int tier_pct, int priority) = 0;

    virtual void tier_activate(fds_tier_type_e media, int tier_pct) = 0;
    virtual void tier_adjust_consumption(fds_tier_type_e media, int tier_pct) = 0;
    virtual void tier_cool_down(fds_uint32_t duration_sec) = 0;
    virtual void tier_deactivate() = 0;

    TierStat &tier_get_stat() const;
    void tier_assign_stat(TierStat &tier_stat);
    void tier_assign_thrpool(fds_threadpool &pool);

  protected:
    TierStat                  &tier_stat;
    TierTimeSpec const        &tier_time_spec;
    fds_threadpool            &tier_thrpool;

    fds_uint32_t              tier_cur_duration;
    fds_tier_type_e           tier_cur_media[TIER_MAX_MEDIA_TYPES];
    int                       tier_med_pct_consume[TIER_MAX_MEDIA_TYPES];
};

class TierPolFuncSpec
{
  public:
    TierPolicyFuncSpec(char const *const fn_name, fds_threadpool &pool);
    ~TierPolicyFuncSpec();

    virtual void
    tier_sched_spec(TierTimeSpec const &spec, TierPolAction &action);

    virtual void
    tier_sched_alter(TierTimeSpec const &spec, fds_uint64_t vol_uuid);

    virtual void
    tier_sched_stop(fds_uint64_t vol_uuid);

    virtual TierPolAction &
    tier_sched_remove(TierPolicyAudit &audit, fds_uint64_t vol_uuid);

  protected:
    char const *const        tier_fn_name;
    ChainList                tier_time_spec;
    ChainList                tier_act_policy;
    ChainList                tier_stat;
    fds_threadpool           &tier_pool;
    fds_mutex                tier_fn_mtx;

    void tier_sched_run_jobs();
};

class TierPolicyExe
{
  public:
  protected:
};

class TierPolicyAudit
{
};

} // namespace fds

#endif /* STORMGR_INCLUDE_TIER_POLICY_H_ */
