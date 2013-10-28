/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#ifndef STOR_MGR_INCLUDE_POLICY_RPC_H_
#define STOR_MGR_INCLUDE_POLICY_RPC_H_

#include <string>
#include <fds_module.h>
#include <net-proxies/vol_policy.h>

class VolPolicyRPC : public virtual Module
{
  public:
    VolPolicyRPC(char const *const name)
        : Module(name), pol_client(nullptr), pol_server(nullptr) {}
    ~VolPolicyRPC() {}

    virtual int  mod_init(SysParams const *const param);
    virtual void mod_startup();
    virtual void mod_shutdown();

    virtual void rpc_bind_server(VolPolicyServer &server)
    {
        pol_server = *server;
    }
    int rpc_init_ice_client(Ice::CommunicatorPtr comm, std::string srv_id);
    int rpc_register_ice_server(Ice::CommunicatorPtr  comm,
                                Ice::ObjectAdapterPtr adapter);

  protected:
    VolPolicyClnt            *pol_client;
    VolPolicyServer          *pol_server;
};

#endif /* STOR_MGR_INCLUDE_POLICY_RPC_H_ */
