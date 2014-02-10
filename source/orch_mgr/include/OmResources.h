/*
 * Copyright 2013 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_ORCH_MGR_INCLUDE_OMRESOURCES_H_
#define SOURCE_ORCH_MGR_INCLUDE_OMRESOURCES_H_

#include <vector>
#include <string>
#include <list>
#include <unordered_map>
#include <fds_typedefs.h>
#include <fds_err.h>
#include <NetSession.h>
#include <platform/node-inventory.h>

namespace fds {

class OM_NodeDomainMod;
class OM_ControlRespHandler;

namespace fpi = FDS_ProtocolInterface;
typedef boost::shared_ptr<fpi::FDSP_ControlPathReqClient> NodeAgentCpReqClientPtr;
typedef boost::shared_ptr<netControlPathClientSession>    NodeAgentCpSessionPtr;

/**
 * Agent interface to communicate with the remote node.  This is the communication
 * end-point to the node.
 *
 * It's normal that the node agent is there but the transport may not be availble.
 * We'll provide methods to establish the transport in the background and error
 * handling model when the transport is broken.
 */
class OM_SmAgent : public SmAgent
{
  public:
    typedef boost::intrusive_ptr<OM_SmAgent> pointer;
    typedef boost::intrusive_ptr<const OM_SmAgent> const_ptr;

    explicit OM_SmAgent(const NodeUuid &uuid);
    virtual ~OM_SmAgent();

    static inline OM_SmAgent::pointer agt_cast_ptr(Resource::pointer ptr) {
        return static_cast<OM_SmAgent *>(get_pointer(ptr));
    }
    void setCpSession(NodeAgentCpSessionPtr session);

    /**
     * Returns the client end point for the node. The function
     * is not constanct since the member pointer returned
     * is mutable by the caller.
     */
    NodeAgentCpReqClientPtr getCpClient() const;

  protected:
    NodeAgentCpSessionPtr   ndCpSession;
    std::string             ndSessionId;
    NodeAgentCpReqClientPtr ndCpClient;

    virtual int node_calc_stor_weight();
};

typedef OM_SmAgent                      OM_DmAgent;
typedef OM_SmAgent                      OM_AmAgent;
typedef std::list<OM_SmAgent::pointer>  NodeList;

// -------------------------------------------------------------------------------------
// Common OM Node Container
// -------------------------------------------------------------------------------------
class OM_SmContainer : public SmContainer
{
  public:
    typedef boost::intrusive_ptr<OM_SmContainer> pointer;

    OM_SmContainer();
    virtual ~OM_SmContainer();

    void om_splice_nodes_pend(NodeList *addNodes, NodeList *rmNodes);

    virtual void agent_activate(NodeAgent::pointer agent);
    virtual void agent_deactivate(NodeAgent::pointer agent);

    virtual Error agent_register(const NodeUuid       &uuid,
                                 const FdspNodeRegPtr  msg,
                                 NodeAgent::pointer   *out);
    virtual Error agent_unregister(const NodeUuid &uuid, const std::string &name);

    static inline OM_SmContainer::pointer agt_cast_ptr(RsContainer::pointer ptr) {
        return static_cast<OM_SmContainer *>(get_pointer(ptr));
    }
  protected:
    NodeList                                 node_up_pend;
    NodeList                                 node_down_pend;
    boost::shared_ptr<OM_ControlRespHandler> ctrlRspHndlr;

    virtual Resource *rs_new(const ResourceUUID &uuid) {
        return new OM_SmAgent(uuid);
    }
};

class OM_DmContainer : public DmContainer
{
  public:
    OM_DmContainer();
    virtual ~OM_DmContainer() {}

    virtual Error agent_register(const NodeUuid       &uuid,
                                 const FdspNodeRegPtr  msg,
                                 NodeAgent::pointer   *out);

    static inline OM_DmContainer::pointer agt_cast_ptr(RsContainer::pointer ptr) {
        return static_cast<OM_DmContainer *>(get_pointer(ptr));
    }
  protected:
    boost::shared_ptr<OM_ControlRespHandler> ctrlRspHndlr;

