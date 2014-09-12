/*
 * Copyright 2013 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_PLATFORM_NODE_INVENTORY_H_
#define SOURCE_INCLUDE_PLATFORM_NODE_INVENTORY_H_

#include <vector>
#include <string>
#include <ostream>
#include <boost/shared_ptr.hpp>
#include <fds_ptr.h>
#include <fds_error.h>
#include <fds_resource.h>
#include <fds_module.h>
#include <serialize.h>
#include <platform/platform-rpc.h>

// Forward declarations
namespace FDS_ProtocolInterface {
    class PlatNetSvcClient;
}  // namespace FDS_ProtocolInterface

namespace fpi = FDS_ProtocolInterface;

namespace fds {
struct node_data;
class ShmObjRO;
class SmSvcEp;
class DmSvcEp;
class AmSvcEp;
class OmSvcEp;
class PmSvcEp;
class EpSvc;
class EpSvcImpl;
class EpSvcHandle;
class AgentContainer;

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

const fds_uint32_t NODE_DO_PROXY_ALL_SVCS = (NODE_SVC_SM | NODE_SVC_DM | NODE_SVC_AM);

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
    fds_uint64_t   ssd_capacity;
    fds_uint32_t   ssd_latency_max;
    fds_uint32_t   ssd_latency_min;
} node_capability_t;

class NodeInvData
{
  public:
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
    fds_uint32_t             nd_metasync_port;
    fds_uint32_t             nd_disk_type;
    node_capability_t        nd_capability;

    std::string              nd_node_name;
    std::string              nd_node_root;
    FdspNodeType             nd_node_type;
    FdspNodeState            nd_node_state;
    fds_uint64_t             nd_dlt_version;
    friend std::ostream& operator<< (std::ostream &os, const NodeInvData& node);
};

struct NodeServices : serialize::Serializable {
    NodeUuid sm,dm,am;

    inline void reset() {
        sm.uuid_set_val(0);
        dm.uuid_set_val(0);
        am.uuid_set_val(0);
    }

    uint32_t virtual write(serialize::Serializer*  s) const;
    uint32_t virtual read(serialize::Deserializer* s);
    friend std::ostream& operator<< (std::ostream &os, const NodeServices& node);
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

    inline std::string get_node_root() const {
        return node_inv->nd_node_root;
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
    inline FdspNodeType node_get_svc_type() { return node_svc_type; }

    /**
     * Fill in the inventory for this agent based on data provided by the message.
     */
    void svc_info_frm_shm(fpi::SvcInfo *svc) const;
    void node_info_frm_shm(struct node_data *out) const;
    void node_fill_shm_inv(const ShmObjRO *shm, int ro, int rw, FdspNodeType id);
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
    virtual void init_stor_cap_msg(fpi::StorCapMsg *msg) const;
    virtual void init_plat_info_msg(fpi::NodeInfoMsg *msg) const;
    virtual void init_node_info_msg(fpi::NodeInfoMsg *msg) const;

    /**
     * Convert from message format to POD type used in shared memory.
     */
    static void node_info_msg_to_shm(const NodeInfoMsg *msg, struct node_data *rec);
    static void node_info_msg_frm_shm(bool am, int ro_idx, NodeInfoMsg *msg);
    static void node_stor_cap_to_shm(const fpi::StorCapMsg *msg, struct node_stor_cap *);
    static void node_stor_cap_frm_shm(fpi::StorCapMsg *msg, const struct node_stor_cap *);

  protected:
    NodeInvData             *node_inv;
    FdspNodeType             node_svc_type;
    int                      node_ro_idx;
    int                      node_rw_idx;

    virtual ~NodeInventory() {}
    explicit NodeInventory(const NodeUuid &uuid)
        : Resource(uuid), node_inv(NULL),
          node_svc_type(fpi::FDSP_PLATFORM), node_ro_idx(-1), node_rw_idx(-1) {}

    const ShmObjRO *node_shm_ctrl() const;
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

    /**
     * Return the storage weight -- currently capacity in GB / 10
     */
    virtual fds_uint64_t node_stor_weight() const;
    virtual void         node_set_weight(fds_uint64_t weight);

    /**
     * Establish/shutdown the communication with the peer.
     */
    virtual void node_agent_up();
    virtual void node_agent_down();

    /**
     * Return the RPC handler for services bound to the control port.
     */
    virtual boost::shared_ptr<fpi::FDSP_ControlPathReqClient>
    node_ctrl_rpc(boost::intrusive_ptr<EpSvcHandle> *eph);

    virtual boost::shared_ptr<fpi::PlatNetSvcClient>
    node_svc_rpc(boost::intrusive_ptr<EpSvcHandle> *eph);

  protected:
    friend class AgentContainer;
    boost::intrusive_ptr<EpSvcHandle>                 nd_eph;
    boost::intrusive_ptr<EpSvcHandle>                 nd_ctrl_eph;
    boost::shared_ptr<fpi::PlatNetSvcClient>          nd_svc_rpc;
    boost::shared_ptr<fpi::FDSP_ControlPathReqClient> nd_ctrl_rpc;

    virtual ~NodeAgent();
    explicit NodeAgent(const NodeUuid &uuid);

    virtual void agent_publish_ep();
    void agent_bind_ep(boost::intrusive_ptr<EpSvcImpl>, boost::intrusive_ptr<EpSvc>);
};

