/*
 * Copyright 2013 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_ORCH_MGR_INCLUDE_OMRESOURCES_H_
#define SOURCE_ORCH_MGR_INCLUDE_OMRESOURCES_H_

#include <string>
#include <unordered_map>
#include <fds_module.h>
#include <fds_resource.h>
#include <fdsp/FDSP_types.h>
#include <concurrency/Mutex.h>
#include <OmTypes.h>

namespace fds {

class OM_ClusDomainMod;

/**
 * Replacement for NodeInfo object.
 */
class NodeInventory : public Resource
{
  public:
    typedef boost::intrusive_ptr<NodeInventory> pointer;

    int  node_index() const { return 0; }
    void node_name(std::string *name) const {}
    ResourceUUID get_uuid() const { return nd_uuid; }

    /**
     * Return the storage weight of the node in normalized unit from 0...1000
     */
    virtual int node_stor_weight() const { return 200; }

    /**
     * Return the mutex protecting this object.
     */
    inline fds_mutex *node_mutex() {
        return &nd_mtx;
    }

  protected:
    friend class OM_ClusDomainMod;

    Sha1Digest               nd_checksum;
    ResourceUUID             nd_uuid;
    fds_uint64_t             nd_gbyte_cap;               /**< capacity in GB unit */
    fds_mutex                nd_mtx;                     /**< protecting mutex. */

    /* TODO: (vy) just porting from NodeInfo now. */
    fds_uint32_t             nd_ip_addr;
    fds_uint32_t             nd_data_port;
    fds_uint32_t             nd_ctrl_port;

    explicit NodeInventory(const ResourceUUID &uuid)
            : nd_uuid(uuid), nd_mtx("node mtx") {}
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
    typedef boost::intrusive_ptr<NodeAgent> pointer;

    explicit NodeAgent(const ResourceUUID &uuid) : NodeInventory(uuid) {}
    ~NodeAgent() {}

  protected:
    friend class OM_ClusDomainMod;
};

/**
 * Type that maps a node's UUID to its agent object.
 */
typedef std::unordered_map<NodeUuid, NodeAgent::pointer, UuidHash> NodeMap;
typedef FDS_ProtocolInterface::FDSP_RegisterNodeTypePtr  FdspNodeRegMsg;

/**
 * Cluster domain manager.  Manage all nodes connected and known to the domain.
 * These nodes may not be in ClusterMap membership.
 */
class OM_NodeDomainMod : public Module
{
  public:
    explicit OM_NodeDomainMod(char const *const name);
    ~OM_NodeDomainMod();

    static OM_NodeDomainMod *om_local_domain() { return NULL; }

    /**
     * Iterate through the list of nodes by index 0...n to retrieve their
     * agent objects.
     */
    virtual int om_avail_nodes();
    NodeAgent::pointer om_node_info(int node_num);
    NodeAgent::pointer om_node_info(const ResourceUUID &uuid);

    /**
     * Register node info to the domain manager.
     */
    virtual void
    om_reg_node_info(const ResourceUUID   &uuid,
                     const FdspNodeRegMsg *msg);

    virtual void om_del_node_info(const ResourceUUID &uuid);
    virtual void om_persist_node_info(int node_num);

    /**
     * Module methods
     */
    virtual int  mod_init(SysParams const *const param);
    virtual void mod_startup();
    virtual void mod_shutdown();

  protected:
};

extern OM_NodeDomainMod      gl_OMNodeDomainMod;

}  // namespace fds
#endif  // SOURCE_ORCH_MGR_INCLUDE_OMRESOURCES_H_
