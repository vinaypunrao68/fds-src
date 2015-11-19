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
             << "tokens, will finish allocating total " << numTokens;
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
    LOGDEBUG << "!->PlacementDiff constructor";
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

        LOGDEBUG << "!->Eval node:" << std::hex
                 << l1_uuid.uuid_get_val() << std::dec;
        curDlt->getTokens(&node_l1_toks, l1_uuid, 0);
        new_toks = newPlacement->tokens(l1_uuid);
        old_toks = node_l1_toks.size();
        if (new_toks != old_toks)
            l1_diff_toks[l1_uuid] = new_toks - old_toks;
        LOGDEBUG << "newToks:" << new_toks << " - " << "old_toks:" << old_toks;
        if (new_toks > old_toks) {
            l1toks_transfer += (new_toks - old_toks);
        } else {
            l1toks_remove += (old_toks - new_toks);
        }

        LOGDEBUG << "!-Tokens transferred:" << l1toks_transfer << " &&"
                 <<" Tokens removed:" << l1toks_remove;
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
                                   DLT *newDlt,
                                   fds_uint32_t      numPrimarySMs) {
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
                                    DLT *newDlt,
                                    fds_uint32_t numPrimarySMs) {
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
        computeInitialDlt(currMap, newDlt, numPrimarySMs);
        return err;
    }

    FDS_PLOG(getLog()) << "ConsistHashAlgorithm: recompute new DLT for "
                       << (currMap->getAddedServices(fpi::FDSP_STOR_MGR)).size()
                       << " SM additions and "
                       << (currMap->getRemovedServices(fpi::FDSP_STOR_MGR)).size()
                       << " SM deletions";

    // check if DLT update is valid
    NodeUuidSet rmNodes = currMap->getRemovedServices(fpi::FDSP_STOR_MGR);
    NodeUuidSet addNodes = currMap->getAddedServices(fpi::FDSP_STOR_MGR);
    err = checkUpdateValid(currMap, currDlt, numPrimarySMs, rmNodes);
    if (!err.ok()) {
        LOGERROR << "DLT update is not valid " << err;
        return err;
    }

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

    // if there are no SM additions/deletions, do not re-shuffle, since
    // we want resync due to failed services to be minimal
    if ((rmNodes.size() > 0) || (addNodes.size() > 0)) {
        // re-compute DLT without taking into account failed SMs
        handleDltChange(currMap, currDlt, newDlt);
        LOGDEBUG << "ConsistHash: Finished updating DLT (no failed SM part)";
    }

    if (numPrimarySMs > 0) {
        // try not to have failed services as primaries
        NodeUuidSet nonFailedSms = currMap->getNonfailedServices(fpi::FDSP_STOR_MGR);
        NodeUuidSet allSms = currMap->getServiceUuids(fpi::FDSP_STOR_MGR);
        fds_uint32_t nonFailedCnt = nonFailedSms.size();
        if (nonFailedCnt == total_nodes) {
            LOGDEBUG << "There are no failed SMs, we are done with DLT update";
        } else if ((nonFailedCnt < numPrimarySMs) ||
                   (numPrimarySMs >= newDlt->getDepth())) {
            LOGWARN << "We don't have enough non-failed SMs to place into primaries";
        } else {
            // rotate columns to place non-failed SMs into primary rows
            LOGDEBUG << "Failed SMs count " << nonFailedCnt
                     << " out of " << total_nodes << " total SMs";
            newDlt->generateNodeTokenMap();
            demoteFailedPrimaries(newDlt, numPrimarySMs, nonFailedSms);

            // if several SMs failed, above method may set some cells to 0
            // fill in these cells with non-failed SMs
            newDlt->generateNodeTokenMap();
            fillEmptyCells(numTokens, newDlt, numPrimarySMs, nonFailedSms, allSms);
        }
    }
    LOGDEBUG << "ConsistHash: Finished updating DLT";
    return err;
}

