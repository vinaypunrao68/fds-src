/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#include <OmTier.h>
#include <queue>
#include <string>
#include <iostream>
#include <orchMgr.h>

using namespace std;
namespace fds {

void
Orch_VolPolicyServ::serv_recvTierPolicyReq(const fdp::tier_pol_time_unit &tier)
{
    FDS_ProtocolInterface::FDSP_TierPolicyPtr sm_data(
        new FDS_ProtocolInterface::FDSP_TierPolicy);
    localDomainInfo *dom = gl_orch_mgr->om_GetDomainInfo(DEFAULT_LOC_DOMAIN_ID);

    cout << "Receive tier policy" << endl;

    sm_data->tier_vol_uuid      = tier.tier_vol_uuid;
    sm_data->tier_domain_uuid   = tier.tier_domain_uuid;
    sm_data->tier_domain_policy = tier.tier_domain_policy;
    sm_data->tier_media         = tier.tier_media;
    sm_data->tier_prefetch_algo = tier.tier_prefetch;
    sm_data->tier_media_pct     = tier.tier_media_pct;
    sm_data->tier_interval_sec  = tier.tier_period.ts_sec;

    if (tier.tier_domain_policy == false) {
        dom->domain_ptr->sendTierPolicyToSMNodes(sm_data);
    } else {
        // Get all volumes in the domain and send out the command.
        // Using this queue is overkill...
        int vol_cnt;
        std::queue<fds_uint64_t> vol_ids;
        FdsLocalDomain *loc = dom->domain_ptr;

        sm_data->tier_domain_uuid   = 0;
        sm_data->tier_domain_policy = false;
        loc->dom_mutex->lock();
        for (auto it = loc->volumeMap.begin();
             it != loc->volumeMap.end(); it++) {
            VolumeInfo *vol = it->second;
            vol_ids.push(vol->volUUID);
        }
        loc->dom_mutex->unlock();

        for (vol_cnt = 0; !vol_ids.empty(); vol_cnt++) {
            sm_data->tier_vol_uuid = vol_ids.front();
            vol_ids.pop();

            cout << "Appling tier polity to vol "
                << sm_data->tier_vol_uuid << endl;
            dom->domain_ptr->sendTierPolicyToSMNodes(sm_data);
        }
        cout << "Applied tier policy to " << vol_cnt << " volumes." << endl;
    }
}

void
Orch_VolPolicyServ::serv_recvAuditTierPolicy(const fdp::tier_pol_audit &audit)
{
    FDS_ProtocolInterface::FDSP_TierPolicyAuditPtr sm_data(
        new FDS_ProtocolInterface::FDSP_TierPolicyAudit);
    localDomainInfo *dom = gl_orch_mgr->om_GetDomainInfo(DEFAULT_LOC_DOMAIN_ID);

    // We have no way to send back result to CLI
    //
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
