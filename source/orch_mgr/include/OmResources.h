/*
 * Copyright 2013 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_ORCH_MGR_INCLUDE_OMRESOURCES_H_
#define SOURCE_ORCH_MGR_INCLUDE_OMRESOURCES_H_

#include <vector>
#include <string>
#include <list>
#include <fstream>
#include <unordered_map>
#include <fds_typedefs.h>
#include <fds_err.h>
#include <OmVolume.h>
#include <NetSession.h>
#include <fds_placement_table.h>
#include <platform/node-inventory.h>
#include <dlt.h>

namespace fds {

class PerfStats;
class FdsAdminCtrl;
class OM_NodeDomainMod;
class OM_ControlRespHandler;

typedef boost::shared_ptr<fpi::FDSP_ControlPathReqClient> NodeAgentCpReqClientPtr;
typedef netControlPathClientSession *                     NodeAgentCpSessionPtr;

/**
 * TODO(Vy): temp. interface for now to define generic node message.
 */
typedef struct om_node_msg_s
{
    fpi::FDSP_MsgCodeType             nd_msg_code;
    union {
        fpi::FDSP_ThrottleMsgTypePtr *nd_throttle;
        fpi::FDSP_DMT_TypePtr        *nd_dmt_tab;
    } u;
} om_node_msg_t;

/**
 * Agent interface to communicate with the remote node.  This is the communication
 * end-point to the node.
 *
 * It's normal that the node agent is there but the transport may not be availble.
 * We'll provide methods to establish the transport in the background and error
 * handling model when the transport is broken.
 */
class OM_SmAgent : public NodeAgent
{
  public:
    typedef boost::intrusive_ptr<OM_SmAgent> pointer;
    typedef boost::intrusive_ptr<const OM_SmAgent> const_ptr;

    explicit OM_SmAgent(const NodeUuid &uuid);
    virtual ~OM_SmAgent();

    static inline OM_SmAgent::pointer agt_cast_ptr(Resource::pointer ptr) {
        return static_cast<OM_SmAgent *>(get_pointer(ptr));
    }
    inline fpi::FDSP_MgrIdType om_agent_type() const {
        return ndMyServId;
    }
    void setCpSession(NodeAgentCpSessionPtr session, fpi::FDSP_MgrIdType myId);

    /**
     * Returns uuid of the node running this service
     */
    inline NodeUuid get_parent_uuid() const {
        return parentUuid;
    }

    /**
     * TODO(Vy): see if we can move this to NodeAgent, which is generic to all processes.
     * Returns the client end point for the node. The function
     * is not constanct since the member pointer returned
     * is mutable by the caller.
     */
    inline NodeAgentCpReqClientPtr getCpClient() const {
        return ndCpClient;
    }
    inline NodeAgentCpReqClientPtr getCpClient(std::string *id) const {
        *id = ndSessionId;
        return ndCpClient;
    }

    /**
     * Send this node agent info as an event to notify the peer node.
     * TODO(Vy): it would be a cleaner interface to:
     * - Formalized messages in inheritance tree.
     * - API to format a message.
     * - API to send a message.
     */
    virtual void om_send_myinfo(NodeAgent::pointer peer);
    virtual void om_send_node_cmd(const om_node_msg_t &msg);

    virtual void om_send_reg_resp(const Error &err);
    virtual void om_send_vol_cmd(VolumeInfo::pointer vol, fpi::FDSP_MsgCodeType cmd);
    virtual void om_send_vol_cmd(VolumeInfo::pointer    vol,
                                 std::string           *vname,
                                 fpi::FDSP_MsgCodeType  cmd);

    virtual void om_send_dlt(const DLT *curDlt);
    virtual void init_msg_hdr(FDSP_MsgHdrTypePtr msgHdr) const;

  protected:
    NodeAgentCpSessionPtr   ndCpSession;
    std::string             ndSessionId;
    fpi::FDSP_MgrIdType     ndMyServId;
    NodeAgentCpReqClientPtr ndCpClient;

