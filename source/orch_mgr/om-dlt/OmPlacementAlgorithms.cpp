/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <vector>
#include <map>
#include <list>
#include <set>
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
                                    DLT *newDlt) {
    Error err(ERR_OK);
    fds_uint32_t total_nodes = currMap->getNumMembers();
    fds_verify(newDlt != NULL);

    // Compute DLT from scratch if this is the first version
    if ((currDlt == NULL) || (total_nodes <= 2)) {
        FDS_PLOG(getLog()) << "ConsistHashAlgorithm: compute new DLT for "
                           << total_nodes << " nodes";
        return computeInitialDlt(currMap, newDlt);
    }

    FDS_PLOG(getLog()) << "ConsistHashAlgorithm: recompute new DLT for"
                       << (currMap->getAddedNodes()).size()
                       << " node additions and "
                       << (currMap->getRemovedNodes()).size()
                       << " node deletions";

    // TODO(Anna): we don't support changes in DLT width yet
    fds_verify(newDlt->getWidth() == currDlt->getWidth());

    // Calculate placement weight map from scratch because by
    // adding/removing nodes the relative placement weights changed as well
    WeightMapPtr weight_map(new WeightMap(currMap, currDlt));
    weight_map->debug_print(getLog());

    // Update the first row of the DLT to take into account
    // added nodes. Currently we assume that newDlt is completely
    // empty, so the unchanged cells are also copied from currDlt
    // to newDlt. If no nodes were added to cluster map, the
    // function just copies the first row from currDlt to newDlt
    err = handleNewNodesPrimary(currMap, currDlt, newDlt, weight_map);

    // update the first row of newDlt to take into account node removals
    err = handleRmNodesPrimary(currMap, newDlt, weight_map);

    // Fill in the remaining rows
    if (err.ok())
        err = updateReplicaRows(total_nodes, newDlt->getNumTokens(), newDlt);

    FDS_PLOG_SEV(getLog(), fds_log::debug)
            << "ConsistHash: Finished updating DLT";
    return err;
}

