/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <string>
#include <platform/platform-lib.h>

namespace fds {

Platform *gl_PlatformSvc;

// -------------------------------------------------------------------------------------
// Common Platform Services
// -------------------------------------------------------------------------------------
Platform::Platform(char const *const         name,
                   DomainNodeInv::pointer    node_inv,
                   DomainClusterMap::pointer cluster,
                   DomainResources::pointer  resources,
                   OmAgent::pointer          master)
    : Module(name), plf_master(master), plf_node_inv(node_inv),
      plf_clus_map(cluster), plf_resources(resources)
{
    plf_node_evt         = NULL;
    plf_vol_evt          = NULL;
    plf_throttle_evt     = NULL;
    plf_migrate_evt      = NULL;
    plf_tier_evt         = NULL;
    plf_bucket_stats_evt = NULL;
}

Platform::~Platform() {}

/**
 * Platform node/cluster methods.
 */

// plf_reg_node_info
// -----------------
//
void
Platform::plf_reg_node_info(const NodeUuid &uuid, const FdspNodeRegPtr msg)
{
    NodeAgent::pointer     newNode;
    DomainNodeInv::pointer local;

    local = plf_node_inventory();
    Error err = local->dc_register_node(uuid, msg, &newNode);
}

// plf_del_node_info
// -----------------
//
void
Platform::plf_del_node_info(const NodeUuid &uuid, const std::string &name)
{
    DomainNodeInv::pointer local;

    local = plf_node_inventory();
    Error err = local->dc_unregister_node(uuid, name);
}

// plf_update_cluster
// ------------------
//
void
Platform::plf_update_cluster()
{
}

// plf_persist_inventory
// ---------------------
//
void
Platform::plf_persist_inventory(const NodeUuid &uuid)
{
}

/**
 * Module methods.
 */
int
Platform::mod_init(SysParams const *const param)
{
    Module::mod_init(param);
    return 0;
}

void
Platform::mod_startup()
{
}

void
Platform::mod_shutdown()
{
}

}  // namespace fds