Error
ConsistHashAlgorithm::checkUpdateValid(const ClusterMap *curMap,
                                       const DLT *curDlt,
                                       fds_uint32_t numPrimarySMs,
                                       const NodeUuidSet& rmNodes) {
    Error err(ERR_OK);

    // Update is invalid if numPrimarySMs > 0, and all primaries are removed
    if (numPrimarySMs != 0) {
        fds_uint64_t numTokens = curDlt->getNumTokens();
        fds_uint32_t primDepth = numPrimarySMs;
        if (primDepth > curDlt->getDepth()) {
            primDepth = curDlt->getDepth();
        }
        for (fds_token_id i = 0; i < numTokens; i++) {
            DltTokenGroupPtr col = curDlt->getNodes(i);
            fds_uint32_t rmCount = 0;
            for (fds_uint32_t j = 0; j < primDepth; ++j) {
                NodeUuid uuid = col->get(j);
                if (rmNodes.count(uuid) > 0) {
                    //LOGDEBUG << "!Incrementing removed count by uuid:" << std::hex << uuid.uuid_get_val() << std::dec;
                    ++rmCount;
                }
            }
            if (rmCount >= primDepth) {
                LOGERROR << "At least one column in DLT has all primaries marked "
                         << "for removal; invalid DLT update";
                return ERR_INVALID_ARG;
            }
        }
    }

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

    LOGDEBUG << "!->HANDLE DltChange";
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

fds_uint32_t
ConsistHashAlgorithm::numOfFailedSMsInToken(const DltTokenGroupPtr& col,
                                            fds_uint32_t tokenId,
                                            fds_uint32_t checkDepth,
                                            const NodeUuidSet& nonFailedSms) {
    fds_uint32_t count = 0;
    for (fds_uint32_t j = 0; j < checkDepth; ++j) {
        NodeUuid uuid = col->get(j);
        if (nonFailedSms.count(uuid) == 0) {
            ++count;
        }
    }
    return count;
}

/*
 * If numPrimarySMs == 0, noop
 * Bubble up non-failed SMs in each column;
 *    -- If all primaries fail, secondaries will take their place;
 *    -- If the whole column failed, do not change this column
 *    -- If after replacing failed primaries with secondaries, there
 *    are still failed SMs on primary slots, set these slots to 0
 *    (the caller method will replace 0s with non-failed SM uuids)
 */
void
ConsistHashAlgorithm::demoteFailedPrimaries(DLT* newDLT,
                                            fds_uint32_t numPrimarySMs,
                                            const NodeUuidSet& nonFailedSms) {
    if (numPrimarySMs == 0) {
        // this param tells we don't care about 'failed' state of SMs
        return;
    }
    // check every DLT token
    fds_uint64_t numTokens = newDLT->getNumTokens();
    fds_uint32_t primDepth = numPrimarySMs;
    if (primDepth > newDLT->getDepth()) {
        primDepth = newDLT->getDepth();
    }

    LOGNORMAL << "Before demote: " << *newDLT;
    for (fds_token_id i = 0; i < numTokens; i++) {
        DltTokenGroupPtr col = newDLT->getNodes(i);
        fds_uint32_t primFailedSmsCount = numOfFailedSMsInToken(col, i,
                                                                numPrimarySMs, nonFailedSms);

        if (primFailedSmsCount == 0) {
            // none of primaries failed, no-one to demote
            LOGNORMAL << "Token " << i << ": no primary failure";
            continue;  // go to next DLT token
        } else if (primFailedSmsCount >= numPrimarySMs) {
            // all primaries failed
            LOGNORMAL << "Token " << i << ": all primary SMs failed";
            // if all SMs failed in this column (including secondaries)
            // nothing to do for that column as well
            fds_uint32_t failedSmsCount = numOfFailedSMsInToken(col, i,
                                                                newDLT->getDepth(), nonFailedSms);
            if (failedSmsCount >= newDLT->getDepth()) {
                LOGNORMAL << "Token " << i << ": all SMs failed in this column";
                continue;  // go to the next DLT token
            }
        }

        // there is at least one failed primary and at least one non-failed secondary
        fds_uint32_t secondary_j = primDepth;
        for (fds_uint32_t j = 0; j < primDepth; ++j) {
            NodeUuid uuid = col->get(j);
            if (nonFailedSms.count(uuid) == 0) {
                // this primary failed
                NodeUuid nonfailedUuid = col->get(secondary_j);
                while (nonFailedSms.count(nonfailedUuid) == 0) {
                    // this secondary also failed, try another one
                    ++secondary_j;
                    if (secondary_j >= newDLT->getDepth()) {
                        // ran out of secondaries...
                        break;
                    }
                    nonfailedUuid = col->get(secondary_j);
                }
                if (secondary_j < newDLT->getDepth()) {
                    // found secondary to replace failed primary
                    newDLT->setNode(i, secondary_j, uuid);
                    newDLT->setNode(i, j, nonfailedUuid);
                    col->set(secondary_j, uuid);
                    col->set(j, nonfailedUuid);
                    LOGNORMAL << "Column " << i << ": Exchanging row " << j <<
                            " and " << secondary_j;
                    ++secondary_j;
                }
                if (secondary_j >= newDLT->getDepth()) {
                    // ran out of secondaries...
                    break;  // with next column
                }
            }
        }

        // if we are here, at least one of the primaries is non-failed;
        // replace all failed primaries with 0s (so they can be filled
        // later with non-failed SM uuids)
        for (fds_uint32_t j = 0; j < primDepth; ++j) {
            NodeUuid uuid = col->get(j);
            if (nonFailedSms.count(uuid) == 0) {
                // this primary is in 'failed' state, replace with 0
                newDLT->setNode(i, j, 0);
            }
        }
    }

    LOGNORMAL << "After demote: " << *newDLT;
}

void
ConsistHashAlgorithm::computeInitialDlt(const ClusterMap *curMap,
                                        DLT* newDLT,
                                        fds_uint32_t numPrimarySMs)
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

    // if we take into account 'failed' state, do some more re-shuffling
    NodeUuidSet emptySet;
    if (numPrimarySMs > 0) {
        // try not to have failed services as primaries
        NodeUuidSet nonFailedSms = curMap->getNonfailedServices(fpi::FDSP_STOR_MGR);
        NodeUuidSet allSms = curMap->getServiceUuids(fpi::FDSP_STOR_MGR);
        fds_uint32_t nonFailedCnt = nonFailedSms.size();
        if ((nonFailedCnt < numPrimarySMs) ||
            (numPrimarySMs >= col_depth)) {
            // we don't have enough non-failed SMs to place into primaries
            // just fill in cells with uuids independent whether they failed or not
            fillEmptyCells(numTokens, newDLT, 0, curMap->getServiceUuids(fpi::FDSP_STOR_MGR),
                           curMap->getServiceUuids(fpi::FDSP_STOR_MGR));
            return;
        }

        // if we are taking into account failed services, rotate columns
        // to place non-failed services into primary rows
        newDLT->generateNodeTokenMap();
        demoteFailedPrimaries(newDLT, numPrimarySMs, nonFailedSms);

        // since we are computing DLT from scratch, find columns that have
        // all SM uuids that are in 'failed' state, and set the whole column to 0
        newDLT->generateNodeTokenMap();
        for (fds_token_id i = 0; i < numTokens; i++) {
            DltTokenGroupPtr col = newDLT->getNodes(i);
            fds_uint32_t failedSMsCount = numOfFailedSMsInToken(col, i,
                                                                col_depth, nonFailedSms);
            if (failedSMsCount == col_depth) {
                for (fds_uint32_t j = 0; j < col_depth; ++j) {
                    newDLT->setNode(i, j, 0);
                }
            }
        }
        newDLT->generateNodeTokenMap();

        // Fill in empty cells and remaining rows
        fillEmptyCells(numTokens, newDLT, numPrimarySMs, nonFailedSms, allSms);
        return;
    }

    // Fill in the remaining rows
    fillEmptyCells(numTokens, newDLT, 0, curMap->getServiceUuids(fpi::FDSP_STOR_MGR),
                   curMap->getServiceUuids(fpi::FDSP_STOR_MGR));
}

