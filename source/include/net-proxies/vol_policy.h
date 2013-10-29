/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#ifndef INCLUDE_NET_PROXY_VOL_POLICY_H_
#define INCLUDE_NET_PROXY_VOL_POLICY_H_

#include <fdsp/orch_proto.h>
#include <fdsp/FDSP.h>
#include <Ice/Ice.h>
#include <string>
#include <fds_module.h>

// XXX: temp. encoding for now to use simple tier schedule.
//
#define TIER_SCHED_ACTIVATE            0
#define TIER_SCHED_DEACTIVATE          1

// End points to receive get/set for volume policies.
//
#define ORCH_MGR_POLICY_ID                "OrchMgr-Policy"
#define STOR_MGR_POLICY_ID                "StorMgr-Policy"

namespace opi = orch_ProtocolInterface;
namespace fdp = FDS_ProtocolInterface;

namespace fds {

// Transport neutral interface for client code to set/get volume policies.
//
class VolPolicyClnt
{
  public:
    VolPolicyClnt(std::string serv_id) : net_serv_id(serv_id) {}
    ~VolPolicyClnt() {}

    virtual int clnt_setTierPolicy(struct opi::tier_pol_time_unit &req) = 0;
    virtual int clnt_getAuditTier(struct opi::tier_pol_audit &audit) = 0;

  protected:
    std::string              net_serv_id;
};

// Ice-specific client side transport.
//
class Ice_VolPolicyClnt : public virtual VolPolicyClnt
{
  public:
    Ice_VolPolicyClnt(Ice::CommunicatorPtr comm, std::string serv_id);
    ~Ice_VolPolicyClnt();

    int clnt_setTierPolicy(struct opi::tier_pol_time_unit &req);
    int clnt_getAuditTier(struct opi::tier_pol_audit &audit);

    int clnt_bind_ice_client(Ice::CommunicatorPtr comm, std::string srv);
  private:
    opi::orch_PolicyReqPrx   clnt_mgr;
};

// Transport neutral interface for server code to provide services for volume
// policies.
//
class VolPolicyServ
{
  public:
    VolPolicyServ(char const *const name) : pol_serv(name) {}
    ~VolPolicyServ() {}

    virtual void serv_recvTierPolicyReq(const opi::tier_pol_time_unit &req) = 0;
    virtual void serv_recvAuditTierPolicy(const opi::tier_pol_audit &audit) = 0;

    // Plugin in with current OMClient code.
    virtual void
    serv_recvTierPolicyReq(const fdp::FDSP_TierPolicyPtr &tier) = 0;

    virtual void
    serv_recvTierPolicyAuditReq(const fdp::FDSP_TierPolicyAuditPtr &tier) = 0;

  protected:
    char const *const        pol_serv;
};

// Ice-specific server side handler.
//
class Ice_VolPolicyServ : public virtual opi::orch_PolicyReq
{
  public:
    Ice_VolPolicyServ(std::string serv_id, VolPolicyServ &server)
        : net_serv_id(serv_id), net_server(server) {}
    ~Ice_VolPolicyServ() {}

    int serv_registerIceAdapter(Ice::CommunicatorPtr  comm,
                                Ice::ObjectAdapterPtr adapter);

    // serv_recvTierPolicyReq
    // ----------------------
    //
    void applyTierPolicy(const opi::tier_pol_time_unit &req,
                         const Ice::Current &)
    {
        net_server.serv_recvTierPolicyReq(req);
    }

    // serv_recvAuditTierPolicy
    // ------------------------
    //
    void auditTierPolicy(const opi::tier_pol_audit &audit,
                         const Ice::Current &)
    {
        net_server.serv_recvAuditTierPolicy(audit);
    }

  protected:
    std::string              net_serv_id;
    VolPolicyServ            &net_server;
};

} // namespace fds

#endif /* INCLUDE_NET_PROXY_VOL_POLICY_H_ */
