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
#include <boost/msm/back/state_machine.hpp>
#include <fds_typedefs.h>
#include <fds_error.h>
#include <OmVolume.h>
#include <platform/node-inventory.h>
#include <fdsp/FDSP_ControlPathResp.h>
#include <dlt.h>
#include <fds_dmt.h>
#include <kvstore/configdb.h>

namespace FDS_ProtocolInterface {
    class FDSP_ControlPathReqClient;
    class FDSP_ControlPathRespProcessor;
    class FDSP_ControlPathRespIf;
}
typedef netClientSessionEx<fpi::FDSP_ControlPathReqClient,
                fpi::FDSP_ControlPathRespProcessor,
                fpi::FDSP_ControlPathRespIf> netControlPathClientSession;

namespace fds {

class PerfStats;
class FdsAdminCtrl;
class OM_NodeDomainMod;
class OM_ControlRespHandler;
struct NodeDomainFSM;

typedef boost::shared_ptr<fpi::FDSP_ControlPathReqClient> NodeAgentCpReqClientPtr;
typedef netControlPathClientSession *                     NodeAgentCpSessionPtr;
typedef boost::msm::back::state_machine<NodeDomainFSM> FSM_NodeDomain;

/**
 * Agent interface to communicate with the remote node.  This is the communication
 * end-point to the node.
 *
 * It's normal that the node agent is there but the transport may not be availble.
 * We'll provide methods to establish the transport in the background and error
 * handling model when the transport is broken.
 */
class OM_NodeAgent : public NodeAgent
{
  public:
    typedef boost::intrusive_ptr<OM_NodeAgent> pointer;
    typedef boost::intrusive_ptr<const OM_NodeAgent> const_ptr;

    explicit OM_NodeAgent(const NodeUuid &uuid, fpi::FDSP_MgrIdType type);
    virtual ~OM_NodeAgent();

    static inline OM_NodeAgent::pointer agt_cast_ptr(Resource::pointer ptr) {
        return static_cast<OM_NodeAgent *>(get_pointer(ptr));
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

    virtual void om_send_reg_resp(const Error &err);
    // this is the new function we shall try on using service layer
    virtual void om_send_node_throttle_lvl(fpi::FDSP_ThrottleMsgTypePtr);
    virtual Error om_send_vol_cmd(VolumeInfo::pointer vol,
                                  fpi::FDSPMsgTypeId      cmd_type,
                                  fpi::FDSP_NotifyVolFlag = fpi::FDSP_NOTIFY_VOL_NO_FLAG);
    virtual Error om_send_vol_cmd(VolumeInfo::pointer    vol,
                                  std::string           *vname,
                                  fpi::FDSPMsgTypeId      cmd_type,
                                  fpi::FDSP_NotifyVolFlag = fpi::FDSP_NOTIFY_VOL_NO_FLAG);
    void om_send_vol_cmd_resp(VolumeInfo::pointer     vol,
                      fpi::FDSPMsgTypeId      cmd_type,
                      EPSvcRequest* rpcReq,
                      const Error& error,
                      boost::shared_ptr<std::string> payload);

    virtual Error om_send_dlt(const DLT *curDlt);
    virtual Error om_send_dlt_close(fds_uint64_t cur_dlt_version);
    virtual Error om_send_dmt(const DMTPtr& curDmt);
    virtual Error om_send_dmt_close(fds_uint64_t dmt_version);
    virtual Error om_send_scavenger_cmd(fpi::FDSP_ScavengerCmd cmd);
    virtual Error om_send_pushmeta(fpi::FDSP_PushMetaPtr& meta_msg);
    virtual Error om_send_stream_reg_cmd(fds_int32_t regId,
                                         fds_bool_t bAll);
    virtual Error om_send_qosinfo(fds_uint64_t total_rate);
    virtual void init_msg_hdr(fpi::FDSP_MsgHdrTypePtr msgHdr) const;

  private:
    void om_send_one_stream_reg_cmd(const fpi::StreamingRegistrationMsg& reg,
                                    const NodeUuid& stream_dest_uuid);

  protected:
    NodeAgentCpSessionPtr   ndCpSession;
    std::string             ndSessionId;
    fpi::FDSP_MgrIdType     ndMyServId;
    NodeAgentCpReqClientPtr ndCpClient;
    NodeUuid                parentUuid;  // Uuid of the node running the service

    virtual int node_calc_stor_weight();
};

typedef OM_NodeAgent                    OM_SmAgent;
typedef OM_NodeAgent                    OM_DmAgent;
typedef OM_NodeAgent                    OM_AmAgent;
typedef std::list<OM_NodeAgent::pointer>  NodeList;

/**
 * Agent interface to communicate with the platform service on the remote node.
 * This is the communication end-point to the node.
 */
class OM_PmAgent : public OM_NodeAgent
{
  public:
    typedef boost::intrusive_ptr<OM_PmAgent> pointer;
    typedef boost::intrusive_ptr<const OM_PmAgent> const_ptr;

