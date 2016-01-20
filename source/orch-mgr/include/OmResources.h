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
#include <OMMonitorWellKnownPMs.h>

#include "platform/agent_container.h"
#include "platform/domain_container.h"
#include "fdsp/sm_types_types.h"
#include <dlt.h>
#include <fds_dmt.h>
#include <kvstore/configdb.h>
#include <concurrency/RwLock.h>
#include <DltDmtUtil.h>
#include <fds_timer.h>

namespace FDS_ProtocolInterface {
    struct CtrlNotifyDMAbortMigration;
    struct CtrlNotifySMAbortMigration;
    struct CtrlNotifyDMTClose;
    struct CtrlNotifyDLTClose;
    struct CtrlNotifyDMTUpdate;
    struct CtrlDMMigrateMeta;
    struct CtrlNotifyDMStartMigrationMsg;
    using CtrlNotifyDMAbortMigrationPtr = boost::shared_ptr<CtrlNotifyDMAbortMigration>;
    using CtrlNotifySMAbortMigrationPtr = boost::shared_ptr<CtrlNotifySMAbortMigration>;
    using CtrlNotifyDMTClosePtr = boost::shared_ptr<CtrlNotifyDMTClose>;
    using CtrlNotifyDLTClosePtr = boost::shared_ptr<CtrlNotifyDLTClose>;
    using CtrlNotifyDMTUpdatePtr = boost::shared_ptr<CtrlNotifyDMTUpdate>;
    using CtrlDMMigrateMetaPtr = boost::shared_ptr<CtrlDMMigrateMeta>;
    using CtrlNotifyDMStartMigrationMsgPtr =
    		boost::shared_ptr<CtrlNotifyDMStartMigrationMsg>;
}  // namespace FDS_ProtocolInterface

namespace fds {

class PerfStats;
class FdsAdminCtrl;
class OM_NodeDomainMod;
class OM_ControlRespHandler;
struct NodeDomainFSM;

typedef boost::msm::back::state_machine<NodeDomainFSM> FSM_NodeDomain;
typedef boost::shared_ptr<fpi::SvcInfo> SvcInfoPtr;

/**
 * Agent interface to communicate with the remote node.  This is the communication
 * end-point to the node.
 *
 * It's normal that the node agent is there but the transport may not be available.
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
    // TODO remove - not in use and incorrect
    inline fpi::FDSP_MgrIdType om_agent_type() const {
        return ndMyServId;
    }

    /**
     * Returns uuid of the node running this service
     */
    inline NodeUuid get_parent_uuid() const {
        return parentUuid;
    }

    /**
     * Call this method when service is successfully deployed in the domain
     * (SM/DM is in DLT/DMT). Sets service state to active.
     */
    virtual void handle_service_deployed();
    /**
     * Calls set_not_state() based on state stored in configDB service map
     */
    virtual void set_state_from_svcmap();

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
    virtual Error om_send_sm_abort_migration(fds_uint64_t committedDltVersion,
                                             fds_uint64_t targetDltVersion);
    virtual Error om_send_dm_abort_migration(fds_uint64_t dmtVersion);
    void om_send_abort_sm_migration_resp(fpi::CtrlNotifySMAbortMigrationPtr msg,
                                      EPSvcRequest* req,
                                      const Error& error,
                                      boost::shared_ptr<std::string> payload);
    void om_send_abort_dm_migration_resp(fpi::CtrlNotifyDMAbortMigrationPtr msg,
            EPSvcRequest* req,
            const Error& error,
            boost::shared_ptr<std::string> payload);

    virtual Error om_send_dlt_close(fds_uint64_t cur_dlt_version);
    void    om_send_dlt_resp(fpi::CtrlNotifyDLTUpdatePtr msg, EPSvcRequest* rpcReq,
                               const Error& error,
                               boost::shared_ptr<std::string> payload);

    void om_send_dlt_close_resp(fpi::CtrlNotifyDLTClosePtr msg,
            EPSvcRequest* req,
            const Error& error,
            boost::shared_ptr<std::string> payload);

