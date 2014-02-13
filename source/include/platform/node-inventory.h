/*
 * Copyright 2013 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_PLATFORM_NODE_INVENTORY_H_
#define SOURCE_INCLUDE_PLATFORM_NODE_INVENTORY_H_

#include <string>
#include <boost/shared_ptr.hpp>
#include <fds_err.h>
#include <fds_resource.h>
#include <fds_typedefs.h>
#include <fds_module.h>
#include <NetSession.h>

namespace fds {

namespace fpi = FDS_ProtocolInterface;
typedef fpi::FDSP_RegisterNodeType     FdspNodeReg;
typedef fpi::FDSP_RegisterNodeTypePtr  FdspNodeRegPtr;
typedef fpi::FDSP_NodeState            FdspNodeState;
typedef fpi::FDSP_MgrIdType            FdspNodeType;

/**
 * POD types for common node inventory.
 */
typedef struct _node_capability_t
{
    fds_uint64_t   disk_capacity;
    fds_uint32_t   disk_iops_max;
    fds_uint32_t   disk_iops_min;
    fds_uint32_t   disk_latency_max;
    fds_uint32_t   disk_latency_min;
    fds_uint32_t   ssd_iops_max;
    fds_uint32_t   ssd_iops_min;
    fds_uint32_t   ssd_capacity;
    fds_uint32_t   ssd_latency_max;
    fds_uint32_t   ssd_latency_min;
} node_capability_t;

class NodeInvData
{
  public:
    typedef boost::shared_ptr<NodeInvData>       pointer;
    typedef boost::shared_ptr<const NodeInvData> const_ptr;

    Sha1Digest               nd_checksum;
    NodeUuid                 nd_uuid;
    fds_uint64_t             nd_gbyte_cap;       /**< capacity in GB unit */

    /* TODO: (vy) just porting from NodeInfo now. */
    fds_uint32_t             nd_ip_addr;
    std::string              nd_ip_str;
    fds_uint32_t             nd_data_port;
    fds_uint32_t             nd_ctrl_port;
    fds_uint32_t             nd_migration_port;
    fds_uint32_t             nd_disk_type;
    node_capability_t        nd_capability;

    std::string              nd_node_name;
    FdspNodeType             nd_node_type;
    FdspNodeState            nd_node_state;
};

/**
 * Replacement for NodeInfo object.
 */
class NodeInventory : public Resource
{
  public:
    typedef boost::intrusive_ptr<NodeInventory> pointer;
    typedef boost::intrusive_ptr<const NodeInventory> const_ptr;

    inline NodeUuid get_uuid() const {
        return rs_get_uuid();
    }
    inline std::string get_node_name() const {
        return node_inv->nd_node_name;
    }
    inline std::string get_ip_str() const {
        return node_inv->nd_ip_str;
    }
    inline fds_uint32_t get_ctrl_port() const {
        return node_inv->nd_ctrl_port;
    }
    inline NodeInvData::const_ptr get_inventory_data() const {
        return NodeInvData::const_ptr(node_inv);
    }
    inline FdspNodeState node_state() const {
        return node_inv->nd_node_state;
    }
    inline const node_capability_t &node_capability() const {
        return node_inv->nd_capability;
    }
    /**
     * Fill in the inventory for this agent based on data provided by the message.
     */
    void node_fill_inventory(const FdspNodeRegPtr msg);
    void node_update_inventory(const FdspNodeRegPtr msg);
    void set_node_state(FdspNodeState state);

    /**
     * Format the message header to default values.
     */
    virtual void init_msg_hdr(fpi::FDSP_MsgHdrTypePtr msgHdr) const;

    /**
     * Format the node info pkt with data from this agent obj.
     */
    virtual void init_node_info_pkt(fpi::FDSP_Node_Info_TypePtr pkt) const;

  protected:
    const NodeInvData       *node_inv;

    virtual ~NodeInventory() {}
    explicit NodeInventory(const NodeUuid &uuid) : Resource(uuid) {}
    void node_set_inventory(NodeInvData const *const inv);
};

/**
 * --------------------------------------------------------------------------------------
 * Agent interface to communicate with the remote node.  This is the communication
 * end-point to the node.
 *
 * It's normal that the node agent is there but the transport may not be availble.
 * We'll provide methods to establish the transport in the background and error
 * handling model when the transport is broken.
 * --------------------------------------------------------------------------------------
 */
class NodeAgent : public NodeInventory
{
  public:
    typedef boost::intrusive_ptr<NodeAgent> pointer;
    typedef boost::intrusive_ptr<const NodeAgent> const_ptr;

    static inline NodeAgent::pointer agt_cast_ptr(Resource::pointer ptr) {
        return static_cast<NodeAgent *>(get_pointer(ptr));
    }
    /**
     * Return the storage weight of the node in normalized unit from 0...1000
     */
    virtual fds_uint32_t node_stor_weight() const;
    virtual void         node_set_weight(fds_uint64_t weight);

