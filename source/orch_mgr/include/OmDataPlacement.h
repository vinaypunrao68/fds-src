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
#include <fds_err.h>
#include <concurrency/Mutex.h>
#include <OmTypes.h>

namespace fds {

    /*
     * TODO: Change map to use a UUID
     */
    typedef std::unordered_map<NodeStrName, NodeStrName> NodeMap;
    typedef std::atomic<fds_uint64_t> AtomicMapVersion;

    /**
     * Defines the current state of the cluster at given points in time.
     */
    class ClusterMap {
  private:
        NodeMap           currentNodeMap;  /**< Current storage nodes in cluster */
        AtomicMapVersion  version;         /**< Current version of the map.
                                              The version is monotonically
                                              increasing. */
        Sha1Digest        checksum;        /**< Content Checksum */
        fds_mutex         *mapMutex;       /**< Protects the map */
        

  public:
        ClusterMap();
        ~ClusterMap();

        /**
         * Need some functions to serialize the map
         */        
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
}  // namespace fds

#endif  // SOURCE_ORCH_MGR_INCLUDE_OMDATAPLACEMENT_H_
