/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <vector>
#include <map>
#include <unordered_set>

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
                                   const WeightMap* curWeightMap,
                                   DLT *newDlt) {
    fds_verify(newDlt != NULL);


    Error err(ERR_OK);
    ClusterMap::const_iterator rr_it = curMap->cbegin();

    fds_uint32_t total_num_nodes = curMap->getNumMembers();
    fds_uint32_t column_depth = newDlt->getDepth();
    // Ensure we have enough nodes to fill columns with
    // unique nodes
    fds_verify(column_depth <= total_num_nodes);
    fds_uint64_t numTokens = pow(2, newDlt->getWidth());
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
                                    const WeightMap* curWeightMap,
                                    DLT *newDlt) {
    Error err(ERR_OK);
    fds_uint32_t total_nodes = currMap->getNumMembers();
    fds_verify(total_nodes > 0);
    fds_verify(newDlt != NULL);

    // Compute DLT from scratch if this is the first version
    //    if ((currDlt == NULL) ||
    //     (total_nodes <= 2)) {
    computeInitialDlt(currMap, newDlt);
    //    }

    // Update primary tokens, add/remove nodes one by one

    return err;
}


Error
ConsistHashAlgorithm::computeInitialDlt(const ClusterMap *curMap,
                                        DLT* newDLT)
{
    Error err(ERR_OK);
    NodeUuid cur_uuid;
    fds_uint32_t total_nodes = curMap->getNumMembers();
    fds_uint32_t col_depth = newDLT->getDepth();
    fds_uint64_t numTokens = pow(2, newDLT->getWidth());
    ClusterMap::const_iterator node_it = curMap->cbegin();

    // For efficiency, if we have just one node, set the first
    // row of DLT to this node uuid and return
    if (total_nodes == 1) {
        NodeUuid uuid = (node_it->second)->get_uuid();
        for (fds_token_id i = 0; i < numTokens; i++) {
            newDLT->setNode(i, 0, uuid);
        }
        return err;
    }

    // calculate number of primary tokens to give to each node
    fds_uint32_t count = 0;
    double total_weight = curMap->getTotalStorWeight();
    std::unordered_map<NodeUuid, fds_uint32_t, UuidHash> node_toks;
    for (node_it = curMap->cbegin();
         node_it != curMap->cend();
         ++node_it) {
        double rel_weight =
                ((*node_it).second)->node_stor_weight() / total_weight;
        fds_uint32_t toks = rel_weight * numTokens;  // rounded down
        node_toks[(*node_it).first] = toks;  // node_uuid -> num tokens
        count += toks;
    }

    // since we had to round down each node's tokens to the nearest
    // integer, and we may still have tokens to give out, give remaining
    // tokens to first nodes in the node map (one token per node)
    node_it = curMap->cbegin();
    while (count < numTokens) {
        fds_verify(node_it != curMap->cend());
        node_toks[node_it->first]++;
        node_it++;
        count++;
    }

    // we want to assign primary tokens to nodes in predictable
    // way, so we are creating ordered map of number of tokens
    // to vector of nodes, and assigning primary tokens to nodes
    // with weighted round robin starting with nodes with most tokens.
    std::map<fds_uint32_t, std::vector<NodeUuid>> tok_map;
    for (node_it = curMap->cbegin();
         node_it != curMap->cend();
         ++node_it) {
        fds_uint32_t toks = node_toks[node_it->first];
        tok_map[toks].push_back(node_it->first);
    }
    node_toks.clear();  // we will use 'tok_map' from now on

    // Assign first row of DLT using weighted round robin of nodes
    // We start filling in nodes that have highest number of tokens,
    // to decrease the chance that same node appears back to back +
    // the assignment will be the same independent of order of nodes
    // (in terms of their weight).
    fds_uint32_t start_token = 0;
    std::map<fds_uint32_t, std::vector<NodeUuid>>::const_reverse_iterator tok_crit;
    for (tok_crit = tok_map.crbegin();
         tok_crit != tok_map.crend();
         tok_crit++) {
        for (std::vector<NodeUuid>::const_iterator nit = (tok_crit->second).cbegin();
             nit != (tok_crit->second).cend();
             ++nit) {
            cur_uuid = *nit;
            fds_uint32_t toks = tok_crit->first;
            fds_uint32_t next_token = start_token;
            // place node with 'cur_uuid" to 'toks' spots
            // in the first row of DLT table
            std::cout << "Node " << cur_uuid.uuid_get_val()
                      << " toks " << toks << std::endl;
            for (fds_uint32_t i = 0; i < toks; ++i) {
                fds_uint32_t tok = next_token;
                // if we are somewhere in the middle of assigning
                // tokens to nodes, spot that we calculated maybe
                // already taken, and if so, search for next available
                while ((tok < numTokens) &&
                       (newDLT->getPrimary(tok) != 0)) {
                    ++tok;
                }
                if (tok >= numTokens) {
                    // must be one of the last tokens, just search
                    // the whole first row of DLT
                    tok = 0;
                    while ((tok < numTokens) &&
                           (newDLT->getPrimary(tok) != 0)) {
                        ++tok;
                    }
                    fds_verify(tok < numTokens);
                    next_token = tok;
                }
                newDLT->setNode(tok, 0, cur_uuid);
                if (i < (toks-1))
                    next_token += ((numTokens - tok - 1) / (toks - i - 1));
            }
            start_token++;
        }
    }

    // Fill in the remaining rows
    err = updateReplicaRows(total_nodes, numTokens, newDLT);

    return err;
}

Error
ConsistHashAlgorithm::updateReplicaRows(fds_uint32_t numNodes,
                                        fds_uint64_t numTokens,
                                        DLT* newDLT)
{
    Error err(ERR_OK);
    fds_uint32_t col_depth = newDLT->getDepth();
    fds_uint32_t replica_count = col_depth - 1;
    fds_uint32_t cur_col;

    // make sure we don't fill more columns than nodes
    if (numNodes < col_depth) {
        replica_count = numNodes-1;
    }

    std::unordered_set<NodeUuid, UuidHash> replica_set;
    std::unordered_set<NodeUuid, UuidHash>::const_iterator set_cit;
    for (fds_token_id i = 0; i < numTokens; ++i) {
        // walk ring clockwise and find replica count unique node uuids
        replica_set.clear();
        fds_token_id ind = i;  // add primary first
        while (replica_set.size() <= replica_count) {
            replica_set.insert(newDLT->getPrimary(ind));
            ind = (ind + 1) % numTokens;
        }
        // update the column in DLT
        fds_uint32_t j = replica_count;
        for (set_cit = replica_set.cbegin();
             set_cit != replica_set.cend();
             ++set_cit) {
            if (*set_cit != newDLT->getPrimary(i)) {
                newDLT->setNode(i, j, *set_cit);
                --j;
            }
        }
    }

    return err;
}

}  // namespace fds