    explicit OM_PmAgent(const NodeUuid &uuid);
    virtual ~OM_PmAgent();

    static inline OM_PmAgent::pointer agt_cast_ptr(Resource::pointer ptr) {
        return static_cast<OM_PmAgent *>(get_pointer(ptr));
    }
    /**
     * Allowing only one type of service per Platform
     */
    fds_bool_t service_exists(FDS_ProtocolInterface::FDSP_MgrIdType svc_type) const;
    /**
     * Send 'activate services' message to Platform
     */
    Error send_activate_services(fds_bool_t activate_sm,
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

    /**
     * @return uuid of service that was unregistered
     */
    NodeUuid handle_unregister_service(FDS_ProtocolInterface::FDSP_MgrIdType svc_type);

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

    virtual void init_msg_hdr(fpi::FDSP_MsgHdrTypePtr msgHdr) const;

  protected:
    OM_SmAgent::pointer     activeSmAgent;  // pointer to active SM service or NULL
    OM_DmAgent::pointer     activeDmAgent;  // pointer to active DM service or NULL
    OM_AmAgent::pointer     activeAmAgent;  // pointer to active AM service or NULL
};

// -------------------------------------------------------------------------------------
// Common OM Node Container
// -------------------------------------------------------------------------------------
class OM_AgentContainer : public AgentContainer
{
  public:
    virtual Error agent_register(const NodeUuid       &uuid,
                                 const FdspNodeRegPtr  msg,
                                 NodeAgent::pointer   *out,
                                 bool                  activate = true);
    virtual Error agent_unregister(const NodeUuid &uuid, const std::string &name);

    virtual void agent_activate(NodeAgent::pointer agent);
    virtual void agent_deactivate(NodeAgent::pointer agent);

    /**
     * Move all pending nodes to addNodes and rmNodes
     * The second function only moves nodes that are in 'filter_nodes'
     * set and leaves other pending nodes pending
     */
    void om_splice_nodes_pend(NodeList *addNodes, NodeList *rmNodes);
    void om_splice_nodes_pend(NodeList *addNodes,
                              NodeList *rmNodes,
                              const NodeUuidSet& filter_nodes);

  protected:
    boost::shared_ptr<OM_ControlRespHandler> ctrlRspHndlr;

    NodeList                                 node_up_pend;
    NodeList                                 node_down_pend;

    explicit OM_AgentContainer(FdspNodeType id);
    virtual ~OM_AgentContainer() {}
};

class OM_PmContainer : public OM_AgentContainer
{
  public:
    typedef boost::intrusive_ptr<OM_PmContainer> pointer;

    OM_PmContainer();
    virtual ~OM_PmContainer() {}

    /**
     * Consult persistent database before registering the node agent.
     */
    virtual Error agent_register(const NodeUuid       &uuid,
                                 const FdspNodeRegPtr  msg,
                                 NodeAgent::pointer   *out,
                                 bool                  activate = true);
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
     * @param returns uuid of service that was unregistered
     */
    NodeUuid handle_unregister_service(const NodeUuid& node_uuid,
                                       const std::string& node_name,
                                       FDS_ProtocolInterface::FDSP_MgrIdType svc_type);

    static inline OM_PmContainer::pointer agt_cast_ptr(RsContainer::pointer ptr) {
        return static_cast<OM_PmContainer *>(get_pointer(ptr));
    }

  protected:
    uint nodeNameCounter = 0;

    virtual Resource *rs_new(const ResourceUUID &uuid) {
        return new OM_PmAgent(uuid);
    }
};

class OM_SmContainer : public OM_AgentContainer
{
  public:
    typedef boost::intrusive_ptr<OM_SmContainer> pointer;

    OM_SmContainer();
    virtual ~OM_SmContainer() {}

    static inline OM_SmContainer::pointer agt_cast_ptr(RsContainer::pointer ptr) {
        return static_cast<OM_SmContainer *>(get_pointer(ptr));
    }

