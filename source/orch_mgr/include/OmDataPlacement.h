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
#include <list>

#include <fds_types.h>
#include <fds_err.h>
#include <fds_module.h>
#include <fds_placement_table.h>
#include <concurrency/Mutex.h>
#include <OmTypes.h>
#include <OmResources.h>

namespace fds {

    /**
     * Type that maps a Node's UUID to it's agent descriptor.
     */
    typedef std::unordered_map<NodeUuid,
            boost::shared_ptr<NodeAgent>,
            UuidHash> NodeMap;
    typedef std::atomic<fds_uint64_t> AtomicMapVersion;

    /**
     * Defines the current state of the cluster at given points in time.
     * The cluster map specifies the current members of the cluster.
     */
    class ClusterMap : public Module {
  protected:
        NodeMap           currClustMap;  /**< Current storage nodes in cluster */

        /**
         * Cached list of nodes added since the previous
         * DLT to create the current cluster map.
         */
        std::list<NodeUuid> addedNodes;
        /**
         * Cached list of nodes removed since the previous
         * DLT to create the current cluster map.
         */
        std::list<NodeUuid> removedNodes;

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

        typedef std::unordered_map<NodeUuid,
                boost::shared_ptr<NodeAgent>,
                UuidHash>::const_iterator const_iterator;
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
         * Update the current cluster map.
         */
        Error updateMap(const std::list<boost::shared_ptr<NodeAgent>> &addNodes,
                        const std::list<boost::shared_ptr<NodeAgent>> &rmNodes);

        /**
         * Returns a copy of the list of nodes added
         * since previous cluster map version.
         */
        std::list<NodeUuid> getAddedNodes() const;
        /**
         * Returns a copy of the list of nodes removed
         * since previous cluster map version.
         */
        std::list<NodeUuid> getRemovedNodes() const;

        /**
         * Module methods.
         */
        virtual int  mod_init(SysParams const *const param);
        virtual void mod_startup();
        virtual void mod_shutdown();
    };

    /**
     * Abstract base class that defines the interface for a
     * data placement algorithm.
     * The classes are intended to be stateless. They take all
     * required input via parameters and return a new DLT.
     */
    class PlacementAlgorithm {
  private:
  protected:
  public:
        enum AlgorithmTypes {
            RoundRobin  = 0,
            ConsistHash = 1,
        };
        virtual Error computeNewDlt(const ClusterMap *currMap,
                                    const FdsDlt     *currDlt,
                                    fds_uint64_t      depth,
                                    fds_uint64_t      width,
                                    FdsDlt           *newDlt) = 0;
    };

    class RoundRobinAlgorithm : public PlacementAlgorithm {
  public:
        Error computeNewDlt(const ClusterMap *currMap,
                            const FdsDlt     *currDlt,
                            fds_uint64_t      depth,
                            fds_uint64_t      width,
                            FdsDlt           *newDlt);
    };

    class ConsistHashAlgorithm : public PlacementAlgorithm {
  public:
        Error computeNewDlt(const ClusterMap *currMap,
                            const FdsDlt     *currDlt,
                            fds_uint64_t      depth,
                            fds_uint64_t      width,
                            FdsDlt           *newDlt);
    };

    /**
     * Defines the current data placement
     */
    class DataPlacement : public fds::Module {
  private:
        /**
         * Current DLT copy.
         * TODO: Move this over to our new DLT data structure
         * and use a smart pointer (since we pass the structure
         * around internall).
         */
        FdsDlt *curDlt;

        /**
         * The DLT depth defines the maximum number of
         * replicas that can be specified in the DLT.
         */
        fds_uint64_t curDltDepth;
        /**
         * The DLT width defines the number of 'tokens',
         * and thereby the number of columns, that the DLT
         * will have. The width is specified in as an
         * exponent of 2 (e.g., width=16 means 2^16 tokens).
         */
        fds_uint64_t curDltWidth;

        /**
         * Need a data structure to maintain DLT histories.
         */

        /**
         * Current cluster membership. The data placement
         * engine manages membership in conjunction with
         * the placement of data.
         */
        ClusterMap *curClusterMap;

        /**
         * Weight distributions for the current DLT. This
         * structure describes how well balanced our current
         * placement assignment is. We expect this list to
         * be sorted.
         * Currently, the weights are only stored for the
         * primary assignments. We should have a weight
         * distribution from each level in the DLT.
         */
        typedef double WeightDist;
        std::list<WeightDist> curWeightDist;

        /**
         * Current algorithm used to compute new DLTs.
         */
        PlacementAlgorithm *placeAlgo;
        /**
         * Current algorithm type
         */
        PlacementAlgorithm::AlgorithmTypes algoType;

        /**
         * Mutex to protect shared state of the placement engine
         */
        fds_mutex *placementMutex;

  public:
        DataPlacement(PlacementAlgorithm::AlgorithmTypes type,
                      fds_uint64_t width,
                      fds_uint64_t depth);
        ~DataPlacement();

        /*
         * Module members
         */
        int  mod_init(fds::SysParams const *const param);
        void mod_startup();
        void mod_shutdown();

        /**
         * Changes the algorithm being used to compute DLTs.
         */
        void setAlgorithm(PlacementAlgorithm::AlgorithmTypes type);

        /**
         * Reruns DLT computation.
         */
        void computeDlt();

        /**
         * Returns the current version of the DLT.
         */
        const FdsDlt *getCurDlt() const;
    };

    extern ClusterMap gl_OMClusMapMod;
}  // namespace fds

#endif  // SOURCE_ORCH_MGR_INCLUDE_OMDATAPLACEMENT_H_
