/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#include <string>

#include <NetSession.h>
#include <net/net-service-tmpl.hpp>
#include "platform/service-ep-lib.h"
#include "platform/platform-lib.h"

#include "platform/sm_svc_ep.h"

namespace fds
{
    // --------------------------------------------------------------------------------------
    // SM Agent
    // --------------------------------------------------------------------------------------
    SmAgent::SmAgent(const NodeUuid &uuid) : NodeAgent(uuid), sm_sess(NULL), sm_reqt(NULL)
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

    // sm_handshake
    // ------------
    //
    /* virtual */ void SmAgent::sm_handshake(boost::shared_ptr<netSessionTbl> net)
    {
        std::string                                    ip = get_ip_str();
        fds_uint32_t                                   base = node_base_port();
        Platform                                      *plat = Platform::platf_singleton();

        boost::shared_ptr<fpi::FDSP_DataPathRespIf>    resp;
        sm_sess = net->startSession<netDataPathClientSession>(ip, plat->plf_get_my_data_port(
                                                                  base), fpi::FDSP_STOR_MGR,
                                                              1 /* just 1 channel */, resp);

        sm_reqt    = sm_sess->getClient();
        sm_sess_id = sm_sess->getSessionId();

        FDS_PLOG(g_fdslog) << "Handshake with SM: " << ip
        << ", port " << plat->plf_get_my_data_port(base) << ", sess id " << sm_sess_id;
    }

    NodeAgentDpClientPtr SmAgent::get_sm_client()
    {
        return sm_reqt;
    }

    std::string SmAgent::get_sm_sess_id()
    {
        return sm_sess_id;
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
        SmAgent::pointer    sm = agt_cast_ptr<SmAgent>(agent);
        sm->sm_handshake(net);
    }
}  // namespace fds
