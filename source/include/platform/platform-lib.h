/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_PLATFORM_PLATFORM_LIB_H_
#define SOURCE_INCLUDE_PLATFORM_PLATFORM_LIB_H_

#include <string>
#include <dlt.h>
#include <fds_module.h>
#include <fdsp/FDSP_types.h>
#include <fdsp/FDSP_types.h>
#include <NetSession.h>
#include <platform/node-inventory.h>

namespace fds {

class PlatRpcReq;
typedef boost::shared_ptr<fpi::FDSP_ControlPathReqClient>     NodeAgentCpSessPtr;
typedef boost::shared_ptr<fpi::FDSP_OMControlPathReqClient>   NodeAgentCpOmClientPtr;

// -------------------------------------------------------------------------------------
// Node Inventory and Cluster Map
// -------------------------------------------------------------------------------------
class DomainNodeInv : public DomainContainer
{
  public:
    virtual ~DomainNodeInv();
    DomainNodeInv(char const *const       name);
    DomainNodeInv(char const *const       name,
                  OmAgent::pointer        master,
                  SmContainer::pointer    sm,
                  DmContainer::pointer    dm,
                  AmContainer::pointer    am,
                  OmContainer::pointer    om);
};

class DomainClusterMap : public DomainNodeInv
{
  public:
    virtual ~DomainClusterMap();
    DomainClusterMap(char const *const       name);
    DomainClusterMap(char const *const       name,
                     OmAgent::pointer        master,
                     SmContainer::pointer    sm,
                     DmContainer::pointer    dm,
                     AmContainer::pointer    am,
                     OmContainer::pointer    om);
};

// -------------------------------------------------------------------------------------
// Common Platform Resources
// -------------------------------------------------------------------------------------
class DomainResources
{
  public:
    typedef boost::intrusive_ptr<DomainResources> pointer;
    typedef boost::intrusive_ptr<const DomainResources> const_ptr;

    virtual ~DomainResources();
    DomainResources(char const *const name);

  protected:
    ResourceUUID               drs_tent_id;
    ResourceUUID               drs_domain_id;

    const DLT                 *drs_dlt;
    DLTManager                 drs_dltMgr;
    float                      drs_cur_throttle_lvl;

    // Will use new DMT
    int                        drs_dmt_version;
    Node_Table_Type            drs_dmt;

  private:
    mutable boost::atomic<int>  rs_refcnt;
    friend void intrusive_ptr_add_ref(const DomainResources *x) {
        x->rs_refcnt.fetch_add(1, boost::memory_order_relaxed);
    }
    friend void intrusive_ptr_release(const DomainResources *x) {
        if (x->rs_refcnt.fetch_sub(1, boost::memory_order_release) == 1) {
            boost::atomic_thread_fence(boost::memory_order_acquire);
            delete x;
        }
    }
};

// -------------------------------------------------------------------------------------
// Generic Platform Event Handler
// -------------------------------------------------------------------------------------
class PlatEvent
{
  public:
    typedef boost::intrusive_ptr<PlatEvent> pointer;
    typedef boost::intrusive_ptr<const PlatEvent> const_ptr;

    virtual ~PlatEvent();
    PlatEvent(char const *const         name,
              DomainResources::pointer  mgr,
              DomainClusterMap::pointer clus);
    virtual void plat_evt_handler(const FDSP_MsgHdrTypePtr &hdr) = 0;

  protected:
    char const *const          pe_name;
    DomainResources::pointer   pe_resources;
    DomainClusterMap::pointer  pe_clusmap;

