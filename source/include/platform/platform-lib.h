/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_PLATFORM_PLATFORM_LIB_H_
#define SOURCE_INCLUDE_PLATFORM_PLATFORM_LIB_H_

#include <string>
#include <dlt.h>
#include <fds-shmobj.h>
#include <fds_process.h>
#include <fds_typedefs.h>
#include <kvstore/platformdb.h>
#include <platform/node-inventory.h>

namespace fds {

class Platform;

/**
 * On disk format: POD type to persist the node's inventory.
 */
typedef struct plat_node_data
{
    fds_uint32_t              nd_chksum;
    fds_uint32_t              nd_has_data;        /**< true/false if has valid data.  */
    fds_uint32_t              nd_magic;
    fds_uint64_t              nd_node_uuid;
    fds_uint32_t              nd_node_number;
    fds_uint32_t              nd_plat_port;
    fds_uint32_t              nd_om_port;
    fds_uint32_t              nd_flag_run_sm;
    fds_uint32_t              nd_flag_run_dm;
    fds_uint32_t              nd_flag_run_am;
    fds_uint32_t              nd_flag_run_om;
} plat_node_data_t;

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
                  PmContainer::pointer    pm,
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
                     PmContainer::pointer    pm,
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
    PlatEvent(char const *const          name,
              DomainResources::pointer   mgr,
              DomainClusterMap::pointer  clus,
              const Platform            *plat);

    virtual void plat_evt_handler(const FDSP_MsgHdrTypePtr &hdr) = 0;

  protected:
    char const *const          pe_name;
    DomainResources::pointer   pe_resources;
    DomainClusterMap::pointer  pe_clusmap;
    const Platform            *pe_platform;

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
    NodePlatEvent(DomainResources::pointer   mgr,
                  DomainClusterMap::pointer  clus,
                  const Platform            *plat)
        : PlatEvent("NodeEvent", mgr, clus, plat) {}

    virtual void plat_evt_handler(const FDSP_MsgHdrTypePtr &hdr);

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
    typedef boost::intrusive_ptr<VolPlatEvent> pointer;
    typedef boost::intrusive_ptr<const VolPlatEvent> const_ptr;

    virtual ~VolPlatEvent() {}
    VolPlatEvent(DomainResources::pointer   mgr,
                 DomainClusterMap::pointer  clus,
                 const Platform            *plat)
        : PlatEvent("VolEvent", mgr, clus, plat) {}

    virtual void plat_evt_handler(const FDSP_MsgHdrTypePtr &hdr);

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
extern Platform *gl_PlatformSvc;

class Platform : public Module
{
  public:
    typedef Platform const *const      ptr;

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
    static inline void platf_assign_singleton(Platform *ptr) { gl_PlatformSvc = ptr; }
    static inline Platform     *platf_singleton() { return gl_PlatformSvc; }
    static inline Platform::ptr platf_const_singleton() { return gl_PlatformSvc; }

    inline DomainNodeInv::pointer    plf_node_inventory() { return plf_node_inv; }
    inline DomainClusterMap::pointer plf_cluster_map() { return plf_clus_map; }
    inline DomainResources::pointer  plf_node_resoures() { return plf_resources; }

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
    static inline PmContainer::pointer plf_pm_nodes() {
        return platf_singleton()->plf_node_inv->dc_get_pm_nodes();
    }
    static inline const NodeUuid &plf_my_node_uuid() {
        return platf_singleton()->plf_my_uuid;
    }
    static inline fds_uint32_t plf_ctrl_port(fds_uint32_t base) { return base; }
    static inline fds_uint32_t plf_conf_port(fds_uint32_t base) { return base + 1; }
    static inline fds_uint32_t plf_data_port(fds_uint32_t base) { return base + 2; }
    static inline fds_uint32_t plf_nsvc_port(fds_uint32_t base) { return base + 4; }
    static inline fds_uint32_t plf_migration_port(fds_uint32_t base) { return base + 3; }

