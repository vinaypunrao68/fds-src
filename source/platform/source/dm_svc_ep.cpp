/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#include "platform/dm_svc_ep.h"
#include "platform/dm_agent.h"
#include "platform/node_agent.h"

namespace fds
{
    /**
     * --------------------------------------------------------------------------------------
     * DM Service EndPoint.
     * --------------------------------------------------------------------------------------
     */
    DmSvcEp::~DmSvcEp()
    {
    }

    DmSvcEp::DmSvcEp(NodeAgent::pointer agent, fds_uint32_t maj, fds_uint32_t min,
                     NodeAgentEvt::pointer plugin) : EpSvc(agent->get_uuid(), fpi::FDSP_DATA_MGR)
    {
        ep_set_plugin(plugin);
    }

    // svc_receive_msg
    // ---------------
    //
    void DmSvcEp::svc_receive_msg(const fpi::AsyncHdr &msg)
    {
    }
}  // namespace fds