    virtual Error om_send_dmt(const DMTPtr& curDmt);
    void    om_send_dmt_resp(fpi::CtrlNotifyDMTUpdatePtr msg, EPSvcRequest* rpcReq,
                               const Error& error,
                               boost::shared_ptr<std::string> payload);

    void om_send_dmt_close_resp(fpi::CtrlNotifyDMTClosePtr msg,
            EPSvcRequest* req,
            const Error& error,
            boost::shared_ptr<std::string> payload);

    virtual Error om_send_dmt_close(fds_uint64_t dmt_version);
    virtual Error om_send_scavenger_cmd(fpi::FDSP_ScavengerCmd cmd);
    virtual Error om_send_pullmeta(fpi::CtrlNotifyDMStartMigrationMsgPtr& meta_msg);
    void om_pullmeta_resp(EPSvcRequest* req,
                          const Error& error,
                          boost::shared_ptr<std::string> payload);
    virtual Error om_send_stream_reg_cmd(fds_int32_t regId,
                                         fds_bool_t bAll);
    /*
     * FS-2561 ( Tinius )
     */
    virtual Error om_send_stream_de_reg_cmd( fds_int32_t regId );
    // FS-2561
    virtual Error om_send_qosinfo(fds_uint64_t total_rate);
    virtual Error om_send_shutdown();
    void om_send_shutdown_resp(EPSvcRequest* req,
                               const Error& error,
                               boost::shared_ptr<std::string> payload);
    virtual void init_msg_hdr(fpi::FDSP_MsgHdrTypePtr msgHdr) const;

    /*
     * In the case where when we're adding a new node but was unsuccessful,
     * such as when DM or SM migration fails, the node is going to be in the
     * addedSM or addedDM cluster map. Because it remains in the map, it becomes
     * problematic for the OM as it doesn't clean them up.
     * To clean them up, this method will take care of it, and to activate this
     * cleanup, simply shut down a node from the CLI. This can be done safely
     * in a customer environment.
     * Once cleaned up, more nodes can be added to the domain.
     */
    void cleanup_added_node();

  private:
    void om_send_one_stream_reg_cmd(const apis::StreamingRegistrationMsg& reg,
                                    const NodeUuid& stream_dest_uuid);
    /*
     * FS-2561 ( Tinius )
     */
    void om_send_one_stream_de_reg_cmd(
      const fds_int32_t regId,
      const NodeUuid& stream_dest_uuid );
    // FS-2561

  protected:
    std::string             ndSessionId;
    fpi::FDSP_MgrIdType     ndMyServId;
    NodeUuid                parentUuid;  // Uuid of the node running the service

    uint32_t                dm_migration_abort_timeout;

    virtual int node_calc_stor_weight();
};

typedef OM_NodeAgent                    OM_SmAgent;
typedef OM_NodeAgent                    OM_DmAgent;
typedef OM_NodeAgent                    OM_AmAgent;
typedef std::list<OM_NodeAgent::pointer>  NodeList;