  private:
    mutable boost::atomic<int>  rs_refcnt;
    friend void intrusive_ptr_add_ref(const PlatEvent *x) {
        x->rs_refcnt.fetch_add(1, boost::memory_order_relaxed);
    }
    friend void intrusive_ptr_release(const PlatEvent *x) {
        if (x->rs_refcnt.fetch_sub(1, boost::memory_order_release) == 1) {
            boost::atomic_thread_fence(boost::memory_order_acquire);
            delete x;
        }
    }
};

class NodePlatEvent : public PlatEvent
{
  public:
    typedef boost::intrusive_ptr<NodePlatEvent> pointer;
    typedef boost::intrusive_ptr<const NodePlatEvent> const_ptr;

    virtual ~NodePlatEvent() {}
    NodePlatEvent(DomainResources::pointer  mgr,
                  DomainClusterMap::pointer clus)
        : PlatEvent("NodeEvent", mgr, clus) {}

    virtual void plat_evt_handler(const FDSP_MsgHdrTypePtr &hdr);

  protected:
    virtual int
    plat_recvNodeEvent(const FDSP_MsgHdrTypePtr &hdr, const FDSP_Node_Info_Type &evt);

    virtual int
    plat_recvMigrationEvent(const FDSP_MsgHdrTypePtr &hdr, const FDSP_DLT_Data_Type &dlt);

    virtual int
    plat_recvDLTStartMigration(const FDSP_MsgHdrTypePtr    &hdr,
                               const FDSP_DLT_Data_TypePtr &dlt);

    virtual int
    plat_recvDLTUpdate(const FDSP_MsgHdrTypePtr &hdr, const FDSP_DLT_Data_Type &dlt);

    virtual int
    plat_recvDMTUpdate(const FDSP_MsgHdrTypePtr &hdr, const FDSP_DMT_Type &dmt);
};

class VolPlatEvent : public PlatEvent
{
  public:
    typedef boost::intrusive_ptr<NodePlatEvent> pointer;
    typedef boost::intrusive_ptr<const NodePlatEvent> const_ptr;

    virtual ~VolPlatEvent() {}
    VolPlatEvent(DomainResources::pointer  mgr,
                 DomainClusterMap::pointer clus)
        : PlatEvent("VolEvent", mgr, clus) {}

    virtual void plat_evt_handler(const FDSP_MsgHdrTypePtr &hdr);

  protected:
    virtual int
    plat_set_throttle(const FDSP_MsgHdrTypePtr      &hdr,
                      const FDSP_ThrottleMsgTypePtr &msg);

    virtual int
    plat_bucket_stat(const FDSP_MsgHdrTypePtr          &hdr,
                     const FDSP_BucketStatsRespTypePtr &msg);

    virtual int
    plat_add_vol(const FDSP_MsgHdrTypePtr &hdr, const FDSP_NotifyVolTypePtr &add);

    virtual int
    plat_rm_vol(const FDSP_MsgHdrTypePtr &hdr, const FDSP_NotifyVolTypePtr &rm);
};

// -------------------------------------------------------------------------------------
// Common Platform Services
// -------------------------------------------------------------------------------------
class Platform;
extern Platform *gl_PlatformSvc;

class Platform : public Module
{
  public:
    virtual ~Platform();
    Platform(char const *const         name,
             FDSP_MgrIdType            node_type,
             DomainNodeInv::pointer    node_inv,
             DomainClusterMap::pointer cluster,
             DomainResources::pointer  resources,
             OmAgent::pointer          master);

