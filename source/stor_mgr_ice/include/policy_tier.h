/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#ifndef STORMGR_INCLUDE_TIER_POLICY_H_
#define STORMGR_INCLUDE_TIER_POLICY_H_

#include <fds_types.h>
#include <fdsp/orch_proto.h>

namespace fds {

typedef orch_ProtocolInterface::tier_media_type_e fds_tier_type_e;

class TierTimeSpec
{
};

class TierActionSpec
{
  public:
    TierActionSpec(char const *const name);
    ~TierActionSpec();


  protected:
};

class TierStat
{
  public:
    TierStat();
    ~TierStat();

  protected:
};

class TierAction
{
  public:
    TierAction(fds_uint64_t vol_uuid);
    ~TierAction();

    virtual void tier_warm_up(fds_uint64_t duration, int pct, int priority) = 0;
    virtual void tier_activate(int pct) = 0;
    virtual void tier_adjust_media(fds_tier_type_e media, int pct) = 0;
    virtual void tier_cool_down() = 0;
    TierStat const &const tier_get_stat() const;

  protected:
};

class TierPolicyFunction
{
};

class TierPolicyExe
{
};

class TierPolicyStat
{
};

class TierPolicyAudit
{
};

} // namespace fds

#endif /* STORMGR_INCLUDE_TIER_POLICY_H_ */
