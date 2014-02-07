/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <stdlib.h>
#include <OmResources.h>
#include <OmConstants.h>
#include <fds_err.h>

namespace fds {

NodeInventory::NodeInventory(const NodeUuid &uuid) : Resource(uuid)
{
    nd_gbyte_cap   = 0;
    nd_ip_addr     = 0;
    nd_data_port   = 0;
    nd_ctrl_port   = 0;
}

NodeInventory::~NodeInventory() {}

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
NodeInventory::node_update_info(const NodeUuid *uuid, const FdspNodeRegPtr msg)
{
    if (uuid != NULL) {
        rs_uuid = *uuid;
    } else {
        // FIXME: (vy) change this to real uuid.
        fds_verify(rs_uuid.uuid_get_val() == 0);
        rs_uuid.uuid_set_val(random());
    }
    rs_mtx.lock();
    nd_ip_addr          = msg->ip_lo_addr;
    nd_ip_str           = netSession::ipAddr2String(nd_ip_addr);
    nd_data_port        = msg->data_port;
    nd_ctrl_port        = msg->control_port;
    nd_node_name        = msg->node_name;
    nd_node_type        = msg->node_type;
    nd_node_state       = FDS_ProtocolInterface::FDS_Node_Up;
    nd_disk_iops_max    = msg->disk_info.disk_iops_max;
    nd_disk_iops_min    = msg->disk_info.disk_iops_min;
    nd_disk_latency_max = msg->disk_info.disk_latency_max;
    nd_disk_latency_min = msg->disk_info.disk_latency_min;
    nd_ssd_iops_max     = msg->disk_info.ssd_iops_max;
    nd_ssd_iops_min     = msg->disk_info.ssd_iops_min;
    nd_ssd_capacity     = msg->disk_info.ssd_capacity;
    nd_ssd_latency_max  = msg->disk_info.ssd_latency_max;
    nd_ssd_latency_min  = msg->disk_info.ssd_latency_min;
    nd_disk_type        = msg->disk_info.disk_type;
    strncpy(rs_name, nd_ip_str.c_str(), RS_NAME_MAX - 1);
    rs_mtx.unlock();

    // TODO(vy): fix the weight.
    nd_gbyte_cap = nd_ssd_capacity;
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
NodeAgent::getCpClient() {
    return ndCpClient;
}

// ---------------------------------------------------------------------------------
// OM Node Container
// ---------------------------------------------------------------------------------
OM_NodeContainer::OM_NodeContainer() : RsContainer() {}
OM_NodeContainer::~OM_NodeContainer() {}

// rs_new
// ------
//
Resource *
OM_NodeContainer::rs_new()
{
    return new NodeAgent(NodeUuid(0));
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

    rs_mtx.lock();
    rs_register_mtx(agent);
    node_up_pend.push_back(agent);
    rs_mtx.unlock();

    std::cout << "Actiate node " << std::hex << node_idx << ", uuid "
        << agent->rs_uuid.uuid_get_val() << ", ptr " << agent << std::endl;
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

    std::cout << "Deactivate node " << std::hex << node_idx << ", uuid "
        << agent->rs_uuid.uuid_get_val() << ", ptr " << agent << std::endl;

    rs_mtx.lock();
    rs_unregister_mtx(agent);
    node_down_pend.push_back(agent);
    rs_mtx.unlock();
}

}  // namespace fds