//
// Updates first row of DLT to take into account added nodes
// If there is no added nodes in cluster map, the function just
// copies the first row of curDlt to the first row of newDlt
//
Error
ConsistHashAlgorithm::handleNewNodesPrimary(const ClusterMap *curMap,
                                            const DLT *curDlt,
                                            DLT *newDlt,
                                            const WeightMapPtr& weightMap) {
    Error err(ERR_OK);
    typedef std::unordered_map<NodeUuid,
                               std::set<fds_token_id>,
                               UuidHash> NodeTokenSetMap;

    NodeTokenSetMap node_toks;  // cached map of node uuids -> primary toks
    NodeTokenSetMap new_node_toks;  // new node uuids -> primary toks (stolen)
    TokenList target_tokens;  // list tokens that we want to steal (if possible)
    std::map<fds_token_id,
            std::unordered_set<NodeUuid, UuidHash>> ttok_nodes;  // tgt tok -> uuid
    fds_uint32_t numTokens = newDlt->getNumTokens();
    std::unordered_set<NodeUuid, UuidHash> addedNodes = curMap->getAddedNodes();

    // cache mapping of node uuids -> set of primary tokens
    for (fds_uint32_t i = 0; i < curDlt->getNumTokens(); ++i) {
        NodeUuid uuid = curDlt->getPrimary(i);
        (node_toks[uuid]).insert(i);
    }

    // calculate number of tokens to give to new nodes
    double total_weight = curMap->getTotalStorWeight();
    for (std::unordered_set<NodeUuid, UuidHash>::const_iterator cit = addedNodes.cbegin();
         cit != addedNodes.cend();
         ++cit) {
        OM_SmAgent::pointer agent =
                OM_NodeDomainMod::om_local_domain()->om_sm_agent(*cit);
        double rel_weight =
                agent->node_stor_weight() / total_weight;
        fds_uint32_t toks = rel_weight * numTokens;  // rounded down
        fds_verify(toks > 0);

        // get node uuid that is most over-subscribed
        NodeUuid uuid = weightMap->getHighestWeightNode();

        // steal a token with lowest token id from node 'uuid'
        weightMap->updateHighestLowestWeightNode(node_toks[uuid].size() - 1,
                                                 node_toks[uuid].size(),
                                                 numTokens,
                                                 true);
        weightMap->debug_print(getLog());
        std::set<fds_token_id>::iterator sit = node_toks[uuid].begin();
        fds_token_id stolen_token = *sit;
        (node_toks[uuid]).erase(sit);
        new_node_toks[*cit].insert(stolen_token);

        FDS_PLOG_SEV(getLog(), fds_log::debug)
                << "Node 0x" << std::hex << (*cit).uuid_get_val() << std::dec
                << " will steal " << toks << " tokens" << std::endl
                << "Steal first token " << stolen_token
                << " from node " << std::hex << uuid.uuid_get_val()
                << std::dec;

        // calculate 'ideal' tokens for this new node
        // including the first stolen token
        fds_uint32_t i = 1;
        fds_uint32_t next_token = stolen_token;
        fds_uint32_t end_token = (stolen_token > 0) ?
                stolen_token - 1 : (numTokens - 1);

        while (i < toks) {
            next_token += ((numTokens - next_token - 1) / (toks - i));
            next_token = next_token % numTokens;
            target_tokens.push_back(next_token);
            ttok_nodes[next_token].insert(*cit);
            ++i;
        }
    }

    // steal the remaining tokens
    while (target_tokens.size() > 0) {
        // get uuid of the most over-subscribed node
        NodeUuid src_uuid = weightMap->getHighestWeightNode();

        // we will steal token from this node, so tell placement
        // weight map to decrement number of tokens for this node
        // steal a token with lowest token id from node 'uuid'
        weightMap->updateHighestLowestWeightNode(node_toks[src_uuid].size() - 1,
                                                 node_toks[src_uuid].size(),
                                                 numTokens,
                                                 true);
        weightMap->debug_print(getLog());

        // for all tokens that belong to node with uuid 'src_uuid'
        // find token with the closest distance to one of the target tokens
        fds_token_id target_token;  // token from target_tokens list
        fds_token_id stolen_token;  // token that we are stealing
        // below func will remove corresponding entries in 'target tokens'
        // and 'node_toks[src_uuid]' arrays
        getMatchedTokenPair(&target_tokens,
                            &target_token,
                            &node_toks[src_uuid],
                            &stolen_token);

        // transfer best matched token from src_uuid node to new node whose
        // target token we just matched
        std::unordered_set<NodeUuid, UuidHash>::iterator it =
                (ttok_nodes[target_token]).begin();

        FDS_PLOG_SEV(getLog(), fds_log::debug)
                << "Steal from node " << std::hex << src_uuid.uuid_get_val()
                << std::dec << " token " << stolen_token
                << " to node " << std::hex << (*it).uuid_get_val() << std::dec
                << " (ideal token " << target_token << ")";

        ttok_nodes[target_token].erase(*it);
        fds_verify((new_node_toks[*it]).count(stolen_token) == 0);
        new_node_toks[*it].insert(stolen_token);
    }

    // At this point 'node_toks' contains all primary tokens of all nodes in curDlt
    // and 'new_node_toks' contain all tokens we assigned to added nodes
    // Fill in the first row of the new DLT
    node_toks.insert(new_node_toks.begin(), new_node_toks.end());
    for (NodeTokenSetMap::const_iterator nit = node_toks.cbegin();
         nit != node_toks.cend();
         ++nit) {
        for (std::set<fds_token_id>::const_iterator it = (nit->second).cbegin();
             it != (nit->second).cend();
             ++it) {
            newDlt->setNode(*it, 0, nit->first);
        }
    }

    return err;
}