    NodeUuid                parentUuid;  // Uuid of the node running the service

    virtual int node_calc_stor_weight();
};

typedef OM_SmAgent                      OM_DmAgent;
typedef OM_SmAgent                      OM_AmAgent;
typedef std::list<OM_SmAgent::pointer>  NodeList;


/**
 * Agent interface to communicate with the platform service on the remote node.
 * This is the communication end-point to the node.
 */
class OM_PmAgent : public NodeAgent
{
  public:
    typedef boost::intrusive_ptr<OM_PmAgent> pointer;
    typedef boost::intrusive_ptr<const OM_PmAgent> const_ptr;

    explicit OM_PmAgent(const NodeUuid &uuid);
    virtual ~OM_PmAgent();

    static inline OM_PmAgent::pointer agt_cast_ptr(Resource::pointer ptr) {
        return static_cast<OM_PmAgent *>(get_pointer(ptr));
    }
    void setCpSession(NodeAgentCpSessionPtr session);

    /**
     * TODO(Vy): see if we can move this to NodeAgent, which is generic to all processes.
     * Returns the client end point for the node. The function
     * is not constanct since the member pointer returned
     * is mutable by the caller.
     */
    inline NodeAgentCpReqClientPtr getCpClient() const {
        return ndCpClient;
    }
    inline NodeAgentCpReqClientPtr getCpClient(std::string *id) const {
        *id = ndSessionId;
        return ndCpClient;
    }

    /**
     * Allowing only one type of service per Platform
     */
    fds_bool_t service_exists(FDS_ProtocolInterface::FDSP_MgrIdType svc_type) const;
    /**
     * Send 'activate services' message to Platform
     */
    void send_activate_services(fds_bool_t activate_sm,
                                fds_bool_t activate_dm,
                                fds_bool_t activate_am);

    /**
     * Tell platform Agent about new active service
     * Platform Agent keeps a pointer to active services node agents
     * These functions just set/reset pointers to appropriate service
     * agents and do not actually register or deregister node agents
     */
    Error handle_register_service(FDS_ProtocolInterface::FDSP_MgrIdType svc_type,
                                  NodeAgent::pointer svc_agent);
    void handle_unregister_service(FDS_ProtocolInterface::FDSP_MgrIdType svc_type);

    /**
     * If any service running on this node matches given 'svc_uuid'
     * handle its deregister
     */
    void handle_unregister_service(const NodeUuid& svc_uuid);

    inline OM_SmAgent::pointer get_sm_service() {
        return activeSmAgent;
    }
    inline OM_DmAgent::pointer get_dm_service() {
        return activeDmAgent;
    }
    inline OM_AmAgent::pointer get_am_service() {
        return activeAmAgent;
    }

    virtual void init_msg_hdr(FDSP_MsgHdrTypePtr msgHdr) const;

  protected:
    NodeAgentCpSessionPtr   ndCpSession;
    std::string             ndSessionId;
    NodeAgentCpReqClientPtr ndCpClient;

    OM_SmAgent::pointer     activeSmAgent;  // pointer to active SM service or NULL
    OM_DmAgent::pointer     activeDmAgent;  // pointer to active DM service or NULL
    OM_AmAgent::pointer     activeAmAgent;  // pointer to active AM service or NULL
};

// -------------------------------------------------------------------------------------
// Common OM Node Container
// -------------------------------------------------------------------------------------
class OM_PmContainer : public PmContainer
{
  public:
    typedef boost::intrusive_ptr<OM_PmContainer> pointer;

    OM_PmContainer();
    virtual ~OM_PmContainer() {}

    virtual Error agent_register(const NodeUuid       &uuid,
                                 const FdspNodeRegPtr  msg,
                                 NodeAgent::pointer   *out);

