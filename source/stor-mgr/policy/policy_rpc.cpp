/*
 * Copyright 2013-2014 Formation Data Systems, Inc.
 */
#include "policy_rpc.h"

#include <boost/date_time/posix_time/posix_time.hpp>
#include <iostream>

#include "fds_assert.h"
#include "StorMgr.h"
#include "TierEngine.h"

namespace fds {

VolPolicyRPC        gl_SMVolPolicy("SM Vol Policy Module");
SM_VolPolicyServ    sg_SMVolPolicyServ;

extern ObjectStorMgr  *objStorMgr;

int
VolPolicyRPC::mod_init(SysParams const *const param)
{
    Module::mod_init(param);
    return 0;
}

void
VolPolicyRPC::mod_startup()
{
}

void
VolPolicyRPC::mod_shutdown()
{
}

int
VolPolicyRPC::rpc_init_client()
{
    // todo: init client.  Passin appropriate params
    return -1;
}

int
VolPolicyRPC::rpc_register_server()
{
    // todo: init client.  Passin appropriate params
    return 0;
}

void
SM_VolPolicyServ::serv_recvTierPolicyReq(const fdp::tier_pol_time_unitPtr &req)
{
}

void
SM_VolPolicyServ::serv_recvAuditTierPolicy(const fdp::tier_pol_auditPtr &audit)
{
}

// Plugin in with current OMClient code.
void
SM_VolPolicyServ::serv_recvTierPolicyReq(const fdp::FDSP_TierPolicyPtr &tier)
{
    StorMgrVolume      *vol;
    StorMgrVolumeTable *vol_tab = objStorMgr->sm_getVolTables();

    // TODO(vy): the interface here should be lock vol obj with refcnt so that it
    // won't be deleted while we're using it.
    //
    vol = vol_tab->getVolume(tier->tier_vol_uuid);
    if (vol == nullptr) {
        // TODO(vy): No way to return the result back to caller.
        //
        GLOGERROR << "Could not find volume uuid "
            << tier->tier_vol_uuid;
        return;
    }
    VolumeDesc *desc = vol->voldesc;
    if (tier->tier_interval_sec != TIER_SCHED_DEACTIVATE) {
        // TODO(Anna) are we using this path? if yes, need to port
        // to new SM org...
        // objStorMgr->tierEngine->migrator->startRankTierMigration();
        if (tier->tier_media == fdp::TIER_MEDIA_SSD) {
            desc->mediaPolicy = fdp::FDSP_MEDIA_POLICY_SSD;
        } else if (tier->tier_media == fdp::TIER_MEDIA_HDD) {
            desc->mediaPolicy = fdp::FDSP_MEDIA_POLICY_HDD;
        } else if (tier->tier_media == fdp::TIER_MEDIA_HYBRID) {
            desc->mediaPolicy = fdp::FDSP_MEDIA_POLICY_HYBRID;
        } else {
            desc->mediaPolicy = fdp::FDSP_MEDIA_POLICY_HYBRID_PREFCAP;
        }
        desc->tier_duration_sec = tier->tier_interval_sec;
        desc->tier_start_time =
            boost::posix_time::second_clock::universal_time();
    } else {
        // Stop the migration
        // TODO(Anna) are we using this path? if yes, need to port
        // to new SM org...
        // objStorMgr->tierEngine->migrator->stopRankTierMigration();
        desc->tier_duration_sec = 0;
        desc->mediaPolicy = fdp::FDSP_MEDIA_POLICY_HDD;
    }
    using std::cout;
    using std::endl;
    cout << "Recv tier Policy Request" << endl;
    cout << "Vol uuid " << tier->tier_vol_uuid << endl;
    cout << "tier media " << tier->tier_media << endl;
    cout << "tier media pct " << tier->tier_media_pct << endl;
    cout << "tier duration " << tier->tier_interval_sec << endl;
}

void
SM_VolPolicyServ::
serv_recvTierPolicyAuditReq(const fdp::FDSP_TierPolicyAuditPtr &tier)
{
}

}  // namespace fds
