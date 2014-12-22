/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#include <platform/service-ep-lib.h>
#include <platform/node-inventory.h>

#include "platform/om_node_agent_evt.h"

namespace fds
{
    /**
     * ---------------------------------------------------------------------------------
     * OM Agent Event Handlers
     * ---------------------------------------------------------------------------------
     */
    OmNodeAgentEvt::~OmNodeAgentEvt()
    {
    }

    OmNodeAgentEvt::OmNodeAgentEvt(NodeAgent::pointer agent) : NodeAgentEvt(agent)
    {
    }

    // ep_connected
    // ------------
    //
    void OmNodeAgentEvt::ep_connected()
    {
        LOGDEBUG << "[Plat] OM agent is connected";
    }

    // ep_down
    // -------
    //
    void OmNodeAgentEvt::ep_down()
    {
    }

    // svc_up
    // ------
    //
    void OmNodeAgentEvt::svc_up(EpSvcHandle::pointer eph)
    {
    }

    // svc_down
    // --------
    //
    void OmNodeAgentEvt::svc_down(EpSvc::pointer svc, EpSvcHandle::pointer eph)
    {
    }
}  // namespace fds