  protected:
    virtual Resource *rs_new(const ResourceUUID &uuid) {
        return new OM_SmAgent(uuid, fpi::FDSP_STOR_MGR);
    }
};

class OM_DmContainer : public OM_AgentContainer
{
  public:
    typedef boost::intrusive_ptr<OM_DmContainer> pointer;

    OM_DmContainer();
    virtual ~OM_DmContainer() {}

    static inline OM_DmContainer::pointer agt_cast_ptr(RsContainer::pointer ptr) {
        return static_cast<OM_DmContainer *>(get_pointer(ptr));
    }

  protected:
    virtual Resource *rs_new(const ResourceUUID &uuid) {
        return new OM_DmAgent(uuid, fpi::FDSP_DATA_MGR);
    }
};

class OM_AmContainer : public OM_AgentContainer
{
  public:
    OM_AmContainer();
    virtual ~OM_AmContainer() {}

    static inline OM_AmContainer::pointer agt_cast_ptr(RsContainer::pointer ptr) {
        return static_cast<OM_AmContainer *>(get_pointer(ptr));
    }
  protected:
    virtual Resource *rs_new(const ResourceUUID &uuid) {
        return new OM_AmAgent(uuid, fpi::FDSP_STOR_HVISOR);
    }
};

// -----------------------------------------------------------------------------------
// Domain Container
// -----------------------------------------------------------------------------------
class OM_NodeContainer : public DomainContainer
{
  public:
    OM_NodeContainer();
    virtual ~OM_NodeContainer();

    typedef boost::intrusive_ptr<OM_NodeContainer> pointer;
    typedef boost::intrusive_ptr<const OM_NodeContainer> const_ptr;
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
    inline Error om_create_vol(const fpi::FDSP_MsgHdrTypePtr &hdr,
                               const FdspCrtVolPtr           &creat_msg,
                               fds_bool_t from_omcontrol_path) {
        return om_volumes->om_create_vol(hdr, creat_msg, from_omcontrol_path);
    }
    inline Error om_snap_vol(const fpi::FDSP_MsgHdrTypePtr &hdr,
                             const FdspCrtVolPtr      &snap_msg) {
        return om_volumes->om_snap_vol(hdr, snap_msg);
    }
    inline Error om_delete_vol(const fpi::FDSP_MsgHdrTypePtr &hdr,
                               const FdspDelVolPtr &del_msg) {
        return om_volumes->om_delete_vol(hdr, del_msg);
    }
    inline Error om_modify_vol(const FdspModVolPtr &mod_msg) {
        return om_volumes->om_modify_vol(mod_msg);
    }
    inline Error om_attach_vol(const fpi::FDSP_MsgHdrTypePtr &hdr,
                               const FdspAttVolCmdPtr   &attach) {
        return om_volumes->om_attach_vol(hdr, attach);
    }
    inline Error om_detach_vol(const fpi::FDSP_MsgHdrTypePtr &hdr,
                               const FdspAttVolCmdPtr   &detach) {
        return om_volumes->om_detach_vol(hdr, detach);
    }
    inline void om_test_bucket(const boost::shared_ptr<fpi::AsyncHdr>    &hdr,
                               const fpi::FDSP_TestBucket *req) {
        return om_volumes->om_test_bucket(hdr, req);
    }

    inline bool addVolume(const VolumeDesc& desc) {
        return om_volumes->addVolume(desc);
    }

    virtual void om_set_throttle_lvl(float level);
    virtual void om_send_bucket_stats(fds_uint32_t, const NodeUuid&, fds_uint32_t);
    virtual void om_handle_perfstats_from_am(const fpi::FDSP_VolPerfHistListType &list,
                                             const std::string start_timestamp);

