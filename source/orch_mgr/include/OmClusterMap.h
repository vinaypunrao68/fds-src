/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

/** Defines the classes used for data placement and routing tables */
#ifndef SOURCE_ORCH_MGR_INCLUDE_OMCLUSTERMAP_H_
#define SOURCE_ORCH_MGR_INCLUDE_OMCLUSTERMAP_H_

#include <unordered_map>
#include <list>
#include <string>
#include <atomic>
#include <unordered_set>

#include <fds_types.h>
#include <fds_typedefs.h>
#include <fds_err.h>
#include <fds_module.h>
#include <fds_placement_table.h>
#include <concurrency/Mutex.h>
#include <OmResources.h>

namespace fds {

/**
 * Type that maps a Node's UUID to it's agent descriptor.
 */
typedef std::atomic<fds_uint64_t>           AtomicMapVersion;
typedef std::unordered_map<NodeUuid, OM_SmAgent::pointer, UuidHash> NodeMap;
typedef std::unordered_set<NodeUuid, UuidHash> NodeUuidSet;

/**
 * Defines the current state of the cluster at given points in time.
 * The cluster map specifies the current members of the cluster.
 */
class ClusterMap : public Module {
 protected:
    NodeMap           currClustMap;  /**< Current storage nodes in cluster */

    /**
     * Cached set of nodes added since the previous
     * DLT to create the current cluster map.
     */
    std::unordered_set<NodeUuid, UuidHash> addedNodes;
    /**
     * Cached list of nodes removed since the previous
     * DLT to create the current cluster map.
     */
    std::unordered_set<NodeUuid, UuidHash> removedNodes;

    /**
     * Current version of the map.
     * The version is monotonically
     * increasing.
     */
    AtomicMapVersion  version;
    Sha1Digest        checksum;   /**< Content Checksum */
    fds_mutex         *mapMutex;  /**< Protects the map */

 public:
    ClusterMap();
    ~ClusterMap();

    typedef NodeMap::const_iterator const_iterator;
    /**
     * Returns a const map iterator. Note
     * the iterator is NOT thread safe, so
     * the placement lock should be held
     * during iteration.
     */
    const_iterator cbegin() const;
    const_iterator cend() const;

    /**
     * Need some functions to serialize the map
     */

    /**
     * Returns the current number of cluster members.
     */
    fds_uint32_t getNumMembers() const;
    /**
     * Returns member info based on the nodes membership
     * index number.
     */
    const NodeAgent *om_member_info(int node_idx);
    /**
     * Returns member info based on the nodes UUID.
     */
    const NodeAgent *om_member_info(const NodeUuid &uuid);
    /**
     * Returns total storage weight of all cluster members 
     */
    fds_uint64_t getTotalStorWeight() const;

    /**
     * Update the current cluster map.
     */
    Error updateMap(const NodeList &addNodes,
                    const NodeList &rmNodes);

    /**
     * Returns a copy of the list of nodes added
     * since previous cluster map version.
     */
    std::unordered_set<NodeUuid, UuidHash> getAddedNodes() const;
    /**
     * Returns a copy of the list of nodes removed
     * since previous cluster map version.
     */
    std::unordered_set<NodeUuid, UuidHash> getRemovedNodes() const;

    /**
     * Module methods.
     */
    virtual int  mod_init(SysParams const *const param);
    virtual void mod_startup();
    virtual void mod_shutdown();
};

extern ClusterMap gl_OMClusMapMod;
}  // namespace fds

#endif  // SOURCE_ORCH_MGR_INCLUDE_OMCLUSTERMAP_H_
