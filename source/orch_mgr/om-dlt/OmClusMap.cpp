/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <list>
#include <iostream>

#include <OmResources.h>
#include <OmDataPlacement.h>

namespace fds {

ClusterMap gl_OMClusMapMod;

ClusterMap::ClusterMap()
        : Module("OM Cluster Map"),
          version(0) {
    mapMutex = new fds_mutex("cluster map mutex");
}

ClusterMap::~ClusterMap() {
    delete mapMutex;
}

ClusterMap::const_iterator
ClusterMap::cbegin() const {
    return currClustMap.cbegin();
}

ClusterMap::const_iterator
ClusterMap::cend() const {
    return currClustMap.cend();
}

int
ClusterMap::mod_init(SysParams const *const param) {
    Module::mod_init(param);
    std::cout << "ClusterMap init is called " << std::endl;
    return 0;
}

void
ClusterMap::mod_startup() {
}

void
ClusterMap::mod_shutdown() {
}

fds_uint32_t
ClusterMap::getNumMembers() const {
    return currClustMap.size();
}

Error
ClusterMap::updateMap(const std::list<NodeAgent::pointer> &addNodes,
                      const std::list<NodeAgent::pointer> &rmNodes) {
    Error    err(ERR_OK);
    NodeUuid uuid;
    fds_uint32_t removed;

    mapMutex->lock();

    addedNodes.clear();
    removedNodes.clear();

    // Remove nodes from the map
    for (auto it = rmNodes.cbegin(); it != rmNodes.cend(); it++) {
        uuid = (*it)->get_uuid();
        removed = currClustMap.erase(uuid);
        // For now, assume it's incorrect to try and remove
        // a node that doesn't exist
        fds_verify(removed == 1);
        removedNodes.push_back(uuid);
    }

    // Add nodes to the map
    for (auto it = addNodes.cbegin(); it != addNodes.cend(); it++) {
        uuid = (*it)->get_uuid();
        // For now, assume it's incorrect to add a node
        // that already exists
        fds_verify(currClustMap.count(uuid) == 0);

        currClustMap[uuid] = (*it);
        addedNodes.push_back(uuid);
    }

    // Increase the version following the update
    version++;
    mapMutex->unlock();
    return err;
}

std::list<NodeUuid>
ClusterMap::getAddedNodes() const {
    /*
     * TODO: We should ensure that we're not
     * in the process of updating the cluster map
     * as this list may be in the process of being
     * modifed.
     */
    return addedNodes;
}

std::list<NodeUuid>
ClusterMap::getRemovedNodes() const {
    /*
     * TODO: We should ensure that we're not
     * in the process of updating the cluster map
     * as this list may be in the process of being
     * modifed.
     */
    return removedNodes;
}
}  // namespace fds
