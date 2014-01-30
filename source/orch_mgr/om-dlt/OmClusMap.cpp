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
    : Module("OM Cluster Map") {
    mapMutex = boost::shared_ptr<fds_mutex>(new fds_mutex("cluster map mutex"));
}

ClusterMap::~ClusterMap() {
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

int
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

    // Remove nodes from the map
    for (auto it = rmNodes.cbegin(); it != rmNodes.cend(); it++) {
        uuid = (*it)->get_uuid();
        removed = currClustMap.erase(uuid);
        // For now, assume it's incorrect to try and remove
        // a node that doesn't exist
        fds_verify(removed == 1);
    }

    // Add nodes to the map
    for (auto it = addNodes.cbegin(); it != addNodes.cend(); it++) {
        uuid = (*it)->get_uuid();
        // For now, assume it's incorrect to add a node
        // that already exists
        fds_verify(currClustMap.count(uuid) == 0);

        currClustMap[uuid] = (*it);
    }

    // Increase the version following the update
    version++;
    mapMutex->unlock();
    return err;
}
}  // namespace fds
