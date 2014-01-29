/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <list>

#include <OmDataPlacement.h>

namespace fds {
/**********
 * Functions definitions for cluster
 * map
 **********/
ClusterMap::ClusterMap() : Module("cluster map") {
}

ClusterMap::~ClusterMap() {
}

int
ClusterMap::getNumMembers() const {
    return currClustMap.size();
}

Error
ClusterMap::updateMap(std::list<boost::shared_ptr<NodeAgent>> addNodes,
                      std::list<boost::shared_ptr<NodeAgent>> rmNodes) {
    Error    err(ERR_OK);
    NodeUuid uuid;
    fds_uint32_t removed;

    mapMutex->lock();

    /*
     * Remove nodes from the map
     */
    for (std::list<boost::shared_ptr<NodeAgent>>::const_iterator it = rmNodes.cbegin();
         it != rmNodes.cend();
         it++) {
        uuid = (*it)->get_uuid();
        removed = currClustMap.erase(uuid);
        // For now, assume it's incorrect to try and remove
        // a node that doesn't exist
        fds_verify(removed == 1);
    }

    mapMutex->unlock();
    return err;
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
