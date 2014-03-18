/*
 * Copyright 2013 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_PLATFORM_NODE_INVENTORY_H_
#define SOURCE_INCLUDE_PLATFORM_NODE_INVENTORY_H_

#include <string>
#include <boost/shared_ptr.hpp>
#include <fds_err.h>
#include <fds_resource.h>
#include <fds_module.h>
#include <platform/platform-rpc.h>

namespace fds {

typedef fpi::FDSP_RegisterNodeType     FdspNodeReg;
typedef fpi::FDSP_RegisterNodeTypePtr  FdspNodeRegPtr;
typedef fpi::FDSP_NodeState            FdspNodeState;
typedef fpi::FDSP_MgrIdType            FdspNodeType;
typedef fpi::FDSP_ActivateNodeTypePtr  FdspNodeActivatePtr;

typedef boost::shared_ptr<fpi::FDSP_ControlPathReqClient>     NodeAgentCpReqtSessPtr;
typedef boost::shared_ptr<fpi::FDSP_OMControlPathReqClient>   NodeAgentCpOmClientPtr;
typedef boost::shared_ptr<PlatRpcResp>                        OmRespDispatchPtr;

typedef boost::shared_ptr<fpi::FDSP_DataPathReqClient>        NodeAgentDpClientPtr;
typedef boost::shared_ptr<PlatDataPathResp>                   NodeAgentDpRespPtr;

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
    // typedef boost::shared_ptr<NodeInvData>       pointer;
    // typedef boost::shared_ptr<const NodeInvData> const_ptr;

    // TODO(Andrew): Add back a better checksum library
    // Sha1Digest               nd_checksum;
    NodeUuid                 nd_uuid;
    NodeUuid                 nd_service_uuid;
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
    fds_uint64_t             nd_dlt_version;
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
    inline const NodeInvData *get_inventory_data() const {
        return node_inv;
    }
    inline FdspNodeState node_state() const {
        return node_inv->nd_node_state;
    }
    inline const node_capability_t &node_capability() const {
        return node_inv->nd_capability;
    }
    inline const fds_uint64_t node_dlt_version() const {
      return node_inv->nd_dlt_version;
    }

    /**
     * Fill in the inventory for this agent based on data provided by the message.
     */
    void node_fill_inventory(const FdspNodeRegPtr msg);
    void node_update_inventory(const FdspNodeRegPtr msg);
    void set_node_state(FdspNodeState state);
    void set_node_dlt_version(fds_uint64_t dlt_version);

    /**
     * Format the node info pkt with data from this agent obj.
     */
    virtual void init_msg_hdr(fpi::FDSP_MsgHdrTypePtr msgHdr) const;
    virtual void init_node_info_pkt(fpi::FDSP_Node_Info_TypePtr pkt) const;
    virtual void init_node_reg_pkt(fpi::FDSP_RegisterNodeTypePtr pkt) const;

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

class PmAgent : public NodeAgent
{
  public:
    typedef boost::intrusive_ptr<PmAgent> pointer;
    typedef boost::intrusive_ptr<const PmAgent> const_ptr;

    PmAgent(const NodeUuid &uuid) : NodeAgent(uuid) {}
    virtual ~PmAgent() {}

    static inline PmAgent::pointer agt_cast_ptr(NodeAgent::pointer ptr) {
        return static_cast<PmAgent *>(get_pointer(ptr));
    }
};

class SmAgent : public NodeAgent
{
  public:
    typedef boost::intrusive_ptr<SmAgent> pointer;
    typedef boost::intrusive_ptr<const SmAgent> const_ptr;

    SmAgent(const NodeUuid &uuid);
    virtual ~SmAgent();

    static inline SmAgent::pointer agt_cast_ptr(NodeAgent::pointer ptr) {
        return static_cast<SmAgent *>(get_pointer(ptr));
    }

    virtual void
    sm_handshake(boost::shared_ptr<netSessionTbl> net, NodeAgentDpRespPtr sm_resp);

  protected:
    netDataPathClientSession  *sm_sess;
    NodeAgentDpClientPtr       sm_reqt;
    std::string                sm_sess_id;
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

    OmAgent(const NodeUuid &uuid);
    virtual ~OmAgent();

    static inline OmAgent::pointer agt_cast_ptr(NodeAgent::pointer ptr) {
        return static_cast<OmAgent *>(get_pointer(ptr));
    }
    /**
     * Packet format functions.
     */
    void init_msg_hdr(fpi::FDSP_MsgHdrTypePtr msgHdr) const;
    void init_node_reg_pkt(fpi::FDSP_RegisterNodeTypePtr pkt) const;
    void om_register_node(fpi::FDSP_RegisterNodeTypePtr);

    virtual void
    om_handshake(boost::shared_ptr<netSessionTbl> net,
                 OmRespDispatchPtr                om_disp,
                 std::string                      om_ip,
                 fds_uint32_t                     om_port);