    /**
     * Short cuts to retrieve important objects.
     */
    static inline void platf_assign_singleton(Platform *ptr) {
        gl_PlatformSvc = ptr;
    }
    static inline Platform *platf_singleton() {
        return gl_PlatformSvc;
    }
    static inline SmContainer::pointer plf_sm_nodes() {
        return platf_singleton()->plf_node_inv->dc_get_sm_nodes();
    }
    static inline SmContainer::pointer plf_sm_cluster() {
        return platf_singleton()->plf_clus_map->dc_get_sm_nodes();
    }
    static inline DmContainer::pointer plf_dm_nodes() {
        return platf_singleton()->plf_node_inv->dc_get_dm_nodes();
    }
    static inline DmContainer::pointer plf_dm_cluster() {
        return platf_singleton()->plf_clus_map->dc_get_dm_nodes();
    }
    static inline AmContainer::pointer plf_am_nodes() {
        return platf_singleton()->plf_node_inv->dc_get_am_nodes();
    }
    static inline AmContainer::pointer plf_am_cluster() {
        return platf_singleton()->plf_clus_map->dc_get_am_nodes();
    }
    static inline const NodeUuid &plf_my_node_uuid() {
        return platf_singleton()->plf_my_uuid;
    }
    inline DomainNodeInv::pointer plf_node_inventory() {
        return plf_node_inv;
    }
    inline DomainClusterMap::pointer plf_cluster_map() {
        return plf_clus_map;
    }
    inline DomainResources::pointer plf_node_resoures() {
        return plf_resources;
    }

    /**
     * Platform node/cluster inventory methods.
     */
    virtual void plf_reg_node_info(const NodeUuid &uuid, const FdspNodeRegPtr msg);
    virtual void plf_del_node_info(const NodeUuid &uuid, const std::string &name);
    virtual void plf_update_cluster();
    virtual void plf_persist_inventory(const NodeUuid &uuid);

    /**
     * Resource inventory methods.
     */
    virtual void plf_create_domain(const FdspCrtDomPtr &msg);
    virtual void plf_delete_domain(const FdspCrtDomPtr &msg);

    /**
     * Module methods.
     */
    virtual int  mod_init(SysParams const *const param);
    virtual void mod_startup();
    virtual void mod_shutdown();

  protected:
    FDSP_MgrIdType             plf_node_type;
    NodeUuid                   plf_my_uuid;
    std::string                plf_my_node_name;
    std::string                plf_my_ip;
    std::string                plf_om_ip_str;
    fds_uint32_t               plf_om_conf_port;
    fds_uint32_t               plf_my_ctrl_port;
    fds_uint32_t               plf_my_data_port;
    fds_uint32_t               plf_my_migration_port;
    fds_bool_t                 plf_test_mode;

    OmAgent::pointer           plf_master;
    DomainNodeInv::pointer     plf_node_inv;
    DomainClusterMap::pointer  plf_clus_map;
    DomainResources::pointer   plf_resources;

    /**
     * Specific platform event handlers.
     */
    PlatEvent::pointer         plf_node_evt;
    PlatEvent::pointer         plf_vol_evt;
    PlatEvent::pointer         plf_throttle_evt;
    PlatEvent::pointer         plf_migrate_evt;
    PlatEvent::pointer         plf_tier_evt;
    PlatEvent::pointer         plf_bucket_stats_evt;

    /* Server variables. */
    boost::shared_ptr<netSessionTbl>  plf_net_sess;
    boost::shared_ptr<PlatRpcReq>     plf_rpc_reqt;  /**< rpc handler for OM reqt.   */
    boost::shared_ptr<std::thread>    plf_rpc_thrd;  /**< thread running rpc handler */
    netControlPathServerSession      *plf_rpc_sess;  /**< associated session.        */

    /* Client variables. */
    NodeAgentCpSessPtr                plf_clnt_sess; /**< client ctrl path session.  */
    NodeAgentCpOmClientPtr            plf_client;    /**< client OM ctrl path.       */
    std::string                       plf_client_id;

    /**
     * Required Factory method.
     */
    virtual PlatRpcReq *plat_creat_rpc_handler() = 0;

  private:
    void plf_rpc_server_thread();
};

class PlatRpcReq : public fpi::FDSP_ControlPathReqIf
{
  public:
    PlatRpcReq();
    virtual ~PlatRpcReq();

    void NotifyAddVol(const FDSP_MsgHdrType &, const FDSP_NotifyVolType &);
    virtual void NotifyAddVol(fpi::FDSP_MsgHdrTypePtr    &msg_hdr,
                              fpi::FDSP_NotifyVolTypePtr &vol_msg);

