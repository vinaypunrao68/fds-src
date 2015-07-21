/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <vector>
#include <OmVolumePlacement.h>
#include <OmClusterMap.h>

namespace fds {

// node -> diff map, where diff = targer # cells - actual # cells
// assigned to this node in a row
typedef std::unordered_map<NodeUuid, double, UuidHash> NodeDiffMapType;

class DmtRowBalancer {
  public:
    DmtRowBalancer(const NodeUuidSet& nodes,
                   DMT* dmt, fds_uint32_t row,
                   DmtRowBalancer* prev_row_balancer) {
        double num_nodes = nodes.size();
        double target = dmt->getNumColumns() / num_nodes;
        for (NodeUuidSet::const_iterator cit = nodes.cbegin();
             cit != nodes.cend();
             ++cit) {
            fds_uint32_t actual = getNodeCount(*cit, row, dmt);
            double debt = 0;
            if (prev_row_balancer) {
                debt = prev_row_balancer->getDiff(*cit);
            }
            node_diff[*cit] = target - actual + debt;
        }
        my_row = row;
    }
    ~DmtRowBalancer() {}

    void balanceWithinColumn(DMT* dmt, fds_uint32_t max_depth);
    void balance(DMT* dmt);
    Error balancePrimary(DMT* dmt, fds_uint32_t primDepth);

    friend std::ostream& operator<< (std::ostream &out,
                                     const DmtRowBalancer& balancer);

  private:  // methods
    inline double getDiff(const NodeUuid& uuid) {
        if (node_diff.count(uuid) > 0) {
            return node_diff[uuid];
        }
        return 0;
    }
    inline void transferOne(const NodeUuid& from_uuid,
                            const NodeUuid& to_uuid) {
        if (from_uuid.uuid_get_val() != 0) {
            fds_verify(node_diff.count(from_uuid) > 0);
            node_diff[from_uuid]++;
        }
        fds_verify(node_diff.count(to_uuid) > 0);
        node_diff[to_uuid]--;
    }
    fds_uint32_t getNodeCount(const NodeUuid& node_uuid,
                              fds_uint32_t row,
                              DMT* dmt);
    double getMaxDiffNode(NodeUuid* ret_node,
                          const DmtColumnPtr& col) const;

