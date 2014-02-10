/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <stdlib.h>
#include <string>
#include <OmResources.h>
#include <OmConstants.h>
#include <fds_err.h>

namespace fds {

NodeInventory::NodeInventory(const NodeUuid &uuid)
    : nd_uuid(uuid), nd_mtx("node mtx")
{
    nd_gbyte_cap   = 0;
    nd_ip_addr     = 0;
    nd_data_port   = 0;
    nd_ctrl_port   = 0;
}

NodeInventory::~NodeInventory() {}

void NodeInventory::init_msg_hdr(FDSP_MsgHdrTypePtr msgHdr) const
{
    msgHdr->minor_ver = 0;
    msgHdr->msg_id =  1;

    msgHdr->major_ver = 0xa5;
    msgHdr->minor_ver = 0x5a;

    msgHdr->num_objects = 1;
    msgHdr->frag_len = 0;
    msgHdr->frag_num = 0;

    msgHdr->tennant_id = 0;
    msgHdr->local_domain_id = 0;
    msgHdr->src_node_name = "";

    msgHdr->err_code = FDS_ProtocolInterface::FDSP_ERR_SM_NO_SPACE;
    msgHdr->result = FDS_ProtocolInterface::FDSP_ERR_OK;
}

// node_stor_weight
// ----------------

// node_stor_weight
// ----------------
//
fds_uint32_t
NodeInventory::node_stor_weight() const
{
    return nd_gbyte_cap;
}

void
NodeInventory::node_set_weight(fds_uint64_t weight) {
    nd_gbyte_cap = weight;
}

int
NodeInventory::node_calc_stor_weight()
{
    return 0;
}

void
NodeInventory::node_update_info(const FdspNodeRegPtr msg)
{
    fds_verify(nd_uuid.uuid_get_val() != 0);

    nd_mtx.lock();
    nd_ip_addr          = msg->ip_lo_addr;
    nd_ip_str           = netSession::ipAddr2String(nd_ip_addr);
    nd_data_port        = msg->data_port;
    nd_ctrl_port        = msg->control_port;
    nd_node_name        = msg->node_name;
    nd_node_type        = msg->node_type;
    nd_node_state       = FDS_ProtocolInterface::FDS_Start_Migration;
    nd_capability.disk_iops_max    = msg->disk_info.disk_iops_max;
    nd_capability.disk_iops_min    = msg->disk_info.disk_iops_min;
    nd_capability.disk_latency_max = msg->disk_info.disk_latency_max;
    nd_capability.disk_latency_min = msg->disk_info.disk_latency_min;
    nd_capability.ssd_iops_max     = msg->disk_info.ssd_iops_max;
    nd_capability.ssd_iops_min     = msg->disk_info.ssd_iops_min;
    nd_capability.ssd_capacity     = msg->disk_info.ssd_capacity;
    nd_capability.ssd_latency_max  = msg->disk_info.ssd_latency_max;
    nd_capability.ssd_latency_min  = msg->disk_info.ssd_latency_min;
    nd_disk_type        = msg->disk_info.disk_type,
    nd_mtx.unlock();

    // TODO(vy): fix the weight.
    nd_gbyte_cap = nd_capability.ssd_capacity;
}

NodeAgent::NodeAgent(const NodeUuid &uuid)
    : NodeInventory(uuid)
{
}

NodeAgent::NodeAgent(const NodeUuid &uuid,
                     fds_uint64_t nd_weight)
        : NodeInventory(uuid)
{
    nd_gbyte_cap = nd_weight;
}

NodeAgent::~NodeAgent()
{
}

void
NodeAgent::setCpSession(NodeAgentCpSessionPtr session) {
    ndCpSession = session;

    ndSessionId = ndCpSession->getSessionId();
    ndCpClient = ndCpSession->getClient();
        FDS_PLOG_SEV(g_fdslog, fds_log::error)
                << "Established connection with new node";
}

NodeAgentCpReqClientPtr
NodeAgent::getCpClient() const {
    return ndCpClient;
}

OM_NodeContainer::OM_NodeContainer()
    : RsContainer(), node_inuse(OM_MAX_CONNECTED_NODES), node_mtx("container")
{
    node_cur_idx = 0;
}

OM_NodeContainer::~OM_NodeContainer()
{
}

// om_node_info
// ------------
//
NodeAgent::pointer
OM_NodeContainer::om_node_info(fds_uint32_t node_idx)
{
    if (node_idx < node_cur_idx) {
        return node_inuse[node_idx];
    }
    return NULL;
}

NodeAgent::pointer
OM_NodeContainer::om_node_info(const NodeUuid& uuid)
{
    // Key lookup should be thread-safe.
    if (node_map.count(uuid) > 0)
        return node_map[uuid];

    return NULL;
}

// om_new_node
// -----------
//
NodeAgent::pointer
OM_NodeContainer::om_new_node(const NodeUuid& uuid)
{
    fds_uint32_t  idx;
    NodeAgent    *agent;

    agent = new NodeAgent(uuid);
    node_mtx.lock();
    idx = node_cur_idx;
    if (idx < OM_MAX_CONNECTED_NODES) {
        fds_verify(node_inuse[idx] == NULL);

        node_cur_idx++;
        agent->nd_index = idx;
        node_inuse[idx] = agent;
    } else {
        delete agent;
        agent = NULL;
    }
    node_mtx.unlock();
    return agent;
}

// om_activate_node
// ----------------
//
void
OM_NodeContainer::om_activate_node(fds_uint32_t node_idx)
{
    NodeAgent::pointer agent;

    agent = om_node_info(node_idx);
    fds_verify(agent != NULL);
    fds_verify(agent->nd_uuid.uuid_get_val() != 0);

    node_mtx.lock();
    node_map[agent->nd_uuid] = agent;
    node_up_pend.push_back(agent);
    node_mtx.unlock();

    std::cout << "Actiate node " << std::hex << node_idx << ", uuid "
        << agent->nd_uuid.uuid_get_val() << ", ptr " << agent << std::endl;
}

// om_deactivate_node
// ------------------
//
void
OM_NodeContainer::om_deactivate_node(fds_uint32_t node_idx)
{
    NodeAgent::pointer agent;

    agent = om_node_info(node_idx);
    fds_verify(agent != NULL);
    fds_verify(agent->nd_uuid.uuid_get_val() != 0);

    std::cout << "Deactivate node " << std::hex << node_idx << ", uuid "
        << agent->nd_uuid.uuid_get_val() << ", ptr " << agent << std::endl;

    node_mtx.lock();
    node_map.erase(agent->nd_uuid);
    node_down_pend.push_back(agent);
    node_mtx.unlock();
}

// om_ref_node
// -----------
//
void
OM_NodeContainer::om_ref_node(NodeAgent::pointer node, fds_bool_t act)
{
    node_mtx.lock();
    fds_verify(node_inuse[node->node_index()] == NULL);

    node_inuse[node->node_index()] = node;
    if (act == true) {
        node_map[node->nd_uuid.uuid_get_val()] = node;
    }
    node_mtx.unlock();
}

// om_deref_node
// -------------
//
void
OM_NodeContainer::om_deref_node(NodeAgent::pointer node)
{
    node_mtx.lock();
    fds_verify(node_inuse[node->node_index()] == node);

    node_inuse[node->node_index()] = NULL;
    node_map[node->nd_uuid.uuid_get_val()] = NULL;
    node_mtx.unlock();
}

}  // namespace fds
