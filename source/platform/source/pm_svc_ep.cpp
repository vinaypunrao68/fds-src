/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#include <platform/service-ep-lib.h>
#include <platform/node-inventory.h>

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

    /**
     * --------------------------------------------------------------------------------------
     * SM Service EndPoint.
     * --------------------------------------------------------------------------------------
     */
    SmSvcEp::~SmSvcEp()
    {
    }

    SmSvcEp::SmSvcEp(NodeAgent::pointer agent, fds_uint32_t maj, fds_uint32_t min,
                     NodeAgentEvt::pointer plugin) : EpSvc(agent->get_uuid(), fpi::FDSP_STOR_MGR)
    {
        ep_set_plugin(plugin);
    }

    // svc_receive_msg
    // ---------------
    //
    void SmSvcEp::svc_receive_msg(const fpi::AsyncHdr &msg)
    {
    }

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
