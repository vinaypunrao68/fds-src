/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <OmDataPlacement.h>

namespace fds {
/**********
 * Functions definitions for data
 * placement
 **********/
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
