/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#include <NetSession.h>
#include <net/net-service-tmpl.hpp>
#include "platform/service-ep-lib.h"
#include "platform/platform-lib.h"

#include "platform/node_data.h"

#include "platform/pm_svc_ep.h"

namespace fds
{
    // --------------------------------------------------------------------------------------
    // PM Agent
    // --------------------------------------------------------------------------------------
    PmAgent::~PmAgent()
    {
    }

    PmAgent::PmAgent(const NodeUuid &uuid) : NodeAgent(uuid)
    {
        pm_ep_svc = Platform::platf_singleton()->plat_new_pm_svc(this, 0, 0);
        NetMgr::ep_mgr_singleton()->ep_register(pm_ep_svc, false);
    }

    EpEvtPlugin::pointer PmAgent::agent_ep_plugin()
    {
        return pm_ep_svc->ep_evt_plugin();
    }

    // agent_svc_info
    // --------------
    //
    void PmAgent::agent_svc_info(fpi::NodeSvcInfo *out) const
    {
        node_data_t    ninfo;

        node_info_frm_shm(&ninfo);
        out->node_addr.assign(ninfo.nd_ip_addr);
        out->node_auto_name.assign(ninfo.nd_auto_name);

        out->node_base_uuid.svc_uuid = rs_uuid.uuid_get_val();
        out->node_base_port = ninfo.nd_base_port;
        out->node_state     = fpi::FDS_Node_Up;
        out->node_svc_mask  = NODE_DO_PROXY_ALL_SVCS;

        agent_svc_fillin(out, &ninfo, fpi::FDSP_STOR_MGR);
        agent_svc_fillin(out, &ninfo, fpi::FDSP_DATA_MGR);
        agent_svc_fillin(out, &ninfo, fpi::FDSP_STOR_HVISOR);
        agent_svc_fillin(out, &ninfo, fpi::FDSP_ORCH_MGR);
        agent_svc_fillin(out, &ninfo, fpi::FDSP_PLATFORM);
    }

    // agent_svc_fillin
    // ----------------
    // Fill in service specific info based on the node info from shared memory.
    //
    void PmAgent::agent_svc_fillin(fpi::NodeSvcInfo    *out, const node_data_t   *ninfo,
                                   fpi::FDSP_MgrIdType type) const
    {
        int             port;
        NodeUuid        base, svc;
        fpi::SvcInfo    data;
        fpi::SvcUuid    uuid;

        base.uuid_set_val(ninfo->nd_node_uuid);
        Platform::plf_svc_uuid_from_node(base, &svc, type);

        data.svc_id.svc_uuid.svc_uuid = svc.uuid_get_val();
        data.svc_id.svc_name.assign(ninfo->nd_assign_name);
        data.svc_auto_name.assign(ninfo->nd_auto_name);

        data.svc_status = SVC_STATUS_ACTIVE;
        data.svc_type   = type;
        data.svc_port   = Platform::plf_svc_port_from_node(ninfo->nd_base_port, type);

        out->node_svc_list.push_back(data);
    }

    // agent_ep_svc
    // ------------
    //
    PmSvcEp::pointer
    PmAgent::agent_ep_svc()
    {
        return pm_ep_svc;
    }

    // agent_bind_ep
    // -------------
    //
    void PmAgent::agent_bind_ep()
    {
        EpSvcImpl::pointer    ep = NetPlatform::nplat_singleton()->nplat_my_ep();
        NodeAgent::agent_bind_ep(ep, pm_ep_svc);
    }
}  // namespace fds
