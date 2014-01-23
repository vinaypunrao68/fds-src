/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#ifndef INCLUDE_NET_PROXY_VOL_POLICY_H_
#define INCLUDE_NET_PROXY_VOL_POLICY_H_

// #include <fdsp/orch_proto_types.h>
// #include <fdsp/orch_PolicyReq.h>
#include <fdsp/FDSP_types.h>
#include <string>
#include <fds_module.h>

// XXX: temp. encoding for now to use simple tier schedule.
//
#define TIER_SCHED_ACTIVATE            (0xffff)
#define TIER_SCHED_DEACTIVATE          0

// XXX: until we have a good RPC gen header.
//
#define TIER_MAX_MEDIA_TYPES           (4)

// End points to receive get/set for volume policies.
//
#define ORCH_MGR_POLICY_ID                "OrchMgr-Policy"
#define STOR_MGR_POLICY_ID                "StorMgr-Policy"

namespace fdp = FDS_ProtocolInterface;

namespace fds {

// Transport neutral interface for client code to set/get volume policies.
//
class VolPolicyClnt
{
  public:
    VolPolicyClnt(std::string serv_id) : net_serv_id(serv_id) {}
    ~VolPolicyClnt() {}

    virtual int clnt_setTierPolicy(struct fdp::tier_pol_time_unit &req) = 0;
    virtual int clnt_getAuditTier(struct fdp::tier_pol_audit &audit) = 0;

  protected:
    std::string              net_serv_id;
};

// Thrift-specific client side transport.
//
class Thrift_VolPolicyClnt : public virtual VolPolicyClnt
{
  public:
    Thrift_VolPolicyClnt(/*Ice::CommunicatorPtr comm, */std::string serv_id);
    ~Thrift_VolPolicyClnt();

    int clnt_setTierPolicy(struct fdp::tier_pol_time_unit &req);
    int clnt_getAuditTier(struct fdp::tier_pol_audit &audit);

    // int clnt_bind_ice_client(Ice::CommunicatorPtr comm, std::string srv);
  private:
    // TODO(thrift)
    // opi::orch_PolicyReqPrx   clnt_mgr;
};

// Transport neutral interface for server code to provide services for volume
// policies.
//
class VolPolicyServ
{
  public:
    VolPolicyServ(char const *const name) : pol_serv(name) {}
    ~VolPolicyServ() {}

    virtual void serv_recvTierPolicyReq(const fdp::tier_pol_time_unitPtr &req) = 0;
    virtual void serv_recvAuditTierPolicy(const fdp::tier_pol_auditPtr &audit) = 0;

    // Plugin in with current OMClient code.
    virtual void
    serv_recvTierPolicyReq(const fdp::FDSP_TierPolicyPtr &tier) = 0;

    virtual void
    serv_recvTierPolicyAuditReq(const fdp::FDSP_TierPolicyAuditPtr &tier) = 0;

  protected:
    char const *const        pol_serv;
};

// Thrift-specific server side handler.
//
class Thrift_VolPolicyServ /*: virtual public fdp::orch_PolicyReqIf */
{
  public:
    Thrift_VolPolicyServ(std::string serv_id, VolPolicyServ &server)
            : net_serv_id(serv_id), net_server(server) {}
    ~Thrift_VolPolicyServ() {}

    /*
    int serv_registerIceAdapter(Ice::CommunicatorPtr  comm,
                                Ice::ObjectAdapterPtr adapter);
    */

    // serv_recvTierPolicyReq
    // ----------------------
    //
    void applyTierPolicy(const fdp::tier_pol_time_unit& policy) {
        /* Don't do anything here. This stub is just to keep cpp compiler happy */
    }
    void applyTierPolicy(fdp::tier_pol_time_unitPtr& policy) {
        net_server.serv_recvTierPolicyReq(policy);
    }

    // serv_recvAuditTierPolicy
    // ------------------------
    //
    void auditTierPolicy(const fdp::tier_pol_audit& audit) {
        /* Don't do anything here. This stub is just to keep cpp compiler happy */        
    }
    void auditTierPolicy(fdp::tier_pol_auditPtr& audit) {
        net_server.serv_recvAuditTierPolicy(audit);
    }

  protected:
    std::string              net_serv_id;
    VolPolicyServ            &net_server;
};

} // namespace fds

#endif /* INCLUDE_NET_PROXY_VOL_POLICY_H_ */
