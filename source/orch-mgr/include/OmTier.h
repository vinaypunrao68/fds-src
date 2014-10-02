/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#ifndef SOURCE_ORCH_MGR_INCLUDE_OMTIER_H_
#define SOURCE_ORCH_MGR_INCLUDE_OMTIER_H_

#include <net-proxies/vol_policy.h>

namespace fds {

class Orch_VolPolicyServ : public virtual VolPolicyServ
{
  public:
    Orch_VolPolicyServ() : VolPolicyServ("OM Vol Policy") {}
    ~Orch_VolPolicyServ() {}

    virtual void serv_recvTierPolicyReq(const fdp::tier_pol_time_unitPtr &req);
    virtual void serv_recvAuditTierPolicy(const fdp::tier_pol_auditPtr &audit);

    // Plugin in with current OMClient code.
    virtual void
    serv_recvTierPolicyReq(const fdp::FDSP_TierPolicyPtr &tier);

    virtual void
    serv_recvTierPolicyAuditReq(const fdp::FDSP_TierPolicyAuditPtr &tier);
};

}  // namespace fds

#endif  // SOURCE_ORCH_MGR_INCLUDE_OMTIER_H_