/**
 * Down cast a node agent intrusive pointer.
 */
template <class T>
static inline T *agt_cast_ptr(NodeAgent::pointer agt) {
    return static_cast<T *>(get_pointer(agt));
}

template <class T>
static inline T *agt_cast_ptr(Resource::pointer rs) {
    return static_cast<T *>(get_pointer(rs));
}

/*
 * -------------------------------------------------------------------------------------
 * Specific node agent type setup for peer to peer communication.
 * -------------------------------------------------------------------------------------
 */
class PmAgent : public NodeAgent
{
  public:
    typedef boost::intrusive_ptr<PmAgent> pointer;
    typedef boost::intrusive_ptr<const PmAgent> const_ptr;

    virtual ~PmAgent();
    virtual void agent_bind_ep();
    virtual void agent_svc_info(fpi::NodeSvcInfo *out) const;

    PmAgent(const NodeUuid &uuid);
    boost::intrusive_ptr<PmSvcEp> agent_ep_svc();

  protected:
    boost::intrusive_ptr<PmSvcEp>      pm_ep_svc;

    void agent_svc_fillin(fpi::NodeSvcInfo *,
                          const struct node_data *, fpi::FDSP_MgrIdType) const;
};

class SmAgent : public NodeAgent
{
  public:
    typedef boost::intrusive_ptr<SmAgent> pointer;
    typedef boost::intrusive_ptr<const SmAgent> const_ptr;

    virtual ~SmAgent();
    virtual void agent_bind_ep();
    virtual void
    sm_handshake(boost::shared_ptr<netSessionTbl> net, NodeAgentDpRespPtr sm_resp);

    SmAgent(const NodeUuid &uuid);
    boost::intrusive_ptr<SmSvcEp> agent_ep_svc();

    NodeAgentDpClientPtr get_sm_client();
    std::string get_sm_sess_id();

  protected:
    netDataPathClientSession      *sm_sess;
    NodeAgentDpClientPtr           sm_reqt;
    std::string                    sm_sess_id;
    boost::intrusive_ptr<SmSvcEp>  sm_ep_svc;
};

class DmAgent : public NodeAgent
{
  public:
    typedef boost::intrusive_ptr<DmAgent> pointer;
    typedef boost::intrusive_ptr<const DmAgent> const_ptr;

    virtual ~DmAgent();
    virtual void agent_bind_ep();

    DmAgent(const NodeUuid &uuid);
    boost::intrusive_ptr<DmSvcEp> agent_ep_svc();

  protected:
    boost::intrusive_ptr<DmSvcEp>  dm_ep_svc;
};

class AmAgent : public NodeAgent
{
  public:
    typedef boost::intrusive_ptr<AmAgent> pointer;
    typedef boost::intrusive_ptr<const AmAgent> const_ptr;

    virtual ~AmAgent();
    virtual void agent_bind_ep();

    AmAgent(const NodeUuid &uuid);
    boost::intrusive_ptr<AmSvcEp> agent_ep_svc();

  protected:
    boost::intrusive_ptr<AmSvcEp>  am_ep_svc;
};

class OmAgent : public NodeAgent
{
  public:
    typedef boost::intrusive_ptr<OmAgent> pointer;
    typedef boost::intrusive_ptr<const OmAgent> const_ptr;

    virtual ~OmAgent();
    virtual void agent_bind_ep();

    OmAgent(const NodeUuid &uuid);
    boost::intrusive_ptr<OmSvcEp> agent_ep_svc();

    /**
     * Packet format functions.
     */
    void init_msg_hdr(fpi::FDSP_MsgHdrTypePtr msgHdr) const;
    void init_node_reg_pkt(fpi::FDSP_RegisterNodeTypePtr pkt) const;
    void om_register_node(fpi::FDSP_RegisterNodeTypePtr);

    /**
     * TODO(Vy): remove this API and use the net service one.
     */
    virtual void
    om_handshake(boost::shared_ptr<netSessionTbl> net,
                 OmRespDispatchPtr                om_disp,
                 std::string                      om_ip,
                 fds_uint32_t                     om_port);