  protected:
    virtual ~NodeAgent() {}
    explicit NodeAgent(const NodeUuid &uuid) : NodeInventory(uuid) {}
};

class SmAgent : public NodeAgent
{
  public:
    typedef boost::intrusive_ptr<SmAgent> pointer;
    typedef boost::intrusive_ptr<const SmAgent> const_ptr;

    SmAgent(const NodeUuid &uuid) : NodeAgent(uuid) {}
    virtual ~SmAgent() {}

    static inline SmAgent::pointer agt_cast_ptr(NodeAgent::pointer ptr) {
        return static_cast<SmAgent *>(get_pointer(ptr));
    }
};

class DmAgent : public NodeAgent
{
  public:
    typedef boost::intrusive_ptr<DmAgent> pointer;
    typedef boost::intrusive_ptr<const DmAgent> const_ptr;

    DmAgent(const NodeUuid &uuid) : NodeAgent(uuid) {}
    virtual ~DmAgent() {}

    static inline DmAgent::pointer agt_cast_ptr(NodeAgent::pointer ptr) {
        return static_cast<DmAgent *>(get_pointer(ptr));
    }
};

class AmAgent : public NodeAgent
{
  public:
    typedef boost::intrusive_ptr<AmAgent> pointer;
    typedef boost::intrusive_ptr<const AmAgent> const_ptr;

    AmAgent(const NodeUuid &uuid) : NodeAgent(uuid) {}
    virtual ~AmAgent() {}

    static inline AmAgent::pointer agt_cast_ptr(NodeAgent::pointer ptr) {
        return static_cast<AmAgent *>(get_pointer(ptr));
    }
};

class OmAgent : public NodeAgent
{
  public:
    typedef boost::intrusive_ptr<OmAgent> pointer;
    typedef boost::intrusive_ptr<const OmAgent> const_ptr;

    OmAgent(const NodeUuid &uuid) : NodeAgent(uuid) {}
    virtual ~OmAgent() {}

    static inline OmAgent::pointer agt_cast_ptr(NodeAgent::pointer ptr) {
        return static_cast<OmAgent *>(get_pointer(ptr));
    }
};

// -------------------------------------------------------------------------------------
// Common Agent Containers
// -------------------------------------------------------------------------------------
class AgentContainer : public RsContainer
{
  public:
    typedef boost::intrusive_ptr<AgentContainer> pointer;

    /**
     * Iter loop to extract NodeAgent ptr:
     */
    static inline NodeAgent::pointer agt_from_iter(RsContainer::const_iterator it) {
        return static_cast<NodeAgent *>(get_pointer(*it));
    }
    template <typename T>
    void agent_foreach(T arg, void (*fn)(T arg, NodeAgent::pointer elm)) {
        for (const_iterator it = cbegin(); it != cend(); it++) {
            (*fn)(arg, agt_from_iter(it));
        }
    }
    template <typename T1, typename T2>
    void agent_foreach(T1 a1, T2 a2, void (*fn)(T1, T2, NodeAgent::pointer elm)) {
        for (const_iterator it = cbegin(); it != cend(); it++) {
            (*fn)(a1, a2, agt_from_iter(it));
        }
    }
    template <typename T1, typename T2, typename T3>
    void agent_foreach(T1 a1, T2 a2, T3 a3,
                       void (*fn)(T1, T2, T3, NodeAgent::pointer elm)) {
        for (const_iterator it = cbegin(); it != cend(); it++) {
            (*fn)(a1, a2, a3, agt_from_iter(it));
        }
    }
    template <typename T1, typename T2, typename T3, typename T4>
    void agent_foreach(T1 a1, T2 a2, T3 a3, T4 a4,
                       void (*fn)(T1, T2, T3, T4, NodeAgent::pointer elm)) {
        for (const_iterator it = cbegin(); it != cend(); it++) {
            (*fn)(a1, a2, a3, a4, agt_from_iter(it));
        }
    }

    /**
     * Return the generic NodeAgent::pointer from index position or its uuid.
     */
    inline NodeAgent::pointer agent_info(fds_uint32_t idx) {
        if (idx < rs_cur_idx) {
            return NodeAgent::agt_cast_ptr(rs_array[idx]);
        }
        return NULL;
    }
    inline NodeAgent::pointer agent_info(const NodeUuid &uuid) {
        return NodeAgent::agt_cast_ptr(rs_get_resource(uuid));
    }

    /**
     * Activate the agent obj so that it can be looked up by name or uuid.
     */
    virtual void agent_activate(NodeAgent::pointer agent) {
        rs_register(agent);
    }
    virtual void agent_deactivate(NodeAgent::pointer agent) {
        rs_unregister(agent);
    }

    /**
     * Register and unregister a node agent.
     */
    virtual Error agent_register(const NodeUuid       &uuid,
                                 const FdspNodeRegPtr  msg,
                                 NodeAgent::pointer   *out);
    virtual Error agent_unregister(const NodeUuid &uuid, const std::string &name);

