/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#include <net/net-service-tmpl.hpp>
#include "platform/node-inventory.h"
#include "platform/platform-lib.h"

#include "platform/platform.h"
#include "platform/am_svc_ep.h"

namespace fds
{
    // --------------------------------------------------------------------------------------
    // AmAgent
    // --------------------------------------------------------------------------------------
    AmAgent::~AmAgent()
    {
    }

    AmAgent::AmAgent(const NodeUuid &uuid) : NodeAgent(uuid)
    {
        node_svc_type = fpi::FDSP_STOR_HVISOR;
        am_ep_svc     = Platform::platf_singleton()->plat_new_am_svc(this, 0, 0);
        NetMgr::ep_mgr_singleton()->ep_register(am_ep_svc, false);
    }

    // agent_ep_plugin
    // ---------------
    //
    EpEvtPlugin::pointer AmAgent::agent_ep_plugin()
    {
        return am_ep_svc->ep_evt_plugin();
    }

    // agent_ep_svc
    // ------------
    //
    AmSvcEp::pointer AmAgent::agent_ep_svc()
    {
        return am_ep_svc;
    }

    // agent_bind_ep
    // -------------
    //
    void AmAgent::agent_bind_ep()
    {
        EpSvcImpl::pointer    ep = NetPlatform::nplat_singleton()->nplat_my_ep();
        NodeAgent::agent_bind_ep(ep, am_ep_svc);
    }
}  // namespace fds