  protected:
    netOMControlPathClientSession *om_sess;        /** the rpc session to OM.  */
    NodeAgentCpOmClientPtr         om_reqt;        /**< handle to send reqt to OM.  */
    std::string                    om_sess_id;
    boost::intrusive_ptr<OmSvcEp>  om_ep_svc;
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
            NodeAgent::pointer cur = agt_cast_ptr<NodeAgent>(rs_array[i]);
            if (rs_array[i] != NULL) {
                (*fn)(arg, cur);
            }
        }
    }
    template <typename T1, typename T2>
    void agent_foreach(T1 a1, T2 a2, void (*fn)(T1, T2, NodeAgent::pointer elm)) {
        for (fds_uint32_t i = 0; i < rs_cur_idx; i++) {
            NodeAgent::pointer cur = agt_cast_ptr<NodeAgent>(rs_array[i]);
            if (rs_array[i] != NULL) {
                (*fn)(a1, a2, cur);
            }
        }
    }
    template <typename T1, typename T2, typename T3>
    void agent_foreach(T1 a1, T2 a2, T3 a3,
                       void (*fn)(T1, T2, T3, NodeAgent::pointer elm)) {
        for (fds_uint32_t i = 0; i < rs_cur_idx; i++) {
            NodeAgent::pointer cur = agt_cast_ptr<NodeAgent>(rs_array[i]);
            if (rs_array[i] != NULL) {
                (*fn)(a1, a2, a3, cur);
            }
        }
    }
    template <typename T1, typename T2, typename T3, typename T4>
    void agent_foreach(T1 a1, T2 a2, T3 a3, T4 a4,
                       void (*fn)(T1, T2, T3, T4, NodeAgent::pointer elm)) {
        for (fds_uint32_t i = 0; i < rs_cur_idx; i++) {
            NodeAgent::pointer cur = agt_cast_ptr<NodeAgent>(rs_array[i]);
            if (rs_array[i] != NULL) {
                (*fn)(a1, a2, a3, a4, cur);
            }
        }
    }

    /**
     * iterator that returns number of agents that completed function successfully
     * Only iterates through active agents (for which we called agent_activate, and
     * did not call agent_deactivate).
     */
    template <typename T>
    fds_uint32_t agent_ret_foreach(T arg, Error (*fn)(T arg, NodeAgent::pointer elm)) {
        fds_uint32_t count = 0;
        for (fds_uint32_t i = 0; i < rs_cur_idx; i++) {
            Error err(ERR_OK);
            NodeAgent::pointer cur = agt_cast_ptr<NodeAgent>(rs_array[i]);
            if ((rs_array[i] != NULL) &&
                (rs_get_resource(cur->get_uuid()) != NULL)) {
                err = (*fn)(arg, cur);
                if (err.ok()) {
                    ++count;
                }
            }
        }
        return count;
    }
    template <typename T1, typename T2, typename T3>
    fds_uint32_t agent_ret_foreach(T1 a1, T2 a2, T3 a3,
                                   Error (*fn)(T1, T2, T3, NodeAgent::pointer elm)) {
        fds_uint32_t count = 0;
        for (fds_uint32_t i = 0; i < rs_cur_idx; i++) {
            Error err(ERR_OK);
            NodeAgent::pointer cur = agt_cast_ptr<NodeAgent>(rs_array[i]);
            if (rs_array[i] != NULL) {
                err = (*fn)(a1, a2, a3, cur);
                if (err.ok()) {
                    ++count;
                }
            }
        }
        return count;
    }

    /**
     * Return the generic NodeAgent::pointer from index position or its uuid.
     */
    inline NodeAgent::pointer agent_info(fds_uint32_t idx) {
        if (idx < rs_cur_idx) {
            return agt_cast_ptr<NodeAgent>(rs_array[idx]);
        }
        return NULL;
    }
    inline NodeAgent::pointer agent_info(const NodeUuid &uuid) {
        return agt_cast_ptr<NodeAgent>(rs_get_resource(uuid));
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
                                 NodeAgent::pointer   *out,
                                 bool                  activate = true);
    virtual Error agent_unregister(const NodeUuid &uuid, const std::string &name);

    /**
     * @param shm (i) - the shared memory segment to access inventory data.
     * @param out (i/o) - NULL if the agent obj hasn't been allocated.
     * @param ro, rw (i) - indices to get the node inventory data RO or RW (-1 invalid).
     * @param bool (i) - true if want to register, publish the node/service endpoint.
     */
    virtual void
    agent_register(const ShmObjRO *shm, NodeAgent::pointer *out, int ro, int rw);

    /**
     * Establish RPC connection with the remte agent.
     */
    virtual void
    agent_handshake(boost::shared_ptr<netSessionTbl> net,
                    NodeAgentDpRespPtr               resp,
                    NodeAgent::pointer               agent);

  protected:
    FdspNodeType                       ac_id;
    // NetSession fields, need to remove.
    boost::shared_ptr<netSessionTbl>   ac_cpSessTbl;

    virtual ~AgentContainer();
    AgentContainer(FdspNodeType id);
};

