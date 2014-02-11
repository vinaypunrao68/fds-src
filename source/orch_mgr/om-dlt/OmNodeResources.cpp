/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <stdlib.h>
#include <string>
#include <OmResources.h>
#include <OmConstants.h>
#include <fds_err.h>

namespace fds {

// ---------------------------------------------------------------------------------
// OM SM NodeAgent
// ---------------------------------------------------------------------------------
OM_SmAgent::~OM_SmAgent() {}
OM_SmAgent::OM_SmAgent(const NodeUuid &uuid) : NodeAgent(uuid) {}

int
OM_SmAgent::node_calc_stor_weight()
{
    return 0;
}

// setCpSession
// ------------
//
void
OM_SmAgent::setCpSession(NodeAgentCpSessionPtr session)
{
    ndCpSession = session;
    ndSessionId = ndCpSession->getSessionId();
    ndCpClient  = ndCpSession->getClient();

    FDS_PLOG_SEV(g_fdslog, fds_log::normal) << "Established connection with new node";
}

NodeAgentCpReqClientPtr
OM_SmAgent::getCpClient() const
{
    return ndCpClient;
}

// om_send_myinfo
// --------------
// Send a node event taken from the new node agent to a peer agent.
//
void
OM_SmAgent::om_send_myinfo(NodeAgent::pointer peer)
{
    if (peer->get_node_name() == get_node_name()) {
        return;
    }
    fpi::FDSP_MsgHdrTypePtr     m_hdr(new fpi::FDSP_MsgHdrType);
    fpi::FDSP_Node_Info_TypePtr n_inf(new fpi::FDSP_Node_Info_Type);

    this->init_msg_hdr(m_hdr);
    this->init_node_info_pkt(n_inf);

    m_hdr->msg_code        = fpi::FDSP_MSG_NOTIFY_NODE_ADD;
    m_hdr->msg_id          = 0;
    m_hdr->tennant_id      = 1;
    m_hdr->local_domain_id = 1;
    OM_SmAgent::agt_cast_ptr(peer)->ndCpClient->NotifyNodeAdd(m_hdr, n_inf);

    FDS_PLOG_SEV(g_fdslog, fds_log::normal)
        << "Send node info from " << get_node_name()
        << " to " << peer->get_node_name() << std::endl;
}

void
OM_SmAgent::init_msg_hdr(FDSP_MsgHdrTypePtr msgHdr) const
{
    msgHdr->minor_ver = 0;
    msgHdr->msg_id =  1;

    msgHdr->major_ver = 0xa5;
    msgHdr->minor_ver = 0x5a;

    msgHdr->src_id = FDS_ProtocolInterface::FDSP_ORCH_MGR;
    msgHdr->dst_id = FDS_ProtocolInterface::FDSP_STOR_MGR;

    msgHdr->num_objects = 1;
    msgHdr->frag_len = 0;
    msgHdr->frag_num = 0;

    msgHdr->tennant_id = 0;
    msgHdr->local_domain_id = 0;
    msgHdr->src_node_name = "";

    msgHdr->session_uuid = ndSessionId;

    msgHdr->err_code = FDS_ProtocolInterface::FDSP_ERR_SM_NO_SPACE;
    msgHdr->result = FDS_ProtocolInterface::FDSP_ERR_OK;
}

// ---------------------------------------------------------------------------------
// OM SM NodeAgent Container
// ---------------------------------------------------------------------------------
OM_SmContainer::OM_SmContainer() : SmContainer(FDSP_ORCH_MGR)
{
    ctrlRspHndlr = boost::shared_ptr<OM_ControlRespHandler>(new OM_ControlRespHandler());
}

OM_SmContainer::~OM_SmContainer()
{
}

// agent_activate
// --------------
//
void
OM_SmContainer::agent_activate(NodeAgent::pointer agent)
{
    FDS_PLOG_SEV(g_fdslog, fds_log::normal)
            << "Activate node uuid " << std::hex
            << "0x" << agent->get_uuid().uuid_get_val() << std::dec;

    rs_mtx.lock();
    rs_register_mtx(agent);
    node_up_pend.push_back(OM_SmAgent::agt_cast_ptr(agent));
    rs_mtx.unlock();
}

// agent_deactivate
// ----------------
//
void
OM_SmContainer::agent_deactivate(NodeAgent::pointer agent)
{
    FDS_PLOG_SEV(g_fdslog, fds_log::normal)
            << "Deactivate node uuid " << std::hex
            << "0x" << agent->get_uuid().uuid_get_val() << std::dec;

    rs_mtx.lock();
    rs_unregister_mtx(agent);
    node_down_pend.push_back(OM_SmAgent::agt_cast_ptr(agent));
    rs_mtx.unlock();
}

// om_splice_nodes_pend
// --------------------
//
void
OM_SmContainer::om_splice_nodes_pend(NodeList *addNodes, NodeList *rmNodes)
{
    rs_mtx.lock();
    addNodes->splice(addNodes->begin(), node_up_pend);
    rmNodes->splice(rmNodes->begin(), node_down_pend);
    rs_mtx.unlock();
}

// agent_register
// --------------
//
Error
OM_SmContainer::agent_register(const NodeUuid       &uuid,
                               const FdspNodeRegPtr  msg,
                               NodeAgent::pointer   *out)
{
    Error err = AgentContainer::agent_register(uuid, msg, out);

    if (OM_NodeDomainMod::om_in_test_mode() || (err != ERR_OK)) {
        return err;
    }
    OM_SmAgent::pointer agent = OM_SmAgent::agt_cast_ptr(*out);
    NodeAgentCpSessionPtr session(
            ac_cpSessTbl->startSession<netControlPathClientSession>(
                agent->get_ip_str(),
                agent->get_ctrl_port(),
                FDSP_STOR_MGR,  // TODO(Andrew): should be just a node
                1,              // just 1 channel for now...
                ctrlRspHndlr));

    fds_verify(agent != NULL);
    fds_verify(session != NULL);
    agent->setCpSession(session);
    return err;
}

// agent_unregister
// ----------------
//
Error
OM_SmContainer::agent_unregister(const NodeUuid &uuid, const std::string &name)
{
    Error err = AgentContainer::agent_unregister(uuid, name);

    return err;
}

// --------------------------------------------------------------------------------------
// OM DM NodeAgent Container
// --------------------------------------------------------------------------------------
OM_DmContainer::OM_DmContainer() : DmContainer(FDSP_ORCH_MGR)
{
    ctrlRspHndlr = boost::shared_ptr<OM_ControlRespHandler>(new OM_ControlRespHandler());
}

// agent_register
// --------------
//
Error
OM_DmContainer::agent_register(const NodeUuid       &uuid,
                               const FdspNodeRegPtr  msg,
                               NodeAgent::pointer   *out)
{
    Error err = AgentContainer::agent_register(uuid, msg, out);

    if (OM_NodeDomainMod::om_in_test_mode() || (err != ERR_OK)) {
        return err;
    }
    OM_DmAgent::pointer agent = OM_DmAgent::agt_cast_ptr(*out);
    NodeAgentCpSessionPtr session(
            ac_cpSessTbl->startSession<netControlPathClientSession>(
                agent->get_ip_str(),
                agent->get_ctrl_port(),
                FDSP_DATA_MGR,  // TODO(Andrew): should be just a node
                1,              // just 1 channel for now...
                ctrlRspHndlr));

    fds_verify(agent != NULL);
    fds_verify(session != NULL);
    agent->setCpSession(session);
    return err;
}

// -------------------------------------------------------------------------------------
// OM AM NodeAgent Container
// -------------------------------------------------------------------------------------
OM_AmContainer::OM_AmContainer() : AmContainer(FDSP_ORCH_MGR)
{
    ctrlRspHndlr = boost::shared_ptr<OM_ControlRespHandler>(new OM_ControlRespHandler());
}

// agent_register
// --------------
//
Error
OM_AmContainer::agent_register(const NodeUuid       &uuid,
                               const FdspNodeRegPtr  msg,
                               NodeAgent::pointer   *out)
{
    Error err = AgentContainer::agent_register(uuid, msg, out);

    if (OM_NodeDomainMod::om_in_test_mode() || (err != ERR_OK)) {
        return err;
    }
    OM_AmAgent::pointer agent = OM_AmAgent::agt_cast_ptr(*out);

    NodeAgentCpSessionPtr session(
            ac_cpSessTbl->startSession<netControlPathClientSession>(
                agent->get_ip_str(),
                agent->get_ctrl_port(),
                FDSP_STOR_HVISOR,  // TODO(Andrew): should be just a node
                1,                 // just 1 channel for now...
                ctrlRspHndlr));

    fds_verify(agent != NULL);
    fds_verify(session != NULL);
    agent->setCpSession(session);
    return err;
}

// --------------------------------------------------------------------------------------
// OM Node Container
// --------------------------------------------------------------------------------------
OM_NodeContainer::~OM_NodeContainer() {}
OM_NodeContainer::OM_NodeContainer()
    : DomainContainer("OM-Domain",
                      NULL,
                      new OM_SmContainer(),
                      new OM_DmContainer(),
                      new OM_AmContainer(),
                      new OmContainer(FDSP_ORCH_MGR),
                      NULL) {}

// om_send_my_info_to_peer
// -----------------------
//
static void
om_send_my_info_to_peer(NodeAgent::pointer me, NodeAgent::pointer peer)
{
    OM_SmAgent::agt_cast_ptr(me)->om_send_myinfo(peer);
}

// om_bcast_new_node
// -----------------
//
void
OM_NodeContainer::om_bcast_new_node(NodeAgent::pointer node, const FdspNodeRegPtr ref)
{
    if (ref->node_type == fpi::FDSP_STOR_HVISOR) {
        return;
    }
    dc_sm_nodes->agent_foreach<NodeAgent::pointer>(node, om_send_my_info_to_peer);
    dc_dm_nodes->agent_foreach<NodeAgent::pointer>(node, om_send_my_info_to_peer);
}

// om_send_peer_info_to_me
// -----------------------
//
static void
om_send_peer_info_to_me(NodeAgent::pointer me, NodeAgent::pointer peer)
{
    OM_SmAgent::agt_cast_ptr(peer)->om_send_myinfo(me);
}

// om_update_node_list
// -------------------
//
void
OM_NodeContainer::om_update_node_list(NodeAgent::pointer node, const FdspNodeRegPtr ref)
{
    dc_sm_nodes->agent_foreach<NodeAgent::pointer>(node, om_send_peer_info_to_me);
    dc_dm_nodes->agent_foreach<NodeAgent::pointer>(node, om_send_peer_info_to_me);
    dc_am_nodes->agent_foreach<NodeAgent::pointer>(node, om_send_peer_info_to_me);
}

}  // namespace fds
