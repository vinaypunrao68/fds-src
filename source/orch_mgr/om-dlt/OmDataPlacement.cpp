/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <unordered_map>
#include <map>
#include <utility>
#include <vector>

#include <orch-mgr/om-service.h>
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
    for (it = weight_map.cbegin();
         it != weight_map.cend();
         it++) {
        const std::vector<NodeUuid> &uuidList = (*it).second;
        for (std::vector<NodeUuid>::const_iterator jt = uuidList.cbegin();
             jt != uuidList.cend();
             jt++) {
            FDS_PLOG_SEV(log, fds_log::debug)
                    << "Node 0x" << std::hex << (*jt).uuid_get_val()
                    << " has load ratio " << std::dec
                    << ((*it).first) << std::endl;
        }
    }
}

/**********
 * Functions definitions for data
 * placement
 **********/
DataPlacement::DataPlacement(PlacementAlgorithm::AlgorithmTypes type,
                             fds_uint64_t width,
                             fds_uint64_t depth)
        : Module("Data Placement Engine"),
          placeAlgo(NULL),
          curDlt(NULL),
          curWeightDist(NULL) {
    placementMutex = new fds_mutex("data placement mutex");

    setAlgorithm(type);
    curDltWidth = width;
    curDltDepth = depth;

    // curClusterMap = new ClusterMap();
}

DataPlacement::~DataPlacement() {
    delete placementMutex;
    // delete curClusterMap;
    if (curDlt != NULL) {
        delete curDlt;
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
    if (curDlt == NULL) {
        version = 0;
    } else {
        version = curDlt->getVersion();
    }
    // If we have fewer members than total replicas
    // use the number of members as the replica count
    fds_uint32_t depth = curDltDepth;
    if (curClusterMap->getNumMembers() < curDltDepth) {
        depth = curClusterMap->getNumMembers();
    }

    // Allocate and compute new DLT
    DLT *newDlt = new DLT(curDltWidth,
                          depth,
                          version,
                          true);
    placementMutex->lock();
    placeAlgo->computeNewDlt(curClusterMap,
                             curDlt,
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
    delete curDlt;
    curDlt = newDlt;
    placementMutex->unlock();
}

/**
 * Commits the current DLT as an 'official'
 * copy. The commit stores the DLT to the
 * permanent DLT history and async notifies
 * others nodes in the cluster about the
 * new version.
 */
Error
DataPlacement::commitDlt() {
    Error err(ERR_OK);

    placementMutex->lock();

    // Commit the current DLT to the
    // official DLT history

    // Async notify other nodes of the new
    // DLT
    for (ClusterMap::const_iterator it = curClusterMap->cbegin();
         it != curClusterMap->cend();
         it++) {
        NodeAgent::pointer na = it->second;
    }

    placementMutex->unlock();

    return err;
}

const DLT*
DataPlacement::getCurDlt() const {
    return curDlt;
}

const ClusterMap*
DataPlacement::getCurClustMap() const {
    return curClusterMap;
}

int
DataPlacement::mod_init(SysParams const *const param) {
    Module::mod_init(param);
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
