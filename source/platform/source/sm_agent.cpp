/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#include <string>

#include <net/net-service-tmpl.hpp>
#include "platform/platform.h"
#include "platform/sm_svc_ep.h"

namespace fds
{
    // --------------------------------------------------------------------------------------
    // SM Agent
    // --------------------------------------------------------------------------------------
    SmAgent::SmAgent(const NodeUuid &uuid) : NodeAgent(uuid)
    {
        node_svc_type = fpi::FDSP_STOR_MGR;
    }

    SmAgent::~SmAgent()
    {
    }

    // agent_ep_plugin
    // ---------------
    //
    EpEvtPlugin::pointer SmAgent::agent_ep_plugin()
    {
        return nullptr;
    }

    // agent_ep_svc
    // ------------
    //
    SmSvcEp::pointer SmAgent::agent_ep_svc()
    {
        return nullptr;
    }

    // agent_bind_ep
    // -------------
    //
    void SmAgent::agent_bind_ep()
    {
    }

    SmContainer::SmContainer(FdspNodeType id) : AgentContainer(id)
    {
        ac_id = fpi::FDSP_STOR_MGR;
    }

    // agent_handshake
    // ---------------
    //
    void SmContainer::agent_handshake(boost::shared_ptr<netSessionTbl> net,
                                      NodeAgent::pointer agent)
    {
    }
}  // namespace fds
