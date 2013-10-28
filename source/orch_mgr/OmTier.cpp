/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#include <net-proxies/vol_policy.h>
#include <string>
#include <iostream>

using namespace std;
namespace fds {

void
VolPolicyServ::serv_recvTierPolicyReq(const opi::tier_pol_time_unit &policy)
{
    cout << "Receive tier policy" << endl;
}

void
VolPolicyServ::serv_recvAuditTierPolicy(const opi::tier_pol_audit &audit)
{
}

} // namespace fds
