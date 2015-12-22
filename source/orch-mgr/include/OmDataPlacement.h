/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

/** Defines the classes used for data placement and routing tables */
#ifndef SOURCE_ORCH_MGR_INCLUDE_OMDATAPLACEMENT_H_
#define SOURCE_ORCH_MGR_INCLUDE_OMDATAPLACEMENT_H_

#include <map>
#include <string>
#include <atomic>
#include <vector>
#include <list>
#include <set>
#include <utility>

#include <fds_globals.h>
#include <fds_types.h>
#include <fds_typedefs.h>
#include <fds_error.h>
#include <fds_module.h>
#include <concurrency/Mutex.h>
#include <OmResources.h>
#include <OmClusterMap.h>
#include <dlt.h>
#include <kvstore/configdb.h>

namespace fds {

    /* Node uuid -> number of tokens map */
    typedef std::unordered_map<NodeUuid, fds_uint32_t, UuidHash> NodeTokCountMap;
    typedef std::unordered_map<NodeUuid, int, UuidHash> NodeTokDiffMap;
    /* Node uuid -> index */
    typedef std::unordered_map<NodeUuid, fds_uint32_t, UuidHash> NodeIndexMap;
    /**
     * Primary node uuid -> secondary node uuid -> number of tokens for
     * primary/secondary group
     */
    typedef std::unordered_map<NodeUuid, NodeTokCountMap, UuidHash> L2GroupTokCountMap;
    typedef std::unordered_map<fds_uint64_t, int> NodesetTokDiffMap;

    /**
     * PlacementMetrics is a helper class that calculates optimal number of tokens per node
     * on each DLT row up to level 4 (primary + 3 replicas).
     * optimalTokens() methods calculate optimal number of tokens, which is a floating
     * point type.
     * tokens() methods for level 1 (primary) and level 2 (secondary) calculate the
     * optimal number of tokens rounded up or down to the nearest integer, which is
     * the number of tokens that are actually given to a node. Since tokens on next
     * DLT levels are calculated based on tokens given in previous levels, the rounding
     * error (tokens() vs. optimalTokens()) increases with each level.
     * tokens() methods for level 3 and 4 requires the caller to pass the number of tokens
     * in previous level, since the class does not keep that info.
     */
    class PlacementMetrics {
  private:
        std::unordered_map<NodeUuid,
                std::pair<double, fds_uint32_t>,
                UuidHash> node_weight_toks;  // node -> weight, optimal primary tokens map
        L2GroupTokCountMap l12_group_toks;  // node l1 -> map of l2 -> group tokens
        fds_uint64_t numTokens;  // current number of primary tokens
        double totalWeight;  // current total store weight

  public:
        /**
         * Constructs and caches optimal number of level 1 and level 2 tokens
         * based on the given cluster map 'cm' and number of primary tokens.
         */
        PlacementMetrics(const ClusterMap *cm,
                         fds_uint64_t tokens,
                         fds_uint32_t dlt_depth);
        ~PlacementMetrics() {}

