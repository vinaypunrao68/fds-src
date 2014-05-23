/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <vector>
#include <map>
#include <list>
#include <set>
#include <unordered_set>
#include <utility>

#include <OmDataPlacement.h>

namespace fds {


/*********
 * Functions definitions for the helper class that calculates
 * data placement metrics
 *********/

// Constructor calculates optimal and rounded (to nearest integer)
// number of primary tokens for all nodes and secondary tokens for
// all node pairs and saves in info in corresponding maps.
PlacementMetrics::PlacementMetrics(const ClusterMap *cm,
                                   fds_uint64_t tokens,
                                   fds_uint32_t dlt_depth)
        : numTokens(tokens),
          totalWeight(cm->getTotalStorWeight()) {
    ClusterMap::const_sm_iterator node_it, it2;
    fds_uint32_t l1_tok_count, l12_tok_count;
    fds_uint32_t token_count = 0;

    fds_verify(totalWeight > 0);
    for (node_it = cm->cbegin_sm(); node_it != cm->cend_sm(); node_it++) {
        NodeUuid uuid = (*node_it).first;
        // Ensure we haven't counted this node before
        fds_verify(node_weight_toks.count(uuid) == 0);

        // Extract node's weight
        fds_uint64_t weight = ((*node_it).second)->node_stor_weight();

        // calculate ideal tokens rounded down
        l1_tok_count = primaryTokens(weight);
        token_count += l1_tok_count;

        // keep the mapping
        std::pair<double, fds_uint32_t> info(weight, l1_tok_count);
        node_weight_toks[uuid] = info;

        LOGDEBUG << "Node " << std::hex << uuid.uuid_get_val() << std::dec
                 << " weight " << weight << " of total " << totalWeight
                 << " optimal rounded down L1 tokens " << l1_tok_count;
    }
    // Since we rounded down all nodes' optimal tokens, assign
    // remaining tokens (so they add up to total number of tokens)
    // to first set of nodes in the node map
    LOGDEBUG << "Allocated token_count " << token_count
             << "tokens, will finish allocatating total " << numTokens;
    node_it = cm->cbegin_sm();
    while (token_count < numTokens) {
        fds_verify(node_it != cm->cend_sm());
        node_weight_toks[node_it->first].second++;
        node_it++;
        token_count++;
    }

    // Calculate number of tokens for l1-l2 group and fill in the map
    if ((cm->getNumMembers(fpi::FDSP_STOR_MGR) < 2) || (dlt_depth < 2)) {
        print();
        return;  // no l1-2 groups, since less than 2 nodes
    }
    for (node_it = cm->cbegin_sm(); node_it != cm->cend_sm(); ++node_it) {
        NodeUuid l1_uuid = (*node_it).first;
        l1_tok_count = std::get<1>(node_weight_toks[l1_uuid]);
        // calculate number of secondary tokens for l1_uuid per other node
        NodeTokCountMap l12_toks;
        token_count = 0;
        for (it2 = cm->cbegin_sm(); it2 != cm->cend_sm(); ++it2) {
            NodeUuid l2_uuid = (*it2).first;
            if (l2_uuid == l1_uuid) {
                continue;  // skip this node
            }
            // round down optimal number of tokens
            l12_tok_count = secondaryTokens(l1_tok_count,
                                            std::get<1>(node_weight_toks[l2_uuid]));

            l12_toks[l2_uuid] = l12_tok_count;
            token_count += l12_tok_count;
        }
        // since we are rounding down level 2 tokens, we may have token_count < toks
        // so we will add 1 more token to first (toks-count) nodes
        // but the difference should not be bigger than number of nodes
        fds_verify((token_count <= l1_tok_count) &&
                   ((l1_tok_count - token_count) <= l12_toks.size()));
        NodeTokCountMap::iterator l12_it = l12_toks.begin();
        while (token_count < l1_tok_count) {
            fds_verify(l12_it != l12_toks.end());
            l12_it->second++;
            l12_it++;
            token_count++;
        }
        l12_group_toks[l1_uuid].swap(l12_toks);
    }

    // print debug level
    print();
}

//
// Print token distribution and L1-2 dispersion in debug mode
//
void PlacementMetrics::print() {
    std::unordered_map<NodeUuid,
            std::pair<double, fds_uint32_t>,
                       UuidHash>::const_iterator cit;
    for (cit = node_weight_toks.cbegin();
         cit!= node_weight_toks.cend();
         ++cit) {
        FDS_PLOG_SEV(g_fdslog, fds_log::debug)
                << "DP: Node "
                << std::hex << (cit->first).uuid_get_val() << std::dec
                << " -- weight " << (cit->second).first
                << "; primary tokens: " << (cit->second).second;
    }
    for (L2GroupTokCountMap::const_iterator l2_cit = l12_group_toks.cbegin();
         l2_cit != l12_group_toks.cend();
         ++l2_cit) {
        NodeUuid l1_uuid = l2_cit->first;
        for (NodeTokCountMap::const_iterator it2 = (l2_cit->second).cbegin();
             it2 != (l2_cit->second).cend();
             ++it2) {
            NodeUuid l2_uuid = it2->first;
            FDS_PLOG_SEV(g_fdslog, fds_log::debug)
                    << "DP: Node group (" << std::hex << l1_uuid.uuid_get_val()
                    << ", " << l2_uuid.uuid_get_val() << std::dec
                    << ") -- tokens " << it2->second;
        }
    }
}

//
// Calculates and caches how many primary tokens need to be transfered
// between all nodes; calculated and caches how many secondary tokens
// need to be transfered between all node groups (primary, secondary).
//
PlacementDiff::PlacementDiff(const PlacementMetricsPtr& newPlacement,
                             const DLT* curDlt,
                             const NodeUuidSet& nodes)
        : l1toks_transfer(0) {
    int new_toks, old_toks;
    NodeUuidSet::const_iterator cit, cit2;
    int index = 0;
    fds_uint32_t l1toks_remove = 0;
    // create node_index
    node_index[NodeUuid()] = index++;
    for (cit = nodes.cbegin(); cit != nodes.cend(); ++cit) {
        node_index[*cit] = index++;
    }
    for (cit = nodes.cbegin(); cit != nodes.cend(); ++cit) {
        TokenList node_l1_toks;
        NodeUuid l1_uuid = *cit;
        curDlt->getTokens(&node_l1_toks, l1_uuid, 0);
        new_toks = newPlacement->tokens(l1_uuid);
        old_toks = node_l1_toks.size();
        if (new_toks != old_toks)
            l1_diff_toks[l1_uuid] = new_toks - old_toks;
        if (new_toks > old_toks) {
            l1toks_transfer += (new_toks - old_toks);
        } else {
            l1toks_remove += (old_toks - new_toks);
        }

        // fill in maps for L1-2 groups token transfers
        NodeTokCountMap l12_old_toks;
        if (curDlt->getDepth() >= 2) {
            for (fds_uint32_t j = 0; j < node_l1_toks.size(); ++j) {
                DltTokenGroupPtr grp = curDlt->getNodes(node_l1_toks[j]);
                NodeUuid l2_uuid = grp->get(1);
                l12_old_toks[l2_uuid]++;
            }
        }
        for (cit2 = nodes.cbegin(); cit2 != nodes.cend(); ++cit2) {
            NodeUuid l2_uuid = *cit2;
            if (l1_uuid == l2_uuid) continue;
            new_toks = newPlacement->tokens(l1_uuid, l2_uuid);
            old_toks = 0;
            if (l12_old_toks.count(l2_uuid) > 0)
                old_toks = l12_old_toks[l2_uuid];
            if (new_toks != old_toks) {
                fds_uint64_t id = nodeSetToId(l1_uuid, l2_uuid);
                l12_diff_toks[id] = new_toks - old_toks;
            }
        }
    }

    fds_verify(l1toks_transfer == l1toks_remove);
    print(nodes);
}

void
PlacementDiff::generateL34Diffs(const PlacementMetricsPtr& newPlacement,
                                const DLT* curDlt,
                                fds_uint32_t rowNum) {
    NodeIndexMap::const_iterator cit, cit2, cit3, cit4;
    fds_uint64_t id;
    fds_uint32_t numTokens = curDlt->getNumTokens();
    if (curDlt->getDepth() <= rowNum) return;
    l123_diff_toks.clear();
    l1234_diff_toks.clear();

    if (rowNum == 2) {
        // for each node1, node2, node3 group, fill in optimal number of toks
        for (cit = node_index.begin(); cit != node_index.end(); ++cit) {
            for (cit2 = node_index.begin(); cit2 != node_index.end(); ++cit2) {
                for (cit3 = node_index.begin(); cit3 != node_index.end(); ++cit3) {
                    id = nodeSetToId(cit->first, cit2->first, cit3->first);
                    // optimal value rounded down
                    l123_diff_toks[id] = newPlacement->optimalTokens(cit->first,
                                                                     cit2->first,
                                                                     cit3->first);
                }
            }
        }
        for (fds_token_id tok = 0; tok < numTokens; ++tok) {
            DltTokenGroupPtr col = curDlt->getNodes(tok);
            id = nodeSetToId(col->get(0), col->get(1), col->get(2));
            l123_diff_toks[id]--;
        }
    } else if (rowNum == 3) {
        // for each node1, node2, node3, node4  group, fill in optimal number of toks
        for (cit = node_index.begin(); cit != node_index.end(); ++cit) {
            for (cit2 = node_index.begin(); cit2 != node_index.end(); ++cit2) {
                for (cit3 = node_index.begin(); cit3 != node_index.end(); ++cit3) {
                    for (cit4 = node_index.begin(); cit4 != node_index.end(); ++cit4) {
                        id = nodeSetToId(cit->first, cit2->first,
                                         cit3->first, cit4->first);
                        // optimal value rounded down
                        l1234_diff_toks[id] = newPlacement->optimalTokens(cit->first,
                                                                          cit2->first,
                                                                          cit3->first,
                                                                          cit4->first);
                    }
                }
            }
        }
        for (fds_token_id tok = 0; tok < numTokens; ++tok) {
            DltTokenGroupPtr col = curDlt->getNodes(tok);
            id = nodeSetToId(col->get(0), col->get(1), col->get(2), col->get(3));
            l1234_diff_toks[id]--;
        }
    }
}

void
PlacementDiff::print(const NodeUuidSet& nodes) {
    NodeUuidSet::const_iterator cit, cit2, cit3;
    FDS_PLOG_SEV(g_fdslog, fds_log::debug)
            << "L1 and L1-2 placement relative to optimal"
            << std::endl << "Number of primary tokens to transfer "
            << l1toks_transfer;
    int total_positive_err = 0;
    int total_negative_err = 0;
    for (cit = nodes.cbegin(); cit != nodes.cend(); ++cit) {
        if (l1_diff_toks.count(*cit) > 0) {
            int diff_toks = l1_diff_toks[*cit];
            FDS_PLOG_SEV(g_fdslog, fds_log::debug)
                    << "Node " << std::hex << (*cit).uuid_get_val() << std::dec
                    << " " << diff_toks;
            if (diff_toks > 0)
                total_positive_err += diff_toks;
            else
                total_negative_err += diff_toks;
        } else {
            FDS_PLOG_SEV(g_fdslog, fds_log::debug)
                    << "Node " << std::hex << (*cit).uuid_get_val() << std::dec << "  0";
        }
    }
    FDS_PLOG(g_fdslog) << "DP: L1 total positive err " << total_positive_err;
    FDS_PLOG(g_fdslog) << "DP: L1 total negative err " << total_negative_err;
    total_positive_err = 0;
    total_negative_err = 0;
    for (cit = nodes.cbegin(); cit != nodes.cend(); ++cit) {
        for (cit2 = nodes.cbegin(); cit2 != nodes.cend(); ++cit2) {
            fds_uint64_t id = nodeSetToId(*cit, *cit2);
            if (l12_diff_toks.count(id) > 0) {
                int diff_toks = l12_diff_toks[id];
                if (diff_toks == 0) continue;
                FDS_PLOG_SEV(g_fdslog, fds_log::debug)
                        << "Node group " << std::hex << (*cit).uuid_get_val() << ":"
                        << (*cit2).uuid_get_val() << std::dec << " " << diff_toks;
                if (diff_toks > 0)
                    total_positive_err += diff_toks;
                else
                    total_negative_err += diff_toks;
            }
        }
    }
    FDS_PLOG(g_fdslog) << "DP: L1-2 group total positive err " << total_positive_err;
    FDS_PLOG(g_fdslog) << "DP: L1-2 group total negative err " << total_negative_err;
    if (l123_diff_toks.size() == 0) return;
    total_positive_err = 0;
    total_negative_err = 0;
    for (cit = nodes.cbegin(); cit != nodes.cend(); ++cit) {
        for (cit2 = nodes.cbegin(); cit2 != nodes.cend(); ++cit2) {
            for (cit3 = nodes.cbegin(); cit3 != nodes.cend(); ++cit3) {
                fds_uint64_t id = nodeSetToId(*cit, *cit2, *cit3);
                if (l123_diff_toks.count(id) > 0) {
                    int diff_toks = l123_diff_toks[id];
                    if (diff_toks == 0) continue;
                    FDS_PLOG_SEV(g_fdslog, fds_log::debug)
                            << "Node group " << std::hex << (*cit).uuid_get_val() << ","
                            << (*cit2).uuid_get_val() << "," << (*cit3).uuid_get_val()
                            << std::dec << " " << diff_toks;
                    if (diff_toks > 0)
                        total_positive_err += diff_toks;
                    else
                        total_negative_err += diff_toks;
                }
            }
        }
    }
    FDS_PLOG(g_fdslog) << "DP: L1-2-3 group total positive err " << total_positive_err;
    FDS_PLOG(g_fdslog) << "DP: L1-2-3 group total negative err " << total_negative_err;
}

//
// Transfer primary token from node with uuid 'l1_uuid' to a different
// node, which is returned as 'new_l1_uuid'.
// I.e., given existing primary 'l1_uuid' and secondary 'l2_uuid', find
// new node uuid that should steal token from 'l1_uuid'.
//
fds_bool_t
PlacementDiff::optimalTransferL1Token(const NodeUuid& l1_uuid,
                                      const NodeUuid& l2_uuid,
                                      NodeUuid* new_l1_uuid) {
    if (l1toks_transfer == 0)
        return false;  // no L1 tokens to transfer

    // check if this node group can give any tokens
    if ((l1_diff_toks.count(l1_uuid) == 0) ||
        (l1_diff_toks[l1_uuid] >= 0)) {
        return false;
    }
    fds_uint64_t cur_nsid = nodeSetToId(l1_uuid, l2_uuid);
    if ((l12_diff_toks.count(cur_nsid) == 0) ||
        (l12_diff_toks[cur_nsid] >= 0)) {
        return false;
    }
    // decide which node to give the token to
    // (l1_uuid, l2_uuid) --> (new_l1_uuid, l2_uuid)
    for (NodeIndexMap::iterator it = node_index.begin();
         it != node_index.end();
         ++it) {
        NodeUuid new_uuid = it->first;
        fds_uint64_t new_node_index = nodeSetToId(new_uuid, l2_uuid);
        if (new_uuid == l2_uuid) continue;
        if ((l12_diff_toks.count(new_node_index) > 0) &&
            (l12_diff_toks[new_node_index] > 0) &&
            (l1_diff_toks.count(new_uuid) > 0) &&
            (l1_diff_toks[new_uuid] > 0)) {
            // found primary token to give
            *new_l1_uuid = new_uuid;
            l12_diff_toks[new_node_index]--;

            // we gave L1 token to new_uuid, l2_uuid nodeset
            l1_diff_toks[new_uuid]--;
            if (l1_diff_toks[new_uuid] == 0)
                l1_diff_toks.erase(new_uuid);
            // and took it from l1_uuid
            l12_diff_toks[cur_nsid]++;
            l1_diff_toks[l1_uuid]++;
            if (l1_diff_toks[l1_uuid] == 0)
                l1_diff_toks.erase(l1_uuid);

            l1toks_transfer--;

            FDS_PLOG_SEV(g_fdslog, fds_log::debug)
                    << "DP: Optimal transfer L1 token from Node group ("
                    << std::hex << l1_uuid.uuid_get_val() << ","
                    << l2_uuid.uuid_get_val() << ") to ("
                    << (*new_l1_uuid).uuid_get_val() << ","
                    << l2_uuid.uuid_get_val() << ")" << std::dec;
            return true;
        }
    }
    return false;
}

//
// Transfer primary token from 'l1_uuid' not taking into
// account keeping optimal L1-2 dispersion. So, gives token
// to the first node that needs to take L1 tokens. Also updates
// dispersion metric appropriately.
//
fds_bool_t
PlacementDiff::transferL1Token(const NodeUuid& l1_uuid,
                               const NodeUuid& l2_uuid,
                               NodeUuid* new_l1_uuid) {
    if (l1toks_transfer == 0)
        return false;  // no L1 tokens to transfer

    // check if this node group can give any tokens
    if ((l1_diff_toks.count(l1_uuid) == 0) ||
        (l1_diff_toks[l1_uuid] >= 0)) {
        return false;
    }

    // give to the first node that needs L1 token
    NodeTokDiffMap::iterator it = l1_diff_toks.begin();
    while ((it != l1_diff_toks.end()) && (it->second <= 0)) {
        ++it;
    }
    // if we have primary tokens to remove, we must
    // have primary tokens to add
    fds_verify(it != l1_diff_toks.end());
    fds_verify(it->second > 0);
    *new_l1_uuid = it->first;
    it->second--;
    l1toks_transfer--;

    // update maps
    if (it->second == 0) {
        l1_diff_toks.erase(it);
    }
    l1_diff_toks[l1_uuid]++;
    if (l1_diff_toks[l1_uuid] == 0) {
        l1_diff_toks.erase(l1_uuid);
    }

    // if we have secondary nodes, update l1-2 diff tokens
    // for keeping track of dispersion
    if (l2_uuid.uuid_get_val() != 0) {
        // we gave token to group new_l1_uuid, l2_uuid
        fds_uint64_t old_nsid = nodeSetToId(l1_uuid, l2_uuid);
        fds_uint64_t new_nsid = nodeSetToId(*new_l1_uuid, l2_uuid);
        if (l12_diff_toks.count(new_nsid) > 0)
            l12_diff_toks[new_nsid]--;
        else
            l12_diff_toks[new_nsid] = -1;
        // we removed token from group l1_uuid, l2_uuid
        if (l12_diff_toks.count(old_nsid) > 0)
            l12_diff_toks[old_nsid]++;
        else
            l12_diff_toks[old_nsid] = 1;
    }

    FDS_PLOG_SEV(g_fdslog, fds_log::debug)
            << "DP: NON-optimal transfer L1 token from Node group (" << std::hex
            << l1_uuid.uuid_get_val() << "," << l2_uuid.uuid_get_val()
            << ") to (" << (*new_l1_uuid).uuid_get_val() << ","
            << l2_uuid.uuid_get_val() << ")" << std::dec;

    return true;
}

//
// try to transfer L2 token from group (l1_uuid, l2_uuid)
// to (l1_uuid, new_l2_uuid) -- find new_l2_uuid to transfer
// token to maintaining optimal dispersion
//
fds_bool_t
PlacementDiff::transferL2Token(const NodeUuid& l1_uuid,
                               const NodeUuid& l2_uuid,
                               NodeUuid* new_l2_uuid) {
    fds_uint64_t old_nsid = nodeSetToId(l1_uuid, l2_uuid);
    fds_uint64_t new_nsid = 0;
    if ((l12_diff_toks.count(old_nsid) == 0) ||
        (l12_diff_toks[old_nsid] >= 0)) {
        return false;
    }

    // decide which node to give the token to
    fds_bool_t found = false;
    for (NodeIndexMap::iterator it = node_index.begin();
         it != node_index.end();
         ++it) {
        if (it->first == l1_uuid) continue;
        new_nsid = nodeSetToId(l1_uuid, it->first);
        if (l12_diff_toks[new_nsid] > 0) {
            *new_l2_uuid = it->first;
            l12_diff_toks[new_nsid]--;
            l12_diff_toks[old_nsid]++;
            found = true;
            break;
        }
    }
    fds_verify(found == true);

    FDS_PLOG_SEV(g_fdslog, fds_log::debug)
            << "DP: transfer L2 token from Node group (" << std::hex
            << l1_uuid.uuid_get_val() << "," << l2_uuid.uuid_get_val()
            << ") to (" << l1_uuid.uuid_get_val() << ","
            << (*new_l2_uuid).uuid_get_val() << ")" << std::dec;
    return found;
}

fds_bool_t
PlacementDiff::transferL34Token(const DltTokenGroupPtr& col,
                                fds_uint32_t row,
                                NodeUuid* new_uuid) {
    fds_verify(row < col->getLength());
    bool bret = false;
    if (row == 2) {
        bret = transferL3Token(col->get(0),
                               col->get(1),
                               col->get(2),
                               new_uuid);
    } else if (row == 3) {
        bret = transferL4Token(col->get(0),
                               col->get(1),
                               col->get(2),
                               col->get(3),
                               new_uuid);
    }
    return bret;
}


//
// Try to transfer token from group (l1_uuid, l2_uuid, l3_uuid)
// to (l1_uuid, l2_uuid, new_l3_uuid)
//
fds_bool_t
PlacementDiff::transferL3Token(const NodeUuid& l1_uuid,
                               const NodeUuid& l2_uuid,
                               const NodeUuid& l3_uuid,
                               NodeUuid* new_l3_uuid) {
    fds_uint64_t old_nsid = nodeSetToId(l1_uuid, l2_uuid, l3_uuid);
    fds_uint64_t new_nsid = 0;
    if (l123_diff_toks.size() == 0)
        return false;  // either already optimal or L3 diffs not generated
    if ((l123_diff_toks.count(old_nsid) == 0) ||
        (l123_diff_toks[old_nsid] >= 0)) {
        return false;  // this node set does not want to give tokens
    }
    // see if we can give this token to another node group
    // = replace l3_uuid with some other uuid
    fds_bool_t found = false;
    for (NodeIndexMap::iterator it = node_index.begin();
         it != node_index.end();
         ++it) {
        new_nsid = nodeSetToId(l1_uuid, l2_uuid, it->first);
        if (l123_diff_toks.count(new_nsid) && l123_diff_toks[new_nsid] > 0) {
            *new_l3_uuid = it->first;
            l123_diff_toks[new_nsid]--;
            l123_diff_toks[old_nsid]++;
            found = true;
            break;
        }
    }
    if (!found) return false;
    FDS_PLOG_SEV(g_fdslog, fds_log::debug)
            << "DP: transfer L3 token from Node group (" << std::hex
            << l1_uuid.uuid_get_val() << "," << l2_uuid.uuid_get_val()
            << "," << l3_uuid.uuid_get_val() << ") to ("
            << l1_uuid.uuid_get_val() << "," << l2_uuid.uuid_get_val()
            << "," << (*new_l3_uuid).uuid_get_val() << ")" << std::dec;
    return true;
}

//
// Try to transfer token from group (l1_uuid, l2_uuid, l3_uuid, l4_uuid)
// to (l1_uuid, l2_uuid, l3_uuid, new_l4_uuid)
//
fds_bool_t
PlacementDiff::transferL4Token(const NodeUuid& l1_uuid,
                               const NodeUuid& l2_uuid,
                               const NodeUuid& l3_uuid,
                               const NodeUuid& l4_uuid,
                               NodeUuid* new_l4_uuid) {
    fds_uint64_t old_nsid = nodeSetToId(l1_uuid, l2_uuid, l3_uuid, l4_uuid);
    fds_uint64_t new_nsid = 0;
    if (l1234_diff_toks.size() == 0)
        return false;  // either already optimal or L3 diffs not generated
    if ((l1234_diff_toks.count(old_nsid) == 0) ||
        (l1234_diff_toks[old_nsid] >= 0)) {
        return false;  // this node set does not want to give tokens
    }
    // see if we can give this token to another node group
    // = replace l3_uuid with some other uuid
    fds_bool_t found = false;
    for (NodeIndexMap::iterator it = node_index.begin();
         it != node_index.end();
         ++it) {
        new_nsid = nodeSetToId(l1_uuid, l2_uuid, l3_uuid, it->first);
        if (l1234_diff_toks.count(new_nsid) && l1234_diff_toks[new_nsid] > 0) {
            *new_l4_uuid = it->first;
            l1234_diff_toks[new_nsid]--;
            l1234_diff_toks[old_nsid]++;
            found = true;
            break;
        }
    }
    if (!found) return false;
    FDS_PLOG_SEV(g_fdslog, fds_log::debug)
            << "DP: transfer L4 token from Node group (" << std::hex
            << l1_uuid.uuid_get_val() << "," << l2_uuid.uuid_get_val()
            << "," << l3_uuid.uuid_get_val() << ","
            << l4_uuid.uuid_get_val() << ") to ("
            << l1_uuid.uuid_get_val() << "," << l2_uuid.uuid_get_val()
            << "," << l4_uuid.uuid_get_val()
            << "," << (*new_l4_uuid).uuid_get_val() << ")" << std::dec;
    return true;
}

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
    ClusterMap::const_sm_iterator rr_it = curMap->cbegin_sm();

