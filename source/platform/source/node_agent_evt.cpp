/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#include "platform/node_agent_evt.h"
// #include "platform/dm_agent.h"
#include "platform/node_agent.h"

namespace fds
{
    /**
     * --------------------------------------------------------------------------------------
     * Event handlers
     * --------------------------------------------------------------------------------------
     */
    NodeAgentEvt::~NodeAgentEvt()
    {
    }

    NodeAgentEvt::NodeAgentEvt(NodeAgent::pointer agent) : na_owner(agent)
    {
    }

    // ep_connected
    // ------------
    //
    void NodeAgentEvt::ep_connected()
    {
    }

    // ep_down
    // -------
    //
    void NodeAgentEvt::ep_down()
    {
    }

    // svc_up
    // ------
    //
    void NodeAgentEvt::svc_up(EpSvcHandle::pointer eph)
    {
    }

    // svc_down
    // --------
    //
    void NodeAgentEvt::svc_down(EpSvc::pointer svc, EpSvcHandle::pointer eph)
    {
    }
}  // namespace fds