/**
 * Down cast a node container intrusive pointer.
 */
template <class T>
static inline T *agt_cast_ptr(AgentContainer::pointer ptr) {
    return static_cast<T *>(get_pointer(ptr));
}

template <class T>
static inline T *agt_cast_ptr(RsContainer::pointer ptr) {
    return static_cast<T *>(get_pointer(ptr));
}

class PmContainer : public AgentContainer
{
  public:
    typedef boost::intrusive_ptr<PmContainer> pointer;
    PmContainer(FdspNodeType id) : AgentContainer(id) { ac_id = fpi::FDSP_PLATFORM; }

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
    DmContainer(FdspNodeType id) : AgentContainer(id) { ac_id = fpi::FDSP_DATA_MGR; }

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
    AmContainer(FdspNodeType id) : AgentContainer(id) { ac_id = fpi::FDSP_STOR_HVISOR; }

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
    OmContainer(FdspNodeType id) : AgentContainer(id) { ac_id = fpi::FDSP_ORCH_MGR; }

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
                    AgentContainer::pointer sm,
                    AgentContainer::pointer dm,
                    AgentContainer::pointer am,
                    AgentContainer::pointer pm,
                    AgentContainer::pointer om);

    /**
     * Register and unregister an agent to the domain.
     * TODO(Vy): retire this function.
     */
    virtual Error dc_register_node(const NodeUuid       &uuid,
                                   const FdspNodeRegPtr  msg,
                                   NodeAgent::pointer   *agent);

    virtual void
    dc_register_node(const ShmObjRO     *shm,
                     NodeAgent::pointer *agent, int ro, int rw, fds_uint32_t mask = 0);

    virtual Error dc_unregister_node(const NodeUuid &uuid, const std::string &name);
    virtual Error dc_unregister_agent(const NodeUuid &uuid, FdspNodeType type);

    /**
     * Return the agent container matching with the node type.
     */
    AgentContainer::pointer dc_container_frm_msg(FdspNodeType node_type);

    /**
     * Get service info in all nodes connecting to this domain.
     */
    void dc_node_svc_info(fpi::DomainNodes &ret);

    /**
     * Find the node agent matching with the given uuid.
     */
    NodeAgent::pointer dc_find_node_agent(const NodeUuid &uuid);

    /**
     * Domain iteration plugin
     */
    inline void dc_foreach_am(ResourceIter *iter) {
        dc_am_nodes->rs_foreach(iter);
    }
    inline void dc_foreach_sm(ResourceIter *iter) {
        dc_sm_nodes->rs_foreach(iter);
    }
    inline void dc_foreach_dm(ResourceIter *iter) {
        dc_dm_nodes->rs_foreach(iter);
    }
    inline void dc_foreach_pm(ResourceIter *iter) {
        dc_pm_nodes->rs_foreach(iter);
    }
    inline void dc_foreach_om(ResourceIter *iter) {
        dc_om_nodes->rs_foreach(iter);
    }
    /**
     * Get methods for different containers.
     */
    inline SmContainer::pointer dc_get_sm_nodes() {
        return agt_cast_ptr<SmContainer>(dc_sm_nodes);
    }
    inline DmContainer::pointer dc_get_dm_nodes() {
        return agt_cast_ptr<DmContainer>(dc_dm_nodes);
    }
    inline AmContainer::pointer dc_get_am_nodes() {
        return agt_cast_ptr<AmContainer>(dc_am_nodes);
    }
    inline PmContainer::pointer dc_get_pm_nodes() {
        return agt_cast_ptr<PmContainer>(dc_pm_nodes);
    }
    inline OmContainer::pointer dc_get_om_nodes() {
        return agt_cast_ptr<OmContainer>(dc_om_nodes);
    }

  protected:
    OmAgent::pointer         dc_om_master;
    AgentContainer::pointer  dc_om_nodes;
    AgentContainer::pointer  dc_sm_nodes;
    AgentContainer::pointer  dc_dm_nodes;
    AgentContainer::pointer  dc_am_nodes;
    AgentContainer::pointer  dc_pm_nodes;

  private:
    INTRUSIVE_PTR_DEFS(DomainContainer, rs_refcnt);
};

}  // namespace fds
#endif  // SOURCE_INCLUDE_PLATFORM_NODE_INVENTORY_H_
