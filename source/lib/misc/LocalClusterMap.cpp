/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <fds_process.h>
#include <fds_resource.h>
#include <LocalClusterMap.h>

namespace fds {

LocalClusterMap::LocalClusterMap()
        : Module("local cluster map") {
}

LocalClusterMap::~LocalClusterMap() {
}

int
LocalClusterMap::mod_init(SysParams const *const param) {
    return 0;
}

void
LocalClusterMap::mod_startup() {
}

void
LocalClusterMap::mod_shutdown() {
}

int
LocalClusterMap::getNodeInfo(fds_uint64_t nodeUuid,
                             fds_uint32_t *nodeIpAddr,
                             fds_uint32_t *nodePort,
                             int *nodeState) {
    lcmLock.write_lock();
    if (clusterMembers.count(nodeUuid) == 0) {
        // Return error if the node isn't in the cluster
        lcmLock.write_unlock();
        return -1;
    }

    node_info_t node = clusterMembers[nodeUuid];
    fds_verify(node.node_id == nodeUuid);

    *nodeIpAddr = node.node_ip_address;
    *nodePort   = node.port;
    *nodeState  = node.node_state;
    lcmLock.write_unlock();

    return 0;
}

NodeMigReqClientPtr
LocalClusterMap::getMigClient(fds_uint64_t node_id) {
    lcmLock.write_lock();
    NodeMigReqClientPtr client = NULL;
    if (clusterMembers.count(node_id) > 0) {
        client = clusterMembers[node_id].ndMigClient;
    }
    lcmLock.write_unlock();

    return client;
}

fds_uint32_t
LocalClusterMap::getNodeMigPort(NodeUuid uuid) {
    fds_uint32_t port;
    lcmLock.read_lock();
    fds_verify(clusterMembers.count(uuid.uuid_get_val()) != 0);
    port = clusterMembers[uuid.uuid_get_val()].mig_port;
    lcmLock.read_unlock();

    return port;
}

fds_uint32_t
LocalClusterMap::getNodeMetaSyncPort(NodeUuid uuid) {
    fds_uint32_t port;
    lcmLock.read_lock();
    fds_verify(clusterMembers.count(uuid.uuid_get_val()) != 0);
    port = clusterMembers[uuid.uuid_get_val()].meta_sync_port;
    lcmLock.read_unlock();

    return port;
}


Error
LocalClusterMap::addNode(node_info_t *node,
                         fpi::FDSP_MgrIdType myRole,
                         fpi::FDSP_MgrIdType nodeRole) {
    Error err(ERR_OK);

    FDS_PLOG_SEV(g_fdslog, fds_log::debug)
            << "Adding new node " << node->node_id
            << " to local cluster map";

    lcmLock.write_lock();
    if (clusterMembers.count(node->node_id) != 0) {
        err = ERR_DUPLICATE;
        lcmLock.write_unlock();
        return err;
    }

    // Create a migration endpoint to sm nodes
    /*
    if ((myRole == FDS_ProtocolInterface::FDSP_STOR_MGR) &&
        (nodeRole == FDS_ProtocolInterface::FDSP_STOR_MGR)) {
        node->ndMigSession = static_cast<NodeMigSessionPtr>(
            lcmSessTbl->startSession<netMigrationPathClientSession>(
                node->node_ip_address,
                node->mig_port,
                FDSP_MIGRATION_MGR,
                1,
                migRspHndlr));
        // TODO(Andrew): Handle scenarios where we can't establish
        // connection with the existing node instead of ignoring them
        if (node->ndMigSession != NULL) {
            node->ndMigSessionId = node->ndMigSession->getSessionId();
            node->ndMigClient = node->ndMigSession->getClient();
            FDS_PLOG_SEV(g_fdslog, fds_log::debug)
                    << "Create new endpoint to node " << node->node_id;
        }
    }
    */

    clusterMembers[node->node_id] = (*node);
    lcmLock.write_unlock();

    FDS_PLOG_SEV(g_fdslog, fds_log::debug)
            << "Added new node " << node->node_id
            << " to local cluster map";

    return err;
}

}  // namespace fds
