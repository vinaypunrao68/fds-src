/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#include <string>

#include "platform/node_data.h"
#include "node_platform.h"

namespace fds
{
    AgentContainer::AgentContainer(FdspNodeType id) : RsContainer()
    {
        ac_id        = id;
    }

    // agent_handshake
    // ---------------
    //
    void AgentContainer::agent_handshake(boost::shared_ptr<netSessionTbl> net,
                                         NodeAgent::pointer agent)
    {
    }

    // --------------------------------------------------------------------------------------
    // AgentContainer
    // --------------------------------------------------------------------------------------
    AgentContainer::~AgentContainer()
    {
    }

    // agent_register
    // --------------
    //
    Error AgentContainer::agent_register(const NodeUuid       &uuid, const FdspNodeRegPtr msg,
                                         NodeAgent::pointer   *out,
                                         bool activate)
    {
        Error err(ERR_OK);
        fds_bool_t            add;
        std::string           name;
        NodeAgent::pointer    agent;

        add   = false;
        *out  = NULL;
        name  = msg->node_name;
        agent = agt_cast_ptr<NodeAgent>(agent_info(uuid));

        if (agent == NULL)
        {
            LOGDEBUG << "AgentContainer::agent_register: Agent not found for UUID: "
                     << std::hex << uuid << std::dec << ". Allocating as new.";
            add   = activate;
            agent = agt_cast_ptr<NodeAgent>(rs_alloc_new(uuid));
        } else {
            LOGDEBUG << "Agent found for UUID: " << std::hex << uuid << std::dec << " OK.";
            err = ERR_DUPLICATE;
            // This is only for DM
            agent_reactivate(agent);
        }
        agent->node_fill_inventory(msg);
        *out = agent;

        if (add == true)
        {
            LOGDEBUG << "AgentContainer::agent_register: Activating agent for UUID: "
                    << std::hex << uuid <<  ".";
            agent_activate(agent);
        }
        return err;
    }

    // agent_register
    // --------------
    //
    bool AgentContainer::agent_register(const ShmObjRO     *shm, NodeAgent::pointer *out, int ro,
                                        int rw)
    {
        bool                  add, known;
        NodeUuid              svc, node;
        const node_data_t    *info;
        NodeAgent::pointer    agent;

        add = true;
        known = false;

        if (*out == NULL)
        {
            info = shm->shm_get_rec<node_data_t>(ro);
            node.uuid_set_val(info->nd_service_uuid);
            Platform::plf_svc_uuid_from_node(node, &svc, ac_id);

            agent = agt_cast_ptr<NodeAgent>(agent_info(svc));

            if (agent == NULL)
            {
                agent = agt_cast_ptr<NodeAgent>(rs_alloc_new(svc));
            }else {
                add = false;
                known = true;
            }
            *out = agent;
        }else {
            agent = *out;

            if (agent_info(agent->get_uuid()) != NULL)
            {
                known = true;
            }
        }
        agent->node_fill_shm_inv(shm, ro, rw, ac_id);

        if (add == true)
        {
            agent_activate(agent);
        }

        if (known == false)
        {
            agent->agent_publish_ep();
        }
        return known;
    }

    // agent_unregister
    // ----------------
    //
    Error AgentContainer::agent_unregister(const NodeUuid &uuid, const std::string &name)
    {
        NodeAgent::pointer    agent;

        agent = agent_info(uuid);

        if ((agent == NULL) || (name.compare(agent->get_node_name()) != 0))
        {
        	LOGDEBUG << "Unable to find agent for node " << uuid;
            return Error(ERR_NOT_FOUND);
        }
        agent_deactivate(agent);
        return Error(ERR_OK);
    }
}  // namespace fds
