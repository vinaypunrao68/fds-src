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
#include <fds_module.h>
#include <fds_resource.h>
#include <concurrency/Mutex.h>
#include <NetSession.h>

namespace fds {

class OM_NodeContainer;
class OM_ClusDomainMod;
class OM_ControlRespHandler;

typedef FDS_ProtocolInterface::FDSP_RegisterNodeType     FdspNodeReg;
typedef FDS_ProtocolInterface::FDSP_RegisterNodeTypePtr  FdspNodeRegPtr;
typedef FDS_ProtocolInterface::FDSP_NodeState            FdspNodeState;
typedef FDS_ProtocolInterface::FDSP_MgrIdType            FdspNodeType;
typedef std::string                                      FdspNodeName;

/**
 * Replacement for NodeInfo object.
 */
class NodeInventory : public Resource
{
  public:
    typedef boost::intrusive_ptr<NodeInventory> pointer;
    typedef boost::intrusive_ptr<const NodeInventory> const_ptr;

    explicit NodeInventory(const NodeUuid &uuid);
    virtual ~NodeInventory();

    void node_name(std::string *name) const {}

    /**
     * Update the node inventory with new info.
     * @param uuid - (i) null if the node already has a valid uuid.
     */
    virtual void
    node_update_info(const NodeUuid *uuid, const FdspNodeRegPtr msg);

    /**
     * Return the storage weight of the node in normalized unit from 0...1000
     */
    virtual fds_uint32_t node_stor_weight() const;

    /**
     * Set the storage weight of the node
     */
    virtual void node_set_weight(fds_uint64_t weight);

    /**
     * Return the mutex protecting this object.
     */
    inline fds_mutex *node_mutex() {
        return &nd_mtx;
    }
    inline fds_uint32_t node_index() const {
        return nd_index;
    }
    inline NodeUuid get_uuid() const {
        return nd_uuid;
    }

    inline std::string get_ip_str() const {
        return nd_ip_str;
    }

    inline fds_uint32_t get_ctrl_port() const {
        return nd_ctrl_port;
    }

  protected:
    friend class OM_NodeContainer;

    Sha1Digest               nd_checksum;
    NodeUuid                 nd_uuid;
    fds_uint32_t             nd_index;           /**< idx in container.   */
    fds_uint64_t             nd_gbyte_cap;       /**< capacity in GB unit */
    fds_mutex                nd_mtx;             /**< protecting mutex.   */

    /* TODO: (vy) just porting from NodeInfo now. */
    fds_uint32_t             nd_ip_addr;
    std::string              nd_ip_str;
    fds_uint32_t             nd_data_port;
    fds_uint32_t             nd_ctrl_port;
    fds_uint32_t             nd_disk_iops_max;
    fds_uint32_t             nd_disk_iops_min;
    fds_uint32_t             nd_disk_latency_max;
    fds_uint32_t             nd_disk_latency_min;
    fds_uint32_t             nd_ssd_iops_max;
    fds_uint32_t             nd_ssd_iops_min;
    fds_uint32_t             nd_ssd_capacity;
    fds_uint32_t             nd_ssd_latency_max;
    fds_uint32_t             nd_ssd_latency_min;
    fds_uint32_t             nd_disk_type;

    FdspNodeName             nd_node_name;
    FdspNodeType             nd_node_type;
    FdspNodeState            nd_node_state;

    virtual int node_calc_stor_weight();
};

typedef boost::shared_ptr<FDS_ProtocolInterface::FDSP_ControlPathReqClient>
        NodeAgentCpReqClientPtr;
typedef boost::shared_ptr<netControlPathClientSession>
        NodeAgentCpSessionPtr;

/**
 * Agent interface to communicate with the remote node.  This is the communication
 * end-point to the node.
 *
 * It's normal that the node agent is there but the transport may not be availble.
 * We'll provide methods to establish the transport in the background and error
 * handling model when the transport is broken.
 */
class NodeAgent : public NodeInventory
{
  public:
    typedef boost::intrusive_ptr<NodeAgent> pointer;
    typedef boost::intrusive_ptr<const NodeAgent> const_ptr;

