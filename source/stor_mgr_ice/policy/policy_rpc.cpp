/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#include <policy_rpc.h>
#include <iostream>
#include <fds_assert.h>

namespace fds {

VolPolicyRPC        gl_SMVolPolicy("SM Vol Policy Module");
SM_VolPolicyServ    sg_SMVolPolicyServ;

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