        /**
         * Optimal value for token distribution for this node
         * @return 0 if 'uuid' of unknown node is requested
         */ 
        inline double optimalTokens(const NodeUuid& uuid) {
            if (node_weight_toks.count(uuid) == 0)
                return 0;
            return primaryTokens(std::get<0>(node_weight_toks[uuid]));
        }
        /**
         * Optimal value for token dispersion layer 2
         * (optimal number of tokens in layer 2 for node 'second_uuid'
         * when 'prim_uuid' node is primary).
         */
         inline double optimalTokens(const NodeUuid& l1_uuid,
                                     const NodeUuid& l2_uuid) {
             if (l1_uuid == l2_uuid)
                 return 0;
             return secondaryTokens(optimalTokens(l1_uuid), optimalTokens(l2_uuid));
         }
         /**
          * Optimal value for token dispersion layer 3
          * Optimal number of tokens in layer 3 for node group
          * l1_uuid (primary), l2_uuid (secondary), l3_uuid
          */
         inline double optimalTokens(const NodeUuid& l1_uuid,
                                     const NodeUuid& l2_uuid,
                                     const NodeUuid& l3_uuid) {
             double toks_l12_group = optimalTokens(l1_uuid, l2_uuid);
             if ((l3_uuid == l1_uuid) || (l3_uuid == l2_uuid))
                 return 0;
             return thirdLevelTokens(optimalTokens(l1_uuid),
                                     optimalTokens(l2_uuid),
                                     optimalTokens(l3_uuid),
                                     toks_l12_group);
         }
         /**
          * Optimal value for token dispersion layer 4
          */
         inline double optimalTokens(const NodeUuid& l1_uuid,
                                     const NodeUuid& l2_uuid,
                                     const NodeUuid& l3_uuid,
                                     const NodeUuid& l4_uuid) {
             double toks_l123_group = optimalTokens(l1_uuid, l2_uuid, l3_uuid);
             if ((l4_uuid == l1_uuid) || (l4_uuid == l2_uuid) || (l4_uuid == l3_uuid))
                 return 0;
             return fourthLevelTokens(optimalTokens(l1_uuid), optimalTokens(l2_uuid),
                                      optimalTokens(l3_uuid), optimalTokens(l4_uuid),
                                      toks_l123_group);
         }
        /**
         * Optimal integer value for token distribution for this node
         * which is the number of primary tokens actually given to this node
         * @return 0 if 'uuid' of unknown node is requested
         */
        inline fds_uint32_t tokens(const NodeUuid& uuid) {
            if (node_weight_toks.count(uuid) == 0)
                return 0;
            return std::get<1>(node_weight_toks[uuid]);
        }
         /**
          * Optimal integer number of tokens in layer 2 for a node 'second_uuid"
          * when 'prim_uuid' node is primary, given actual number of primary tokens
          * assigned to those nodes.
          */
         inline fds_uint32_t tokens(const NodeUuid& l1_uuid,
                                    const NodeUuid& l2_uuid) {
             if (l12_group_toks.count(l1_uuid) == 0)
                 return 0;
             if (l12_group_toks[l1_uuid].count(l2_uuid) == 0)
                 return 0;
             return (l12_group_toks[l1_uuid])[l2_uuid];
         }
         /**
          * (Sub)Optimal number of tokens in layer 3 for a node with uuid 'l3_uuid'
          * when 'l1_uuid' node is primary and 'l2_uuid' node is secondary, given
          * a constraint that secondary tokens assigned to group (l1_uuid, l2_uuid)
          * is 'toks_l12_group'. Using actual number of primary tokens assigned to
          * each of these nodes 
          */
         inline double tokens(const NodeUuid& l1_uuid,
                              const NodeUuid& l2_uuid,
                              const NodeUuid& l3_uuid,
                              double toks_l12_group) {
             return thirdLevelTokens(tokens(l1_uuid), tokens(l2_uuid),
                                     tokens(l3_uuid), toks_l12_group);
         }
         /**
          * (Sub)Optimal number of tokens in layer 4 for a node with uuid 'l4_uuid'
          * when 'l1_uuid' not is primary and 'l2_uuid' node is secondary, and
          * 'l3_uuid' is 3rd level, with a constraint that 3rd level tokens assigned
          * to group (l1_uuid, l2_uuid, l3_uuid) is 'toks_l123_group'. Using actual
          * number of primary tokens assigned to each of these nodes
          */
         inline double tokens(const NodeUuid& l1_uuid,
                              const NodeUuid& l2_uuid,
                              const NodeUuid& l3_uuid,
                              const NodeUuid& l4_uuid,
                              double toks_l123_group) {
             return fourthLevelTokens(tokens(l1_uuid), tokens(l2_uuid),
                                      tokens(l3_uuid), tokens(l4_uuid),
                                      toks_l123_group);
         }

         void print();

  private:
         inline double primaryTokens(double node_weight) {
             return (node_weight / totalWeight) * numTokens;
         }
         inline double secondaryTokens(double prim_l1_toks,
                                       double second_l1_toks) {
             fds_verify(numTokens >= prim_l1_toks);
             return prim_l1_toks * second_l1_toks / (numTokens - prim_l1_toks);
         }
         inline double thirdLevelTokens(double prim_l1_toks,
                                        double second_l1_toks,
                                        double third_l1_toks,
                                        double l12_group_toks) {
             double remainder = numTokens - prim_l1_toks - second_l1_toks;
             if (l12_group_toks == 0) return 0;
             fds_verify(remainder > 0);
             return l12_group_toks * third_l1_toks / remainder;
         }
         inline double fourthLevelTokens(double prim_l1_toks,
                                         double second_l1_toks,
                                         double third_l1_toks,
                                         double fourth_l1_toks,
                                         double l123_group_toks) {
             double remainder =
                     numTokens - prim_l1_toks - second_l1_toks - third_l1_toks;
             if (l123_group_toks == 0) return 0;
             fds_verify(remainder > 0);
             return l123_group_toks * fourth_l1_toks / remainder;
         }
    };
    typedef boost::shared_ptr<PlacementMetrics> PlacementMetricsPtr;

