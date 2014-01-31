/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

/** Defines the classes used for data placement and routing tables */
#ifndef SOURCE_ORCH_MGR_INCLUDE_OMDATAPLACEMENT_H_
#define SOURCE_ORCH_MGR_INCLUDE_OMDATAPLACEMENT_H_

#include <map>
#include <string>
#include <atomic>

#include <fds_types.h>
#include <fds_typedefs.h>
#include <fds_err.h>
#include <fds_module.h>
#include <fds_placement_table.h>
#include <concurrency/Mutex.h>
#include <OmResources.h>
#include <OmClusterMap.h>
#include <dlt.h>

namespace fds {
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
        typedef std::map<WeightDist, NodeUuid> WeightMap;
        WeightMap curWeightDist;

        /**
         * Computes the weight distribution, given a cluster
         * map and dlt.
         */
        static void computeWeightDist(const ClusterMap *cm,
                                      const FdsDlt     *dlt,
                                      WeightMap        *sortedWeights);

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
         * Updates members of the cluster.
         * In general, this should necessitate a DLT
         * recomputation.
         */
        Error updateMembers(const NodeList &addNodes,
                            const NodeList &rmNodes);

        /**
         * Reruns DLT computation.
         */
        void computeDlt();

        /**
         * Returns the current version of the DLT.
         */
        const FdsDlt *getCurDlt() const;
        /**
         * Returns the current version of the cluster map.
         */
        const ClusterMap *getCurClustMap() const;
    };
}  // namespace fds

#endif  // SOURCE_ORCH_MGR_INCLUDE_OMDATAPLACEMENT_H_
