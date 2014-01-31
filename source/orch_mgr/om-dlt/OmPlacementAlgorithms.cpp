/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <vector>

#include <OmDataPlacement.h>

namespace fds {
/**********
 * Functions definitions for placement
 * algorithms
 **********/

/**
 * Compute DLT based on round robin. This approach ignores
 * the current DLT and just recomputes a new dlt based on
 * the cluster map.
 *
 * Note the cluster map iterator must be held by the caller.
 */
Error
RoundRobinAlgorithm::computeNewDlt(const ClusterMap *curMap,
                                   const DLT *curDlt,
                                   fds_uint64_t depth,
                                   fds_uint64_t width,
                                   DLT *newDlt) {
    fds_verify(newDlt != NULL);


    Error err(ERR_OK);
    ClusterMap::const_iterator rr_it = curMap->cbegin();

    fds_uint32_t total_num_nodes = curMap->getNumMembers();
    fds_uint32_t column_depth = depth;
    // Ensure we have enough nodes to fill columns with
    // unique nodes
    fds_verify(column_depth <= total_num_nodes);
    fds_uint64_t numTokens = pow(2, width);
    DltTokenGroup tg(column_depth);

    // Iterate over each token column and add
    // a new tokengroup of size depth().
    for (fds_token_id i = 0; i < numTokens; i++) {
        // If we've reached the end of the cluster map's
        // list, round robin back to the beginning
        ClusterMap::const_iterator nl_it = rr_it;
        if (nl_it == curMap->cend()) {
            continue;
        }

        // Iterate over the column and set the uuids
        for (fds_uint32_t j = 0; j < column_depth; j++) {
            NodeUuid uuid = (nl_it->second)->get_uuid();
            tg.set(j, uuid);
            nl_it++;
            if (nl_it == curMap->cend()) {
                nl_it = curMap->cbegin();
            }
        }
        newDlt->setNodes(i, tg);

        // Move the starting point for the list
        // and reset it to the beginning if we've
        // looped around.
        rr_it++;
        if (rr_it == curMap->cend()) {
            rr_it = curMap->cbegin();
        }
    }

    return err;
}


/**
 * Note the cluster map iterator must be held by the caller.
 */
Error
ConsistHashAlgorithm::computeNewDlt(const ClusterMap *currMap,
                                    const DLT *currDlt,
                                    fds_uint64_t depth,
                                    fds_uint64_t width,
                                    DLT *newDlt) {
    Error err(ERR_OK);
    return err;
}
}  // namespace fds
