/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#include <platform/service-ep-lib.h>
#include <platform/node-inventory.h>

#include "platform/node_agent_evt.h"

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
