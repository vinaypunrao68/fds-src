/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#include <platform/service-ep-lib.h>
#include <platform/node-inventory.h>

namespace fds
{
    /**
     * --------------------------------------------------------------------------------------
     * PM Service EndPoint.
     * --------------------------------------------------------------------------------------
     */
    PmSvcEp::~PmSvcEp()
    {
    }

    PmSvcEp::PmSvcEp(NodeAgent::pointer agent, fds_uint32_t maj, fds_uint32_t min,
                     NodeAgentEvt::pointer plugin) : EpSvc(agent->get_uuid(), fpi::FDSP_PLATFORM)
    {
        ep_set_plugin(plugin);
    }

    // svc_receive_msg
    // ---------------
    //
    void PmSvcEp::svc_receive_msg(const fpi::AsyncHdr &msg)
    {
    }
}  // namespace fds
