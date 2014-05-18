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

    void balanceWithinColumn(DMT* dmt);
    void balance(DMT* dmt);

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
 * and any row below (does not consider rows above 'my_row').
 */
void DmtRowBalancer::balanceWithinColumn(DMT *dmt) {
    fds_verify(my_row < dmt->getDepth());
    for (fds_uint32_t i = 0; i < dmt->getNumColumns(); ++i) {
        DmtColumnPtr col = dmt->getNodeGroup(i);
        NodeUuid my_row_uuid = col->get(my_row);
        if (getDiff(my_row_uuid) < -0.5) {
            // can descrease number of cells occupied by this node
            for (fds_uint32_t j = (my_row + 1); j < dmt->getDepth(); ++j) {
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
    balanceWithinColumn(dmt);

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
                             DMT* newDmt) {
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

void RRAlgorithm::updateDMT(const ClusterMap* curMap,
                            const DMTPtr& curDmt,
                            DMT* newDmt) {
    // this algorithm always ignores previous DMT, and
    // computed new DMT from scratch
    computeDMT(curMap, newDmt);
}

/******* RoundRobinDynamic algorithm inmplementation ******/

/**
 * Compute new DMT from scratch based on nodes in cluster map
 */
void DynamicRRAlgorithm::computeDMT(const ClusterMap* curMap,
                                    DMT* newDmt) {
    Error err(ERR_OK);
    ClusterMap::const_dm_iterator it, start_it;
    fds_uint32_t total_nodes = curMap->getNumMembers(fpi::FDSP_DATA_MGR);
    fds_uint32_t col_depth = newDmt->getDepth();
    fds_uint32_t num_cols = newDmt->getNumColumns();
    fds_verify(col_depth <= total_nodes);

    LOGNORMAL << "Computing new DMT for " << total_nodes << " DMs";

    // simple round-robin for initial DMT -- this already distributes
    // volume ranges on DMs evenly
    DmtColumn col(col_depth);
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

void DynamicRRAlgorithm::updateDMT(const ClusterMap* curMap,
                                   const DMTPtr& curDmt,
                                   DMT* newDmt) {
    Error err(ERR_OK);
    fds_uint32_t total_nodes = curMap->getNumMembers(fpi::FDSP_DATA_MGR);

    // if we have only one node in the cluster map or the width of the
    // DMT changed, compute the DMT from scratch
    if ((total_nodes < 2) ||
        (newDmt->getNumColumns() != curDmt->getNumColumns())) {
        computeDMT(curMap, newDmt);
        return;
    }

    NodeUuidSet rmNodes = curMap->getRemovedServices(fpi::FDSP_DATA_MGR);
    NodeUuidSet addNodes = curMap->getAddedServices(fpi::FDSP_DATA_MGR);
    LOGNORMAL << "Recomputing new DMT for " << addNodes.size()
              << " DM additions and " << rmNodes.size()
              << " DM deletions, total resulting DMs " << total_nodes;

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

    // balance first row -- this row excludes just added nodes
    NodeUuidSet nodes;
    for (ClusterMap::const_dm_iterator cit = curMap->cbegin_dm();
         cit != curMap->cend_dm();
         ++cit) {
        if (addNodes.count(cit->first) == 0) {
            nodes.insert(cit->first);
        }
    }
    DmtRowBalancerPtr prim_balancer(new DmtRowBalancer(nodes, newDmt, 0, NULL));
    LOGDEBUG << "Before balance: " << *prim_balancer;
    // actually balance the primary row
    prim_balancer->balanceWithinColumn(newDmt);
    LOGDEBUG << "After balance: " << *prim_balancer;

    // include added nodes in the list of nodes and balance replica rows
    nodes.insert(addNodes.begin(), addNodes.end());
    std::vector<DmtRowBalancerPtr> row_balancers;
    row_balancers.push_back(prim_balancer);
    for (fds_uint32_t j = 1; j < newDmt->getDepth(); ++j) {
        DmtRowBalancerPtr prev_row_balancer = row_balancers[j-1];
        DmtRowBalancerPtr j_balancer(
            new DmtRowBalancer(nodes, newDmt, j, prev_row_balancer.get()));
        LOGDEBUG << "Before balance: " << *j_balancer;
        j_balancer->balance(newDmt);
        LOGDEBUG << "After balance: " << *j_balancer;
        row_balancers.push_back(j_balancer);
    }
}
}  // namespace fds
