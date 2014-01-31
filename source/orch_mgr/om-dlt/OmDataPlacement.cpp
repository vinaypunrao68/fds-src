/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <unordered_map>
#include <utility>

#include <OmDataPlacement.h>

namespace fds {
/**********
 * Functions definitions for data
 * placement
 **********/
void
DataPlacement::computeWeightDist(const ClusterMap *cm,
                                 const FdsDlt     *dlt,
                                 WeightMap        *sortedWeights) {
    // Count the weights for each node and the total
    fds_uint32_t totalWeight = 0;
    // Pairs each SMs weight and token count
    std::unordered_map<NodeUuid,
                       std::pair<fds_uint32_t, fds_uint32_t>,
                       UuidHash> smCounts;
    for (ClusterMap::const_iterator it = cm->cbegin();
         it != cm->cend();
         it++) {
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
            fds_panic("Received unknown data placement algorithm type",
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
    FdsDlt *newDlt = new FdsDlt(curDltWidth, curDltDepth);
    placementMutex->lock();
    placeAlgo->computeNewDlt(curClusterMap,
                             curDlt,
                             curDltDepth,
                             curDltWidth,
                             newDlt);
    // TODO(Andrew): We should version the (now) old DLT
    // before we delete it and replace it with the
    // new DLT. We should also update the DLT's
    // internal version.
    delete curDlt;
    curDlt = newDlt;
    placementMutex->unlock();
}

const FdsDlt*
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
