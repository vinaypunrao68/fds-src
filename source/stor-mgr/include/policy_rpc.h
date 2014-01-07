/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#ifndef STOR_MGR_INCLUDE_POLICY_RPC_H_
#define STOR_MGR_INCLUDE_POLICY_RPC_H_

#include <string>
#include <fds_module.h>
#include <net-proxies/vol_policy.h>

namespace fds {

class VolPolicyRPC : public virtual Module
{
  public:
    VolPolicyRPC(char const *const name)
        : Module(name), pol_client(nullptr), pol_server(nullptr) {}
    ~VolPolicyRPC() {}

    virtual int  mod_init(SysParams const *const param);
    virtual void mod_startup();
    virtual void mod_shutdown();

    virtual void rpc_bind_server(VolPolicyServ &server)
    {
        pol_server = &server;
    }
    // Ice transport specific functions.
    //
    int rpc_init_ice_client(Ice::CommunicatorPtr comm, std::string srv_id);
    int rpc_register_ice_server(Ice::CommunicatorPtr  comm,
                                Ice::ObjectAdapterPtr adapter,
                                std::string           serv_id);

  protected:
    // Client proxy can be generic type.
    VolPolicyClnt            *pol_client;

    // Server adapter must be transport specific.
    Ice_VolPolicyServ        *pol_ice_adapter;
    VolPolicyServ            *pol_server;
};

class SM_VolPolicyServ : public virtual VolPolicyServ
{
  public:
    SM_VolPolicyServ() : VolPolicyServ("SM Vol Policy") {}
    ~SM_VolPolicyServ() {}

    virtual void serv_recvTierPolicyReq(const opi::tier_pol_time_unit &req);
    virtual void serv_recvAuditTierPolicy(const opi::tier_pol_audit &audit);

    // Plugin in with current OMClient code.
    virtual void
    serv_recvTierPolicyReq(const fdp::FDSP_TierPolicyPtr &tier);

    virtual void
    serv_recvTierPolicyAuditReq(const fdp::FDSP_TierPolicyAuditPtr &tier);
};

extern VolPolicyRPC          gl_SMVolPolicy;
extern SM_VolPolicyServ      sg_SMVolPolicyServ;

} // namespace fds
#endif /* STOR_MGR_INCLUDE_POLICY_RPC_H_ */
