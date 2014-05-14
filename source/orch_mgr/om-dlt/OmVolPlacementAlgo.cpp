/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <OmVolumePlacement.h>
#include <OmClusterMap.h>

namespace fds {

/******* RoundRobin algorithm implementation ******/

Error RRAlgorithm::computeDMT(const ClusterMap* curMap,
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

    return err;
}

Error RRAlgorithm::updateDMT(const ClusterMap* curMap,
                             const DMTPtr& curDmt,
                             DMT* newDmt) {
    // this algorithm always ignores previous DMT, and
    // computed new DMT from scratch
    return computeDMT(curMap, newDmt);
}

/******* RoundRobinDynamic algorithm inmplementation ******/

Error DynamicRRAlgorithm::computeDMT(const ClusterMap* curMap,
                                    DMT* newDMT) {
    Error err(ERR_OK);

    return err;
}

Error DynamicRRAlgorithm::updateDMT(const ClusterMap* curMap,
                                    const DMTPtr& curDmt,
                                    DMT* newDMT) {
    Error err(ERR_OK);

    return err;
}
}  // namespace fds