    fds_uint32_t total_num_nodes = curMap->getNumMembers(fpi::FDSP_STOR_MGR);
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
        ClusterMap::const_sm_iterator nl_it = rr_it;
        if (nl_it == curMap->cend_sm()) {
            continue;
        }

        // Iterate over the column and set the uuids
        for (fds_uint32_t j = 0; j < column_depth; j++) {
            NodeUuid uuid = (nl_it->second)->get_uuid();
            tg.set(j, uuid);
            nl_it++;
            if (nl_it == curMap->cend_sm()) {
                nl_it = curMap->cbegin_sm();
            }
        }
        newDlt->setNodes(i, tg);

        // Move the starting point for the list
        // and reset it to the beginning if we've
        // looped around.
        rr_it++;
        if (rr_it == curMap->cend_sm()) {
            rr_it = curMap->cbegin_sm();
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
    fds_uint32_t total_nodes = currMap->getNumMembers(fpi::FDSP_STOR_MGR);
    fds_verify(newDlt != NULL);

    // Compute DLT from scratch if this is the first version
    // or current DLT only had one node (one row) so in any
    // case the number of token transfers will be the same so
    // no need to do anything smart,
    // or if the DLT width changed (in this case, all object id
    // to token mapping changed
    if ((currDlt == NULL) || (currDlt->getDepth() < 2) ||
        (newDlt->getWidth() != currDlt->getWidth())) {
        FDS_PLOG(getLog()) << "ConsistHashAlgorithm: compute new DLT for "
                           << total_nodes << " SMs";
        computeInitialDlt(currMap, newDlt);
        return err;
    }

    FDS_PLOG(getLog()) << "ConsistHashAlgorithm: recompute new DLT for "
                       << (currMap->getAddedServices(fpi::FDSP_STOR_MGR)).size()
                       << " SM additions and "
                       << (currMap->getRemovedServices(fpi::FDSP_STOR_MGR)).size()
                       << " SM deletions";

    // at this point both DLTs have same number of tokens
    fds_uint32_t numTokens = newDlt->getNumTokens();
    fds_uint32_t min_depth = (currDlt->getDepth() < newDlt->getDepth()) ?
            currDlt->getDepth() : newDlt->getDepth();
    for (fds_uint32_t i = 0; i < numTokens; ++i) {
        DltTokenGroupPtr column = currDlt->getNodes(i);
        for (fds_uint32_t j = 0; j < min_depth; ++j) {
            NodeUuid uuid = column->get(j);
            newDlt->setNode(i, j, uuid);
        }
    }

    handleDltChange(currMap, currDlt, newDlt);

    FDS_PLOG_SEV(getLog(), fds_log::debug)
            << "ConsistHash: Finished updating DLT";
    return err;
}

//
// Updates DLT based on updated cluster map.
// Assumes that newDlt is a copy of curDlt.
// Number of primary tokens in new dlt and old dlt must be the same
// (otherwise must compute Dlt from scratch via 'computeInitialDlt()')
//
void
ConsistHashAlgorithm::handleDltChange(const ClusterMap *curMap,
                                      const DLT *curDlt,
                                      DLT *newDlt) {
    fds_uint32_t rm_tokens;
    NodeUuid new_uuid;
    fds_bool_t bret;
    NodeUuidSet::const_iterator cit, cit2, cit3;
    fds_uint32_t numTokens = newDlt->getNumTokens();
    NodeUuidSet rmNodes = curMap->getRemovedServices(fpi::FDSP_STOR_MGR);

    // should not be called if new DLT width has changed
    fds_verify(curDlt->getWidth() == newDlt->getWidth());

    // build a list of all nodes including removed nodes
    NodeUuidSet nodes = curMap->getRemovedServices(fpi::FDSP_STOR_MGR);
    for (ClusterMap::const_sm_iterator cit = curMap->cbegin_sm();
         cit != curMap->cend_sm();
         ++cit) {
        nodes.insert(cit->first);
    }

    // Calculate new optimal number of primary and secondary tokens
    // to give to each node (this is done in PlacementMetrics constructor)
    PlacementMetricsPtr metricsPtr(
        new PlacementMetrics(curMap, numTokens, newDlt->getDepth()));

    // Calculate exact number of token transfers for L1 tokens and
    // L1-2 groups tokens
    PlacementDiffPtr diffPtr(new PlacementDiff(metricsPtr, curDlt, nodes));

    // Transfer L1 tokens that give us optimal dispersion
    // (L1-2 group optimal number of tokens)
    for (cit = nodes.cbegin(); cit != nodes.cend(); ++cit) {
        // get number of primary tokens to steal from this node
        rm_tokens = diffPtr->rmL1Tokens(*cit);
        if (rm_tokens == 0) continue;

        // get a list of all primary tokens of this node
        TokenList node_l1_toks;
        curDlt->getTokens(&node_l1_toks, *cit, 0);
        fds_verify(node_l1_toks.size() >= rm_tokens);

        // Transfer L1 tokens from this node to other nodes maintaining optimal dispersion
        TokenList::iterator tok_it = node_l1_toks.begin();
        // however, if there is only one row, there is no requirement for dispersion
        if (newDlt->getDepth() > 1) {
            while ((rm_tokens > 0) && (tok_it != node_l1_toks.end())) {
                DltTokenGroupPtr column = curDlt->getNodes(*tok_it);
                if (diffPtr->optimalTransferL1Token(*cit, column->get(1), &new_uuid)) {
                    newDlt->setNode(*tok_it, 0, new_uuid);
                    tok_it = node_l1_toks.erase(tok_it);
                    rm_tokens--;
                } else {
                    ++tok_it;
                }
            }
        }

        // for remaining tokens to steal, steal from nodes regardless optimal
        // dispersion, but update dispersion metric appropriately
        tok_it = node_l1_toks.begin();
        for (fds_uint32_t i = 0; i < rm_tokens; ++i) {
            fds_verify(tok_it != node_l1_toks.end());
            DltTokenGroupPtr col = curDlt->getNodes(*tok_it);
            bret = diffPtr->transferL1Token(
                col->get(0),
                (col->getLength() > 1) ? col->get(1) : NodeUuid(),
                &new_uuid);
            fds_verify(bret == true);
            newDlt->setNode(*tok_it, 0, new_uuid);
            ++tok_it;
        }
    }

    // if only one row in DLT, nothing else to do
    if (newDlt->getDepth() < 2) {
        diffPtr->print(nodes);
        return;
    }

    // transfer L2 tokens
    newDlt->generateNodeTokenMap();
    for (cit = nodes.cbegin(); cit != nodes.cend(); ++cit) {
        // get a list of all primary tokens of this node
        TokenList node_l1_toks;
        newDlt->getTokens(&node_l1_toks, *cit, 0);

        // transfer tokens
        for (TokenList::iterator tok_it = node_l1_toks.begin();
             tok_it != node_l1_toks.end();
             ++tok_it) {
            DltTokenGroupPtr col = newDlt->getNodes(*tok_it);
            if (diffPtr->transferL2Token(col->get(0), col->get(1), &new_uuid)) {
                newDlt->setNode(*tok_it, 1, new_uuid);
            } else {
                // make sure that L1 and L2 uuids are unique
                // haven't seen this case happen yet, but safeguard anyway
                if (col->get(0) == col->get(1)) {
                    fds_token_id ind = (*tok_it + 1) % numTokens;
                    new_uuid = newDlt->getPrimary(ind);
                    while (new_uuid == col->get(0)) {
                        ind = (ind + 1) % numTokens;
                        new_uuid = newDlt->getPrimary(ind);
                    }
                    newDlt->setNode(*tok_it, 1, new_uuid);
                }
            }
        }
    }

    diffPtr->print(nodes);

    // transfer L3 and then L4 tokens tokens
    for (fds_uint64_t row = 2; row < 4; ++row) {
        if (newDlt->getDepth() <= row) break;

        newDlt->generateNodeTokenMap();
        diffPtr->generateL34Diffs(metricsPtr, newDlt, row);

        for (cit = nodes.cbegin(); cit != nodes.cend(); ++cit) {
            // get a list of all primary tokens of this node
            TokenList node_l1_toks;
            newDlt->getTokens(&node_l1_toks, *cit, 0);

            // transfer tokens
            for (TokenList::iterator tok_it = node_l1_toks.begin();
                 tok_it != node_l1_toks.end();
                 ++tok_it) {
                DltTokenGroupPtr col = newDlt->getNodes(*tok_it);
                if (diffPtr->transferL34Token(col, row, &new_uuid)) {
                    newDlt->setNode(*tok_it, row, new_uuid);
                } else {
                    // make sure this is unique node and not node to remove
                    if ((col->get(0) == col->get(row)) ||
                        (col->get(1) == col->get(row)) ||
                        ((row > 2) && (col->get(2) == col->get(row))) ||
                        ((col->get(row)).uuid_get_val() == 0) ||
                        (rmNodes.count(col->get(row)) > 0)) {
                        fds_token_id ind = (*tok_it + 1) % numTokens;
                        new_uuid = newDlt->getPrimary(ind);
                        while ((new_uuid == col->get(0)) ||
                               (new_uuid == col->get(1)) ||
                               ((row > 2) && (new_uuid == col->get(2)))) {
                            ind = (ind + 1) % numTokens;
                            new_uuid = newDlt->getPrimary(ind);
                        }
                        newDlt->setNode(*tok_it, row, new_uuid);
                    }
                }
            }
        }
    }

    diffPtr->print(nodes);
}

void
ConsistHashAlgorithm::computeInitialDlt(const ClusterMap *curMap,
                                        DLT* newDLT)
{
    NodeUuid cur_uuid;
    fds_uint32_t total_nodes = curMap->getNumMembers(fpi::FDSP_STOR_MGR);
    fds_uint32_t col_depth = newDLT->getDepth();
    fds_uint64_t numTokens = pow(2, newDLT->getWidth());
    ClusterMap::const_sm_iterator node_it = curMap->cbegin_sm();

    if (total_nodes == 0) {
        // we assume that newDLT is already initialized to 0s
        // so nothing to do if total_nodes == 0
        return;
    } else if (total_nodes == 1) {
        // For efficiency, if we have just one node, set the first
        // row of DLT to this node uuid and return
        NodeUuid uuid = (node_it->second)->get_uuid();
        for (fds_token_id i = 0; i < numTokens; i++) {
            newDLT->setNode(i, 0, uuid);
        }
        return;
    }
    // calculate number of primary tokens to give to each node
    // this is done in PlacementMetrics constructor
    PlacementMetricsPtr metricsPtr(new PlacementMetrics(curMap, numTokens, col_depth));

    // We assign actual tokens to nodes by assigning the first set tokens to
    // the first node, then second set of tokens to the second node and so on
    ClusterMap::const_sm_iterator it2, it3, it4;
    fds_uint32_t tok_idx = 0;
    for (node_it = curMap->cbegin_sm(); node_it != curMap->cend_sm(); ++node_it) {
        // Fill in the first row
        NodeUuid l1_uuid = (*node_it).first;
        fds_uint32_t l1_toks = metricsPtr->tokens(l1_uuid);
        fds_verify((tok_idx + l1_toks) <= numTokens);
        for (fds_uint32_t rel_idx = 0; rel_idx < l1_toks; ++rel_idx) {
            newDLT->setNode(tok_idx+rel_idx, 0, l1_uuid);
        }
        if (col_depth < 2) {
            tok_idx += l1_toks;
            continue;
        }
        // Fill in the second row when the primary is node 'uuid'
        // = columns [tok_idx ... tok_idx + toks)
        fds_uint32_t l2_idx = tok_idx;
        for (it2 = curMap->cbegin_sm(); it2 != curMap->cend_sm(); ++it2) {
            NodeUuid l2_uuid = (*it2).first;
            if (l2_uuid == l1_uuid) {
                continue;  // skip this node
            }
            // get optimal number of secondary tokens for group l1_uuid,l2_uuid
            fds_uint32_t l2_toks_count = metricsPtr->tokens(l1_uuid, l2_uuid);
            // assign this l2_uuid to l2_toks_count cells in level 2
            fds_verify((l2_idx + l2_toks_count) <= numTokens);
            for (fds_uint32_t rel_idx = 0; rel_idx < l2_toks_count; ++rel_idx) {
                newDLT->setNode(l2_idx + rel_idx, 1, l2_uuid);
            }
            // fill in third row -- for rows 3 and 4 we just fill in
            // rounded down number of tokens and then will fill in
            // the un-filled cells with values (using ring approach)
            // so it is less/more simple calculation and it is ok to be
            // less precise on those levels (for now)
            fds_uint32_t l3_idx = l2_idx;
            l2_idx += l2_toks_count;
            if (col_depth < 3) continue;
            for (ClusterMap::const_sm_iterator it3 = curMap->cbegin_sm();
                 it3 != curMap->cend_sm();
                 ++it3) {
                NodeUuid l3_uuid = (*it3).first;
                if ((l3_uuid == l1_uuid) || (l3_uuid == l2_uuid)) {
                    continue;  // should be unique nodes in same column
                }
                // calculate number of l3 tokens rounded down
                fds_uint32_t l3_toks = metricsPtr->tokens(l1_uuid,
                                                          l2_uuid,
                                                          l3_uuid,
                                                          l2_toks_count);
                for (fds_uint32_t rel_idx = 0;
                     rel_idx < l3_toks;
                     ++rel_idx) {
                    newDLT->setNode(l3_idx + rel_idx, 2, l3_uuid);
                }
                fds_uint32_t l4_idx = l3_idx;
                l3_idx += l3_toks;
                if (col_depth < 4) continue;
                for (ClusterMap::const_sm_iterator it4 = curMap->cbegin_sm();
                     it4 != curMap->cend_sm();
                     ++it4) {
                    NodeUuid l4_uuid = (*it4).first;
                    if ((l4_uuid == l1_uuid) || (l4_uuid == l2_uuid) ||
                        (l4_uuid == l3_uuid)) {
                        continue;  // should be unique nodes in same col
                    }
                    // calculate number of l4 tokens for 'l4_uuid' that
                    // follows 'uuid', 'l2_uuid', l3_uuid' rounded down
                    fds_uint32_t l4_toks = metricsPtr->tokens(l1_uuid, l2_uuid,
                                                              l3_uuid, l4_uuid,
                                                              l3_toks);
                    for (fds_uint32_t rel_idx = 0;
                         rel_idx < l4_toks;
                         ++rel_idx) {
                        newDLT->setNode(l4_idx + rel_idx, 3, l4_uuid);
                    }
                    l4_idx += l4_toks;
                }
            }
        }
        fds_verify(l2_idx == (tok_idx + l1_toks));
        tok_idx += l1_toks;
    }

    // Fill in the remaining rows
    fillEmptyReplicaCells(total_nodes, numTokens, newDLT);
}

//
// The initial compute fully fills in first two rows, but may have
// some unfilled cells in rows #3 and #4 (indexes 2 and 3). This function
// fills in those cells by walking the ring, and fills in rows #5, etc
// if we have higher DLT depth than 4.
//
void
ConsistHashAlgorithm::fillEmptyReplicaCells(fds_uint32_t numNodes,
                                            fds_uint64_t numTokens,
                                            DLT *newDLT) {
    NodeUuid cur_uuid;
    fds_uint32_t cur_row;
    fds_uint32_t col_depth = newDLT->getDepth();

    // We should not have more columns than nodes
    fds_verify(col_depth <= numNodes);
    std::unordered_set<NodeUuid, UuidHash> col_set;
    for (fds_token_id i = 0; i < numTokens; ++i) {
        col_set.clear();
        // create a set of either filled in cells in this column
        // or uuids from first row by walking the ring
        DltTokenGroupPtr column = newDLT->getNodes(i);
        cur_uuid = column->get(0);
        fds_verify(cur_uuid.uuid_get_val() != 0);
        col_set.insert(cur_uuid);
        for (fds_uint32_t j = 1; j < col_depth; ++j) {
            cur_uuid = column->get(j);
            if (cur_uuid.uuid_get_val() != 0) {
                col_set.insert(cur_uuid);
            }
        }
        // update the column in DLT, but only empty cells
        // by walking the ring clockwise finding unique uuids
        fds_token_id ind = (i+1) % numTokens;
        cur_row = 1;
        while (col_set.size() < col_depth) {
            cur_uuid = column->get(cur_row);
            // first find non-filled cell
            while (cur_uuid.uuid_get_val() != 0) {
                ++cur_row;
                cur_uuid = column->get(cur_row);
            }
            // then find unique uuid by walking the ring
            cur_uuid = newDLT->getPrimary(ind);
            while (col_set.count(cur_uuid) != 0) {
                ind = (ind + 1) % numTokens;
                cur_uuid = newDLT->getPrimary(ind);
            }
            col_set.insert(cur_uuid);
            newDLT->setNode(i, cur_row, cur_uuid);
        }
    }
}
}  // namespace fds
