/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <unordered_map>
#include <set>
#include <ClusterCommMgr.h>

namespace fds {
ClusterCommMgr::ClusterCommMgr(OMgrClient *om_client)
    : ClusterCommMgr(om_client, nullptr)
{
}

ClusterCommMgr::ClusterCommMgr(OMgrClient *om_client, netSessionTblPtr nst)
{
    om_client_ = om_client;
    nst_ = nst;
}
/**
 * Partitions the tokens by primary sm that contains the tokens
 * @param tokens
 * @return
 */
NodeTokenTbl ClusterCommMgr::
partition_tokens_by_node(const std::set<fds_token_id> &tokens)
{
    NodeTokenTbl tbl;
    // TODO(Rao): We should use the current DLT here.
    const DLT* dlt = om_client_->getPreviousDLT();
    for (auto itr : tokens) {
        NodeUuid node_id = dlt->getPrimary(itr);
        tbl[node_id].insert(itr);
    }
    return tbl;
}
/**
 * TODO(Rao):  We are getting node info for a token from previous dlt.  The assumption
 * is when this call invoked we are currently undergoing migration.
 * Returns migration ip and port for node identified by node_id
 * @param token_id
 * @param ip
 * @param port
 * @return
 */
bool ClusterCommMgr::get_node_mig_ip_port(const fds_token_id& token_id,
        uint32_t &ip, uint32_t &port)
{
    int state;
    const DLT* dlt = om_client_->getPreviousDLT();
    NodeUuid node_id = dlt->getPrimary(token_id);
    int result = om_client_->getNodeInfo(node_id.uuid_get_val(), &ip, &port, &state);
    fds_verify(result == 0);
    port = om_client_->getNodeMigPort(node_id);
    return true;
}

/**
 *
 * @param node_id
 * @return
 */
template <class Type>
boost::shared_ptr<Type> ClusterCommMgr::get_client_session(const NodeUuid &node_id)
{
//    int state;
//    int result = om_client_->getNodeInfo(node_id.uuid_get_val(), &ip, &port, &state);
//    nst_->
    return nullptr;
}

/**
 * Returns the current dlt
 * @return
 */
const DLT* ClusterCommMgr::get_dlt()
{
    return om_client_->getCurrentDLT();
}

OMgrClient* ClusterCommMgr::get_om_client()
{
    return om_client_;
}

netSessionTblPtr ClusterCommMgr::get_nst()
{
    return nst_;
}
}  // namespace fds
