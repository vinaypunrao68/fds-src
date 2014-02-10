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
OM_SmAgent::OM_SmAgent(const NodeUuid &uuid) : SmAgent(uuid) {}

int
OM_SmAgent::node_calc_stor_weight()
{
    return 0;
}

void
OM_SmAgent::setCpSession(NodeAgentCpSessionPtr session)
{
    ndCpSession = session;
    ndSessionId = ndCpSession->getSessionId();
    ndCpClient  = ndCpSession->getClient();

    FDS_PLOG_SEV(g_fdslog, fds_log::debug) << "Established connection with new node";
}

NodeAgentCpReqClientPtr
OM_SmAgent::getCpClient() const
{
    return ndCpClient;
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
    FDS_PLOG_SEV(g_fdslog, fds_log::debug)
        << "Actiate node uuid "
        << agent->get_uuid().uuid_get_val() << ", ptr " << agent << std::endl;

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
    FDS_PLOG_SEV(g_fdslog, fds_log::debug)
        << "Deactivate node uuid "
        << agent->get_uuid().uuid_get_val() << ", ptr " << agent << std::endl;

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

// ---------------------------------------------------------------------------------
// OM DM NodeAgent Container
// ---------------------------------------------------------------------------------
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

// ---------------------------------------------------------------------------------
// OM AM NodeAgent Container
// ---------------------------------------------------------------------------------
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
                FDSP_DATA_MGR,  // TODO(Andrew): should be just a node
                1,              // just 1 channel for now...
                ctrlRspHndlr));

    fds_verify(agent != NULL);
    fds_verify(session != NULL);
    agent->setCpSession(session);
    return err;
}

// ---------------------------------------------------------------------------------
// OM Node Container
// ---------------------------------------------------------------------------------
OM_NodeContainer::~OM_NodeContainer() {}
OM_NodeContainer::OM_NodeContainer()
    : DomainContainer("OM-Domain",
                      NULL,
                      new OM_SmContainer(),
                      new OM_DmContainer(),
                      new OM_AmContainer(),
                      new OmContainer(FDSP_ORCH_MGR),
                      NULL) {}

// om_bcast_new_node
// -----------------
//
void
OM_NodeContainer::om_bcast_new_node(NodeAgent::pointer node, const FdspNodeRegPtr ref)
{
}

// om_update_node_list
// -------------------
//
void
OM_NodeContainer::om_update_node_list(NodeAgent::pointer node, const FdspNodeRegPtr ref)
{
}

}  // namespace fds