    /**
     * Return service uuid from node uuid and service type.
     */
    static void plf_svc_uuid_from_node(const NodeUuid &node, NodeUuid *, FDSP_MgrIdType);

    /**
     * Platform methods.
     */
    bool plf_is_om_node();
    void plf_run_server(bool spawn_thr = false);
    void plf_rpc_om_handshake(fpi::FDSP_RegisterNodeTypePtr pkt);
    void plf_change_info(const plat_node_data_t *ndata);

    /**
     * Platform node/cluster inventory methods.
     */
    virtual void plf_reg_node_info(const NodeUuid &uuid, const FdspNodeRegPtr msg);
    virtual void plf_del_node_info(const NodeUuid &uuid, const std::string &name);
    virtual void plf_update_cluster();

    /**
     * Resource inventory methods.
     */
    virtual void plf_create_domain(const FdspCrtDomPtr &msg);
    virtual void plf_delete_domain(const FdspCrtDomPtr &msg);

    /**
     * Module methods.
     */
    virtual int  mod_init(SysParams const *const param) override;
    virtual void mod_startup() override;
    virtual void mod_shutdown() override;
    virtual void mod_enable_service() override;

    /**
     * Pull out common platform data.
     */
    inline FDSP_MgrIdType plf_get_node_type() const { return plf_node_type; }
    inline fds_uint32_t   plf_get_om_ctrl_port() const { return plf_om_ctrl_port; }
    inline fds_uint32_t   plf_get_om_svc_port() const { return plf_om_svc_port; }
    inline fds_uint32_t   plf_get_my_ctrl_port() const { return plf_my_ctrl_port; }
    inline fds_uint32_t   plf_get_my_conf_port() const { return plf_my_conf_port; }
    inline fds_uint32_t   plf_get_my_data_port() const { return plf_my_data_port; }
    inline fds_uint32_t   plf_get_dm_port() const {
        fds_assert(!"Not impl"); return 0;
    }
    inline fds_uint32_t   plf_get_my_nsvc_port() const { return plf_my_nsvc_port; }
    inline fds_uint32_t   plf_get_my_migration_port() const { return plf_my_migr_port; }

    inline std::string const *const plf_get_my_name() const { return &plf_my_node_name; }
    inline std::string const *const plf_get_my_ip() const { return &plf_my_ip; }
    inline std::string const *const plf_get_om_ip() const { return &plf_om_ip_str; }
    inline std::string const *const plf_get_auto_node_name() const {
        return &plf_my_auto_name;
    }
    inline OmAgent::pointer      plf_om_master() const { return plf_master; }
    inline PmAgent::pointer      plf_domain_ctrl() const { return plf_domain; }
    inline NodeUuid const *const plf_get_my_uuid() const { return &plf_my_uuid; }
    inline NodeUuid const *const plf_get_my_svc_uuid() const { return &plf_my_svc_uuid; }
    inline NodeUuid const *const plf_get_my_plf_svc_uuid() const {
        return &plf_my_plf_svc_uuid;
    }

  protected:
    friend class PlatRpcReqt;
    friend class PlatRpcResp;

    FDSP_MgrIdType             plf_node_type;
    NodeUuid                   plf_my_uuid;           /**< this node HW uuid.         */
    NodeUuid                   plf_my_svc_uuid;       /**< SM/DM/AM... svc uuid.      */
    NodeUuid                   plf_my_plf_svc_uuid;   /**< platform service.          */
    std::string                plf_my_node_name;      /**< user assigned node name.   */
    std::string                plf_my_auto_name;      /**< domain assigned auto node. */
    std::string                plf_my_ip;
    std::string                plf_om_ip_str;
    fds_uint32_t               plf_om_ctrl_port;
    fds_uint32_t               plf_om_svc_port;
    fds_uint32_t               plf_my_ctrl_port;
    fds_uint32_t               plf_my_conf_port;
    fds_uint32_t               plf_my_data_port;
    fds_uint32_t               plf_my_nsvc_port;
    fds_uint32_t               plf_my_migr_port;

