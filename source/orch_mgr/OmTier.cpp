/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#include <OmTier.h>
#include <string>
#include <iostream>
#include <orchMgr.h>

using namespace std;
namespace fds {

void
Orch_VolPolicyServ::serv_recvTierPolicyReq(const opi::tier_pol_time_unit &tier)
{
    FDSP_TierPolicyPtr sm_data;
    localDomainInfo *dom = gl_orch_mgr->om_GetDomainInfo(DEFAULT_LOC_DOMAIN_ID);

    cout << "Receive tier policy" << endl;
    sm_data = new FDSP_TierPolicy();

    sm_data->tier_vol_uuid      = tier.tier_vol_uuid;
    sm_data->tier_media         = tier.tier_media;
    sm_data->tier_prefetch_algo = tier.tier_prefetch;
    sm_data->tier_media_pct     = tier.tier_media_pct;
    sm_data->tier_interval_sec  = tier.tier_period.ts_sec;
    dom->domain_ptr->sendTierPolicyToSMNodes(sm_data);
}

void
Orch_VolPolicyServ::serv_recvAuditTierPolicy(const opi::tier_pol_audit &audit)
{
    FDSP_TierPolicyAuditPtr sm_data;
    localDomainInfo *dom = gl_orch_mgr->om_GetDomainInfo(DEFAULT_LOC_DOMAIN_ID);

    // We have no way to send back result to CLI
    //
    sm_data = new FDSP_TierPolicyAudit;
    sm_data->tier_vol_uuid      = 0;
    sm_data->tier_stat_min_iops = 0;
    dom->domain_ptr->sendTierAuditPolicyToSMNodes(sm_data);
}

void
Orch_VolPolicyServ::serv_recvTierPolicyReq(const fdp::FDSP_TierPolicyPtr &tier)
{
}

void
Orch_VolPolicyServ::
serv_recvTierPolicyAuditReq(const fdp::FDSP_TierPolicyAuditPtr &tier)
{
}

} // namespace fds