    virtual void om_bcast_tier_policy(fpi::FDSP_TierPolicyPtr policy);
    virtual void om_bcast_tier_audit(fpi::FDSP_TierPolicyAuditPtr audit);
    virtual fds_uint32_t  om_bcast_vol_list(NodeAgent::pointer node);
    virtual fds_uint32_t om_bcast_vol_create(VolumeInfo::pointer vol);
    virtual fds_uint32_t om_bcast_vol_snap(VolumeInfo::pointer vol);
    virtual void om_bcast_vol_modify(VolumeInfo::pointer vol);
    virtual fds_uint32_t om_bcast_vol_delete(VolumeInfo::pointer vol,
                                             fds_bool_t check_only);
    virtual fds_uint32_t om_bcast_vol_detach(VolumeInfo::pointer vol);
    virtual void om_bcast_vol_tier_policy(const fpi::FDSP_TierPolicyPtr &tier);
    virtual void om_bcast_vol_tier_audit(const fpi::FDSP_TierPolicyAuditPtr &tier);
    virtual void om_bcast_throttle_lvl(float throttle_level);
    virtual fds_uint32_t om_bcast_dlt(const DLT* curDlt,
                                      fds_bool_t sm_only = false);
    virtual fds_uint32_t om_bcast_dlt_close(fds_uint64_t cur_dlt_version);
    /**
     * Sends scavenger command (e.g. enable, disable, start, stop) to SMs
     */
    virtual void om_bcast_scavenger_cmd(fpi::FDSP_ScavengerCmd cmd);
    /**
     * Sends stream registration to all DMs. If bAll is true, regId is
     * ignored and all known registrations are broadcasted to DMs. Otherwise
     * only registration with regId is broadcasted to DM
     */
    virtual void om_bcast_stream_register_cmd(fds_int32_t regId,
                                              fds_bool_t bAll);
    virtual void om_bcast_stream_reg_list(NodeAgent::pointer node);

    /**
     * conditional broadcast to platform (nodes) to
     * activate SM and DM services on those nodes, but only
     * to those nodes which are in discovered state
     */
    virtual void om_cond_bcast_activate_services(fds_bool_t activate_sm,
                                                 fds_bool_t activate_dm,
                                                 fds_bool_t activate_am);
    virtual Error om_activate_node_services(const NodeUuid& node_uuid,
                                            fds_bool_t activate_sm,
                                            fds_bool_t activate_md,
                                            fds_bool_t activate_am);
    virtual fds_uint32_t om_bcast_dmt(fpi::FDSP_MgrIdType svc_type,
                                      const DMTPtr& curDmt);
    virtual fds_uint32_t om_bcast_dmt_close(fds_uint64_t dmt_version);

    virtual void om_send_me_qosinfo(NodeAgent::pointer me);

  private:
    friend class OM_NodeDomainMod;

    virtual void om_update_capacity(NodeAgent::pointer node, fds_bool_t b_add);
    virtual void om_bcast_new_node(NodeAgent::pointer node, const FdspNodeRegPtr ref);
    virtual void om_update_node_list(NodeAgent::pointer node, const FdspNodeRegPtr ref);

    FdsAdminCtrl             *om_admin_ctrl;
    VolumeContainer::pointer  om_volumes;
    float                     om_cur_throttle_level;

    void om_init_domain();

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
class WaitNdsEvt
{
 public:
    explicit WaitNdsEvt(const NodeUuidSet& sms, const NodeUuidSet& dms)
            : sm_services(sms.begin(), sms.end()),
            dm_services(dms.begin(), dms.end())
            {}
    std::string logString() const {
        return "WaitNdsEvt";
    }

    NodeUuidSet sm_services;
    NodeUuidSet dm_services;
};

class NoPersistEvt
{
 public:
    NoPersistEvt() {}
    std::string logString() const {
        return "NoPersistEvt";
    }
};

class LoadVolsEvt
{
 public:
    LoadVolsEvt() {}
    std::string logString() const {
        return "LoadVolsEvt";
    }
};

class RegNodeEvt
{
 public:
    RegNodeEvt(const NodeUuid& uuid,
               fpi::FDSP_MgrIdType type)
            : svc_uuid(uuid), svc_type(type) {}
    std::string logString() const {
        return "RegNodeEvt";
    }

    NodeUuid svc_uuid;
    fpi::FDSP_MgrIdType svc_type;
};

class TimeoutEvt
{
 public:
    TimeoutEvt() {}
    std::string logString() const {
        return "TimeoutEvt";
    }
};

class DltDmtUpEvt
{
 public:
    explicit DltDmtUpEvt(fpi::FDSP_MgrIdType type) {
        svc_type = type;
    }
    std::string logString() const {
        return "DltDmtUpEvt";
    }

