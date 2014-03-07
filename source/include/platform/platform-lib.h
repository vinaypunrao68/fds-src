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
#include <platform/node-inventory.h>

namespace fds {

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
    explicit PlatEvent(char const *const name);

    virtual void plat_evt_handler() = 0;

  protected:
    char const *const        pe_name;

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
    NodePlatEvent() : PlatEvent("NodeEvent") {}

    virtual void plat_evt_handler();
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
    std::string                plf_om_ip_str;
    std::string                plf_host_ip;
    fds_uint32_t               plf_om_conf_port;
    fds_uint32_t               plf_my_ctrl_port;
    fds_uint32_t               plf_my_data_port;
    fds_uint32_t               plf_my_migration_port;

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
};


};  // namespace fds

#endif  // SOURCE_INCLUDE_PLATFORM_PLATFORM_LIB_H_
