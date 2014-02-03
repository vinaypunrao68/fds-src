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
#include <fdsp/FDSP_types.h>
#include <concurrency/Mutex.h>

namespace fds {

class OM_NodeContainer;
class OM_ClusDomainMod;

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
    virtual int node_stor_weight() const;

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

  protected:
    friend class OM_NodeContainer;

    Sha1Digest               nd_checksum;
    NodeUuid                 nd_uuid;
    fds_uint32_t             nd_index;           /**< idx in container.   */
    fds_uint64_t             nd_gbyte_cap;       /**< capacity in GB unit */
    fds_mutex                nd_mtx;             /**< protecting mutex.   */

    /* TODO: (vy) just porting from NodeInfo now. */
    fds_uint32_t             nd_ip_addr;
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

  protected:
    friend class OM_NodeContainer;
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

    virtual void om_del_node_info(const NodeUuid *uuid);
    virtual void om_persist_node_info(fds_uint32_t node_idx);

    /**
     * Module methods
     */
    virtual int  mod_init(SysParams const *const param);
    virtual void mod_startup();
    virtual void mod_shutdown();

  protected:
};

extern OM_NodeDomainMod      gl_OMNodeDomainMod;

}  // namespace fds
#endif  // SOURCE_ORCH_MGR_INCLUDE_OMRESOURCES_H_