  protected:
    netOMControlPathClientSession *om_sess;        /** the rpc session to OM.  */
    NodeAgentCpOmClientPtr         om_reqt;        /**< handle to send reqt to OM.  */
    std::string                    om_sess_id;
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
    template <typename T>
    void agent_foreach(T arg, void (*fn)(T arg, NodeAgent::pointer elm)) {
        for (fds_uint32_t i = 0; i < rs_cur_idx; i++) {
            NodeAgent::pointer cur = NodeAgent::agt_cast_ptr(rs_array[i]);
            if (rs_array[i] != NULL) {
                (*fn)(arg, cur);
            }
        }
    }
    template <typename T1, typename T2>
    void agent_foreach(T1 a1, T2 a2, void (*fn)(T1, T2, NodeAgent::pointer elm)) {
        for (fds_uint32_t i = 0; i < rs_cur_idx; i++) {
            NodeAgent::pointer cur = NodeAgent::agt_cast_ptr(rs_array[i]);
            if (rs_array[i] != NULL) {
                (*fn)(a1, a2, cur);
            }
        }
    }
    template <typename T1, typename T2, typename T3>
    void agent_foreach(T1 a1, T2 a2, T3 a3,
                       void (*fn)(T1, T2, T3, NodeAgent::pointer elm)) {
        for (fds_uint32_t i = 0; i < rs_cur_idx; i++) {
            NodeAgent::pointer cur = NodeAgent::agt_cast_ptr(rs_array[i]);
            if (rs_array[i] != NULL) {
                (*fn)(a1, a2, a3, cur);
            }
        }
    }
    template <typename T1, typename T2, typename T3, typename T4>
    void agent_foreach(T1 a1, T2 a2, T3 a3, T4 a4,
                       void (*fn)(T1, T2, T3, T4, NodeAgent::pointer elm)) {
        for (fds_uint32_t i = 0; i < rs_cur_idx; i++) {
            NodeAgent::pointer cur = NodeAgent::agt_cast_ptr(rs_array[i]);
            if (rs_array[i] != NULL) {
                (*fn)(a1, a2, a3, a4, cur);
            }
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

    /**
     * Establish RPC connection with the remte agent.
     */
    virtual void
    agent_handshake(boost::shared_ptr<netSessionTbl> net,
                    NodeAgentDpRespPtr               resp,
                    NodeAgent::pointer               agent);

  protected:
    boost::shared_ptr<netSessionTbl>   ac_cpSessTbl;

    virtual ~AgentContainer();
    AgentContainer(FdspNodeType id);
};

class PmContainer : public AgentContainer
{
  public:
    typedef boost::intrusive_ptr<PmContainer> pointer;
    PmContainer(FdspNodeType id) : AgentContainer(id) {}

  protected:
    virtual ~PmContainer() {}
    virtual Resource *rs_new(const ResourceUUID &uuid) {
        return new PmAgent(uuid);
    }
};

class SmContainer : public AgentContainer
{
  public:
    typedef boost::intrusive_ptr<SmContainer> pointer;
    SmContainer(FdspNodeType id);

    virtual void
    agent_handshake(boost::shared_ptr<netSessionTbl> net,
                    NodeAgentDpRespPtr               resp,
                    NodeAgent::pointer               agent);

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
    typedef boost::intrusive_ptr<DomainContainer> pointer;
    typedef boost::intrusive_ptr<const DomainContainer> const_ptr;

    virtual ~DomainContainer();
    explicit DomainContainer(char const *const name);
    DomainContainer(char const *const       name,
                    OmAgent::pointer        master,
                    SmContainer::pointer    sm,
                    DmContainer::pointer    dm,
                    AmContainer::pointer    am,
                    PmContainer::pointer    pm,
                    OmContainer::pointer    om,
                    AgentContainer::pointer node);

    virtual Error dc_register_node(const NodeUuid       &uuid,
                                   const FdspNodeRegPtr  msg,
                                   NodeAgent::pointer   *agent);

    virtual Error dc_unregister_node(const NodeUuid &uuid, const std::string &name);
    virtual Error dc_unregister_agent(const NodeUuid &uuid, FdspNodeType type);

    /**
     * Return the agent container matching with the node type.
     */
    AgentContainer::pointer dc_container_frm_msg(FdspNodeType node_type);

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

    inline PmContainer::pointer dc_get_pm_nodes() {
        return dc_pm_nodes;
    }
    inline void dc_set_pm_nodes(PmContainer::pointer pm) {
        fds_assert(dc_pm_nodes == NULL);
        dc_pm_nodes = pm;
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
    PmContainer::pointer     dc_pm_nodes;
    AgentContainer::pointer  dc_nodes;

  private:
    mutable boost::atomic<int>  rs_refcnt;
    friend void intrusive_ptr_add_ref(const DomainContainer *x) {
        x->rs_refcnt.fetch_add(1, boost::memory_order_relaxed);
    }
    friend void intrusive_ptr_release(const DomainContainer *x) {
        if (x->rs_refcnt.fetch_sub(1, boost::memory_order_release) == 1) {
            boost::atomic_thread_fence(boost::memory_order_acquire);
            delete x;
        }
    }
};

}  // namespace fds
#endif  // SOURCE_INCLUDE_INVENTORY_NODE_INVENTORY_H_
