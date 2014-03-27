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
    FDS_PLOG_SEV(g_fdslog, fds_log::notification) << "ClusterMap init is called ";
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

fds_uint64_t
ClusterMap::getTotalStorWeight() const {
    fds_uint64_t total_weight = 0;
    for (const_iterator it = cbegin();
         it != cend();
         ++it) {
        total_weight += ((*it).second)->node_stor_weight();
    }
    return total_weight;
}

void
ClusterMap::resetPendNodes() {
    mapMutex->lock();
    addedNodes.clear();
    removedNodes.clear();
    mapMutex->unlock();
}

Error
ClusterMap::updateMap(const NodeList &addNodes,
                      const NodeList &rmNodes) {
    Error    err(ERR_OK);
    NodeUuid uuid;
    fds_uint32_t removed;

    mapMutex->lock();

    // Remove nodes from the map
    for (NodeList::const_iterator it = rmNodes.cbegin();
         it != rmNodes.cend();
         it++) {
        uuid = (*it)->get_uuid();
        removed = currClustMap.erase(uuid);
        // For now, assume it's incorrect to try and remove
        // a node that doesn't exist
        fds_verify(removed == 1);
        removedNodes.insert(uuid);
    }

    // Add nodes to the map
    for (NodeList::const_iterator it = addNodes.cbegin();
         it != addNodes.cend();
         it++) {
        uuid = (*it)->get_uuid();
        // For now, assume it's incorrect to add a node
        // that already exists
        fds_verify(currClustMap.count(uuid) == 0);

        currClustMap[uuid] = (*it);
        addedNodes.insert(uuid);
    }

    // Increase the version following the update
    version++;
    mapMutex->unlock();
    return err;
}

//
// Only add to pending removed nodes. Node should not
// exist in cluster map at this point. This method is used on OM
// bringup from persistent state when not all nodes came up and
// we want to remove those nodes from persisted DLT
//
void
ClusterMap::addPendingRmNode(const NodeUuid& rm_uuid)
{
    mapMutex->lock();
    fds_verify(currClustMap.count(rm_uuid) == 0);
    removedNodes.insert(rm_uuid);
    mapMutex->unlock();
}

std::unordered_set<NodeUuid, UuidHash>
ClusterMap::getAddedNodes() const {
    /*
     * TODO: We should ensure that we're not
     * in the process of updating the cluster map
     * as this list may be in the process of being
     * modifed.
     */
    return addedNodes;
}

std::unordered_set<NodeUuid, UuidHash>
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
