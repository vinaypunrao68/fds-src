/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

/** Defines the classes used for data placement and routing tables */
#ifndef SOURCE_ORCH_MGR_INCLUDE_OMDATAPLACEMENT_H_
#define SOURCE_ORCH_MGR_INCLUDE_OMDATAPLACEMENT_H_

#include <unordered_map>
#include <string>
#include <vector>
#include <atomic>

#include <fds_types.h>
#include <fds_module.h>
#include <fds_err.h>
#include <concurrency/Mutex.h>
#include <OmTypes.h>
#include <OmResources.h>

namespace fds {

    /*
     * TODO: Change map to use a UUID
     */
    typedef std::unordered_map<NodeStrName, NodeStrName> NodeMap;
    typedef std::atomic<fds_uint64_t> AtomicMapVersion;

    /**
     * Defines the current state of the cluster at given points in time.
     */
    class ClusterMap : public Module {
  protected:
        NodeMap           currentNodeMap;  /**< Current storage nodes in cluster */
        AtomicMapVersion  version;         /**< Current version of the map. */
                                           //   The version is monotonically
                                           //   increasing.
        Sha1Digest        checksum;        /**< Content Checksum */
        fds_mutex         *mapMutex;       /**< Protects the map */

  public:
        ClusterMap();
        ~ClusterMap();

        /**
         * Need some functions to serialize the map
         */

        /**
         * Iterate through the list of nodes in the membership by index 0...n
         * to retrieve their agent objects.
         */
        int om_member_nodes();
        const NodeAgent *om_member_info(int node_idx);
        const NodeAgent *om_member_info(const ResourceUUID &uuid);

        /**
         * Module methods.
         */
        virtual int  mod_init(SysParams const *const param);
        virtual void mod_startup();
        virtual void mod_shutdown();
    };

    /**
     * Defines the current data placement
     */
    // class DataPlaceAlgo;
    class DataPlacement {
  private:
        // DataPlaceAlgo placmentAlgorithm;

  public:
        DataPlacement();
        ~DataPlacement();
    };


    extern ClusterMap        gl_OMClusMapMod;
}  // namespace fds

#endif  // SOURCE_ORCH_MGR_INCLUDE_OMDATAPLACEMENT_H_
