/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <unordered_map>
#include <utility>
#include <vector>

#include <OmDataPlacement.h>

namespace fds {
/**********
 * Functions definitions for data
 * placement
 **********/
void
DataPlacement::computeWeightDist(const ClusterMap *cm,
                                 const DLT        *dlt,
                                 WeightMap        *sortedWeights) {
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
    // Make sure we counted every token
    // We're assuming all tokens have equal depth
    fds_verify(tokenCount == (dlt->getNumTokens() *
                              dlt->getDepth()));

    // Iterate the map, compute the ratios based on the
    // pairs, and store the result in the weight map
    for (std::unordered_map<NodeUuid,
                       std::pair<double, double>,
                            UuidHash>
            ::const_iterator it = nodeCounts.cbegin();
         it != nodeCounts.cend();
         it++) {
        NodeUuid uuid = (*it).first;
        double weightRatio = ((*it).second).first / totalWeight;
        double tokenRatio = ((*it).second).second / tokenCount;
        LoadRatio lr = weightRatio / tokenRatio;

        if (sortedWeights->count(lr) == 0) {
            // Create a new list for this ratio
            std::vector<NodeUuid> uuidList;
            uuidList.push_back(uuid);
            (*sortedWeights)[lr] = uuidList;
        } else {
            // Append to the list for this ratio
            ((*sortedWeights)[lr]).push_back(uuid);
        }
    }
}

DataPlacement::DataPlacement(PlacementAlgorithm::AlgorithmTypes type,
                             fds_uint64_t width,
                             fds_uint64_t depth)
        : Module("Data Placement Engine"),
          placeAlgo(NULL),
          curDlt(NULL) {
    placementMutex = new fds_mutex("data placement mutex");

    setAlgorithm(type);
    curDltWidth = width;
    curDltDepth = depth;

    curClusterMap = new ClusterMap();
}

DataPlacement::~DataPlacement() {
    delete placementMutex;
    delete curClusterMap;
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
                             curWeightDist,
                             newDlt);

    // Compute DLT's reverse node to token map
    newDlt->generateNodeTokenMap();

    // TODO(Andrew): Compute the DLT's weight distribution
    WeightMap *newWeightMap = new WeightMap();
    computeWeightDist(curClusterMap,
                      newDlt,
                      newWeightMap);

    // TODO(Andrew): Remove this. Just printing for easy debugging.
    for (WeightMap::const_iterator it = newWeightMap->cbegin();
         it != newWeightMap->cend();
         it++) {
        const std::vector<NodeUuid> &uuidList = (*it).second;
        for (std::vector<NodeUuid>::const_iterator jt = uuidList.cbegin();
             jt != uuidList.cend();
             jt++) {
            std::cout << "Node 0x" << std::hex << (*jt).uuid_get_val()
                      << " has load ratio " << std::dec
                      << ((*it).first) << std::endl;
        }
    }

    // TODO(Andrew): We should version the (now) old DLT
    // before we delete it and replace it with the
    // new DLT. We should also update the DLT's
    // internal version.
    delete curDlt;
    curDlt = newDlt;
    curWeightDist = newWeightMap;
    placementMutex->unlock();
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
}

void
DataPlacement::mod_startup() {
}

void
DataPlacement::mod_shutdown() {
}
}  // namespace fds
