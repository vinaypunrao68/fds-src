/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#include <platform/service-ep-lib.h>
#include <platform/node-inventory.h>

namespace fds
{
    /**
     * --------------------------------------------------------------------------------------
     * AM Service EndPoint.
     * --------------------------------------------------------------------------------------
     */
    AmSvcEp::~AmSvcEp()
    {
    }

    AmSvcEp::AmSvcEp(NodeAgent::pointer agent, fds_uint32_t maj, fds_uint32_t min,
                     NodeAgentEvt::pointer plugin) : EpSvc(agent->get_uuid(),
                                                           fpi::FDSP_STOR_HVISOR)
    {
        ep_set_plugin(plugin);
    }

    // svc_receive_msg
    // ---------------
    //
    void AmSvcEp::svc_receive_msg(const fpi::AsyncHdr &msg)
    {
    }
}  // namespace fds
