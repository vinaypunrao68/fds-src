/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

/** Defines the classes used for data placement and routing tables */
#ifndef SOURCE_ORCH_MGR_INCLUDE_OMCLUSTERMAP_H_
#define SOURCE_ORCH_MGR_INCLUDE_OMCLUSTERMAP_H_

#include <unordered_map>
#include <string>
#include <vector>
#include <atomic>
#include <list>

#include <fds_types.h>
#include <fds_err.h>
#include <fds_module.h>
#include <fds_placement_table.h>
#include <concurrency/Mutex.h>
#include <OmTypes.h>
#include <OmResources.h>

namespace fds {

    typedef std::atomic<fds_uint64_t> AtomicMapVersion;

    /**
     * Defines the current state of the cluster at given points in time.
     * The cluster map specifies the current members of the cluster.
     */
    class ClusterMap : public Module {
  protected:
        NodeMap           currClustMap;  /**< Current storage nodes in cluster */
        /**
         * Current version of the map.
         * The version is monotonically
         * increasing.
         */
        AtomicMapVersion  version;
        Sha1Digest        checksum;             /**< Content Checksum */
        boost::shared_ptr<fds_mutex> mapMutex;  /**< Protects the map */

  public:
        ClusterMap();
        ~ClusterMap();

        /**
         * Need some functions to serialize the map
         */

        /**
         * Returns the current number of cluster members.
         */
        int getNumMembers() const;
        /**
         * Returns member info based on the nodes membership
         * index number.
         */
        const NodeAgent *om_member_info(int node_idx);
        /**
         * Returns member info based on the nodes UUID.
         */
        const NodeAgent *om_member_info(const ResourceUUID &uuid);

        /**
         * Update the current cluster map.
         */
        Error updateMap(const std::list<NodeAgent::pointer> &addNodes,
                        const std::list<NodeAgent::pointer> &rmNodes);

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
