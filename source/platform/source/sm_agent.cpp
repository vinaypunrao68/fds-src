/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#include <string>

#include <NetSession.h>
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
        sm_ep_svc     = Platform::platf_singleton()->plat_new_sm_svc(this, 0, 0);
        NetMgr::ep_mgr_singleton()->ep_register(sm_ep_svc, false);
    }

    SmAgent::~SmAgent()
    {
        /* TODO(Vy): shutdown netsession and cleanup stuffs here */
    }

    // agent_ep_plugin
    // ---------------
    //
    EpEvtPlugin::pointer SmAgent::agent_ep_plugin()
    {
        return sm_ep_svc->ep_evt_plugin();
    }

    // agent_ep_svc
    // ------------
    //
    SmSvcEp::pointer SmAgent::agent_ep_svc()
    {
        return sm_ep_svc;
    }

    // agent_bind_ep
    // -------------
    //
    void SmAgent::agent_bind_ep()
    {
        EpSvcImpl::pointer    ep = NetPlatform::nplat_singleton()->nplat_my_ep();
        NodeAgent::agent_bind_ep(ep, sm_ep_svc);
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
