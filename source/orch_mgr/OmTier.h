/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#ifndef ORCH_MGR_OM_TIER_H_
#define ORCH_MGR_OM_TIER_H_

#include <net-proxies/vol_policy.h>

namespace fds {

class Orch_VolPolicyServ : public virtual VolPolicyServ
{
  public:
    Orch_VolPolicyServ() : VolPolicyServ("OM Vol Policy") {}
    ~Orch_VolPolicyServ() {}

    virtual void serv_recvTierPolicyReq(const fdp::tier_pol_time_unit &req);
    virtual void serv_recvAuditTierPolicy(const fdp::tier_pol_audit &audit);

    // Plugin in with current OMClient code.
    virtual void
    serv_recvTierPolicyReq(const fdp::FDSP_TierPolicyPtr &tier);

    virtual void
    serv_recvTierPolicyAuditReq(const fdp::FDSP_TierPolicyAuditPtr &tier);
};

} // namespace fds

#endif /* ORCH_MGR_OM_TIER_H_ */