    /**
     * Relative measure of how far actual primary token distribution
     * from optimal distribution is; same for L1-2 dispersion.
     */
    class PlacementDiff {
  private:
        NodeTokDiffMap l1_diff_toks;  // node -> diff tokens (new - current)
        fds_uint32_t l1toks_transfer;  // total tokens to transfer

        NodesetTokDiffMap l12_diff_toks;  // l1,l2 node set index -> diff tokens
        NodesetTokDiffMap l123_diff_toks;  // l1, l2, l3 node set index -> diff toks
        NodesetTokDiffMap l1234_diff_toks;  // l1, l2, l3, l4 node set index -> diff toks
        NodeIndexMap node_index;  // node uuid -> index

  public:
        PlacementDiff(const PlacementMetricsPtr& newPlacement,
                      const DLT* curDlt,
                      const NodeUuidSet& nodes);

        void generateL34Diffs(const PlacementMetricsPtr& newPlacement,
                              const DLT* curDlt,
                              fds_uint32_t rowNum);

        /**
         * Transfer primary token such that L1-2 optimal dispersion is preserved from
         * node with uuid 'l1_uuid' (where secondary node is 'l2_uuid') to a different
         * node, whose uuid is returned in 'new_l1_uuid'.
         */
        fds_bool_t optimalTransferL1Token(const NodeUuid& l1_uuid,
                                          const NodeUuid& l2_uuid,
                                          NodeUuid* new_l1_uuid);

        /**
         * Transfer primary token (independent of L1-2 dispersion) from node with uuid
         * 'l1_uuid' (where secondary node is 'l2_uuid') to a different node, whose
         * uuid is returned in 'new_l1_uuid'.
         */
        fds_bool_t transferL1Token(const NodeUuid& l1_uuid,
                                   const NodeUuid& l2_uuid,
                                   NodeUuid* new_l1_uuid);

        /**
         * Transfer secondary token to preserve optimal L1-2 dispersion.
         * @return true if token was transfered, false if l1_uuid, l2_uuid
         * group does not have any tokens to give
         */
        fds_bool_t transferL2Token(const NodeUuid& l1_uuid,
                                   const NodeUuid& l2_uuid,
                                   NodeUuid* new_l2_uuid);

        /**
         * If possible, transfer token at row 'row' to preserve optimal
         * L1-2 dispersion. Currently implemented for rows 2 and 3, where
         * count starts at 0.
         */
        fds_bool_t transferL34Token(const DltTokenGroupPtr& col,
                                    fds_uint32_t row,
                                    NodeUuid* new_uuid);

        inline fds_uint32_t rmL1Tokens(const NodeUuid& l1_uuid) {
            if ((l1_diff_toks.count(l1_uuid) == 0) ||
                (l1_diff_toks[l1_uuid] >= 0))
                return 0;
            return (-l1_diff_toks[l1_uuid]);
        }

        void print(const NodeUuidSet& nodes);

  private:
        inline fds_uint64_t nodeSetToId(const NodeUuid& l1_uuid,
                                        const NodeUuid& l2_uuid) {
            fds_verify(node_index.count(l1_uuid) > 0);
            fds_verify(node_index.count(l2_uuid) > 0);
            return node_index.size()*node_index[l2_uuid] + node_index[l1_uuid];
        }
        inline fds_uint64_t nodeSetToId(const NodeUuid& l1_uuid,
                                        const NodeUuid& l2_uuid,
                                        const NodeUuid& l3_uuid) {
            fds_verify(node_index.count(l3_uuid) > 0);
            fds_uint64_t n = node_index.size();
            return n*n*node_index[l3_uuid] + nodeSetToId(l1_uuid, l2_uuid);
        }
        inline fds_uint64_t nodeSetToId(const NodeUuid& l1_uuid,
                                        const NodeUuid& l2_uuid,
                                        const NodeUuid& l3_uuid,
                                        const NodeUuid& l4_uuid) {
            fds_verify(node_index.count(l4_uuid) > 0);
            fds_uint64_t n = node_index.size();
            return n*n*n*node_index[l4_uuid] + nodeSetToId(l1_uuid, l2_uuid, l3_uuid);
         }
        /**
         * If possible, transfer third level token to preserve optimal
         * L1-2 dispersion
         */
        fds_bool_t transferL3Token(const NodeUuid& l1_uuid,
                                   const NodeUuid& l2_uuid,
                                   const NodeUuid& l3_uuid,
                                   NodeUuid* new_l4_uuid);
        /**
         * If possible, transfer third level token to preserve optimal
         * L1-2 dispersion
         */
        fds_bool_t transferL4Token(const NodeUuid& l1_uuid,
                                   const NodeUuid& l2_uuid,
                                   const NodeUuid& l3_uuid,
                                   const NodeUuid& l4_uuid,
                                   NodeUuid* new_l4_uuid);
    };
    typedef boost::shared_ptr<PlacementDiff> PlacementDiffPtr;

