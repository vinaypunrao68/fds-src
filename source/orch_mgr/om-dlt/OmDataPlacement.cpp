/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <unordered_map>
#include <map>
#include <utility>
#include <vector>
#include <string>

#include <orch-mgr/om-service.h>
#include <fds_process.h>
#include <OmDataPlacement.h>

namespace fds {
/**********
 * Functions definitions for placement weight map
 **********/
WeightMap::WeightMap(const ClusterMap *cm,
                     const DLT *dlt) {
    computeWeightDist(cm, dlt);
}

void
WeightMap::reset(const ClusterMap *cm,
                 const DLT *dlt) {
    weight_map.clear();
    computeWeightDist(cm, dlt);
}

void
WeightMap::computeWeightDist(const ClusterMap *cm,
                             const DLT        *dlt) {
    // Count the weights for each node and the total
    double totalWeight = 0;
    double tokenCount  = 0;

    // Iterate the cluster map and compute the cluster's
    // total weight, pairs each nodes weight and token count
    // using the DLT's reverse map
    std::unordered_map<NodeUuid,
                       std::pair<double, double>,
                       UuidHash> nodeCounts;
    for (ClusterMap::const_iterator it = cm->cbegin();
         it != cm->cend();
         it++) {
        NodeUuid uuid = (*it).first;
        // Ensure we haven't counted this node before
        fds_verify(nodeCounts.count(uuid) == 0);

        // Extract node's weight and token count
        fds_uint32_t weight = ((*it).second)->node_stor_weight();
        totalWeight += weight;
        const TokenList tl = dlt->getTokens(uuid);
        fds_uint32_t numTokens = tl.size();
        tokenCount += numTokens;

        // Cache the mapping
        std::pair<double, double> info(weight, numTokens);
        nodeCounts[uuid] = info;
    }
    // Make sure we counted every node
    fds_verify(nodeCounts.size() == cm->getNumMembers());

    // Ok if tokenCount we counted is less than total
    // number of tokens in DLT because DLT may still contain
    // nodes that were removed from cluster map
    fds_verify(tokenCount <= (dlt->getNumTokens() *
                              dlt->getDepth()));

    // Iterate the map, compute the ratios based on the
    // pairs, and store the result in the weight map
    double totalDltTokens = dlt->getNumTokens() * dlt->getDepth();
    for (std::unordered_map<NodeUuid,
                       std::pair<double, double>,
                            UuidHash>
            ::const_iterator it = nodeCounts.cbegin();
         it != nodeCounts.cend();
         it++) {
        NodeUuid uuid = (*it).first;
        double weightRatio = ((*it).second).first / totalWeight;
        double tokenRatio = ((*it).second).second / totalDltTokens;
        LoadRatio lr = tokenRatio / weightRatio;
        addNode(uuid, lr);
    }
}

void
WeightMap::addNode(NodeUuid node_uuid,
                   LoadRatio placement_weight) {
    if (weight_map.count(placement_weight) == 0) {
        // Create a new list for this ratio
        std::vector<NodeUuid> uuidList;
        uuidList.push_back(node_uuid);
        weight_map[placement_weight] = uuidList;
    } else {
        // Append to the list for this ratio
        (weight_map[placement_weight]).push_back(node_uuid);
    }
}

NodeUuid
WeightMap::getLowestWeightNode() const{
    std::map<LoadRatio, std::vector<NodeUuid>>::const_iterator it =
            weight_map.begin();
    fds_verify(it != weight_map.end());
    fds_verify((it->second).size() > 0);
    return (it->second).back();
}

NodeUuid
WeightMap::getHighestWeightNode() const{
    std::map<LoadRatio, std::vector<NodeUuid>>::const_reverse_iterator rit =
            weight_map.rbegin();
    fds_verify(rit != weight_map.rend());
    fds_verify((rit->second).size() > 0);
    return (rit->second).back();
}

void
WeightMap::updateHighestLowestWeightNode(fds_uint32_t new_tokens,
                                         fds_uint32_t old_tokens,
                                         fds_uint32_t total_tokens,
                                         fds_bool_t b_highest) {
    // remove the node from its current location since load ratio will change
    LoadRatio lr;
    NodeUuid uuid;
    fds_verify(total_tokens > 0);
    // for simplicity of implementatiob we don't allow 'old_tokens'==0
    // not necessary right now, but if that becomes necessary, will need
    // to pass relative weight to calculate 'new_lr'
    fds_verify(old_tokens > 0);

    if (b_highest) {
        std::map<LoadRatio, std::vector<NodeUuid>>::reverse_iterator rit =
                weight_map.rbegin();
        lr = rit->first;
        uuid = (rit->second).back();
        (rit->second).pop_back();
        if ((rit->second).size() == 0) {
            weight_map.erase(lr);
        }
    } else {
        std::map<LoadRatio, std::vector<NodeUuid>>::iterator it =
                weight_map.begin();
        lr = it->first;
        uuid = (it->second).back();
        (it->second).pop_back();
        if ((it->second).size() == 0) {
            weight_map.erase(lr);
        }
    }

    // update placement weigh
    double totalTokens = total_tokens;
    double tokenRatio = new_tokens / totalTokens;
    double oldTokenRatio = old_tokens / totalTokens;
    LoadRatio new_lr = tokenRatio / (oldTokenRatio / lr);

    addNode(uuid, new_lr);
}

void
WeightMap::debug_print(fds_log* log) const {
    std::map<LoadRatio, std::vector<NodeUuid>>::const_iterator it;
    FDS_PLOG_SEV(log, fds_log::debug) << "Placement Weight Map: ";
    for (it = weight_map.cbegin();
         it != weight_map.cend();
         it++) {
        const std::vector<NodeUuid> &uuidList = (*it).second;
        for (std::vector<NodeUuid>::const_iterator jt = uuidList.cbegin();
             jt != uuidList.cend();
             jt++) {
            FDS_PLOG_SEV(log, fds_log::debug)
                    << "Node 0x" << std::hex << (*jt).uuid_get_val()
                    << " has load ratio " << std::dec << ((*it).first);
        }
    }
}

/**********
 * Functions definitions for data
 * placement
 **********/
DataPlacement::DataPlacement()
        : Module("Data Placement Engine"),
          placeAlgo(NULL),
          commitedDlt(NULL),
          newDlt(NULL),
          curWeightDist(NULL) {
    placementMutex = new fds_mutex("data placement mutex");
    curClusterMap = &gl_OMClusMapMod;
}

DataPlacement::~DataPlacement() {
    delete placementMutex;
    // delete curClusterMap;
    if (commitedDlt != NULL) {
        delete commitedDlt;
    }
    if (newDlt != NULL) {
        delete newDlt;
    }
}

void
DataPlacement::setAlgorithm(PlacementAlgorithm::AlgorithmTypes type) {
    placementMutex->lock();
    delete placeAlgo;
    switch (type) {
        case PlacementAlgorithm::AlgorithmTypes::RoundRobin:
            placeAlgo = new RoundRobinAlgorithm();
            break;
        case PlacementAlgorithm::AlgorithmTypes::ConsistHash:
            placeAlgo = new ConsistHashAlgorithm();
            break;

        default:
            fds_panic("Received unknown data placement algorithm type %u",
                      type);
    }
    algoType = type;
    placementMutex->unlock();
}

Error
DataPlacement::updateMembers(const NodeList &addNodes,
                             const NodeList &rmNodes) {
    placementMutex->lock();
    Error err = curClusterMap->updateMap(addNodes, rmNodes);
    // TODO(Andrew): We should be recomputing the DLT here.
    placementMutex->unlock();

    return err;
}

void
DataPlacement::computeDlt() {
    // Currently always create a new empty DLT.
    // Will change to be relative to the current.
    fds_uint64_t version;
    fds_verify(newDlt == NULL);

    if (commitedDlt == NULL) {
        version = DLT_VER_INVALID + 1;
    } else {
        version = commitedDlt->getVersion() + 1;
    }
    // If we have fewer members than total replicas
    // use the number of members as the replica count
    fds_uint32_t depth = curDltDepth;
    if (curClusterMap->getNumMembers() < curDltDepth) {
        depth = curClusterMap->getNumMembers();
    }

    // Allocate and compute new DLT
    newDlt = new DLT(curDltWidth,
                     depth,
                     version,
                     true);
    placementMutex->lock();
    placeAlgo->computeNewDlt(curClusterMap,
                             commitedDlt,
                             newDlt);

    // Compute DLT's reverse node to token map
    newDlt->generateNodeTokenMap();

    // TODO(Andrew): Compute the DLT's weight distribution
    if (curWeightDist == NULL) {
        curWeightDist = new WeightMap(curClusterMap, newDlt);
    } else {
        curWeightDist->reset(curClusterMap, newDlt);
    }

    // TODO(Andrew): Remove this. Just printing for easy debugging.
    curWeightDist->debug_print(g_fdslog);

    // TODO(Andrew): We should version the (now) old DLT
    // before we delete it and replace it with the
    // new DLT. We should also update the DLT's
    // internal version.
    placementMutex->unlock();
}

/**
 * Begins a rebalance command between the nodes affect
 * in the current DLT change.
 */
Error
DataPlacement::beginRebalance() {
    Error err(ERR_OK);

    FDS_ProtocolInterface::FDSP_MsgHdrTypePtr msgHdr(
            new FDS_ProtocolInterface::FDSP_MsgHdrType());
    FDS_ProtocolInterface::FDSP_DLT_Data_TypePtr dltMsg(
            new FDS_ProtocolInterface::FDSP_DLT_Data_Type());

    placementMutex->lock();
    // find all nodes which need to do migration (=have new tokens)
    rebalanceNodes.clear();
    for (ClusterMap::const_iterator cit = curClusterMap->cbegin();
         cit != curClusterMap->cend();
         ++cit) {
        TokenList new_toks, old_toks;
        newDlt->getTokens(&new_toks, cit->first, 0);
        if (commitedDlt) {
            commitedDlt->getTokens(&old_toks, cit->first, 0);
        }
        // TODO(anna) TokenList should be a set so we can easily
        // search rather than vector -- anyway we shouldn't have duplicate
        // tokens in the TokenList
        for (fds_uint32_t i = 0; i < new_toks.size(); ++i) {
            fds_bool_t found = false;
            for (fds_uint32_t j = 0; j < old_toks.size(); ++j) {
                if (new_toks[i] == old_toks[j]) {
                    found = true;
                    break;
                }
            }
            // at least one new token for this node
            if (!found) {
                rebalanceNodes.insert(cit->first);
                break;
            }
        }
    }

    // pupulate dlt message -- same for all the nodes that need to migrate toks
    dltMsg->dlt_type= true;
    newDlt->getSerialized(dltMsg->dlt_data);

    // send start migration message to all nodes that have new tokens
    for (NodeUuidSet::const_iterator nit = rebalanceNodes.cbegin();
         nit != rebalanceNodes.cend();
         ++nit) {
        NodeUuid  uuid = *nit;
        OM_SmAgent::pointer na = OM_NodeDomainMod::om_local_domain()->om_sm_agent(uuid);
        NodeAgentCpReqClientPtr naClient = na->getCpClient();
        na->set_node_state(FDS_ProtocolInterface::FDS_Start_Migration);
        na->init_msg_hdr(msgHdr);
        msgHdr->msg_code = FDS_ProtocolInterface::FDSP_MSG_NOTIFY_MIGRATION;

        FDS_PLOG_SEV(g_fdslog, fds_log::notification)
                << "Sending the DLT migration request to node 0x"
                << std::hex << uuid.uuid_get_val() << std::dec;

        // invoke the RPC
        naClient->NotifyStartMigration(msgHdr, dltMsg);
    }
    placementMutex->unlock();

    FDS_PLOG_SEV(g_fdslog, fds_log::notification)
            << "Sent DLT migration event to " << rebalanceNodes.size() << " nodes";
    newDlt->dump();
    return err;
}

/**
 * Commits the current DLT as an 'official'
 * copy. The commit stores the DLT to the
 * permanent DLT history.
 */
void
DataPlacement::commitDlt() {
    fds_verify(newDlt != NULL);
    placementMutex->lock();
    if (commitedDlt) {
        delete commitedDlt;
        commitedDlt = NULL;
    }
    commitedDlt = newDlt;
    newDlt = NULL;
    placementMutex->unlock();
}

const DLT*
DataPlacement::getCommitedDlt() const {
    return commitedDlt;
}

fds_uint64_t
DataPlacement::getLatestDltVersion() const {
    if (newDlt) {
        return newDlt->getVersion();
    } else if (commitedDlt) {
        return commitedDlt->getVersion();
    }
    return DLT_VER_INVALID;
}

const ClusterMap*
DataPlacement::getCurClustMap() const {
    return curClusterMap;
}

NodeUuidSet
DataPlacement::getRebalanceNodes() const {
    return rebalanceNodes;
}

int
DataPlacement::mod_init(SysParams const *const param) {
    Module::mod_init(param);

    FdsConfigAccessor conf_helper(g_fdsprocess->get_conf_helper());
    std::string algo_type_str = conf_helper.get<std::string>("placement_algo");
    curDltWidth = conf_helper.get<int>("token_factor");
    curDltDepth = conf_helper.get<int>("replica_factor");

    PlacementAlgorithm::AlgorithmTypes type =
            PlacementAlgorithm::AlgorithmTypes::ConsistHash;
    if (algo_type_str.compare("ConsistHash") == 0) {
        type = PlacementAlgorithm::AlgorithmTypes::ConsistHash;
    } else if (algo_type_str.compare("RoundRobin") == 0) {
        type = PlacementAlgorithm::AlgorithmTypes::RoundRobin;
    } else {
        FDS_PLOG_SEV(g_fdslog, fds_log::warning)
                <<"DataPlacement: unknown placement algorithm type in "
                << "config file, will use Consistent Hashing algorith";
    }

    FDS_PLOG_SEV(g_fdslog, fds_log::notification)
            << "DataPlacement: DLT width " << curDltWidth
            << ", dlt depth " << curDltDepth
            << ", algorithm " << algo_type_str;

    setAlgorithm(type);

    curClusterMap = OM_Module::om_singleton()->om_clusmap_mod();

    return 0;
}

void
DataPlacement::mod_startup() {
}

void
DataPlacement::mod_shutdown() {
}
}  // namespace fds