    PmAgent::pointer           plf_domain;
    OmAgent::pointer           plf_master;
    DomainNodeInv::pointer     plf_node_inv;
    DomainClusterMap::pointer  plf_clus_map;
    DomainResources::pointer   plf_resources;

    /**
     * Specific platform event handlers.
     */
    NodePlatEvent::pointer     plf_node_evt;
    VolPlatEvent::pointer      plf_vol_evt;
    PlatEvent::pointer         plf_throttle_evt;
    PlatEvent::pointer         plf_migrate_evt;
    PlatEvent::pointer         plf_tier_evt;
    PlatEvent::pointer         plf_bucket_stats_evt;

    /* Server attributes. */
    boost::shared_ptr<netSessionTbl>  plf_net_sess;
    boost::shared_ptr<PlatRpcReqt>    plf_rpc_reqt;  /**< rpc handler for OM reqt.   */
    boost::shared_ptr<std::thread>    plf_rpc_thrd;  /**< thread running rpc handler */
    OmRespDispatchPtr                 plf_om_resp;   /**< RPC client response disp.  */
    NodeAgentDpRespPtr                plf_dpath_resp;
    netControlPathServerSession      *plf_my_sess;

    /**
     * Required Factory methods.
     */
    virtual PlatRpcReqt      *plat_creat_reqt_disp() = 0;
    virtual PlatRpcResp      *plat_creat_resp_disp() = 0;
    virtual PlatDataPathResp *plat_creat_dpath_resp() { return NULL; }

  private:
    void plf_rpc_server_thread();
};

/**
 * FDS Platform daemon process.
 */
class PlatformProcess : public FdsProcess
{
  public:
    virtual ~PlatformProcess();
    PlatformProcess(int argc, char *argv[],
                    const std::string &cfg_path,
                    const std::string &log_file,
                    Platform *platform, Module **vec);
    /* Exposed for mock testing */
    PlatformProcess() {}

    virtual void proc_pre_startup() override;

    /**
     * Derrive ports for different node services from a common base.
     */
    static inline fds_uint32_t
    plf_get_platform_port(fds_uint32_t base, fds_uint32_t node) {
        return base + (node * 100);
    }
    /**
     * The base port for all services is the platform daemon.  Other services such as
     * SM/DM/AM base ports are derrived from the platform port.
     */
    static inline fds_uint32_t plf_get_sm_port(fds_uint32_t plat) { return plat + 10; }
    static inline fds_uint32_t plf_get_dm_port(fds_uint32_t plat) { return plat + 20; }
    static inline fds_uint32_t plf_get_am_port(fds_uint32_t plat) { return plat + 30; }

    /**
     * Return platform manager from the global singleton.
     */
    static inline Platform *plf_manager() {
        return static_cast<PlatformProcess *>(g_fdsprocess)->plf_mgr;
    }

  protected:
    Platform                 *plf_mgr;
    kvstore::PlatformDB      *plf_db;

    fds_bool_t                plf_test_mode;
    fds_bool_t                plf_stand_alone;
    std::string               plf_db_key;
    plat_node_data_t          plf_node_data;

    virtual void plf_load_node_data();
    virtual void plf_apply_node_data();
};


/*
 * -------------------------------------------------------------------------------------
 * Platform lib shared memory queue handlers
 * -------------------------------------------------------------------------------------
 */
class PlatLibUuidBind : public ShmqReqIn
{
  public:
    PlatLibUuidBind() : ShmqReqIn() {}
    virtual ~PlatLibUuidBind() {}

    void shmq_handler(const shmq_req_t *in, size_t size) override;
};

extern PlatLibUuidBind  platlib_uuid_bind;

/* TODO(Vy): need to remove this code. */
namespace util { extern std::string get_local_ip(); }

}  // namespace fds

#endif  // SOURCE_INCLUDE_PLATFORM_PLATFORM_LIB_H_