    explicit NodeAgent(const NodeUuid &uuid);
    /**
     * This constructor can probably be removed as it's
     * only needed to for testing and will be generally
     * unuseful in other scenarios.
     */
    NodeAgent(const NodeUuid &uuid, fds_uint64_t nd_weight);
    virtual ~NodeAgent();

    void setCpSession(NodeAgentCpSessionPtr session);
    /**
     * Returns the client end point for the node. The function
     * is not constanct since the member pointer returned
     * is mutable by the caller.
     */
    NodeAgentCpReqClientPtr getCpClient();

  protected:
    friend class            OM_NodeContainer;
    NodeAgentCpSessionPtr   ndCpSession;
    std::string             ndSessionId;
    NodeAgentCpReqClientPtr ndCpClient;
};

/**
 * Type that maps a node's UUID to its agent object.
 */
typedef std::unordered_map<NodeUuid, NodeAgent::pointer, UuidHash> NodeMap;

typedef std::list<NodeAgent::pointer>      NodeList;
typedef std::vector<NodeAgent::pointer>    NodeArray;

// ----------------------------------------------------------------------------
// Common OM Node Container
// ----------------------------------------------------------------------------
class OM_NodeContainer : public RsContainer
{
  public:
    typedef boost::intrusive_ptr<OM_NodeContainer> pointer;
    typedef NodeMap::const_iterator const_iterator;

    OM_NodeContainer();
    virtual ~OM_NodeContainer();

    /**
     * Iterate through the list of nodes by index 0...n to retrieve their
     * agent objects.
     */
    inline int om_avail_nodes() {
        return node_cur_idx;
    }
    NodeAgent::pointer om_node_info(fds_uint32_t node_idx);
    NodeAgent::pointer om_node_info(const NodeUuid *uuid);

    /**
     * Iterate through the list using standard container iterator.
     */
    inline const_iterator cbegin() const {
        return node_map.cbegin();
    }
    inline const_iterator cend() const {
        return node_map.cend();
    }
    /**
     * Thread-safe version of the iterator.
     */
    const_iterator cend(const_iterator *it);

    virtual NodeAgent::pointer om_new_node();
    virtual void om_ref_node(NodeAgent::pointer node, fds_bool_t act = true);
    virtual void om_deref_node(NodeAgent::pointer node);
    virtual void om_activate_node(fds_uint32_t node_idx);
    virtual void om_deactivate_node(fds_uint32_t node_idx);

  protected:
    NodeMap                  node_map;
    fds_uint32_t             node_cur_idx;
    NodeArray                node_inuse;
    NodeList                 node_list;
    fds_mutex                node_mtx;

    NodeList                 node_up_pend;
    NodeList                 node_down_pend;
};

/**
 * Cluster domain manager.  Manage all nodes connected and known to the domain.
 * These nodes may not be in ClusterMap membership.
 */
class OM_NodeDomainMod : public Module, OM_NodeContainer
{
  public:
    explicit OM_NodeDomainMod(char const *const name);
    virtual ~OM_NodeDomainMod();

    static OM_NodeDomainMod *om_local_domain();

    /**
     * Register node info to the domain manager.
     */
    virtual void
    om_reg_node_info(const NodeUuid       *uuid,
                     const FdspNodeRegPtr msg);

    /**
     * Updates cluster map membership and does
     * DLT
     */
    virtual void om_update_cluster();

    virtual void om_del_node_info(const NodeUuid *uuid);
    virtual void om_persist_node_info(fds_uint32_t node_idx);
    virtual void om_update_cluster_map();

    /**
     * Module methods
     */
    virtual int  mod_init(SysParams const *const param);
    virtual void mod_startup();
    virtual void mod_shutdown();

  protected:
    boost::shared_ptr<OM_ControlRespHandler> ctrlRspHndlr;
    boost::shared_ptr<netSessionTbl> omcpSessTbl;

    fds_bool_t test_mode;
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