    void NotifyRmVol(const FDSP_MsgHdrType &, const FDSP_NotifyVolType &);
    virtual void NotifyRmVol(fpi::FDSP_MsgHdrTypePtr    &msg_hdr,
                             fpi::FDSP_NotifyVolTypePtr &vol_msg);

    void NotifyModVol(const FDSP_MsgHdrType &, const FDSP_NotifyVolType &);
    virtual void NotifyModVol(fpi::FDSP_MsgHdrTypePtr    &msg_hdr,
                              fpi::FDSP_NotifyVolTypePtr &vol_msg);

    void AttachVol(const FDSP_MsgHdrType &, const FDSP_AttachVolType &);
    virtual void AttachVol(fpi::FDSP_MsgHdrTypePtr    &msg_hdr,
                           fpi::FDSP_AttachVolTypePtr &vol_msg);

    void DetachVol(const FDSP_MsgHdrType &, const FDSP_AttachVolType &);
    virtual void DetachVol(fpi::FDSP_MsgHdrTypePtr    &msg_hdr,
                           fpi::FDSP_AttachVolTypePtr &vol_msg);

    void NotifyNodeAdd(const FDSP_MsgHdrType &, const FDSP_Node_Info_Type &);
    void NotifyNodeAdd(fpi::FDSP_MsgHdrTypePtr     &msg_hdr,
                       fpi::FDSP_Node_Info_TypePtr &node_info);

    void NotifyNodeRmv(const FDSP_MsgHdrType &, const FDSP_Node_Info_Type &);
    void NotifyNodeRmv(fpi::FDSP_MsgHdrTypePtr     &msg_hdr,
                       fpi::FDSP_Node_Info_TypePtr &node_info);

    void NotifyDLTUpdate(const FDSP_MsgHdrType &, const FDSP_DLT_Data_Type &);
    void NotifyDLTUpdate(fpi::FDSP_MsgHdrTypePtr    &msg_hdr,
                         fpi::FDSP_DLT_Data_TypePtr &dlt_info);

    void NotifyDMTUpdate(const FDSP_MsgHdrType &, const FDSP_DMT_Type &);
    void NotifyDMTUpdate(fpi::FDSP_MsgHdrTypePtr &msg_hdr,
                         fpi::FDSP_DMT_TypePtr   &dmt_info);

    void SetThrottleLevel(const FDSP_MsgHdrType &, const FDSP_ThrottleMsgType &);
    void SetThrottleLevel(fpi::FDSP_MsgHdrTypePtr      &msg_hdr,
                          fpi::FDSP_ThrottleMsgTypePtr &throttle_msg);

    void TierPolicy(const FDSP_TierPolicy &tier);
    void TierPolicy(fpi::FDSP_TierPolicyPtr &tier);

    void TierPolicyAudit(const FDSP_TierPolicyAudit &audit);
    void TierPolicyAudit(fpi::FDSP_TierPolicyAuditPtr &audit);

    void NotifyBucketStats(const fpi::FDSP_MsgHdrType          &msg_hdr,
                           const fpi::FDSP_BucketStatsRespType &buck_stats_msg);
    void NotifyBucketStats(fpi::FDSP_MsgHdrTypePtr          &msg_hdr,
                           fpi::FDSP_BucketStatsRespTypePtr &buck_stats_msg);

    void NotifyStartMigration(const fpi::FDSP_MsgHdrType    &msg_hdr,
                              const fpi::FDSP_DLT_Data_Type &dlt_info);
    void NotifyStartMigration(fpi::FDSP_MsgHdrTypePtr    &msg_hdr,
                              fpi::FDSP_DLT_Data_TypePtr &dlt_info);
};

};  // namespace fds

#endif  // SOURCE_INCLUDE_PLATFORM_PLATFORM_LIB_H_
