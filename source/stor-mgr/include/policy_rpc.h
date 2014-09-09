/*
 * Copyright 2013-2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_STOR_MGR_INCLUDE_POLICY_RPC_H_
#define SOURCE_STOR_MGR_INCLUDE_POLICY_RPC_H_

#include <string>
#include "fds_module.h"
#include "net-proxies/vol_policy.h"

namespace fds {

class VolPolicyRPC : public virtual Module {
    public:
     explicit VolPolicyRPC(char const *const name)
         : Module(name), pol_client(nullptr), pol_server(nullptr) {}
     ~VolPolicyRPC() {}

     virtual int  mod_init(SysParams const *const param);
     virtual void mod_startup();
     virtual void mod_shutdown();

     virtual void rpc_bind_server(VolPolicyServ &server) {
         pol_server = &server;
     }

     int rpc_init_client();
     int rpc_register_server();

    protected:
     // Client proxy can be generic type.
     VolPolicyClnt            *pol_client;
     VolPolicyServ            *pol_server;
};

class SM_VolPolicyServ : public virtual VolPolicyServ {
    public:
     SM_VolPolicyServ() : VolPolicyServ("SM Vol Policy") {}
     ~SM_VolPolicyServ() {}

     virtual void serv_recvTierPolicyReq(const fdp::tier_pol_time_unitPtr &req) override;
     virtual void serv_recvAuditTierPolicy(const fdp::tier_pol_auditPtr &audit) override;

     // Plugin in with current OMClient code.
     virtual void
         serv_recvTierPolicyReq(const fdp::FDSP_TierPolicyPtr &tier) override;

     virtual void
         serv_recvTierPolicyAuditReq(const fdp::FDSP_TierPolicyAuditPtr &tier) override;
};

extern VolPolicyRPC          gl_SMVolPolicy;
extern SM_VolPolicyServ      sg_SMVolPolicyServ;

}  // namespace fds
#endif  // SOURCE_STOR_MGR_INCLUDE_POLICY_RPC_H_
