/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <string>
#include <stdlib.h>
#include <platform/node-inventory.h>

namespace fds {

// --------------------------------------------------------------------------------------
// Node Inventory
// --------------------------------------------------------------------------------------
void
NodeInventory::node_set_inventory(NodeInvData const *const inv)
{
    node_inv = inv;
    strncpy(rs_name, inv->nd_node_name.c_str(), RS_NAME_MAX - 1);
}

// node_fill_inventory
// -------------------
//
void
NodeInventory::node_fill_inventory(const FdspNodeRegPtr msg)
{
    NodeInvData       *data;
    node_capability_t *ncap;

    data = new NodeInvData();
    ncap = &data->nd_capability;

    data->nd_ip_addr        = msg->ip_lo_addr;
    data->nd_ip_str         = netSession::ipAddr2String(data->nd_ip_addr);
    data->nd_data_port      = msg->data_port;
    data->nd_ctrl_port      = msg->control_port;
    data->nd_migration_port = msg->migration_port;
    data->nd_node_name      = msg->node_name;
    data->nd_node_type      = msg->node_type;
    data->nd_node_state     = FDS_ProtocolInterface::FDS_Node_Up;
    data->nd_disk_type      = msg->disk_info.disk_type;

    ncap->disk_iops_max     = msg->disk_info.disk_iops_max;
    ncap->disk_iops_min     = msg->disk_info.disk_iops_min;
    ncap->disk_latency_max  = msg->disk_info.disk_latency_max;
    ncap->disk_latency_min  = msg->disk_info.disk_latency_min;
    ncap->ssd_iops_max      = msg->disk_info.ssd_iops_max;
    ncap->ssd_iops_min      = msg->disk_info.ssd_iops_min;
    ncap->ssd_capacity      = msg->disk_info.ssd_capacity;
    ncap->ssd_latency_max   = msg->disk_info.ssd_latency_max;
    ncap->ssd_latency_min   = msg->disk_info.ssd_latency_min;
    strncpy(rs_name, data->nd_ip_str.c_str(), RS_NAME_MAX - 1);

    // TODO(vy): fix the weight.
    data->nd_gbyte_cap = ncap->ssd_capacity;
    node_inv = data;
}

// node_update_inventory
// ---------------------
//
void
NodeInventory::node_update_inventory(const FdspNodeRegPtr msg)
{
}

// init_msg_hdr
// ------------
//
void
NodeInventory::init_msg_hdr(FDSP_MsgHdrTypePtr msgHdr) const
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

// init_hdr_as_dest
// ----------------
//
void
NodeInventory::init_node_info_pkt(fpi::FDSP_Node_Info_TypePtr pkt) const
{
    pkt->node_id        = 0;
    pkt->node_uuid      = rs_uuid.uuid_get_val();
    pkt->node_type      = node_inv->nd_node_type;
    pkt->node_name      = node_inv->nd_node_name;
    pkt->node_state     = node_inv->nd_node_state;
    pkt->control_port   = node_inv->nd_ctrl_port;
    pkt->data_port      = node_inv->nd_data_port;
    pkt->migration_port = node_inv->nd_migration_port;
}

// set_node_state
// --------------
//
void
NodeInventory::set_node_state(FdspNodeState state)
{
    // TODO(Vy): do this in platform side.
    const_cast<NodeInvData *>(node_inv)->nd_node_state = state;
}

// --------------------------------------------------------------------------------------
// Node Agents
// --------------------------------------------------------------------------------------

// node_stor_weight
// ----------------
//
fds_uint32_t
NodeAgent::node_stor_weight() const
{
    return node_inv->nd_gbyte_cap;
}

// node_set_weight
// ---------------
//
void
NodeAgent::node_set_weight(fds_uint64_t weight)
{
    const_cast<NodeInvData *>(node_inv)->nd_gbyte_cap = weight;
}

AgentContainer::AgentContainer(FdspNodeType id) : RsContainer()
{
    ac_cpSessTbl = boost::shared_ptr<netSessionTbl>(new netSessionTbl(id));
}

AgentContainer::~AgentContainer()
{
    ac_cpSessTbl->endAllSessions();
}

// agent_register
// --------------
//
Error
AgentContainer::agent_register(const NodeUuid       &uuid,
                               const FdspNodeRegPtr  msg,
                               NodeAgent::pointer   *out)
{
    fds_bool_t         add;
    std::string        name;
    NodeAgent::pointer agent;

    add   = false;
    *out  = NULL;
    name  = msg->node_name;
    agent = NodeAgent::agt_cast_ptr(agent_info(uuid));
    if (agent == NULL) {
        add   = true;
        agent = NodeAgent::agt_cast_ptr(rs_alloc_new(uuid));
        agent->node_fill_inventory(msg);

        // TODO(vy): share the inventory here with shared mem.
    } else {
        if (name.compare(agent->get_node_name()) != 0) {
            // TODO(vy): we should assert here because the uuid function isn't good.
            return Error(ERR_DUPLICATE_UUID);
        }
        agent->node_update_inventory(msg);
    }
    *out = agent;
    if (add == true) {
        agent_activate(agent);
    }
    return Error(ERR_OK);
}

// agent_unregister
// ----------------
//
Error
AgentContainer::agent_unregister(const NodeUuid &uuid, const std::string &name)
{
    NodeAgent::pointer  agent;

    agent = agent_info(uuid);
    if ((agent == NULL) ||
        (name.compare(agent->get_node_name()) != 0)) {
        return Error(ERR_NOT_FOUND);
    }
    agent_deactivate(agent);
    return Error(ERR_OK);
}

// --------------------------------------------------------------------------------------
// Node Container
// --------------------------------------------------------------------------------------

DomainContainer::~DomainContainer() {}
DomainContainer::DomainContainer(char const *const name)
    : dc_om_master(NULL), dc_om_nodes(NULL),
      dc_sm_nodes(NULL), dc_dm_nodes(NULL), dc_am_nodes(NULL), dc_nodes(NULL) {}

DomainContainer::DomainContainer(char const *const       name,
                                 OmAgent::pointer        master,
                                 SmContainer::pointer    sm,
                                 DmContainer::pointer    dm,
                                 AmContainer::pointer    am,
                                 OmContainer::pointer    om,
                                 AgentContainer::pointer node)
    : dc_om_master(master), dc_om_nodes(om),
      dc_sm_nodes(sm), dc_dm_nodes(dm), dc_am_nodes(am), dc_nodes(node) {}

// dc_container_frm_msg
// --------------------
// Get the right container based on message type.
//
AgentContainer::pointer
DomainContainer::dc_container_frm_msg(FdspNodeType node_type)
{
    AgentContainer::pointer nodes;

    switch (node_type) {
    case FDS_ProtocolInterface::FDSP_STOR_MGR:
        nodes = dc_sm_nodes;
        break;

    case FDS_ProtocolInterface::FDSP_DATA_MGR:
        nodes = dc_dm_nodes;
        break;

    case FDS_ProtocolInterface::FDSP_STOR_HVISOR:
        nodes = dc_am_nodes;
        break;

    default:
        nodes = dc_om_nodes;
        break;
    }
    fds_verify(nodes != NULL);
    return nodes;
}

// dc_register_node
// ----------------
//
Error
DomainContainer::dc_register_node(const NodeUuid       &uuid,
                                  const FdspNodeRegPtr  msg,
                                  NodeAgent::pointer   *agent)
{
    AgentContainer::pointer nodes;

    nodes = dc_container_frm_msg(msg->node_type);
    return nodes->agent_register(uuid, msg, agent);
}

// dc_unregister_node
// ------------------
//
Error
DomainContainer::dc_unregister_node(const NodeUuid &uuid, const std::string &name)
{
    AgentContainer::pointer nodes;
    NodeAgent::pointer      agent;
    FdspNodeType            svc[] = {
        FDS_ProtocolInterface::FDSP_STOR_MGR,
        FDS_ProtocolInterface::FDSP_DATA_MGR,
        FDS_ProtocolInterface::FDSP_STOR_HVISOR,
        FDS_ProtocolInterface::FDSP_ORCH_MGR
    };

    for (int i = 0; svc[i] != FDS_ProtocolInterface::FDSP_ORCH_MGR; i++) {
        nodes = dc_container_frm_msg(svc[i]);
        if (nodes == NULL) {
            continue;
        }
        nodes->agent_unregister(uuid, name);
    }
    return Error(ERR_OK);
}

// dc_unregister_agent
// -------------------
//
Error
DomainContainer::dc_unregister_agent(const NodeUuid &uuid, FdspNodeType type)
{
    AgentContainer::pointer nodes;
    NodeAgent::pointer      agent;

    nodes = dc_container_frm_msg(type);
    if (nodes != NULL) {
        agent = nodes->agent_info(uuid);
        if (agent != NULL) {
            nodes->agent_deactivate(agent);
            return Error(ERR_OK);
        }
    }
    return Error(ERR_NOT_FOUND);
}

}  // namespace fds