//
// Update the first row of newDlt to take into account node removals.
// Assumes that the first row of 'newDlt' is completely filled
//
Error
ConsistHashAlgorithm::handleRmNodesPrimary(const ClusterMap *curMap,
                                           DLT *newDlt,
                                           const WeightMapPtr& weightMap) {
    Error err(ERR_OK);
    std::unordered_set<NodeUuid, UuidHash> removedNodes = curMap->getRemovedNodes();
    if (removedNodes.size() == 0) {
        return err;  // no nodes were removed, so nothing to do
    }

    // build a cache of node uuid -> number of tokens map
    std::unordered_map<NodeUuid, fds_uint32_t, UuidHash> node_toks;
    for (fds_uint32_t i = 0; i < newDlt->getNumTokens(); ++i) {
        NodeUuid uuid = newDlt->getPrimary(i);
        if (node_toks.count(uuid) == 0) {
            node_toks[uuid] = 1;
        } else {
            ++node_toks[uuid];
        }
    }

    // find all primary tokens of removed nodes and give them away
    fds_uint32_t numTokens = newDlt->getNumTokens();
    for (fds_uint32_t i = 0; i < numTokens; ++i) {
        NodeUuid uuid = newDlt->getPrimary(i);
        if (removedNodes.count(uuid)) {
            // current token belongs to removed node, give it to the most
            // under-subscribed node
            NodeUuid new_uuid = weightMap->getLowestWeightNode();

            // we will steal token from this node, so tell placement
            // weight map to decrement number of tokens for this node
            // steal a token with lowest token id from node 'uuid'
            weightMap->updateHighestLowestWeightNode(node_toks[new_uuid] + 1,
                                                     node_toks[new_uuid],
                                                     numTokens,
                                                     false);  // update lowest weight
            FDS_PLOG_SEV(getLog(), fds_log::debug)
                    << "RM: node " << std::hex << uuid.uuid_get_val()
                    << " -> node " << new_uuid.uuid_get_val() << std::dec
                    << " token " << i;
            weightMap->debug_print(getLog());

            newDlt->setNode(i, 0, new_uuid);
            ++node_toks[new_uuid];
        }
    }

    return err;
}

//
// Finds a pair of tokens, one in target list and one in candidate_set,
// that have a smallest distance (=|target_token - candidate_token|) and
// returns them. Also removes corresponding entries from 'target_list' and
// 'candidate_set'
//
void
ConsistHashAlgorithm::getMatchedTokenPair(TokenList* target_list,
                                          fds_token_id* ret_target_token,
                                          std::set<fds_token_id>* candidate_set,
                                          fds_token_id* ret_matched_token) {
    fds_uint32_t shortest_dist = 0;
    fds_verify((target_list != NULL) && (target_list->size() > 0));
    fds_verify((candidate_set != NULL) && (candidate_set->size() > 0));

    // initialize shortest distance
    TokenList::iterator tgt_it = target_list->begin();
    std::set<fds_token_id>::iterator candidate_it = candidate_set->begin();
    shortest_dist = (*candidate_it > *tgt_it) ?
            (*candidate_it) - (*tgt_it) : (*tgt_it) - (*candidate_it);

    // find the best match
    for (std::set<fds_token_id>::iterator c_it = candidate_set->begin();
         c_it != candidate_set->end();
         ++c_it) {
        // compare with target list
        for (TokenList::iterator t_it = target_list->begin();
             t_it != target_list->end();
             ++t_it) {
            fds_uint32_t dist = (*c_it > *t_it) ?
                    (*c_it) - (*t_it) : (*t_it) - (*c_it);
            if (dist < shortest_dist) {
                tgt_it = t_it;
                candidate_it = c_it;
                shortest_dist = dist;
            }
        }
    }

    FDS_PLOG_SEV(getLog(), fds_log::debug) << "Match: target " << *tgt_it
                                           << " selected candidate " << *candidate_it;

    // matched tokens we return
    *ret_target_token = *tgt_it;
    *ret_matched_token = *candidate_it;

    // remove items from both lists/sets that correspond to matched tokens
    candidate_set->erase(candidate_it);
    target_list->erase(tgt_it);
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

    // we assume that newDLT is already initialized to 0s
    // so nothing to do if total_nodes == 0
    if (total_nodes == 0) {
        return err;
    }

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
            FDS_PLOG_SEV(getLog(), fds_log::normal) << "ConsistHash: Node "
                                                    << std::hex << cur_uuid.uuid_get_val()
                                                    << " toks " << std::dec << toks;
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

    // We should not have more columns than nodes
    fds_verify(col_depth <= numNodes);
    FDS_PLOG_SEV(getLog(), fds_log::debug)
            << "ConsistHash::updateReplicaRows replica count "
            << replica_count << " numNodes " << numNodes;

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