  protected:
    boost::shared_ptr<netSessionTbl>   ac_cpSessTbl;

    virtual ~AgentContainer();
    AgentContainer(FdspNodeType id);
};

class SmContainer : public AgentContainer
{
  public:
    typedef boost::intrusive_ptr<SmContainer> pointer;
    SmContainer(FdspNodeType id) : AgentContainer(id) {}

    static inline SmAgent::pointer agt_from_iter(RsContainer::const_iterator it) {
        return static_cast<SmAgent *>(get_pointer(*it));
    }

  protected:
    virtual ~SmContainer() {}
    virtual Resource *rs_new(const ResourceUUID &uuid) {
        return new SmAgent(uuid);
    }
};

class DmContainer : public AgentContainer
{
  public:
    typedef boost::intrusive_ptr<DmContainer> pointer;
    DmContainer(FdspNodeType id) : AgentContainer(id) {}

    static inline DmAgent::pointer agt_from_iter(RsContainer::const_iterator it) {
        return static_cast<DmAgent *>(get_pointer(*it));
    }

  protected:
    virtual ~DmContainer() {}
    virtual Resource *rs_new(const ResourceUUID &uuid) {
        return new DmAgent(uuid);
    }
};

class AmContainer : public AgentContainer
{
  public:
    typedef boost::intrusive_ptr<AmContainer> pointer;
    AmContainer(FdspNodeType id) : AgentContainer(id) {}

    static inline AmAgent::pointer agt_from_iter(RsContainer::const_iterator it) {
        return static_cast<AmAgent *>(get_pointer(*it));
    }

  protected:
    virtual ~AmContainer() {}
    virtual Resource *rs_new(const ResourceUUID &uuid) {
        return new AmAgent(uuid);
    }
};

class OmContainer : public AgentContainer
{
  public:
    typedef boost::intrusive_ptr<OmContainer> pointer;
    OmContainer(FdspNodeType id) : AgentContainer(id) {}

    static inline OmAgent::pointer agt_from_iter(RsContainer::const_iterator it) {
        return static_cast<OmAgent *>(get_pointer(*it));
    }

  protected:
    virtual ~OmContainer() {}
    virtual Resource *rs_new(const ResourceUUID &uuid) {
        return new OmAgent(uuid);
    }
};

// -------------------------------------------------------------------------------------
// Common Domain Container
// -------------------------------------------------------------------------------------
class DomainContainer
{
  public:
    virtual ~DomainContainer();
    explicit DomainContainer(char const *const name);
    DomainContainer(char const *const       name,
                    OmAgent::pointer        master,
                    SmContainer::pointer    sm,
                    DmContainer::pointer    dm,
                    AmContainer::pointer    am,
                    OmContainer::pointer    om,
                    AgentContainer::pointer node);

    virtual Error dc_register_node(const NodeUuid       &uuid,
                                   const FdspNodeRegPtr  msg,
                                   NodeAgent::pointer   *agent);

    virtual Error dc_unregister_node(const NodeUuid &uuid, const std::string &name);
    virtual Error dc_unregister_agent(const NodeUuid &uuid, FdspNodeType type);

    /**
     * Get/set methods for different containers.
     */
    inline SmContainer::pointer dc_get_sm_nodes() {
        return dc_sm_nodes;
    }
    inline void dc_set_sm_nodes(SmContainer::pointer sm) {
        fds_assert(dc_sm_nodes == NULL);
        dc_sm_nodes = sm;
    }

    inline DmContainer::pointer dc_get_dm_nodes() {
        return dc_dm_nodes;
    }
    inline void dc_set_dm_nodes(DmContainer::pointer dm) {
        fds_assert(dc_dm_nodes == NULL);
        dc_dm_nodes = dm;
    }

    inline AmContainer::pointer dc_get_am_nodes() {
        return dc_am_nodes;
    }
    inline void dc_set_am_nodes(AmContainer::pointer am) {
        fds_assert(dc_am_nodes == NULL);
        dc_am_nodes = am;
    }

    inline OmContainer::pointer dc_get_om_nodes() {
        return dc_om_nodes;
    }
    inline void dc_set_om_nodes(OmContainer::pointer om) {
        fds_assert(dc_om_nodes == NULL);
        dc_om_nodes = om;
    }

  protected:
    OmAgent::pointer         dc_om_master;
    OmContainer::pointer     dc_om_nodes;
    SmContainer::pointer     dc_sm_nodes;
    DmContainer::pointer     dc_dm_nodes;
    AmContainer::pointer     dc_am_nodes;
    AgentContainer::pointer  dc_nodes;

    AgentContainer::pointer dc_container_frm_msg(FdspNodeType node_type);
};

}  // namespace fds
#endif  // SOURCE_INCLUDE_INVENTORY_NODE_INVENTORY_H_
