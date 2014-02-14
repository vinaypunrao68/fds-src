/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <unordered_map>
#include <set>
#include <ClusterCommMgr.h>

namespace fds {
ClusterCommMgr::ClusterCommMgr(OMgrClient *omClient)
{
    omClient_ = omClient;
}

NodeTokenTbl ClusterCommMgr::
partition_tokens_by_node(const std::set<fds_token_id> &tokens)
{
    NodeTokenTbl tbl;
    const DLT* dlt = omClient_->getCurrentDLT();
    for (auto itr : tokens) {
        NodeUuid node_id = dlt->getPrimary(itr);
        tbl[node_id].insert(itr);
    }
    return tbl;
}

bool ClusterCommMgr::get_node_ip_port(const NodeUuid &node_id,
        uint32_t &ip, uint32_t &port)
{
    int state;
    omClient_->getNodeInfo(node_id.uuid_get_val(), &ip, &port, &state);
    return true;
}

}  // namespace fds