    /**
     * Returns true if this node can accept new service.
     * Node (Platform) that will run the service must be discovered
     * and set to active state
     */
    fds_bool_t check_new_service(const NodeUuid& pm_uuid,
                                 FDS_ProtocolInterface::FDSP_MgrIdType svc_role);

    /**
     * Tell platform agent with uuid 'pm_uuid' about new service registered
     */
    Error handle_register_service(const NodeUuid& pm_uuid,
                                  FDS_ProtocolInterface::FDSP_MgrIdType svc_role,
                                  NodeAgent::pointer svc_agent);
    /**
     * Makes sure Platform agents do not point on unregistered services
     * Will search all Platform agents for a service with uuid 'uuid'
     */
    void handle_unregister_service(const NodeUuid& svc_uuid);

    static inline OM_PmContainer::pointer agt_cast_ptr(RsContainer::pointer ptr) {
        return static_cast<OM_PmContainer *>(get_pointer(ptr));
    }

  protected:
    uint nodeNameCounter = 0;
    boost::shared_ptr<OM_ControlRespHandler> ctrlRspHndlr;

    virtual Resource *rs_new(const ResourceUUID &uuid) {
        return new OM_PmAgent(uuid);
    }
};

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
    inline OM_PmContainer::pointer om_pm_nodes() {
        return OM_PmContainer::agt_cast_ptr(dc_pm_nodes);
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
    inline OM_PmAgent::pointer om_pm_agent(const NodeUuid &uuid) {
        return OM_PmAgent::agt_cast_ptr(dc_pm_nodes->agent_info(uuid));
    }
    inline float om_get_cur_throttle_level() {
        return om_cur_throttle_level;
    }
    inline FdsAdminCtrl *om_get_admin_ctrl() {
        return om_admin_ctrl;
    }
    /**
     * Volume proxy functions.
     */
    inline VolumeContainer::pointer om_vol_mgr() {
        return om_volumes;
    }
    inline int om_create_vol(const FDSP_MsgHdrTypePtr &hdr,
                             const FdspCrtVolPtr      &creat_msg) {
        return om_volumes->om_create_vol(hdr, creat_msg);
    }
    inline int om_delete_vol(const FdspDelVolPtr &del_msg) {
        return om_volumes->om_delete_vol(del_msg);
    }
    inline int om_modify_vol(const FdspModVolPtr &mod_msg) {
        return om_volumes->om_modify_vol(mod_msg);
    }
    inline int om_attach_vol(const FDSP_MsgHdrTypePtr &hdr,
                             const FdspAttVolCmdPtr   &attach) {
        return om_volumes->om_attach_vol(hdr, attach);
    }
    inline int om_detach_vol(const FDSP_MsgHdrTypePtr &hdr,
                             const FdspAttVolCmdPtr   &detach) {
        return om_volumes->om_detach_vol(hdr, detach);
    }
    inline void om_test_bucket(const FdspMsgHdrPtr     &hdr,
                               const FdspTestBucketPtr &req) {
        return om_volumes->om_test_bucket(hdr, req);
    }

    inline bool addVolume(const VolumeDesc& desc) {
        return om_volumes->addVolume(desc);
    }

    virtual void om_set_throttle_lvl(float level);
    virtual void om_send_bucket_stats(fds_uint32_t, std::string, fds_uint32_t);
    virtual void om_handle_perfstats_from_am(const FDSP_VolPerfHistListType &list,
                                             const std::string start_timestamp);

    virtual void om_bcast_tier_policy(fpi::FDSP_TierPolicyPtr policy);
    virtual void om_bcast_tier_audit(fpi::FDSP_TierPolicyAuditPtr audit);
    virtual void om_bcast_vol_list(NodeAgent::pointer node);
    virtual void om_bcast_vol_create(VolumeInfo::pointer vol);
    virtual void om_bcast_vol_modify(VolumeInfo::pointer vol);
    virtual void om_bcast_vol_delete(VolumeInfo::pointer vol);
    virtual void om_bcast_vol_tier_policy(const FDSP_TierPolicyPtr &tier);
    virtual void om_bcast_vol_tier_audit(const FDSP_TierPolicyAuditPtr &tier);
    virtual void om_bcast_throttle_lvl(float throttle_level);
    virtual void om_bcast_dlt(const DLT* curDlt);
    /**
     * conditional broadcast to platform (nodes) to
     * activate SM and DM services on those nodes, but only
     * to those nodes which are in discovered state
     */
    virtual void om_cond_bcast_activate_services(fds_bool_t activate_sm,
                                                 fds_bool_t activate_dm,
                                                 fds_bool_t activate_am);

  private:
    friend class OM_NodeDomainMod;

    virtual void om_add_capacity(NodeAgent::pointer node);
    virtual void om_bcast_new_node(NodeAgent::pointer node, const FdspNodeRegPtr ref);
    virtual void om_update_node_list(NodeAgent::pointer node, const FdspNodeRegPtr ref);

    FdsAdminCtrl             *om_admin_ctrl;
    VolumeContainer::pointer  om_volumes;
    float                     om_cur_throttle_level;

    /**
     * TODO(Vy): move this to DMT class.
     */
    fds_uint64_t              om_dmt_ver;
    fds_uint32_t              om_dmt_width;
    fds_uint32_t              om_dmt_depth;
    FdsDmt                   *om_curDmt;
    fds_mutex                 om_dmt_mtx;

    void om_init_domain();
    virtual void om_bcast_dmt_table();
    virtual void om_round_robin_dmt();

    /**
     * Recent history of perf stats OM receives from AM nodes.
     */
    PerfStats                *am_stats;
    /**
     * TODO(Anna): this is temp JSON file, will remove as soon as we implement
     * real stats polling from CLI.
     */
    std::ofstream             json_file;
    void om_printStatsToJsonFile();
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
    static inline OM_NodeContainer *om_loc_domain_ctrl() {
        return om_local_domain()->om_locDomain;
    }
    /**
     * Accessor methods to retrive the local node domain.  Retyping it here to avoid
     * using multiple inheritance for this class.
     */
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
    virtual Error om_del_node_info(const NodeUuid& uuid,
                                   const std::string& node_name);

    /**
     * Notification that OM received migration done message from
     * node with uuid 'uuid' for dlt version 'dlt_version'
     */
    virtual Error om_recv_migration_done(const NodeUuid& uuid,
                                         fds_uint64_t dlt_version);

    /**
     * Notification that OM received DLT update response from
     * node with uuid 'uuid' for dlt version 'dlt_version'
     */
    virtual Error om_recv_sm_dlt_commit_resp(const NodeUuid& uuid,
                                             fds_uint64_t dlt_version);

    /**
     * Updates cluster map membership and does DLT
     */
    virtual void om_update_cluster();
    virtual void om_persist_node_info(fds_uint32_t node_idx);

    /**
     * Domain support.
     */
    virtual int om_create_domain(const FdspCrtDomPtr &crt_domain);
    virtual int om_delete_domain(const FdspCrtDomPtr &crt_domain);

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
class OM_ControlRespHandler : public fpi:: FDSP_ControlPathRespIf {
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

    void NotifyNodeActiveResp(
        const FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
        const FDS_ProtocolInterface::FDSP_Node_Info_Type& node_info_resp);
    void NotifyNodeActiveResp(
        FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
        FDS_ProtocolInterface::FDSP_Node_Info_TypePtr& node_info_resp);

    void NotifyDLTUpdateResp(
        const FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
        const FDS_ProtocolInterface::FDSP_DLT_Resp_Type& dlt_resp);
    void NotifyDLTUpdateResp(
        FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
        FDS_ProtocolInterface::FDSP_DLT_Resp_TypePtr& dlt_resp);

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
