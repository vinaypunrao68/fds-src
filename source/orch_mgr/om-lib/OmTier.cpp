 /*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#include <OmTier.h>
#include <queue>
#include <string>
#include <iostream>
#include <orchMgr.h>
#include <OmResources.h>

namespace fds {

void
Orch_VolPolicyServ::serv_recvTierPolicyReq(const fdp::tier_pol_time_unitPtr &tier)
{
    fpi::FDSP_TierPolicyPtr sm_data(new fpi::FDSP_TierPolicy);
    OM_NodeContainer *local = OM_NodeDomainMod::om_loc_domain_ctrl();

    FDS_PLOG_SEV(g_fdslog, fds_log::normal) << "OrchMgr: Receive tier policy";

    sm_data->tier_vol_uuid      = tier->tier_vol_uuid;
    sm_data->tier_domain_uuid   = tier->tier_domain_uuid;
    sm_data->tier_domain_policy = tier->tier_domain_policy;
    sm_data->tier_media         = tier->tier_media;
    sm_data->tier_prefetch_algo = tier->tier_prefetch;
    sm_data->tier_media_pct     = tier->tier_media_pct;
    sm_data->tier_interval_sec  = tier->tier_period.ts_sec;

    if (tier->tier_domain_policy == false) {
        sm_data->tier_domain_policy = true;
    } else {
        sm_data->tier_domain_uuid   = 0;
        sm_data->tier_domain_policy = false;
    }
    local->om_bcast_tier_policy(sm_data);
}

void
Orch_VolPolicyServ::serv_recvAuditTierPolicy(const fdp::tier_pol_auditPtr &audit)
{
    fpi::FDSP_TierPolicyAuditPtr sm_data(new fpi::FDSP_TierPolicyAudit);
    OM_NodeContainer *local = OM_NodeDomainMod::om_loc_domain_ctrl();

    // We have no way to send back result to CLI
    //
    sm_data->tier_vol_uuid      = 0;
    sm_data->tier_stat_min_iops = 0;
    local->om_bcast_tier_audit(sm_data);
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

}  // namespace fds
