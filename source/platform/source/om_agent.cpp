/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#include <string>

#include <NetSession.h>
#include <net/net-service-tmpl.hpp>
#include <apis/ConfigurationService.h>
#include "fds_module_provider.h"
#include "platform/platform.h"
#include "platform/om_svc_ep.h"

namespace fds
{
    // --------------------------------------------------------------------------------------
    // OM Agent
    // --------------------------------------------------------------------------------------
    OmAgent::~OmAgent()
    {
    }

    OmAgent::OmAgent(const NodeUuid &uuid) : NodeAgent(uuid), om_sess(NULL), om_reqt(NULL)
    {
        node_svc_type = fpi::FDSP_ORCH_MGR;
        om_ep_svc     = Platform::platf_singleton()->plat_new_om_svc(this, 0, 0);
        NetMgr::ep_mgr_singleton()->ep_register(om_ep_svc, false);
    }

    // agent_ep_plugin
    // ---------------
    //
    EpEvtPlugin::pointer
    OmAgent::agent_ep_plugin()
    {
        return om_ep_svc->ep_evt_plugin();
    }

    // agent_ep_svc
    // ------------
    //
    OmSvcEp::pointer OmAgent::agent_ep_svc()
    {
        return om_ep_svc;
    }

    // agent_bind_ep
    // -------------
    //
    void OmAgent::agent_bind_ep()
    {
        EpSvcImpl::pointer    ep = NetPlatform::nplat_singleton()->nplat_my_ep();
        NodeAgent::agent_bind_ep(ep, om_ep_svc);
    }

    // init_msg_hdr
    // ------------
    //
    void OmAgent::init_msg_hdr(fpi::FDSP_MsgHdrTypePtr hdr) const
    {
        NodeInventory::init_msg_hdr(hdr);
        hdr->msg_code     = FDSP_MSG_PUT_OBJ_REQ;  // TODO(Vy): cleanup these codes.
        hdr->dst_id       = FDSP_ORCH_MGR;
        hdr->session_uuid = om_sess_id;
    }

    // init_node_reg_pkt
    // -----------------
    //
    void OmAgent::init_node_reg_pkt(fpi::FDSP_RegisterNodeTypePtr pkt) const
    {
        Platform::ptr    plat = Platform::platf_const_singleton();

        NodeInventory::init_node_reg_pkt(pkt);
        pkt->node_type         = plat->plf_get_node_type();
        pkt->node_uuid.uuid    = plat->plf_get_my_node_uuid()->uuid_get_val();
        pkt->service_uuid.uuid = pkt->node_uuid.uuid + 1 + pkt->node_type;
        pkt->node_name         = *plat->plf_get_my_name();
        pkt->ip_hi_addr        = 0;
        pkt->ip_lo_addr        = fds::str_to_ipv4_addr(*plat->plf_get_my_ip());
        pkt->control_port      = plat->plf_get_my_ctrl_port();
        pkt->data_port         = plat->plf_get_my_data_port();
        pkt->migration_port    = plat->plf_get_my_migration_port();
        pkt->metasync_port     = plat->plf_get_my_metasync_port();
    }

    // om_register_node
    // ----------------
    //
    void OmAgent::om_register_node(fpi::FDSP_RegisterNodeTypePtr reg)
    {
        fpi::FDSP_MsgHdrTypePtr    hdr(new fpi::FDSP_MsgHdrType);

        fds_verify(om_reqt != NULL);
        init_msg_hdr(hdr);
        om_reqt->RegisterNode(hdr, reg);
    }

    // om_handshake
    // ------------
    //
    /* virtual */ void OmAgent::om_handshake(boost::shared_ptr<netSessionTbl> net,
                                             std::string om_ip, fds_uint32_t om_port)
    {
        boost::shared_ptr<fpi::FDSP_OMControlPathRespIf>    resp;

        om_sess =
            net->startSession<netOMControlPathClientSession>(om_ip, om_port, fpi::FDSP_ORCH_MGR,
                                                             1 /* just 1 channel for now */, resp);
        om_reqt    = om_sess->getClient();
        om_sess_id = om_sess->getSessionId();
    }

    // get_om_config_svc
    // ------------
    //
    // TODO(Vy): Make the necessary changes fit it to the svc endpoint model.
    // Now it returns thrift client directly
    boost::shared_ptr<apis::ConfigurationServiceClient> OmAgent::get_om_config_svc()
    {
        using namespace apache::thrift;  // NOLINT
        using namespace apache::thrift::protocol; // NOLINT
        using namespace apache::thrift::transport; // NOLINT

        if (!om_cfg_svc)
        {
            boost::shared_ptr<TTransport>    socket(new TSocket(
                                                        *MODULEPROVIDER()->get_plf_manager()->
                                                        plf_get_om_ip(), 9090));
            boost::shared_ptr<TTransport>    transport(new TFramedTransport(socket));
            boost::shared_ptr<TProtocol>     protocol(new TBinaryProtocol(transport));
            om_cfg_svc = boost::make_shared<apis::ConfigurationServiceClient>(protocol);
            transport->open();
        }
        return om_cfg_svc;
    }
}  // namespace fds