//
// In the original DLT computation algorithm (numPrimarySMs == 0), 
// the initial compute fully fills in first two rows, but may have
// some unfilled cells in rows #3 and #4 (indexes 2 and 3).
// If numPrimarySMs > 0 (when we take into account SM state failed/non-failed)
// we may have unfilled cells in any row
// This function fills in those cells by walking the ring, and fills in rows #5, etc
// if we have higher DLT depth than 4.
// @param replicaCellsOnly true expects that primary is no '0'
//
void
ConsistHashAlgorithm::fillEmptyCells(fds_uint64_t numTokens,
                                     DLT *newDLT,
                                     fds_uint32_t numPrimarySMs,
                                     const NodeUuidSet& nonFailedSms,
                                     const NodeUuidSet& allSms) {
    NodeUuid cur_uuid;
    fds_uint32_t cur_row;
    fds_uint32_t col_depth = newDLT->getDepth();

    std::unordered_set<NodeUuid, UuidHash> col_set;
    for (fds_token_id i = 0; i < numTokens; ++i) {
        col_set.clear();
        // create a set of either filled in cells in this column
        // or uuids from first row by walking the ring
        DltTokenGroupPtr column = newDLT->getNodes(i);
        cur_uuid = column->get(0);
        if (numPrimarySMs == 0) {
            fds_verify(cur_uuid.uuid_get_val() != 0);
        }
        for (fds_uint32_t j = 0; j < col_depth; ++j) {
            cur_uuid = column->get(j);
            if (cur_uuid.uuid_get_val() != 0) {
                col_set.insert(cur_uuid);
            }
        }
        // update the column in DLT, but only empty cells
        // by walking the ring clockwise finding unique uuids
        fds_token_id ind = (i+1) % numTokens;
        fds_uint32_t walkCount = 0;
        fds_uint32_t walkDepth = col_depth;
        if ((numPrimarySMs > 0) && (numPrimarySMs < col_depth)) {
            // we also want the algorithm to be same as original
            // if there are no failed SMs
            if (allSms.size() != nonFailedSms.size()) {
                // first walk the primaries
                walkDepth = numPrimarySMs;
            }
        }
        cur_row = 0;
        while ((cur_row < walkDepth) && (col_set.size() < col_depth)) {
            cur_uuid = column->get(cur_row);
            // first find non-filled cell
            while (cur_uuid.uuid_get_val() != 0) {
                ++cur_row;
                cur_uuid = column->get(cur_row);
            }
            if (cur_row >= walkDepth) {
                break;
            }
            // then find unique uuid by walking the ring
            cur_uuid = newDLT->getPrimary(ind);
            while ((col_set.count(cur_uuid) != 0) ||
                   (cur_uuid.uuid_get_val() == 0) ||
                   (nonFailedSms.count(cur_uuid) == 0)) {
                ind = (ind + 1) % numTokens;
                cur_uuid = newDLT->getPrimary(ind);
                ++walkCount;
                fds_verify(walkCount < 2*numTokens);   // never ending loop!
            }
            col_set.insert(cur_uuid);
            newDLT->setNode(i, cur_row, cur_uuid);
            LOGNORMAL << "Setting col " << i << " row " << cur_row
                      << " to uuid " << cur_uuid.uuid_get_val()
                      << ", col_set.size() " << col_set.size();
            ++cur_row;
        }
        if (walkDepth == col_depth) {
            continue;  // we are done filling in empty slots
        }
        // case when numPrimarySMs > 0, fill in secondary empty slots with
        // uuids of failed or non-failed SMs
        NodeUuidSet::const_iterator it = allSms.cbegin();
        while (col_set.size() < col_depth) {
            fds_verify(cur_row < col_depth);
            cur_uuid = column->get(cur_row);
            // first find non-filled cell
            while (cur_uuid.uuid_get_val() != 0) {
                ++cur_row;
                cur_uuid = column->get(cur_row);
            }
            LOGNORMAL << "Non-filled cell col " << i << " row "
                      << cur_row << ", initial assignment " << *it
                      << " col_set.size() " << col_set.size();
            cur_uuid = *it;
            // find unique uuid by walking the ring
            while (col_set.count(cur_uuid) != 0) {
                ++it;
                fds_verify(it != allSms.cend());
                cur_uuid = *it;
            }
            col_set.insert(cur_uuid);
            newDLT->setNode(i, cur_row, cur_uuid);
            ++cur_row;
        }
    }
}
}  // namespace fds
