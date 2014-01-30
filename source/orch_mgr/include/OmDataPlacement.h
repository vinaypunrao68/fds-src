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
#include <OmClusterMap.h>

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
        virtual Error computeNewDlt(const ClusterMap &currMap,
                                    const std::list<boost::shared_ptr<NodeAgent>>
                                    &addNodes,
                                    const std::list<boost::shared_ptr<NodeAgent>>
                                    &rmNodes,
                                    const FdsDlt &currDlt,
                                    fds_uint64_t depth,
                                    fds_uint64_t width) = 0;
    };

    class RoundRobinAlgorithm : public PlacementAlgorithm {
  public:
        Error computeNewDlt(const ClusterMap &currMap,
                            const std::list<boost::shared_ptr<NodeAgent>> &addNodes,
                            const std::list<boost::shared_ptr<NodeAgent>> &rmNodes,
                            const FdsDlt &currDlt,
                            fds_uint64_t depth,
                            fds_uint64_t width);
    };

    class ConsistHashAlgorithm : public PlacementAlgorithm {
  public:
        Error computeNewDlt(const ClusterMap &currMap,
                            const std::list<boost::shared_ptr<NodeAgent>> &addNodes,
                            const std::list<boost::shared_ptr<NodeAgent>> &rmNodes,
                            const FdsDlt &currDlt,
                            fds_uint64_t depth,
                            fds_uint64_t width);
    };

    /**
     * Defines the current data placement
     */
    class DataPlacement : public fds::Module {
  private:
        /**
         * Current DLT copy.
         * TODO: Move this over to our new DLT data structure
         */
        boost::shared_ptr<FdsDlt> currDlt;

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
         * Current token list. Maps the current
         * list of nodes to their tokens.
         */

        /**
         * Current cluster membership. The data placement
         * engine just stores a reference to the map.
         * It should be managed/update externally.
         */
        boost::shared_ptr<ClusterMap> currClusterMap;

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
        boost::shared_ptr<PlacementAlgorithm> placeAlgo;
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
    };

    extern ClusterMap gl_OMClusMapMod;
}  // namespace fds

#endif  // SOURCE_ORCH_MGR_INCLUDE_OMDATAPLACEMENT_H_
