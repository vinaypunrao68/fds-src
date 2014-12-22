/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#include "platform/om_svc_ep.h"
#include "platform/om_agent.h"
#include "platform/node_agent.h"

namespace fds
{
    /**
     * --------------------------------------------------------------------------------------
     * OM Service EndPoint.
     * --------------------------------------------------------------------------------------
     */
    OmSvcEp::~OmSvcEp()
    {
    }

    OmSvcEp::OmSvcEp(NodeAgent::pointer agent, fds_uint32_t maj, fds_uint32_t min,
                     NodeAgentEvt::pointer plugin) : EpSvc(agent->get_uuid(), fpi::FDSP_ORCH_MGR)
    {
        ep_set_plugin(plugin);
        ep_conn_om     = true;
        ep_conn_domain = true;

        fds_verify(agent->node_get_svc_type() == fpi::FDSP_ORCH_MGR);
        na_owner = agt_cast_ptr<OmAgent>(agent);
    }

    // svc_receive_msg
    // ---------------
    //
    void OmSvcEp::svc_receive_msg(const fpi::AsyncHdr &msg)
    {
    }

    // svc_chg_uuid
    // ------------
    //
    void OmSvcEp::svc_chg_uuid(const NodeUuid &uuid)
    {
    }

    // ep_domain_connected
    // -------------------
    //
    void OmSvcEp::ep_domain_connected()
    {
        ep_conn_domain = true;
        LOGNORMAL << " Connect domain - OM " << ep_conn_om << ", domain: " << ep_conn_domain;

        if (ep_conn_om == true)
        {
            ep_first_om_message();
        }
    }

    // ep_om_connected
    // ---------------
    //
    void OmSvcEp::ep_om_connected()
    {
        ep_conn_om = true;
        LOGNORMAL << " Connect OM - OM: " << ep_conn_om << ", domain: " << ep_conn_domain;

        if (ep_conn_domain == true)
        {
            ep_first_om_message();
        }
    }

    // ep_first_om_message
    // -------------------
    //
    void OmSvcEp::ep_first_om_message()
    {
    }
}  // namespace fds
