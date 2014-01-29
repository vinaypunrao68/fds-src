/*
 * Copyright 2013 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_ORCH_MGR_INCLUDE_OMRESOURCES_H_
#define SOURCE_ORCH_MGR_INCLUDE_OMRESOURCES_H_

#include <string>
#include <fds_module.h>
#include <fds_resource.h>
#include <OmTypes.h>

namespace fds {

class OM_ClusDomainMod;

/**
 * Replacement for NodeInfo object.
 */
class NodeInventory : public Resource
{
  public:
    int  node_index() const { return 0; }
    void node_name(std::string *name) const {}
    ResourceUUID get_uuid() const { return nd_uuid; }

    /**
     * Return the storage weight of the node in normalized unit from 0...1000
     */
    virtual int node_stor_weight() const { return 200; }

  protected:
    friend class OM_ClusDomainMod;

    Sha1Digest               nd_checksum;
    ResourceUUID             nd_uuid;
    fds_uint64_t             nd_gbyte_cap;               /**< capacity in GB unit */

    explicit NodeInventory(ResourceUUID *uuid) : nd_uuid(0) {}
    ~NodeInventory() {}

    virtual int node_calc_stor_weight() { return 0; }
};

/**
 * Agent interface to communicate with the remote node.  This is the communication
 * end-point to the node.
 *
 * It's normal that the node agent is there but the transport may not be availble.
 * We'll provide methods to establish the transport in the background and error
 * handling model when the transport is broken.
 */
class NodeAgent : public NodeInventory
{
  public:
  protected:
    friend class OM_ClusDomainMod;

    explicit NodeAgent(ResourceUUID *uuid) : NodeInventory(uuid) {}
    ~NodeAgent() {}
};

/**
 * Cluster domain manager.  Manage all nodes connected and known to the domain.
 * These nodes may not be in ClusterMap membership.
 */
class OM_ClusDomainMod : public Module
{
  public:
    explicit OM_ClusDomainMod(char const *const name) : Module(name) {}
    ~OM_ClusDomainMod() {}

    static OM_ClusDomainMod *om_local_domain() { return NULL; }

    /**
     * Iterate through the list of nodes by index 0...n to retrieve their
     * agent objects.
     */
    virtual int om_avail_nodes() { return 16; }
    const NodeAgent *om_node_info(int node_num) { return NULL; }
    const NodeAgent *om_node_info(const ResourceUUID &uuid) { return NULL; }

    /**
     * Register node info to the domain manager.
     */
    virtual void om_reg_node_info(const ResourceUUID &uuid) {}
    virtual void om_del_node_info(const ResourceUUID &uuid) {}
    virtual void om_persist_node_info(int node_num) {}

    /**
     * Module methods
     */
    virtual int  mod_init(SysParams const *const param) {}
    virtual void mod_startup() {}
    virtual void mod_shutdown() {}
  protected:
};

extern OM_ClusDomainMod      gl_OMLocNodeDomain;

}  // namespace fds
#endif  // SOURCE_ORCH_MGR_INCLUDE_OMRESOURCES_H_
