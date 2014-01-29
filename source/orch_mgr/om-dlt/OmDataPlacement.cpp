/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <OmDataPlacement.h>

namespace fds {
int
ClusterMap::getNumMembers() const {
    return 0;
}

/**********
 * Functions definitions for data
 * placement
 **********/
DataPlacement::DataPlacement(PlacementAlgorithm::AlgorithmTypes type,
                             fds_uint64_t width,
                             fds_uint64_t depth)
        : Module("Data Placement Engine"),
          placeAlgo(NULL) {
    setAlgorithm(type);
    curDltWidth = width;
    curDltDepth = depth;
}

DataPlacement::~DataPlacement() {
}

void
DataPlacement::setAlgorithm(PlacementAlgorithm::AlgorithmTypes type) {
    placementMutex->lock();
    switch (type) {
        case PlacementAlgorithm::AlgorithmTypes::RoundRobin:
            placeAlgo = boost::shared_ptr<PlacementAlgorithm>(
                new RoundRobinAlgorithm());
            break;
        case PlacementAlgorithm::AlgorithmTypes::ConsistHash:
            placeAlgo = boost::shared_ptr<PlacementAlgorithm>(
                new ConsistHashAlgorithm());
            break;

        default:
            fds_panic("Received unknown data placement algorithm type",
                      type);
    }
    algoType = type;
    placementMutex->unlock();
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