  private:
    fds_uint32_t my_row;
    NodeDiffMapType node_diff;
};
typedef boost::shared_ptr<DmtRowBalancer> DmtRowBalancerPtr;

fds_uint32_t
DmtRowBalancer::getNodeCount(const NodeUuid& node_uuid, fds_uint32_t row, DMT* dmt)
{
    fds_uint32_t count = 0;
    fds_verify(row < dmt->getDepth());
    for (fds_uint32_t i = 0; i < dmt->getNumColumns(); ++i) {
        DmtColumnPtr col = dmt->getNodeGroup(i);
        if (node_uuid == col->get(row)) ++count;
    }
    return count;
}

double DmtRowBalancer::getMaxDiffNode(NodeUuid* ret_node,
                                      const DmtColumnPtr& col) const {
    double max_diff = -10000;
    for (NodeDiffMapType::const_iterator cit = node_diff.cbegin();
         cit != node_diff.cend();
         ++cit) {
        if ((col->find(cit->first) < 0) && (cit->second > max_diff)) {
            max_diff = cit->second;
            (*ret_node) = cit->first;
        }
    }
    return max_diff;
}

/**
 * Balance this row without introducing new nodes within any column
 * Does balancing by exchanging cells of the same column = this row
 * and any row below (does not consider rows above 'my_row') up to max_depth - 1
 */
void DmtRowBalancer::balanceWithinColumn(DMT *dmt, fds_uint32_t max_depth) {
    fds_verify(my_row < max_depth);
    for (fds_uint32_t i = 0; i < dmt->getNumColumns(); ++i) {
        DmtColumnPtr col = dmt->getNodeGroup(i);
        NodeUuid my_row_uuid = col->get(my_row);
        if (getDiff(my_row_uuid) < -0.5) {
            // can descrease number of cells occupied by this node
            for (fds_uint32_t j = (my_row + 1); j < max_depth; ++j) {
                if (getDiff(col->get(j)) > 0.5) {
                    // can increase # of cells occupied by this node
                    transferOne(my_row_uuid, col->get(j));
                    dmt->setNode(i, my_row, col->get(j));
                    dmt->setNode(i, j, my_row_uuid);
                    break;
                }
            }
        }
    }
}

/**
 * Balances the row, where it is a priority to try and replace a cell with
 * the node which is already on the same column (of lower responsibility row).
 * However, it may still replace the cell in this row with a node that is not
 * already in this column
 */
void DmtRowBalancer::balance(DMT* dmt) {
    // first try to balance without introducing new nodes in the column
    balanceWithinColumn(dmt, dmt->getDepth());

    // balance by introducing new nodes in the column
    for (fds_uint32_t i = 0; i < dmt->getNumColumns(); ++i) {
        DmtColumnPtr col = dmt->getNodeGroup(i);
        NodeUuid my_row_uuid = col->get(my_row);
        if ((my_row_uuid.uuid_get_val() == 0) ||
            (getDiff(my_row_uuid) < -0.5)) {
            // will give this cell to node that needs it most
            NodeUuid new_node;
            double diff = getMaxDiffNode(&new_node, col);
            if ((my_row_uuid.uuid_get_val() != 0) && (diff <= 0.5)) continue;
            fds_verify(new_node.uuid_get_val() != 0);
            transferOne(my_row_uuid, new_node);
            dmt->setNode(i, my_row, new_node);
        }
    }
}

/**
 * Balances the row, where it is a priority to try and replace a cell with
 * the node which is already on the same column (of lower responsibility row) but
 * also primary.
 * However, it may still replace an empty cell in this row with a node that is not
 * already in this column.
 * The method does not replace non-empty cell with a node that is not in the column.
 */
Error DmtRowBalancer::balancePrimary(DMT* dmt, fds_uint32_t primDepth) {
    Error err(ERR_OK);
    // first try to balance without introducing new nodes in the column
    balanceWithinColumn(dmt, primDepth);

    // If there are empty cells -- balance by introducing new nodes in the column
    // if we cannot find nodes to replace, the cells will remain zero, and the method
    // returns ERR_NOT_FOUND
    for (fds_uint32_t i = 0; i < dmt->getNumColumns(); ++i) {
        DmtColumnPtr col = dmt->getNodeGroup(i);
        NodeUuid my_row_uuid = col->get(my_row);
        if (my_row_uuid.uuid_get_val() == 0) {
            // will give this cell to node that needs it most
            NodeUuid new_node;
            double diff = getMaxDiffNode(&new_node, col);
            if (new_node.uuid_get_val() != 0) {
                transferOne(my_row_uuid, new_node);
                dmt->setNode(i, my_row, new_node);
            } else {
                err = ERR_NOT_FOUND;
            }
        }
    }

    return err;
}

std::ostream& operator<< (std::ostream &oss,
                          const DmtRowBalancer& balancer) {
    oss << "Dmt diff for row " << balancer.my_row << "\n";
    for (NodeDiffMapType::const_iterator cit = balancer.node_diff.cbegin();
         cit != balancer.node_diff.cend();
         ++cit) {
        oss << "Node 0x" << std::hex << (cit->first).uuid_get_val()
            << std::dec << " diff " << cit->second << ", ";
    }
    return oss;
}

/******* RoundRobin algorithm implementation ******/

void RRAlgorithm::computeDMT(const ClusterMap* curMap,
                             DMT* newDmt,
                             fds_uint32_t numPrimaryDMs) {
    Error err(ERR_OK);
    fds_uint32_t col_depth, num_cols, total_num_nodes;

    fds_verify(newDmt);
    fds_verify(curMap);

    ClusterMap::const_dm_iterator start_it, it;
    num_cols = newDmt->getNumColumns();
    col_depth = newDmt->getDepth();
    total_num_nodes = curMap->getNumMembers(fpi::FDSP_DATA_MGR);
    fds_verify(col_depth <= total_num_nodes);
    DmtColumn col(col_depth);

    LOGNORMAL << "Computing DMT for " << total_num_nodes << " DMs";

    start_it = curMap->cbegin_dm();
    for (fds_uint32_t i = 0; i < num_cols; ++i) {
        it = start_it;
        for (fds_uint32_t j = 0; j < col_depth; ++j) {
            NodeUuid uuid = (it->second)->get_uuid();
            col.set(j, uuid);
            ++it;
            if (it == curMap->cend_dm()) {
                it = curMap->cbegin_dm();
            }
        }

        newDmt->setNodeGroup(i, col);
        ++start_it;
        if (start_it == curMap->cend_dm()) {
            start_it = curMap->cbegin_dm();
        }
    }
}

Error RRAlgorithm::updateDMT(const ClusterMap* curMap,
                            const DMTPtr& curDmt,
                            DMT* newDmt,
                            fds_uint32_t numPrimaryDMs) {
    // this algorithm always ignores previous DMT, and
    // computed new DMT from scratch
    computeDMT(curMap, newDmt, numPrimaryDMs);
    return ERR_OK;
}

/******* RoundRobinDynamic algorithm inmplementation ******/

/**
 * Compute new DMT from scratch based on nodes in cluster map
 */
void DynamicRRAlgorithm::computeDMT(const ClusterMap* curMap,
                                    DMT* newDmt,
                                    fds_uint32_t numPrimaryDMs) {
    Error err(ERR_OK);
    ClusterMap::const_dm_iterator it, start_it, end_it;
    fds_uint32_t total_nodes = curMap->getNumMembers(fpi::FDSP_DATA_MGR);
    fds_uint32_t col_depth = newDmt->getDepth();
    fds_uint32_t num_cols = newDmt->getNumColumns();
    NodeUuidSet nonFailedDms = curMap->getNonfailedServices(fpi::FDSP_DATA_MGR);
    fds_verify(col_depth <= total_nodes);

    LOGNORMAL << "Computing new DMT for " << total_nodes << " DMs";

    // simple round-robin for initial DMT -- this already distributes
    // volume ranges on DMs evenly

    // if numPrimaryDMs == 0, we are not taking into account the state of a
    // DM (failed/not failed) -- to keep the original implementation (beta 2)
    if (numPrimaryDMs == 0) {
        nonFailedDms.clear();
        nonFailedDms = curMap->getServiceUuids(fpi::FDSP_DATA_MGR);
    }

    // If numPrimaryDMs is set, we do our best effort to not place failed DMs
    // on primary rows. However, there are cases when that can happen:
    //   1) number of rows (because of not enough DMs) <= num primaries
    //   2) number of non-failed DMs < numPrimaryDMs

    // this is the number of rows which should have only non-failed DMs
    fds_uint32_t nofailed_depth = numPrimaryDMs;

    // if 'nofailed_depth' must be no greater than number of failed DMs
    fds_uint32_t nofailed_dms_count = nonFailedDms.size();
    if (nofailed_dms_count == total_nodes) {
        // if all DMs ok, must be original round-robin algorithm
        nofailed_depth = col_depth;
    } else if (nofailed_dms_count < nofailed_depth) {
        nofailed_depth = nofailed_dms_count;
    }
    LOGNORMAL << "nofailed_depth = " << nofailed_depth
              << ", nofailed_dms_count = " << nofailed_dms_count;

    // first 'nofailed_depth' rows must not have any failed DMs
    // remaining rows may contain failed DMs
    NodeUuidSet::const_iterator nofail_it, nofail_start_it;
    start_it = curMap->cbegin_dm();
    nofail_start_it = nonFailedDms.cbegin();
    for (fds_uint32_t i = 0; i < num_cols; ++i) {
        DmtColumn col(col_depth);
        nofail_it = nofail_start_it;
        it = start_it;
        // round-robin non-failed DM services in first primary rows
        for (fds_uint32_t j = 0; j < nofailed_depth; ++j) {
            NodeUuid uuid = *nofail_it;
            col.set(j, uuid);
            ++nofail_it;
            if (nofail_it == nonFailedDms.cend()) {
                nofail_it = nonFailedDms.cbegin();
            }
        }
        if (nofailed_depth > 0) {
            ++nofail_start_it;
            if (nofail_start_it == nonFailedDms.cend()) {
                nofail_start_it = nonFailedDms.cbegin();
            }
        }
        // round-robin all DM services in secondary rows
        for (fds_uint32_t j = nofailed_depth; j < col_depth; ++j) {
            NodeUuid uuid = (it->second)->get_uuid();
            // column must contain unique UUIDs
            end_it = it;
            while (col.find(uuid) >= 0) {
                ++it;
                if (it == curMap->cend_dm()) {
                    it = curMap->cbegin_dm();
                }
                uuid = (it->second)->get_uuid();
                // calculation of col_depth must have ensured
                // that we enough unique UUIDs to fill in the column
                fds_verify(it != end_it);
            }
            col.set(j, uuid);
            ++it;
            if (it == curMap->cend_dm()) {
                it = curMap->cbegin_dm();
            }
        }

        newDmt->setNodeGroup(i, col);
        ++start_it;
        if (start_it == curMap->cend_dm()) {
            start_it = curMap->cbegin_dm();
        }
    }
}

Error
DynamicRRAlgorithm::checkUpdateValid(const ClusterMap* curMap,
                                     const DMTPtr& curDmt,
                                     fds_uint32_t numPrimaryDMs,
                                     const NodeUuidSet& rmNodes) {
    Error err(ERR_OK);

    // if numPrimaryDMs is set, we do not support the cases:
    // 1) when all primary DMs are removed from at least one column
    // 2) when one primary DM is removed and one primary is failed in at least one column
    if (numPrimaryDMs != 0) {
        NodeUuidSet nonFailedDms = curMap->getNonfailedServices(fpi::FDSP_DATA_MGR);
        fds_uint32_t num_cols = curDmt->getNumColumns();
        fds_uint32_t primDepth = numPrimaryDMs;
        if (primDepth > curDmt->getDepth()) {
            primDepth = curDmt->getDepth();
        }
        for (fds_uint32_t i = 0; i < num_cols; ++i) {
            DmtColumnPtr col = curDmt->getNodeGroup(i);
            fds_uint32_t rmDms = 0;
            fds_uint32_t failedDms = 0;
            for (fds_uint32_t j = 0; j < primDepth; ++j) {
                NodeUuid uuid = col->get(j);
                if (rmNodes.count(uuid) > 0) {
                    ++rmDms;
                } else if (nonFailedDms.count(uuid) == 0) {
                    ++failedDms;
                }
            }
            if ( (rmDms >= primDepth) ||
                 ((rmDms > 0) && ((rmDms + failedDms) >= primDepth)) ) {
                LOGERROR << "At least one primary DMs is marked for removal and another primary DM "
                         << " failed or removed in column " << i;
                return ERR_INVALID_ARG;
            }
        }
    }

    return err;
}

void
DynamicRRAlgorithm::demoteFailedPrimaries(DMT* newDmt,
                                          fds_uint32_t numPrimaryDMs,
                                          const NodeUuidSet& nonFailedDms,
                                          fds_bool_t computeNew) {
    if (numPrimaryDMs == 0) {
        // version 1 of DMT algorithm does not care about 'failed' state
        return;
    }
    fds_uint32_t num_cols = newDmt->getNumColumns();
    fds_uint32_t primDepth = numPrimaryDMs;
    if (primDepth >= newDmt->getDepth()) {
        // there is nowhere to demote, since everyone is primary
        return;
    }
    LOGDEBUG << "Before demote: " << *newDmt;

    // we demote only in the case when there is at least one primary is
    // not in 'failed' state, otherwise keeping primaries as is
    for (fds_uint32_t i = 0; i < num_cols; ++i) {
        DmtColumnPtr col = newDmt->getNodeGroup(i);
        fds_uint32_t failedDms = 0;
        for (fds_uint32_t j = 0; j < primDepth; ++j) {
            NodeUuid uuid = col->get(j);
            if (nonFailedDms.count(uuid) == 0) {
                ++failedDms;
            }
        }
        LOGDEBUG << "Column " << i << ", failed primary DMs " << failedDms
                 <<" Number of primary rows " << primDepth;
        if ((failedDms >= primDepth) && (!computeNew)) {
            // all primary DMs failed, will promote secondaries to their place
            LOGNORMAL << "All primary DMs failed in column " << i
                      << " , but we are going to promote secondaries to their place";
        }
        if (failedDms == 0) {
            // none of the primaries failed in this column
            continue;
        }
        // try to demote now
        fds_uint32_t secondary_j = primDepth;
        for (fds_uint32_t j = 0; j < primDepth; ++j) {
            NodeUuid uuid = col->get(j);
            if (nonFailedDms.count(uuid) == 0) {
                // exchange this uuid with non-failed secondary
                NodeUuid nonfailedUuid = col->get(secondary_j);
                while ((nonFailedDms.count(nonfailedUuid) == 0) &&
                       (nonfailedUuid.uuid_get_val() != 0)) {
                    // this secondary also failed, try another one
                    ++secondary_j;
                    if (secondary_j >= newDmt->getDepth()) {
                        // ran out of secondaries... cannot demote anything
                        // in this column anymore
                        break;
                    }
                    nonfailedUuid = col->get(secondary_j);
                }
                if (secondary_j < newDmt->getDepth()) {
                    // found secondary to replace failed primary
                    // note that "empty" cell counts as replacable secondary
                    // -- we will fill it next
                    newDmt->setNode(i, secondary_j, uuid);
                    newDmt->setNode(i, j, nonfailedUuid);
                    LOGNORMAL << "Column " << i << ": Exchanging row " << j <<
                            " and " << secondary_j;
                    ++secondary_j;
                }
                if (secondary_j >= newDmt->getDepth()) {
                    // ran out of secondaries... cannot demote anything
                    // in this column anymore
                    break;  // with next column
                }
            }
        }
        // if we couldn't replace failed primaries, but there is at least one non-failed
        // DM from which we can resync, set the failed primary cell to 0, so that
        // another non-failed DM can replace it
        // note that at this point, we already promoted non-failed secondary DMs into primary
        // slots. So, if we still have all primaries failed in newDMT -- this means that
        // all DMs failed and nothing we can do
        if (!computeNew) {
            fds_uint32_t failedCnt = 0;
            DmtColumnPtr modifiedColumn = newDmt->getNodeGroup(i);
            for (fds_uint32_t j = 0; j < primDepth; ++j) {
                NodeUuid uuid = modifiedColumn->get(j);
                if (nonFailedDms.count(uuid) == 0) {
                    ++failedCnt;
                }
            }
            if (failedCnt == primDepth) {
                // nothing we can do in this column...
                LOGWARN << "Column " << i << " has all DMs failed";
                continue;
            } else if (failedCnt == 0) {
                continue;
            }

            // we have at least one primary from which we can resync
            // replace failed DMs with 0
            fds_uint32_t maxCnt = nonFailedDms.size();
            fds_verify(maxCnt > 0);  // because we have at least one non-failed DM
            --maxCnt;
            for (fds_uint32_t j = 0; j < primDepth; ++j) {
                if (maxCnt == 0) {
                    // there are not enough non-failed DMs to replace 0s
                    // so leave remaining failed DMs where they are
                    break;
                }
                NodeUuid uuid = modifiedColumn->get(j);
                if (nonFailedDms.count(uuid) == 0) {
                    newDmt->setNode(i, j, 0);
                    --maxCnt;
                }
            }

            // make sure we don't have first row set to 0
            modifiedColumn = newDmt->getNodeGroup(i);
            NodeUuid firstUuid = modifiedColumn->get(0);
            if (firstUuid.uuid_get_val() == 0) {
                for (fds_uint32_t j = 1; j < primDepth; ++j) {
                    NodeUuid uuid = modifiedColumn->get(j);
                    if (uuid.uuid_get_val() != 0) {
                        newDmt->setNode(i, 0, uuid);
                        newDmt->setNode(i, j, 0);
                        break;
                    }
                }
            }
        }
    }
    LOGDEBUG << "After demote: " << *newDmt;
}

Error DynamicRRAlgorithm::updateDMT(const ClusterMap* curMap,
                                    const DMTPtr& curDmt,
                                    DMT* newDmt,
                                    fds_uint32_t numPrimaryDMs) {
    Error err(ERR_OK);
    fds_uint32_t total_nodes = curMap->getNumMembers(fpi::FDSP_DATA_MGR);

    // if we have only one node in the cluster map or the width of the
    // DMT changed, compute the DMT from scratch
    if ((total_nodes < 2) ||
        (newDmt->getNumColumns() != curDmt->getNumColumns())) {
        computeDMT(curMap, newDmt, numPrimaryDMs);
        return err;
    }

    NodeUuidSet rmNodes = curMap->getRemovedServices(fpi::FDSP_DATA_MGR);
    NodeUuidSet addNodes = curMap->getAddedServices(fpi::FDSP_DATA_MGR);
    LOGNORMAL << "Recomputing new DMT for " << addNodes.size()
              << " DM additions and " << rmNodes.size()
              << " DM deletions, total resulting DMs " << total_nodes;

    // check this update is valid
    err = checkUpdateValid(curMap, curDmt, numPrimaryDMs, rmNodes);
    if (!err.ok()) {
        LOGERROR << "DMT update is not valid " << err;
        return err;
    }

    NodeUuidSet nonFailedDms = curMap->getNonfailedServices(fpi::FDSP_DATA_MGR);
    // if numPrimaryDMs == 0, we are not taking into account the state of a
    // DM (failed/not failed) -- to keep the original implementation (beta 2)
    if (numPrimaryDMs == 0) {
        nonFailedDms.clear();
        nonFailedDms = curMap->getServiceUuids(fpi::FDSP_DATA_MGR);
    }
    LOGNORMAL << "Total DMs: " << total_nodes << ", "
              << " Non-failed DMs: " << nonFailedDms.size()
              << " (using old version where failed state is ignored? " << (numPrimaryDMs == 0)
              << ")";

    // at this point, both DMTs have same number of columns
    fds_uint32_t num_cols = newDmt->getNumColumns();

    // copy old DMT into DMT first (and then we will trade cells)
    // do not copy removed nodes, and bubble up nodes from lower
    // rows to fill the upper cells (promote nodes). This may leave
    // empty (uuid = 0) cells, which we will fill in the next step
    for (fds_uint32_t i = 0; i < num_cols; ++i) {
        DmtColumnPtr col = curDmt->getNodeGroup(i);
        fds_uint32_t new_j = 0;
        for (fds_uint32_t j = 0; j < curDmt->getDepth(); ++j) {
            NodeUuid uuid = col->get(j);
            if (rmNodes.count(uuid) == 0) {
                newDmt->setNode(i, new_j, uuid);
                ++new_j;
                if (new_j >= newDmt->getDepth()) break;
            }
        }
        while (new_j < newDmt->getDepth()) {
            newDmt->setNode(i, new_j, NodeUuid());
            ++new_j;
        }
    }

    // if numPrimaryDMs is set, demote failed DMs from primary to secondary,
    // and replace with non-failed secondaries;
    // if numPrimaryDMs == 0 (original version of DMT calculation), the method
    // is noop
    demoteFailedPrimaries(newDmt, numPrimaryDMs, nonFailedDms, false);

    // balance first row -- this row excludes just added nodes
    // primary rows will exclude failed and added DMs, unless there
    // are no DMs left to fill in emty/failed cells, then primary rows
    // may include added/failed DMs
    NodeUuidSet nodes;
    nodes.insert(nonFailedDms.begin(), nonFailedDms.end());
 
    // balance the first row -- this row excludes just added nodes
    // because it only balances within column (and existing column
    // will not have any new nodes)
    DmtRowBalancerPtr prim_balancer(new DmtRowBalancer(nodes, newDmt, 0, NULL));
    // do not balance first primaries if we are changing the DMT due to failed DMs only
    // so that we are not doing too much domain rebalance in that case
    if ((addNodes.size() > 0) || (rmNodes.size() > 0)) {
        LOGDEBUG << "Before balance: " << *prim_balancer;
        // actually balance the primary row
        fds_uint32_t balanceDepth = newDmt->getDepth();
        if ((numPrimaryDMs > 0) && (numPrimaryDMs < newDmt->getDepth())) {
            balanceDepth = numPrimaryDMs;
        }
        prim_balancer->balanceWithinColumn(newDmt, balanceDepth);
        LOGDEBUG << "After balance: " << *prim_balancer;
    }

    // include added nodes in the list of nodes and balance replica rows
    std::vector<DmtRowBalancerPtr> row_balancers;
    row_balancers.push_back(prim_balancer);
    for (fds_uint32_t j = 1; j < newDmt->getDepth(); ++j) {
        DmtRowBalancerPtr prev_row_balancer = row_balancers[j-1];
        DmtRowBalancerPtr j_balancer(
            new DmtRowBalancer(nodes, newDmt, j, prev_row_balancer.get()));
        LOGDEBUG << "Before balance: " << *j_balancer;
        if (j < numPrimaryDMs) {
            Error balErr = j_balancer->balancePrimary(newDmt, numPrimaryDMs);
            if ((!balErr.ok() || (j == (numPrimaryDMs-1)))) {
                // if we couldn't fill in cells with non-failed DMs
                // most likely remaining primary rows will not be able to be
                // filled in -- so start filling in with failed DMs
                // similarly, ok to fill in secondary rows with failed DMs
                nodes.clear();
                nodes = curMap->getServiceUuids(fpi::FDSP_DATA_MGR);
                nodes.insert(addNodes.begin(), addNodes.end());
            }
            if (!balErr.ok()) {
                // retry with all DMs (including failed)
                DmtRowBalancerPtr j_balancer2(
                    new DmtRowBalancer(nodes, newDmt, j, prev_row_balancer.get()));
                balErr = j_balancer2->balancePrimary(newDmt, numPrimaryDMs);
                fds_verify(balErr.ok());   // can this happen?
                LOGDEBUG << "After balance: " << *j_balancer2;
                row_balancers.push_back(j_balancer2);
                continue;
            }
        } else {
            j_balancer->balance(newDmt);
        }
        LOGDEBUG << "After balance: " << *j_balancer;
        row_balancers.push_back(j_balancer);
    }

    return err;
}
}  // namespace fds
