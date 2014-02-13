/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#include <LocalClusterMap.h>

namespace fds {

LocalClusterMap::LocalClusterMap(boost::shared_ptr<FDS_ProtocolInterface::
                                                   FDSP_MigrationPathRespIf> migHndlr)
        : Module("local cluster map"),
          migRspHndlr(migHndlr) {
    // TODO(Andrew): This should be a generic platform/node interface
    // not one for each possible client OR the map should have one
    // role (e.g., DM, SM, etc...) and use that. We're hard coding to
    // the migration table for now.
    lcmSessTbl = boost::shared_ptr<netSessionTbl>(
        new netSessionTbl(FDSP_MIGRATION_MGR));
}

LocalClusterMap::~LocalClusterMap() {
    lcmSessTbl->endAllSessions();
}

int
LocalClusterMap::mod_init(SysParams const *const param) {
}

void
LocalClusterMap::mod_startup() {
}

void
LocalClusterMap::mod_shutdown() {
}

NodeUuid
LocalClusterMap::getNodeInfo() const {
    NodeUuid tmp;
    return tmp;
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

Error
LocalClusterMap::addNode(node_info_t *node,
                         FDSP_MgrIdType myRole,
                         FDSP_MgrIdType nodeRole) {
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

    clusterMembers[node->node_id] = (*node);
    lcmLock.write_unlock();

    FDS_PLOG_SEV(g_fdslog, fds_log::debug)
            << "Added new node " << node->node_id
            << " to local cluster map";

    return err;
}

}  // namespace fds
