/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#ifndef STORMGR_INCLUDE_TIER_POLICY_H_
#define STORMGR_INCLUDE_TIER_POLICY_H_

#include <fds_types.h>

namespace fds {

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

class TierAction
{
  public:
    TierAction(fds_uint64_t vol_uuid);
    ~TierAction();

    virtual void tier_warm_up(

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
