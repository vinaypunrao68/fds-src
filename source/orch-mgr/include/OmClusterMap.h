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
#include <fds_error.h>
#include <fds_module.h>
#include <concurrency/Mutex.h>
#include <OmResources.h>

namespace fds {

/**
 * Type that maps a Node's UUID to it's agent descriptor.
 */
typedef std::atomic<fds_uint64_t>           AtomicMapVersion;
typedef std::unordered_map<NodeUuid, OM_SmAgent::pointer, UuidHash> SmMap;
typedef std::unordered_map<NodeUuid, OM_DmAgent::pointer, UuidHash> DmMap;

/**
 * Defines the current state of the cluster at given points in time.
 * The cluster map specifies the current members of the cluster.
 */
class ClusterMap : public Module {
 protected:
    SmMap  curSmMap;  /**< Current storage nodes in cluster */
    DmMap  curDmMap;  /**< Current DM services in cluster */

    /**
     * Cached set of SM services added since the previous
     * DLT to create the current cluster map.
     */
    std::unordered_set<NodeUuid, UuidHash> addedSMs;
    /**
     * Cached list of SM services removed since the previous
     * DLT to create the current cluster map.
     */
    std::unordered_set<NodeUuid, UuidHash> removedSMs;
    /**
     * Cached set of DM services added since the currently
     * committed DMT
     */
    std::unordered_set<NodeUuid, UuidHash> addedDMs;
    /**
     * Cached set of DM services removed since the currently
     * committed DMT
     */
    std::unordered_set<NodeUuid, UuidHash> removedDMs;


    /**
     * Current version of the combined SM and DM map
     * The version is monotonically increasing.
     */
    AtomicMapVersion  version;

    // TODO(Andrew): Add back a better checksum library
    // Sha1Digest        checksum;   /**< Content Checksum */
    fds_mutex mapMutex;  /**< Protects the map */

 public:
    ClusterMap();
    ~ClusterMap();

    typedef SmMap::const_iterator const_sm_iterator;
    typedef DmMap::const_iterator const_dm_iterator;

    /**
     * Returns a const map iterator. Note
     * the iterator is NOT thread safe, so
     * the placement lock should be held
     * during iteration.
     */
    const_sm_iterator cbegin_sm() const;
    const_sm_iterator cend_sm() const;
    const_dm_iterator cbegin_dm() const;
    const_dm_iterator cend_dm() const;

    /**
     * Need some functions to serialize the map
     */

    /**
     * Returns the current number of cluster members of
     * a given service type
     * @param svc_type service type: SM or DM
     */
    fds_uint32_t getNumMembers(fpi::FDSP_MgrIdType svc_type) const;

    /**
     * Returns the current number of UP cluster members of
     * a given service type
     * @param svc_type service type: SM or DM
     */
    fds_uint32_t getNumNonfailedMembers(fpi::FDSP_MgrIdType svc_type) const;

    /**
     * Returns member info based on the nodes membership
     * index number.
     */
    const NodeAgent *om_member_info(fpi::FDSP_MgrIdType svc_type,
                                    int node_idx);
    /**
     * Returns member info based on the nodes UUID.
     */
    const NodeAgent *om_member_info(const NodeUuid &uuid);
    /**
     * Returns total storage weight of all SM cluster members 
     */
    fds_uint64_t getTotalStorWeight() const;

    /**
     * Update the current cluster map.
     */
    Error updateMap(fpi::FDSP_MgrIdType svc_type,
                    const NodeList &addNodes,
                    const NodeList &rmNodes);

    /**
     * Returns a copy of the list of services added
     * since previous cluster map version.
     */
    std::unordered_set<NodeUuid, UuidHash>
            getAddedServices(fpi::FDSP_MgrIdType svc_type) const;
    /**
     * Returns a copy of the list of services removed
     * since previous cluster map version.
     */
    std::unordered_set<NodeUuid, UuidHash>
            getRemovedServices(fpi::FDSP_MgrIdType svc_type) const;


    /**
     * Returns a list of services that are in non-failed state
     */
    NodeUuidSet getNonfailedServices(fpi::FDSP_MgrIdType svc_type) const;
    NodeUuidSet getFailedServices(fpi::FDSP_MgrIdType svc_type) const;
    NodeUuidSet getServiceUuids(fpi::FDSP_MgrIdType svc_type) const;

    void resetPendServices(fpi::FDSP_MgrIdType svc_type);
    /**
     * Adds given uuid to removed nodes set, this node should not
     * be in current cluster map. This method is used on OM bringup
     * from persistent state when not all nodes come up and we need to
     * remove those from persisted DLT
     */
    void addPendingRmService(fpi::FDSP_MgrIdType svc_type,
                             const NodeUuid& rm_uuid);

    /**
     * Removes service with uuid 'svc_uuid' from pending added services
     * map and from cluster map
     */
    void rmPendingAddedService(fpi::FDSP_MgrIdType svc_type,
                               const NodeUuid& svc_uuid);

    /**
     * Removes service with uuid 'svc_uuid' from pending added services mao
     * but NOT from cluster map
     */
    void resetPendingAddedService(fpi::FDSP_MgrIdType svc_type,
                                  const NodeUuid& svc_uuid);

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