    /**
     * Abstract base class that defines the interface for a
     * data placement algorithm.
     * The classes are intended to be stateless. They take all
     * required input via parameters and return a new DLT.
     */
    class PlacementAlgorithm {
  private:
  protected:
        inline fds_log* getLog() { return g_fdslog; }
  public:
        enum AlgorithmTypes {
            RoundRobin  = 0,
            ConsistHash = 1,
        };
        /**
         * If 'currDlt' is null, calculates DLT from scratch; otherwise
         * calculated DLT based on previous DLT 'currDlt' and places newly
         * calculated DLT into newDlt
         * @param numPrimarySMs If 0, uses original algorithm which
         * does not take into account state of SM services (failed/not failed)
         *        otherwise, number of primary SMs
         */
        virtual Error computeNewDlt(const ClusterMap *currMap,
                                    const DLT        *currDlt,
                                    DLT              *newDlt,
                                    fds_uint32_t numPrimarySMs) = 0;
        virtual ~PlacementAlgorithm() {}
    };

    class RoundRobinAlgorithm : public PlacementAlgorithm {
  public:
        /**
         * numPrimarySMs is ignored
         */
        Error computeNewDlt(const ClusterMap *currMap,
                            const DLT        *currDlt,
                            DLT              *newDlt,
                            fds_uint32_t numPrimarySMs);
    };

    class ConsistHashAlgorithm : public PlacementAlgorithm {
  public:
        /**
         * If 'currDlt' is null, calculates DLT from scratch; otherwise
         * calculated DLT based on previous DLT 'currDlt' and places newly
         * calculated DLT into newDlt
         * @param[in] numPrimarySMs If 0, uses original algorithm which
         * does not take into account state of SM services (failed/not failed)
         *        otherwise, number of primary SMs
         * @return ERR_OK on success; ERR_INVALID_ARG if at least one column
         * in DLT has all primary SMs removed
         */
        Error computeNewDlt(const ClusterMap *currMap,
                            const DLT        *currDlt,
                            DLT              *newDlt,
                            fds_uint32_t numPrimarySMs);

  private:
        void computeInitialDlt(const ClusterMap *curMap,
                               DLT *newDLT,
                               fds_uint32_t numPrimarySMs);
        /// 'nonFailedSms' must be same as 'allSMs" if numPrimarySMs == 0
        void fillEmptyCells(fds_uint64_t numTokens,
                            DLT *newDLT,
                            fds_uint32_t numPrimarySMs,
                            const NodeUuidSet& nonFailedSms,
                            const NodeUuidSet& allSms);
        void handleDltChange(const ClusterMap *curMap,
                             const DLT *curDlt,
                             DLT *newDlt);
        Error checkUpdateValid(const ClusterMap *curMap,
                               const DLT *curDlt,
                               fds_uint32_t numPrimarySMs,
                               const NodeUuidSet& rmNodes);
        fds_uint32_t numOfFailedSMsInToken(const DltTokenGroupPtr& col,
                                           fds_uint32_t tokenId,
                                           fds_uint32_t checkDepth,
                                           const NodeUuidSet& nonFailedSms);
        void demoteFailedPrimaries(DLT* newDLT,
                                   fds_uint32_t numPrimarySMs,
                                   const NodeUuidSet& nonFailedSms);
    };

    /**
     * Defines the current data placement
     */
    class DataPlacement : public fds::Module {
  private:
        /**
         * Committed DLT copy and new DLT (after we commit new
         * DLT, new DLT is null and we only keep commited copy).
         * TODO: Move this over to our new DLT data structure
         * and use a smart pointer (since we pass the structure
         * around internall).
         */
        DLT *commitedDlt;
        DLT *newDlt;
        DLT *prevDlt;   /// to easily revert

