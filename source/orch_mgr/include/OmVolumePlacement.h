/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

/**
 * Defines the classes for volume placement and routing tables
 */

#ifndef SOURCE_ORCH_MGR_INCLUDE_OMVOLUMEPLACEMENT_H_
#define SOURCE_ORCH_MGR_INCLUDE_OMVOLUMEPLACEMENT_H_

#include <fds_types.h>
#include <fds_typedefs.h>
#include <fds_error.h>
#include <fds_module.h>
#include <concurrency/Mutex.h>
#include <fds_dmt.h>


namespace fds {

    class ClusterMap;

    /**
     * Abstract base class that defines the interface for a
     * volume placement algorithm.
     * The classes are intended to be stateless. They take all
     * required input via parameters and return a new DMT.
     */
    class VolPlacementAlgorithm {
  public:
        enum AlgorithmTypes {
            RoundRobin  = 0,
            RoundRobinDynamic = 1,
        };
        virtual Error updateDMT(const DMTPtr& curDmt,
                                DMT* newDMT) = 0;
        virtual ~VolPlacementAlgorithm() {}
    };

    /**
     * Most simple static round robin algorith that every time
     * computes DMT from scratch using round robin assignment of
     * volumes to DMs.
     */
    class RRAlgorithm: public VolPlacementAlgorithm {
  public:
        virtual Error updateDMT(const DMTPtr& curDmt,
                                DMT* newDMT);
    };

    /**
     * This is a more intelligent version of round robin, where
     * initial DMT computation is round-robin. When new nodes are added,
     * or nodes are removed, we recompute DMT minimizing the migration
     * of volumes between DMs. This algorithm also guarantees that
     * just added node does not become a primary (it may be a primary later,
     * when we add more nodes).
     */
    class DynamicRRAlgorithm: public VolPlacementAlgorithm {
  public:
        virtual Error updateDMT(const DMTPtr& curDmt,
                                DMT* newDMT);
    };

    /**
     * Defines the current volume placement
     */
    class VolumePlacement : public fds::Module {
  public:
        VolumePlacement();
        virtual ~VolumePlacement();

        /*
         * Module members
         */
        int  mod_init(fds::SysParams const *const param);
        void mod_startup();
        void mod_shutdown();

        /**
         * Changes the algorithm being used to compute DMTs
         */
        void setAlgorithm(VolPlacementAlgorithm::AlgorithmTypes type);

        /**
         * Recompute DMT based on added/removed DMs in cluster map
         * The new DMT becomes a target DMT, which can be commited
         * via commitDMT()
         */
        void computeDMT(const ClusterMap* cmap);

        /**
         * Start rebalance volume meta among DM nodes
         */
        Error beginRebalance(const ClusterMap* cmap);

        /**
         * Commits the current DMT as an 'official' copy
         * TODO(xxx) The commit stores the DMT to the permanent DMT history
         */
        void commitDMT();

  private:
        /**
         * Keeps committed DMT and target DMT copy, and possibly a bit
         * longer history of DMT (if set).
         */
        DMTManagerPtr dmtMgr;

        /**
         * The DMT depth defines the maximum number of
         * replicas that can be specified in the DMT
         */
        fds_uint64_t curDmtDepth;

        /**
         * The DMT width defines the number of volume ranges and thereby
         * the number of columns that DMT will have. The width is specified
         * as an exponent of 2 (e.g., width=16 means 2^16 columns in DMT).
         */
        fds_uint64_t curDmtWidth;

        /**
         * Current algorithm used to compute new DMT
         */
        VolPlacementAlgorithm *placeAlgo;

        /**
         * Mutex to protect shared state of the volume placement engine
         */
        fds_mutex placementMutex;
    };
}  // namespace fds

#endif  // SOURCE_ORCH_MGR_INCLUDE_OMVOLUMEPLACEMENT_H_
