/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#ifndef INCLUDE_NET_PROXY_VOL_POLICY_H_
#define INCLUDE_NET_PROXY_VOL_POLICY_H_

#include <fdsp/orch_proto.h>
#include <Ice/Ice.h>
#include <string>

// End points to receive get/set for volume policies.
//
#define ORCH_MGR_POLICY_ID                "OrchMgr-Policy"
#define STOR_MGR_POLICY_ID                "StorMgr-Policy"

namespace opi = orch_ProtocolInterface;

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

  private:
    opi::orch_PolicyReqPrx   net_orch_mgr;
};

// Ice-specific server side handler.
//
class Ice_VolPolicyServ : public virtual opi::orch_PolicyReq
{
  public:
    Ice_VolPolicyServ(std::string serv_id) : net_serv_id(serv_id) {}
    ~Ice_VolPolicyServ() {}

    bool serv_registerIceAdapter(Ice::CommunicatorPtr  comm,
                                 Ice::ObjectAdapterPtr adapter);

    // serv_recvTierPolicyReq
    // ----------------------
    //
    virtual void serv_recvTierPolicyReq(const opi::tier_pol_time_unit &req) = 0;
    void applyTierPolicy(const opi::tier_pol_time_unit &req,
                         const Ice::Current &)
    {
        serv_recvTierPolicyReq(req);
    }

    // serv_recvAuditTierPolicy
    // ------------------------
    //
    virtual void serv_recvAuditTierPolicy(const opi::tier_pol_audit &audit) = 0;
    void auditTierPolicy(const opi::tier_pol_audit &audit,
                         const Ice::Current &)
    {
        serv_recvAuditTierPolicy(audit);
    }

  protected:
    std::string              net_serv_id;
};

// Transport neutral interface for server code to provide services for volume
// policies.
//
class VolPolicyServ : public virtual Ice_VolPolicyServ
{
  public:
    VolPolicyServ(std::string serv_id) : Ice_VolPolicyServ(serv_id) {}
    ~VolPolicyServ() {}

    virtual void serv_recvTierPolicyReq(const opi::tier_pol_time_unit &req);
    virtual void serv_recvAuditTierPolicy(const opi::tier_pol_audit &audit);
};

} // namespace fds

#endif /* INCLUDE_NET_PROXY_VOL_POLICY_H_ */