    virtual Resource *rs_new(const ResourceUUID &uuid) {
        return new OM_DmAgent(uuid);
    }
};

class OM_AmContainer : public AmContainer
{
  public:
    OM_AmContainer();
    virtual ~OM_AmContainer() {}

    virtual Error agent_register(const NodeUuid       &uuid,
                                 const FdspNodeRegPtr  msg,
                                 NodeAgent::pointer   *out);

    static inline OM_AmContainer::pointer agt_cast_ptr(RsContainer::pointer ptr) {
        return static_cast<OM_AmContainer *>(get_pointer(ptr));
    }
  protected:
    boost::shared_ptr<OM_ControlRespHandler> ctrlRspHndlr;

    virtual Resource *rs_new(const ResourceUUID &uuid) {
        return new OM_AmAgent(uuid);
    }
};

class OM_NodeContainer : public DomainContainer
{
  public:
    OM_NodeContainer();
    virtual ~OM_NodeContainer();

    /**
     * Return SM/DM/AM container or SM/DM/AM agent for the local domain.
     */
    inline OM_SmContainer::pointer om_sm_nodes() {
        return OM_SmContainer::agt_cast_ptr(dc_sm_nodes);
    }
    inline OM_AmContainer::pointer om_am_nodes() {
        return OM_AmContainer::agt_cast_ptr(dc_am_nodes);
    }
    inline OM_DmContainer::pointer om_dm_nodes() {
        return OM_DmContainer::agt_cast_ptr(dc_dm_nodes);
    }
    inline OM_SmAgent::pointer om_sm_agent(const NodeUuid &uuid) {
        return OM_SmAgent::agt_cast_ptr(dc_sm_nodes->agent_info(uuid));
    }
    inline OM_AmAgent::pointer om_am_agent(const NodeUuid &uuid) {
        return OM_AmAgent::agt_cast_ptr(dc_am_nodes->agent_info(uuid));
    }
    inline OM_DmAgent::pointer om_dm_agent(const NodeUuid &uuid) {
        return OM_DmAgent::agt_cast_ptr(dc_dm_nodes->agent_info(uuid));
    }

  private:
    friend class OM_NodeDomainMod;

    virtual void om_bcast_new_node(NodeAgent::pointer node, const FdspNodeRegPtr ref);
    virtual void om_update_node_list(NodeAgent::pointer node, const FdspNodeRegPtr ref);
};

/**
 * -------------------------------------------------------------------------------------
 * Cluster domain manager.  Manage all nodes connected and known to the domain.
 * These nodes may not be in ClusterMap membership.
 * -------------------------------------------------------------------------------------
 */
class OM_NodeDomainMod : public Module
{
  public:
    explicit OM_NodeDomainMod(char const *const name);
    virtual ~OM_NodeDomainMod();

    static OM_NodeDomainMod *om_local_domain();
    static inline fds_bool_t om_in_test_mode() {
        return om_local_domain()->om_test_mode;
    }
    /**
     * Accessor methods to retrive the local node domain.  Retyping it here to avoid
     * using multiple inheritance for this class.
     */
    inline OM_NodeContainer *om_domain_ctrl() {
        return om_locDomain;
    }
    inline OM_SmContainer::pointer om_sm_nodes() {
        return om_locDomain->om_sm_nodes();
    }
    inline OM_DmContainer::pointer om_dm_nodes() {
        return om_locDomain->om_dm_nodes();
    }
    inline OM_AmContainer::pointer om_am_nodes() {
        return om_locDomain->om_am_nodes();
    }
    inline OM_SmAgent::pointer om_sm_agent(const NodeUuid &uuid) {
        return om_locDomain->om_sm_agent(uuid);
    }
    inline OM_DmAgent::pointer om_dm_agent(const NodeUuid &uuid) {
        return om_locDomain->om_dm_agent(uuid);
    }
    inline OM_AmAgent::pointer om_am_agent(const NodeUuid &uuid) {
        return om_locDomain->om_am_agent(uuid);
    }