        /**
         * Set of node uuids that are rebalancing 
         */
        NodeUuidSet rebalanceNodes;

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
        fds_mutex placementMutex;

        /**
         * Config db object
         */
        kvstore::ConfigDB* configDB = NULL;

        /**
         * Configuration for number of primary SMs
         * 0 means that we don't care about service state (failed or not)
         * and not resyncing when promoting secondaries;
         * 2 means implementation for 2-primary consistency model
         * Other values for number of primaries also work, but not tested.
         */
        fds_uint32_t numOfPrimarySMs;

        bool        sendMigAbortAfterRestart;
       fds_uint64_t targetVersionForAbort;

  public:
        DataPlacement();
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
         * Returns true if there is no target DLT computed or committed
         * as official version (but not to persistent store)
         */
        fds_bool_t hasNoTargetDlt() const;
        /**
         * Returns true if there is target DLT computed, but not yet commited
         * as official version
         */
        fds_bool_t hasNonCommitedTarget() const;
        /**
         * Returns true if there is target DLT that is commited as official
         * version but not persisted yet
         */
        fds_bool_t hasCommitedNotPersistedTarget() const;

        /**
         * Reruns DLT computation.
         * @return ERR_OK if DLT was successfully recomputed and it is
         * different from the currently commited DLT;
         *         ERR_INVALID_ARG if at least one column in DLT has all
         * primary SMs marked for removal.
         *         ERR_NOT_READY if newly computed DLT is the same as
         * commited DLT (no changes in the cluster map that change DLT)
         */
        Error computeDlt();

        /**
         * Begins token rebalance between nodes in the
         * cluster map. We are async notified of completion
         * at a later time.
         */
        Error beginRebalance();

        /**
         * Commits the current DLT as an 'official' copy
         */
        void commitDlt();
        void commitDlt( const bool unsetTarget );

        /**
         * Stores commited DLT to the permanent DLT history
         */
        void persistCommitedTargetDlt();
        /**
         * Restores cached commited DLT from persistent store
         * and resets target DLT in persistent store
         */
        void undoTargetDltCommit();
        void clearTargetDlt();

        /**
         * Returns the current commited version of the DLT.
         * So if we are in the process of computing/rebalancing
         * nodes for new DLT, this method returns the previous DLT
         * until the new DLT is commited.
         */
        const DLT *getCommitedDlt() const;
        const DLT *getTargetDlt() const;

        /**
         * Get version of the most recent DLT -- either new DLT
         * if we are in the process of computing/rebalancing
         * or commited DLT
         */
        fds_uint64_t getLatestDltVersion() const;
        fds_uint64_t getCommitedDltVersion() const;
        fds_uint64_t getTargetDltVersion() const;

        /**
         * Returns the current version of the cluster map.
         */
        const ClusterMap *getCurClustMap() const;

        /**
         * Returns set of nodes that we are rebalancing
         */
        NodeUuidSet getRebalanceNodes() const;

        void generateNodeTokenMapOnRestart();
        /**
         * Both commited DLT and target DLT must be NULL when
         * this method is called (should be called only during init)
         * Will read commited and target DLT from configDB
         * and save commited DLT as non-commited. DLT will be commited
         * later when all nodes in DLT come up, or DLT will be recomputed.
         * @param sm_service all known SM services
         * @param deployed_sm_services all known SM services that are
         * not in 'discovered' state
         */
        Error loadDltsFromConfigDB(const NodeUuidSet& ms_services,
                                   const NodeUuidSet& deployed_sm_services);

        /**
         * Make sure that current DLT matches given SM services
         * Called on Domain re-activate. DLT must already be loaded and known
         * to DataPlacement.
         */
        Error validateDltOnDomainActivate(const NodeUuidSet& sm_services);

        /**
         * set the config db from orchmgr
         */
        void setConfigDB(kvstore::ConfigDB* configDB);

        inline fds_uint32_t getNumOfPrimarySMs() const {
            return numOfPrimarySMs;
        }

  private:  // methods
        /**
        * Response callback for StartMigration messages sent
        * in rebalance method.
        */
        void startMigrationResp(NodeUuid uuid,
                                fds_uint64_t dltVersion,
                                EPSvcRequest* svcReq,
                                const Error& error,
                                boost::shared_ptr<std::string> payload);
    };
}  // namespace fds

#endif  // SOURCE_ORCH_MGR_INCLUDE_OMDATAPLACEMENT_H_
