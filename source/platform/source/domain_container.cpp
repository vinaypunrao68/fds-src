/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#include <string>

#include "platform/node-inventory.h"
#include "platform/node-inv-shmem.h"

#include "platform/node_data.h"

#include "node_svc_iter.h"


namespace fds
{
    // --------------------------------------------------------------------------------------
    // Node Container
    // --------------------------------------------------------------------------------------

    DomainContainer::~DomainContainer()
    {
    }
    DomainContainer::DomainContainer(char const *const name) : dc_om_master(NULL),
        dc_om_nodes(NULL),
        dc_sm_nodes(NULL), dc_dm_nodes(NULL), dc_am_nodes(NULL), dc_pm_nodes(NULL), rs_refcnt(0)
    {
    }

    DomainContainer::DomainContainer(char const *const name, OmAgent::pointer master,
                                     AgentContainer::pointer sm, AgentContainer::pointer dm,
                                     AgentContainer::pointer am,
                                     AgentContainer::pointer pm,
                                     AgentContainer::pointer om) : dc_om_master(master),
        dc_om_nodes(om),
        dc_sm_nodes(sm), dc_dm_nodes(dm), dc_am_nodes(am), dc_pm_nodes(pm), rs_refcnt(0)
    {
    }

    // dc_container_frm_msg
    // --------------------
    // Get the right container based on message type.
    //
    AgentContainer::pointer DomainContainer::dc_container_frm_msg(FdspNodeType node_type)
    {
        AgentContainer::pointer    nodes;

        switch (node_type)
        {
            case FDS_ProtocolInterface::FDSP_STOR_MGR:
                nodes = dc_sm_nodes;
                break;

            case FDS_ProtocolInterface::FDSP_DATA_MGR:
                nodes = dc_dm_nodes;
                break;

            case FDS_ProtocolInterface::FDSP_STOR_HVISOR:
                nodes = dc_am_nodes;
                break;

            case FDS_ProtocolInterface::FDSP_PLATFORM:
                nodes = dc_pm_nodes;
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
    Error DomainContainer::dc_register_node(const NodeUuid       &uuid, const FdspNodeRegPtr msg,
                                            NodeAgent::pointer   *agent)
    {
        AgentContainer::pointer    nodes;

        LOGDEBUG << "Domain register uuid " << std::hex << uuid.uuid_get_val() << ", svc uuid " <<
        msg->service_uuid.uuid << ", node type " << std::dec << msg->node_type;

        nodes = dc_container_frm_msg(msg->node_type);
        // TODO(Andrew): TOTAL HACK! This sleep prevents a race
        // where the node's control interface isn't initialized
        // yet and we try and connect too early. The real fix
        // should not register until its control interface is
        // fully initialized.
        sleep(2);
        return nodes->agent_register(uuid, msg, agent, true);
    }

    void DomainContainer::dc_register_node(const ShmObjRO     *shm, NodeAgent::pointer *agent,
                                           int ro, int rw,
                                           fds_uint32_t mask)
    {
        bool                       known_node;
        const node_data_t         *node;
        NodeAgent::pointer         tmp;
        AgentContainer::pointer    container;

        fds_verify(ro != -1);
        node = shm->shm_get_rec<node_data_t>(ro);

        container  = dc_container_frm_msg(node->nd_svc_type);
        known_node = container->agent_register(shm, agent, ro, rw);

        if ((known_node == true) || (node->nd_svc_type == fpi::FDSP_ORCH_MGR))
        {
            return;
        }

        fds_verify(node->nd_svc_type == fpi::FDSP_PLATFORM);

        LOGDEBUG << "Platform domain register " << *agent << ", svc uuid " << std::hex <<
        node->nd_service_uuid << ", svc mask " << mask << std::dec;

        if ((mask & fpi::NODE_SVC_SM) != 0)
        {
            tmp = NULL;
            dc_sm_nodes->agent_register(shm, &tmp, ro, rw);
        }

        if ((mask & fpi::NODE_SVC_DM) != 0)
        {
            tmp = NULL;
            dc_dm_nodes->agent_register(shm, &tmp, ro, rw);
        }

        if ((mask & fpi::NODE_SVC_AM) != 0)
        {
            tmp = NULL;
            dc_am_nodes->agent_register(shm, &tmp, ro, rw);
        }

        if ((mask & fpi::NODE_SVC_OM) != 0)
        {
            tmp = NULL;
            dc_om_nodes->agent_register(shm, &tmp, ro, rw);
        }
    }

    // dc_unregister_node
    // ------------------
    //
    Error DomainContainer::dc_unregister_node(const NodeUuid &uuid, const std::string &name)
    {
        AgentContainer::pointer    nodes;
        NodeAgent::pointer         agent;
        FdspNodeType               svc[] =
        {
            fpi::FDSP_STOR_MGR,
            fpi::FDSP_DATA_MGR,
            fpi::FDSP_STOR_HVISOR,
            fpi::FDSP_ORCH_MGR
        };

        for (int i = 0; svc[i] != FDS_ProtocolInterface::FDSP_ORCH_MGR; i++)
        {
            nodes = dc_container_frm_msg(svc[i]);

            if (nodes == NULL)
            {
                continue;
            }
            nodes->agent_unregister(uuid, name);
        }
        return Error(ERR_OK);
    }

    // dc_unregister_agent
    // -------------------
    //
    Error DomainContainer::dc_unregister_agent(const NodeUuid &uuid, FdspNodeType type)
    {
        AgentContainer::pointer    nodes;
        NodeAgent::pointer         agent;

        nodes = dc_container_frm_msg(type);

        if (nodes != NULL)
        {
            agent = nodes->agent_info(uuid);

            if (agent != NULL)
            {
                nodes->agent_deactivate(agent);
                return Error(ERR_OK);
            }
        }
        return Error(ERR_NOT_FOUND);
    }

    // dc_find_node_agent
    // ------------------
    //
    NodeAgent::pointer DomainContainer::dc_find_node_agent(const NodeUuid &uuid)
    {
        AgentContainer::pointer    nodes;
        NodeAgent::pointer         agent;
        fpi::FDSP_MgrIdType        type;

        type  = uuid.uuid_get_type();
        nodes = dc_container_frm_msg(type);

        if (nodes != NULL)
        {
            return nodes->agent_info(uuid);
        }
        return NULL;
    }

    // dc_node_svc_info
    // ----------------
    //
    void DomainContainer::dc_node_svc_info(fpi::DomainNodes &ret)
    {
        NodeSvcIter    iter(ret);
        dc_foreach_pm(&iter);
    }
}  // namespace fds
