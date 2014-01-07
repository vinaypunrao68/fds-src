/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#include <policy_rpc.h>
#include <iostream>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <fds_assert.h>
#include <StorMgr.h>

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
VolPolicyRPC::rpc_init_ice_client(Ice::CommunicatorPtr comm, std::string srv_id)
{
    pol_client = new Ice_VolPolicyClnt(comm, srv_id);
}

int
VolPolicyRPC::rpc_register_ice_server(Ice::CommunicatorPtr  comm,
                                      Ice::ObjectAdapterPtr adapter,
                                      std::string           serv_id)
{
    fds_verify(pol_server != nullptr);
    pol_ice_adapter = new Ice_VolPolicyServ(serv_id, *pol_server);
    return pol_ice_adapter->serv_registerIceAdapter(comm, adapter);
}

void
SM_VolPolicyServ::serv_recvTierPolicyReq(const opi::tier_pol_time_unit &req)
{
}

void
SM_VolPolicyServ::serv_recvAuditTierPolicy(const opi::tier_pol_audit &audit)
{
}

// Plugin in with current OMClient code.
void
SM_VolPolicyServ::serv_recvTierPolicyReq(const fdp::FDSP_TierPolicyPtr &tier)
{
    StorMgrVolume      *vol;
    StorMgrVolumeTable *vol_tab = objStorMgr->sm_getVolTables();

    // TODO: the interface here should be lock vol obj with refcnt so that it
    // won't be deleted while we're using it.
    //
    vol = vol_tab->getVolume(tier->tier_vol_uuid);
    if (vol == nullptr) {
        // TODO: No way to return the result back to caller.
        //
        FDS_PLOG(objStorMgr->sm_log) << "Could not find volume uuid "
            << tier->tier_vol_uuid;
        return;
    }
    VolumeDesc *desc = vol->voldesc;
    if (tier->tier_interval_sec != TIER_SCHED_DEACTIVATE) {
        //
        objStorMgr->tierEngine->migrator->startRankTierMigration();
        if (tier->tier_media == opi::TIER_MEDIA_SSD) {
            desc->volType = fdp::FDSP_VOL_BLKDEV_SSD_TYPE;
        } else if (tier->tier_media == opi::TIER_MEDIA_HDD) {
            desc->volType = fdp::FDSP_VOL_BLKDEV_DISK_TYPE;
        } else if (tier->tier_media == opi::TIER_MEDIA_HYBRID) {
            desc->volType = fdp::FDSP_VOL_BLKDEV_HYBRID_TYPE;
        } else {
            desc->volType = fdp::FDSP_VOL_BLKDEV_HYBRID_PREFCAP_TYPE;
        }
        desc->tier_duration_sec = tier->tier_interval_sec;
        desc->tier_start_time =
            boost::posix_time::second_clock::universal_time();
    } else {
        // Stop the migration
        objStorMgr->tierEngine->migrator->stopRankTierMigration();
        desc->tier_duration_sec = 0;
        desc->volType = fdp::FDSP_VOL_BLKDEV_DISK_TYPE;
    }
    using namespace std;
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

} // namespace fds
