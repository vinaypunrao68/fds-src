/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#include <net-proxies/vol_policy.h>

namespace fds {

Ice_VolPolicyClnt::Ice_VolPolicyClnt(Ice::CommunicatorPtr comm,
                                     std::string          serv_id)
    : VolPolicyClnt(serv_id)
{
    Ice::ObjectPrx proxy = comm->stringToProxy(serv_id);
    net_orch_mgr = opi::orch_PolicyReqPrx::checkedCast(proxy);
}

Ice_VolPolicyClnt::~Ice_VolPolicyClnt()
{
}

int
Ice_VolPolicyClnt::clnt_setTierPolicy(struct opi::tier_pol_time_unit &req)
{
    net_orch_mgr->applyTierPolicy(req);
    return 0;
}

int
Ice_VolPolicyClnt::clnt_getAuditTier(struct opi::tier_pol_audit &audit)
{
    net_orch_mgr->auditTierPolicy(audit);
    return 0;
}

} // namespace fds