    /**
     * Register node info to the domain manager.
     * @return ERR_OK if success, ERR_DUPLICATE if node already
     * registered; ERR_UUID_EXISTS if this is a new node, but 
     * its name produces UUID that already mapped to an existing node
     * name (should ask the user to pick another node name).
     */
    virtual Error
    om_reg_node_info(const NodeUuid &uuid, const FdspNodeRegPtr msg);

    /**
     * Unregister the node matching uuid from the domain manager.
     */
    virtual Error
    om_del_node_info(const NodeUuid& uuid, const std::string& node_name);

    /**
     * Updates cluster map membership and does DLT
     */
    virtual void om_update_cluster();
    virtual void om_persist_node_info(fds_uint32_t node_idx);
    virtual void om_update_cluster_map();

    /**
     * Module methods
     */
    virtual int  mod_init(SysParams const *const param);
    virtual void mod_startup();
    virtual void mod_shutdown();

  protected:
    fds_bool_t                       om_test_mode;
    OM_NodeContainer                *om_locDomain;
};

/**
 * control response handler
 */
class OM_ControlRespHandler : public FDS_ProtocolInterface::
FDSP_ControlPathRespIf {
  public:
    explicit OM_ControlRespHandler();

    void NotifyAddVolResp(
        const FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
        const FDS_ProtocolInterface::FDSP_NotifyVolType& not_add_vol_resp);
    void NotifyAddVolResp(
        FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
        FDS_ProtocolInterface::FDSP_NotifyVolTypePtr& not_add_vol_resp);

    void NotifyRmVolResp(
        const FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
        const FDS_ProtocolInterface::FDSP_NotifyVolType& not_rm_vol_resp);
    void NotifyRmVolResp(
        FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
        FDS_ProtocolInterface::FDSP_NotifyVolTypePtr& not_rm_vol_resp);

    void NotifyModVolResp(
        const FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
        const FDS_ProtocolInterface::FDSP_NotifyVolType& not_mod_vol_resp);
    void NotifyModVolResp(
        FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
        FDS_ProtocolInterface::FDSP_NotifyVolTypePtr& not_mod_vol_resp);

    void AttachVolResp(
        const FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
        const FDS_ProtocolInterface::FDSP_AttachVolType& atc_vol_resp);
    void AttachVolResp(
        FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
        FDS_ProtocolInterface::FDSP_AttachVolTypePtr& atc_vol_resp);

    void DetachVolResp(
        const FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
        const FDS_ProtocolInterface::FDSP_AttachVolType& dtc_vol_resp);
    void DetachVolResp(
        FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
        FDS_ProtocolInterface::FDSP_AttachVolTypePtr& dtc_vol_resp);

    void NotifyNodeAddResp(
        const FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
        const FDS_ProtocolInterface::FDSP_Node_Info_Type& node_info_resp);
    void NotifyNodeAddResp(
        FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
        FDS_ProtocolInterface::FDSP_Node_Info_TypePtr& node_info_resp);

    void NotifyNodeRmvResp(
        const FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
        const FDS_ProtocolInterface::FDSP_Node_Info_Type& node_info_resp);
    void NotifyNodeRmvResp(
        FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
        FDS_ProtocolInterface::FDSP_Node_Info_TypePtr& node_info_resp);

    void NotifyDLTUpdateResp(
        const FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
        const FDS_ProtocolInterface::FDSP_DLT_Data_Type& dlt_info_resp);
    void NotifyDLTUpdateResp(
        FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
        FDS_ProtocolInterface::FDSP_DLT_Data_TypePtr& dlt_info_resp);

    void NotifyDMTUpdateResp(
        const FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
        const FDS_ProtocolInterface::FDSP_DMT_Type& dmt_info_resp);
    void NotifyDMTUpdateResp(
        FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
        FDS_ProtocolInterface::FDSP_DMT_TypePtr& dmt_info_resp);
  private:
        // TODO(Andrew): Add ptr back to resource manager.
};

extern OM_NodeDomainMod      gl_OMNodeDomainMod;

}  // namespace fds
#endif  // SOURCE_ORCH_MGR_INCLUDE_OMRESOURCES_H_