struct NodeSvcEntity {
	NodeSvcEntity() {};
	NodeSvcEntity(const std::basic_string<char> &in_name,
			const fds::ResourceUUID &in_uid, const FdspNodeType &in_type) :
		node_uuid(in_uid),
		node_name(in_name),
		svc_type(in_type)
		{}
	~NodeSvcEntity() {};
    NodeUuid node_uuid;
    std::string node_name;
    FdspNodeType svc_type;
};

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
    fds_bool_t hasRegistered(const FdspNodeRegPtr  msg);
    /**
     * Send 'activate services' message to Platform
     */
    Error send_activate_services(fds_bool_t activate_sm,
                                 fds_bool_t activate_dm,
                                 fds_bool_t activate_am);
    /**
     * Send 'add service' message to Platform
     */
    Error send_add_service(const fpi::SvcUuid svc_uuid, std::vector<fpi::SvcInfo> svcInfos);
    /**
     * Send 'start service' message to Platform
     */
    Error send_start_service(const fpi::SvcUuid svc_uuid, std::vector<fpi::SvcInfo> svcInfos,
                             bool domainRestart, bool startNode);

    void send_start_service_resp(fpi::SvcUuid pmSvcUuid, fpi::SvcChangeInfoList changeList);
    /**
     * Send 'stop service' message to Platform
     */
    Error send_stop_service(std::vector<fpi::SvcInfo> svcInfos,
                            bool stop_sm, bool stop_dm, bool stop_am, bool shutdownNode);

    void send_stop_services_resp(fds_bool_t stop_sm,
                                 fds_bool_t stop_dm,
                                 fds_bool_t stop_am,
                                 fpi::SvcUuid smSvcId,
                                 fpi::SvcUuid dmSvcId,
                                 fpi::SvcUuid amSvcId,
                                 fds_bool_t shutdownNode,
                                 EPSvcRequest* req,
                                 const Error& error,
                                 boost::shared_ptr<std::string> payload);
    /**
     * Send 'remove service' message to Platform
     */
    Error send_remove_service(const NodeUuid& uuid, std::vector<fpi::SvcInfo> svcInfos,
                              bool remove_sm, bool remove_dm, bool remove_am, bool removeNode);

    void send_remove_service_resp(NodeUuid nodeUuid,
                                  bool removeNode,
                                  EPSvcRequest* req,
                                  const Error& error,
                                  boost::shared_ptr<std::string> payload);
    /**
     * Send 'heartbeat check' message to Platform
     */
    Error send_heartbeat_check(const fpi::SvcUuid svc_uuid);
    /**
     * Send 'deactivate services' message to Platform
     */
    Error send_deactivate_services(fds_bool_t deactivate_sm,
                                   fds_bool_t deactivate_dm,
                                   fds_bool_t deactivate_am);

    void send_deactivate_services_resp(fds_bool_t deactivate_sm,
                                       fds_bool_t deactivate_dm,
                                       fds_bool_t deactivate_am,
                                       EPSvcRequest* req,
                                       const Error& error,
                                       boost::shared_ptr<std::string> payload);
    /**
     * Tell platform Agent about new active service
     * Platform Agent keeps a pointer to active services node agents
     * These functions just set/reset pointers to appropriate service
     * agents and do not actually register or deregister node agents
     */
    Error handle_register_service(FDS_ProtocolInterface::FDSP_MgrIdType svc_type,
                                  NodeAgent::pointer svc_agent);

    /**
     * Send 'remove services' message to Platform
     */
    Error send_remove_services(fds_bool_t activate_sm,
                               fds_bool_t activate_dm,
                               fds_bool_t activate_am);

    /**
     * @return uuid of service that was unregistered
     */
    NodeUuid handle_unregister_service(FDS_ProtocolInterface::FDSP_MgrIdType svc_type);

    /**
     * If any service running on this node matches given 'svc_uuid'
     * handle its deregister
     */
    void handle_unregister_service(const NodeUuid& svc_uuid);

    /**
     * Deactivates service on this platform
     * This method just updates local info, does not actually send msg to platform
     * to deactivate; should be called when de-active is sent to platform
     */
    void handle_deactivate_service(const FDS_ProtocolInterface::FDSP_MgrIdType svc_type);

    inline OM_SmAgent::pointer get_sm_service() {
        return activeSmAgent;
    }
    inline OM_DmAgent::pointer get_dm_service() {
        return activeDmAgent;
    }
    inline OM_AmAgent::pointer get_am_service() {
        return activeAmAgent;
    }
    inline void setNodeInfo(boost::shared_ptr<kvstore::NodeInfoType> nodeInfo) {
        this->nodeInfo = nodeInfo;
    }
    inline boost::shared_ptr<kvstore::NodeInfoType> getNodeInfo() {
        return nodeInfo;
    }
    inline fpi::FDSP_AnnounceDiskCapability* getDiskCapabilities() {
        /* NOTE: We could get this info from configdb..for now returning from
         * the cached node info
         */
        return &(nodeInfo->disk_info);
    }

    virtual void init_msg_hdr(fpi::FDSP_MsgHdrTypePtr msgHdr) const;

  protected:
    OM_SmAgent::pointer     activeSmAgent;  // pointer to active SM service or NULL
    OM_DmAgent::pointer     activeDmAgent;  // pointer to active DM service or NULL
    OM_AmAgent::pointer     activeAmAgent;  // pointer to active AM service or NULL
    /* Cached node information.  On activation this information is stored into config db */
    boost::shared_ptr<kvstore::NodeInfoType> nodeInfo;

  private:
    fds_mutex               dbNodeInfoLock;
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

    // In case of a DM, we need to add it to a resync list. Otherwise it's a no-op
    virtual void agent_reactivate(NodeAgent::pointer agent);

    /**
     * For derived classes, this would allow them to return the correct type.
     */
    virtual FDS_ProtocolInterface::FDSP_MgrIdType container_type() const = 0;

    /**
     * Returns a list of nodes that are currently in the container.
     * The method will clear the vector being passed in and populate it.
     */
    const Error populate_nodes_in_container(std::list<fds::NodeSvcEntity> &container_nodes);

    /**
     * Move all pending nodes to addNodes and rmNodes
     * The second function only moves nodes that are in 'filter_nodes'
     * set and leaves other pending nodes pending
     */
    void om_splice_nodes_pend(NodeList *addNodes, NodeList *rmNodes);
    void om_splice_nodes_pend(NodeList *addNodes, NodeList *rmNodes, NodeList *dmResyncNodes);
    void om_splice_nodes_pend(NodeList *addNodes,
                              NodeList *rmNodes,
                              const NodeUuidSet& filter_nodes);

  protected:
    NodeList                                 node_up_pend;
    NodeList                                 node_down_pend;
    NodeList                                 dm_resync_pend;

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

    fds_bool_t hasRegistered(const FdspNodeRegPtr  msg);
    /**
     * Tell platform agent with uuid 'pm_uuid' about new service registered
     */
    Error handle_register_service(const NodeUuid& pm_uuid,
                                  FDS_ProtocolInterface::FDSP_MgrIdType svc_role,
                                  NodeAgent::pointer svc_agent);

    inline FDS_ProtocolInterface::FDSP_MgrIdType container_type() const override {
    	return FDS_ProtocolInterface::FDSP_MgrIdType::FDSP_PLATFORM;
    }
    /**
     * Returns a list of nodes that are currently in the PM container.
     * The method will clear the vector being passed in and populate it.
     */
    const Error populate_nodes_in_container(std::list<NodeSvcEntity> &container_nodes);


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

    inline FDS_ProtocolInterface::FDSP_MgrIdType container_type() const override {
    	return FDS_ProtocolInterface::FDSP_MgrIdType::FDSP_STOR_MGR;
    }
    /**
     * Returns a list of nodes that are currently in the PM container.
     * The method will clear the vector being passed in and populate it.
     */
    const Error populate_nodes_in_container(std::list<NodeSvcEntity> &container_nodes);

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

    inline FDS_ProtocolInterface::FDSP_MgrIdType container_type() const override {
    	return FDS_ProtocolInterface::FDSP_MgrIdType::FDSP_DATA_MGR;
    }
    /**
     * Returns a list of nodes that are currently in the PM container.
     * The method will clear the vector being passed in and populate it.
     */
    const Error populate_nodes_in_container(std::list<NodeSvcEntity> &container_nodes);

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
    typedef boost::intrusive_ptr<OM_AmContainer> pointer;

    OM_AmContainer();
    virtual ~OM_AmContainer() {}

    inline FDS_ProtocolInterface::FDSP_MgrIdType container_type() const override {
    	return FDS_ProtocolInterface::FDSP_MgrIdType::FDSP_ACCESS_MGR;
    }
    /**
     * Returns a list of nodes that are currently in the PM container.
     * The method will clear the vector being passed in and populate it.
     */
    const Error populate_nodes_in_container(std::list<NodeSvcEntity> &container_nodes);

    static inline OM_AmContainer::pointer agt_cast_ptr(RsContainer::pointer ptr) {
        return static_cast<OM_AmContainer *>(get_pointer(ptr));
    }
  protected:
    virtual Resource *rs_new(const ResourceUUID &uuid) {
        return new OM_AmAgent(uuid, fpi::FDSP_ACCESS_MGR);
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

    inline Error om_snap_vol(const fpi::FDSP_MsgHdrTypePtr &hdr,
                             const FdspCrtVolPtr      &snap_msg) {
        return om_volumes->om_snap_vol(hdr, snap_msg);
    }
    inline Error om_modify_vol(const FdspModVolPtr &mod_msg) {
        return om_volumes->om_modify_vol(mod_msg);
    }
    inline Error om_get_volume_descriptor(const boost::shared_ptr<fpi::AsyncHdr>    &hdr,
                                          const std::string& vol_name,
                                          VolumeDesc& desc) {
        return om_volumes->om_get_volume_descriptor(hdr, vol_name, desc);
    }

    inline bool addVolume(const VolumeDesc& desc) {
        return om_volumes->addVolume(desc);
    }

    virtual void om_set_throttle_lvl(float level);

    virtual fds_uint32_t  om_bcast_vol_list(NodeAgent::pointer node);
    virtual void om_bcast_vol_list_to_services(fpi::FDSP_MgrIdType svc_type);
    virtual fds_uint32_t om_bcast_vol_create(VolumeInfo::pointer vol);
    virtual fds_uint32_t om_bcast_vol_snap(VolumeInfo::pointer vol);
    virtual void om_bcast_vol_modify(VolumeInfo::pointer vol);
    virtual fds_uint32_t om_bcast_vol_delete(VolumeInfo::pointer vol,
                                             fds_bool_t check_only);
    virtual fds_uint32_t om_bcast_vol_detach(VolumeInfo::pointer vol);
    virtual void om_bcast_throttle_lvl(float throttle_level);
    virtual fds_uint32_t om_bcast_dlt(const DLT* curDlt,
                                      fds_bool_t to_sm = true,
                                      fds_bool_t to_dm = true,
                                      fds_bool_t to_am = true);
    virtual fds_uint32_t om_bcast_dlt_close(fds_uint64_t cur_dlt_version);
    virtual fds_uint32_t om_bcast_sm_migration_abort(fds_uint64_t cur_dlt_version,
                                                     fds_uint64_t tgt_dlt_version);
    virtual fds_uint32_t om_bcast_shutdown_msg(fpi::FDSP_MgrIdType svc_type);
    virtual fds_uint32_t om_bcast_dm_migration_abort(fds_uint64_t cur_dmt_version);

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

    /*
     * FS-2561 ( Tinius )
     */
    virtual void om_bcast_stream_de_register_cmd( fds_int32_t regId );

    /**
     * conditional broadcast to platform (nodes) to
     * start SM, DM, and AM services on those nodes, but only
     * to those nodes which are in discovered state
     */
    virtual void om_cond_bcast_start_services(fds_bool_t activate_sm,
                                                 fds_bool_t activate_dm,
                                                 fds_bool_t activate_am); // Activate these specific Services on each Node.
    virtual Error om_activate_node_services(const NodeUuid& node_uuid,
                                            fds_bool_t activate_sm,
                                            fds_bool_t activate_md,
                                            fds_bool_t activate_am);

    virtual Error om_add_service(const fpi::SvcUuid& svc_uuid,
                                 std::vector<fpi::SvcInfo> svcInfos);

    virtual Error om_start_service(const fpi::SvcUuid& svc_uuid,
                                   std::vector<fpi::SvcInfo> svcInfos,
                                   bool domainRestart,
                                   bool startNode);

    virtual Error om_stop_service(const fpi::SvcUuid& svc_uuid,
                                  std::vector<fpi::SvcInfo> svcInfos,
                                  bool stop_sm, bool stop_dm, bool stop_am,
                                  bool shutdownNode);

    virtual Error om_remove_service(const fpi::SvcUuid& svc_uuid,
                                    std::vector<fpi::SvcInfo> svcInfos,
                                    bool remove_sm, bool remove_dm,
                                    bool remove_am, bool removeNode);

    virtual Error om_heartbeat_check(const fpi::SvcUuid& svc_uuid);

    virtual void om_cond_bcast_remove_services(fds_bool_t activate_sm,
                                               fds_bool_t activate_dm,
                                               fds_bool_t activate_am); // Remove the Services defined for each Node.

    // broadcast "stop service" message to all PMs in the domain
    virtual fds_uint32_t om_cond_bcast_stop_services(fds_bool_t stop_sm,
                                                     fds_bool_t stop_dm,
                                                     fds_bool_t stop_am);

    virtual fds_uint32_t om_bcast_dmt(fpi::FDSP_MgrIdType svc_type,
                                      const DMTPtr& curDmt);
    virtual fds_uint32_t om_bcast_dmt_close(fds_uint64_t dmt_version);

    virtual void om_send_me_qosinfo(NodeAgent::pointer me);

    /**
    * @brief Broadcasts svcmap around the domain
    */
    virtual void om_bcast_svcmap();

    /**
    * fs-1637: returns true if the services have been activated at least once,
    * otherwise it returns false
    */
    bool have_services_been_activated_once() {
        static bool activate_services_done_once = false;
        bool tmp = activate_services_done_once;
        activate_services_done_once =  true;
        return tmp;
    }

    void setLocalDomain(const LocalDomain localDomain) {
        om_local_domain = std::unique_ptr<LocalDomain>(new LocalDomain(localDomain));
    }
    LocalDomain* getLocalDomain() const {
        return om_local_domain.get();
    }

  private:
    friend class OM_NodeDomainMod;

    virtual void om_update_capacity(OM_PmAgent::pointer pm_agent, fds_bool_t b_add);

    FdsAdminCtrl             *om_admin_ctrl;
    VolumeContainer::pointer  om_volumes;
    float                     om_cur_throttle_level;
    std::unique_ptr<LocalDomain> om_local_domain{nullptr};

    void om_init_domain();

    /**
     * Recent history of perf stats OM receives from AM nodes.
     * TODO(Anna) need to use new stats class
     */
    // PerfStats                *am_stats;
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
    WaitNdsEvt(const NodeUuidSet& sms,
               const NodeUuidSet& dms)
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

class ShutdownEvt
{
 public:
    ShutdownEvt() {}
    std::string logString() const {
        return "ShutdownEvt";
    }
};

struct ShutAckEvt
{
    ShutAckEvt(fpi::FDSP_MgrIdType type,
               const Error& err) {
        svc_type = type;
        error = err;
    }
    std::string logString() const {
        return "ShutAckEvt";
    }

    fpi::FDSP_MgrIdType svc_type;
    Error error;  // error that came with ack
};

struct DeactAckEvt
{
    explicit DeactAckEvt(const Error& err) {
        error = err;
    }
    std::string logString() const {
        return "DeactAckEvt";
    }

    Error error;  // error that came with ack
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
     * Returns true when domain shutdown process finishes.
     * Domain can be re-activated only when domain is in down
     * state; When domain us up, and shutdown process starts,
     * domain is not 'up' anymore but also in not 'down' state.
     */
    static fds_bool_t om_local_domain_down();

    /**
     * "true" if this is called within the Master Domain and hence, the Master OM.
     */
    static fds_bool_t om_master_domain();

    /**
     * Accessors methods to retreive the local node domain.  Retyping it here to avoid
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
    inline OM_NodeAgent::pointer om_all_agent(const NodeUuid &uuid) {
        switch (uuid.uuid_get_val() & 0x0F) {
            case fpi::FDSP_ACCESS_MGR:
            return om_am_agent(uuid);
            case fpi::FDSP_STOR_MGR:
            return om_sm_agent(uuid);
            case fpi::FDSP_DATA_MGR:
            return om_dm_agent(uuid);
        default:
            break;
        }
        return OM_NodeAgent::pointer(nullptr);
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
    virtual Error om_reg_node_info( const NodeUuid &uuid, 
                                    const FdspNodeRegPtr msg );
    virtual Error om_handle_restart( const NodeUuid&      uuid,
                                     const FdspNodeRegPtr msg );

    void setupNewNode(const NodeUuid&      uuid,
                      const FdspNodeRegPtr msg,
                      NodeAgent::pointer   newNode,
                      bool fPrevRegistered);

    /**
     * Activate well known service on an node
     */
    bool om_activate_known_services(bool domainRestart, const NodeUuid& node_uuid);

    /**
    * @brief Registers the service
    *
    * @param svcInfo
    *
    * @return 
    */
    virtual Error om_register_service(boost::shared_ptr<fpi::SvcInfo>& svcInfo);

    virtual void
    om_change_svc_state_and_bcast_svcmap( const NodeUuid& svcUuid,
                                          fpi::FDSP_MgrIdType svcType,
                                          const fpi::ServiceStatus status );
    
    /**
     * Notification that service is down to DLT and DMT state machines
     * @param error timeout error or other error returned by the service
     * @param svcUuid service that is down
     */
    virtual void
    om_service_down(const Error& error,
                    const NodeUuid& svcUuid,
                    fpi::FDSP_MgrIdType svcType);

    /**
     * Notification that service is up to DLT and DMT state machines
     * @param svcUuid service that is up
     */
    virtual void
    om_service_up(const NodeUuid& svcUuid,
                  fpi::FDSP_MgrIdType svcType);

    /**
     * Unregister the node matching uuid from the domain manager.
     */
    virtual Error om_del_services(const NodeUuid& node_uuid,
                                  const std::string& node_name,
                                  fds_bool_t remove_sm,
                                  fds_bool_t remove_dm,
                                  fds_bool_t remove_am);

    /**
     * This will set domain up by calling Domain state machine to move
     * to UP state. Noop if domain is already up.
     * Domain state machine will wait for all services currently
     * in the cluster map to come up, before moving to UP state
     */
    virtual Error om_startup_domain();
    
    /**
     * This will set domain down so that DLT and DMT state machine
     * will not try to add/remove services and send shutdown message
     * to all 
     */
    virtual Error om_shutdown_domain();

    /**
     * Notification that OM received migration done message from
     * node with uuid 'uuid' for dlt version dlt_version
     */
    virtual Error om_recv_migration_done(const NodeUuid& uuid,
                                         fds_uint64_t dlt_version,
                                         const Error& migrError);

    /**
     * Notification that OM received DLT update response from
     * node with uuid 'uuid' for dlt version dlt_version
     */
    virtual Error om_recv_dlt_commit_resp(FdspNodeType node_type,
                                          const NodeUuid& uuid,
                                          fds_uint64_t dlt_version,
                                          const Error& respError);
    /**
     * Notification that OM received DMT update response from
     * node with uuid 'uuid' for dmt version dmt_version
     */
    virtual Error om_recv_dmt_commit_resp(FdspNodeType node_type,
                                          const NodeUuid& uuid,
                                          fds_uint32_t dmt_version,
                                          const Error& respError);

    /**
     * Notification that OM received pull meta response from
     * node with uuid 'uuid'
     */
    virtual Error om_recv_pull_meta_resp(const NodeUuid& uuid,
                                         const Error& respError);

    /**
     * Notification that OM received DLT close response from
     * node with uuid 'uuid' for dlt version dlt_version
     */
    virtual Error om_recv_dlt_close_resp(const NodeUuid& uuid,
                                         fds_uint64_t dlt_version,
                                         const Error& respError);

    /**
     * Notification that OM received DMT close response from
     * node with uuid 'uuid' for dmt version dmt_version
     */
    virtual Error om_recv_dmt_close_resp(const NodeUuid& uuid,
                                         fds_uint64_t dmt_version,
                                         const Error& respError);

    /**
     * Updates cluster map membership and does DLT
     * bool dmPrevRegistered - if the DMT needs to be updated in accordance
     * with DM Resync design, which is to issue the same DMT but ++version.
     */
    virtual void om_dmt_update_cluster(bool dmPrevRegistered = false);
    virtual void om_dmt_waiting_timeout();
    virtual void om_dlt_update_cluster();

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
    void local_domain_event(ShutdownEvt const &evt);
    void local_domain_event(ShutAckEvt const &evt);
    void local_domain_event(DeactAckEvt const &evt);

    /**
     * Methods related to tracking registering services
     * for svcMap updates
     */
    void  addRegisteringSvc(SvcInfoPtr infoPtr);
    Error getRegisteringSvc(SvcInfoPtr& infoPtr, int64_t uuid);
    void  removeRegisteredSvc(int64_t uuid);

    void raiseAbortSmMigrationEvt(NodeUuid uuid);
    void raiseAbortDmMigrationEvt(NodeUuid uuid);

    void setDomainShuttingDown(bool domainDown);
    bool isDomainShuttingDown();
    void addToShutdownList(int64_t uuid);
    void clearFromShutdownList(int64_t uuid);
    void clearShutdownList();
    bool isNodeShuttingDown(int64_t uuid);

    void removeNodeComplete(NodeUuid uuid);

    inline fds_bool_t dmClusterPresent() {
        return volumeGroupDMTFired;
    }

    // used for unit test
    inline void setDmClusterSize(uint32_t size) {
        dmClusterSize = size;
    }
    fds_bool_t checkDmtModVGMode();
    bool isScheduled(FdsTimerTaskPtr&, int64_t id);
    void addToTaskMap(FdsTimerTaskPtr task, int64_t id);
    void removeFromTaskMap(int64_t id);

  protected:
    bool isPlatformSvc(fpi::SvcInfo svcInfo);
    bool isAccessMgrSvc( fpi::SvcInfo svcInfo );
    bool isDataMgrSvc( fpi::SvcInfo svcInfo );
    bool isStorageMgrSvc( fpi::SvcInfo svcInfo );
    bool isKnownPM( fpi::SvcInfo svcInfo );
    bool isKnownService(fpi::SvcInfo svcInfo);
    void fromSvcInfoToFDSP_RegisterNodeTypePtr( 
        fpi::SvcInfo svc, 
        fpi::FDSP_RegisterNodeTypePtr& reg_node_req );
    void fromSvcInfoToFDSP_RegisterNodeTypePtr( 
        boost::shared_ptr<fpi::SvcInfo>& svcInfo,
        fpi::FDSP_RegisterNodeTypePtr& reg_node_req );
    bool isAnyNonePlatformSvcActive( std::vector<fpi::SvcInfo>* pmSvcs,
                                     std::vector<fpi::SvcInfo>* amSvcs,
                                     std::vector<fpi::SvcInfo>* smSvcs,
                                     std::vector<fpi::SvcInfo>* dmSvcs );
    void spoofRegisterSvcs( const std::vector<fpi::SvcInfo> svcs );
    void isAnySvcPendingRemoval( std::vector<fpi::SvcInfo>* removedSvcs );
    void handlePendingSvcRemoval( std::vector<fpi::SvcInfo> removedSvcs );
    

    fds_bool_t                    om_test_mode;
    OM_NodeContainer              *om_locDomain;
    kvstore::ConfigDB             *configDB;

    FSM_NodeDomain                *domain_fsm;
    // to protect access to msm process_event
    fds_mutex                     fsm_lock;

    // Vector to track registering services and
    // locks to protect accesses
    fds_rwlock                    svcRegMapLock;
    std::map<int64_t, SvcInfoPtr> registeringSvcs;

    bool                          domainDown;
    std::vector<int64_t>          shuttingDownNodes;
    uint32_t                      dmClusterSize;

    bool volumeGroupDMTFired;
    std::mutex                    taskMapMutex;
    std::unordered_map<int64_t, FdsTimerTaskPtr> setupNewNodeTaskMap;
};

extern OM_NodeDomainMod      gl_OMNodeDomainMod;

}  // namespace fds
#endif  // SOURCE_ORCH_MGR_INCLUDE_OMRESOURCES_H_