    fpi::FDSP_MgrIdType svc_type;
};

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
     * Initially, local domain is not up and waiting
     * for all the nodes that were up before and in
     * config db to come up again + bring up all known
     * volumes. In this state, the method returns false
     * and all volumes evens are rejected, and most
     * node events are put on hold until local domain is up.
     */
    static fds_bool_t om_local_domain_up();

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
     * Read persistent state from config db and see if we
     * get the cluster to the same state -- will wait for
     * some time to see if all known nodes will come up or
     * otherwise will update DLT (relative to persisted DLT)
     * appropriately. Will also bring up all known volumes.
     */
    virtual Error om_load_state(kvstore::ConfigDB* _configDB);
    virtual Error om_load_volumes();
    virtual fds_bool_t om_rm_sm_configDB(const NodeUuid& uuid);

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
    virtual Error om_del_services(const NodeUuid& node_uuid,
                                  const std::string& node_name,
                                  fds_bool_t remove_sm,
                                  fds_bool_t remove_dm,
                                  fds_bool_t remove_am);

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
    virtual Error om_recv_dlt_commit_resp(FdspNodeType node_type,
                                          const NodeUuid& uuid,
                                          fds_uint64_t dlt_version);
    /**
     * Notification that OM received DMT update response from
     * node with uuid 'uuid' for dmt version 'dmt_version'
     */
    virtual Error om_recv_dmt_commit_resp(FdspNodeType node_type,
                                          const NodeUuid& uuid,
                                          fds_uint32_t dmt_version);

    /**
     * Notification that OM received push meta response from
     * node with uuid 'uuid'
     */
    virtual Error om_recv_push_meta_resp(const NodeUuid& uuid);

    /**
     * Notification that OM received DLT close response from
     * node with uuid 'uuid' for dlt version 'dlt_version'
     */
    virtual Error om_recv_dlt_close_resp(const NodeUuid& uuid,
                                         fds_uint64_t dlt_version);

    /**
     * Notification that OM received DMT close response from
     * node with uuid 'uuid' for dmt version 'dmt_version'
     */
    virtual Error om_recv_dmt_close_resp(const NodeUuid& uuid,
                                         fds_uint64_t dmt_version);

    /**
     * Updates cluster map membership and does DLT
     */
    virtual void om_dmt_update_cluster();
    virtual void om_dlt_update_cluster();
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

    /**
     * Apply an event to domain state machine
     */
    void local_domain_event(WaitNdsEvt const &evt);
    void local_domain_event(DltDmtUpEvt const &evt);
    void local_domain_event(RegNodeEvt const &evt);
    void local_domain_event(TimeoutEvt const &evt);
    void local_domain_event(NoPersistEvt const &evt);

  protected:
    fds_bool_t                       om_test_mode;
    OM_NodeContainer                *om_locDomain;
    kvstore::ConfigDB               *configDB;

    FSM_NodeDomain                  *domain_fsm;
    // to protect access to msm process_event
    fds_mutex                       fsm_lock;
};

/**
 * control response handler
 */
class OM_ControlRespHandler : public fpi::FDSP_ControlPathRespIf {
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

    void NotifySnapVolResp(
        const FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
        const FDS_ProtocolInterface::FDSP_NotifyVolType& not_snap_vol_resp);
    void NotifySnapVolResp(
        FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
        FDS_ProtocolInterface::FDSP_NotifyVolTypePtr& not_snap_vol_resp);

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

    void NotifyDLTCloseResp(
        const FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
        const FDS_ProtocolInterface::FDSP_DLT_Resp_Type& dlt_resp);
    void NotifyDLTCloseResp(
        FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
        FDS_ProtocolInterface::FDSP_DLT_Resp_TypePtr& dlt_resp);

    void NotifyDMTUpdateResp(
        const FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
        const FDS_ProtocolInterface::FDSP_DMT_Resp_Type& dmt_info_resp);
    void NotifyDMTUpdateResp(
        FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
        FDS_ProtocolInterface::FDSP_DMT_Resp_TypePtr& dmt_info_resp);

    void NotifyDMTCloseResp(
        const ::FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
        const ::FDS_ProtocolInterface::FDSP_DMT_Resp_Type& dmt_info_resp);
    void NotifyDMTCloseResp(
        FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
        FDS_ProtocolInterface::FDSP_DMT_Resp_TypePtr& dmt_info_resp);

    void PushMetaDMTResp(
        const ::FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
        const ::FDS_ProtocolInterface::FDSP_PushMeta& push_meta_resp);
    void PushMetaDMTResp(
        FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
        FDS_ProtocolInterface::FDSP_PushMetaPtr& push_meta_resp);

  private:
        // TODO(Andrew): Add ptr back to resource manager.
};

extern OM_NodeDomainMod      gl_OMNodeDomainMod;

}  // namespace fds
#endif  // SOURCE_ORCH_MGR_INCLUDE_OMRESOURCES_H_
