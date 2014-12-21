/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#include <string>

#include <platform/node-inv-shmem.h>
#include "platform/node-inventory.h"

#include "net/SvcRequestPool.h"

#include <net/net-service-tmpl.hpp>
#include "platform/platform-lib.h"

#include "platform/node_work_item.h"
#include "platform/node_data.h"

namespace fds
{
    // --------------------------------------------------------------------------------------
    // Node Agents
    // --------------------------------------------------------------------------------------

    NodeAgent::~NodeAgent()
    {
    }

    NodeAgent::NodeAgent(const NodeUuid &uuid) : NodeInventory(uuid), nd_eph(NULL),
                                                 nd_ctrl_eph(NULL), nd_svc_rpc(NULL),
                                                 nd_ctrl_rpc(NULL), pm_wrk_item(NULL)
    {
    }

    // node_stor_weight
    // ----------------
    //
    fds_uint64_t NodeAgent::node_stor_weight() const
    {
        // lets normalize = nodes have same weight if their
        // capacity is within 10GB diff
        fds_uint64_t    weight = nd_gbyte_cap / 10;

        if (weight < 1)
        {
            weight = 1;
        }
        return weight;
    }

    // node_set_weight
    // ---------------
    //
    void NodeAgent::node_set_weight(fds_uint64_t weight)
    {
        nd_gbyte_cap = weight;
    }

    // agent_ep_plugin
    // ---------------
    //
    EpEvtPlugin::pointer NodeAgent::agent_ep_plugin()
    {
        return NULL;
    }

    // agent_bind_ep
    // -------------
    //
    void NodeAgent::agent_bind_ep(EpSvcImpl::pointer ep, EpSvc::pointer svc)
    {
        ep->ep_bind_service(svc);
    }

    // agent_publish_ep
    // ----------------
    //
    void NodeAgent::agent_publish_ep()
    {
        int             port;
        NodeUuid        uuid;
        std::string     ip;
        fpi::SvcUuid    svc;

        svc.svc_uuid = rs_uuid.uuid_get_val();
        port = NetMgr::ep_mgr_singleton()->ep_uuid_binding(svc, 0, 0, &ip);
        LOGDEBUG << "Agent lookup ep " << std::hex << svc.svc_uuid
        << ", obj " << this << ", svc type " << node_svc_type
        << ", idx " << node_ro_idx << ", rw idx " << node_rw_idx
        << ", ip " << ip << ":" << std::dec << port;
    }

// DJN

    // node_agent_up
    // -------------
    //
    void NodeAgent::node_agent_up()
    {
        if (nd_eph == NULL)
        {
            auto    rpc = node_svc_rpc(&nd_eph);
        }

        if (nd_ctrl_eph == NULL)
        {
            auto    rpc = node_ctrl_rpc(&nd_ctrl_eph);
        }
    }

    // node_agent_down
    // ---------------
    // We should not have concurrent call between node_agent_up and node_agent_down.
    //
    void
    NodeAgent::node_agent_down()
    {
        nd_eph      = NULL;
        nd_ctrl_eph = NULL;
    }

    // node_ctrl_rpc
    // -------------
    //
    boost::shared_ptr<fpi::FDSP_ControlPathReqClient> NodeAgent::node_ctrl_rpc(
        EpSvcHandle::pointer *handle)
    {
        NetMgr                 *net;
        fpi::SvcUuid            peer;
        EpSvcHandle::pointer    eph;

        if (nd_ctrl_eph != NULL)
        {
            goto out;
        }

        peer.svc_uuid = rs_uuid.uuid_get_val();
        net = NetMgr::ep_mgr_singleton();
        eph = net->svc_get_handle<fpi::FDSP_ControlPathReqClient>(peer, 0, NET_SVC_CTRL);

        /* TODO(Vy): wire up the common event plugin to handle error. */
        if (eph != NULL)
        {
            nd_ctrl_eph = eph;
            out:
            *handle     = nd_ctrl_eph;
            nd_ctrl_rpc = nd_ctrl_eph->svc_rpc<fpi::FDSP_ControlPathReqClient>();
            fds_verify(nd_ctrl_rpc != NULL);
            return nd_ctrl_rpc;
        }
        *handle = NULL;
        return NULL;
    }

    // agent_rpc
    // ---------
    //
    boost::shared_ptr<fpi::PlatNetSvcClient> NodeAgent::node_svc_rpc(EpSvcHandle::pointer *handle,
                                                                     int maj,
                                                                     int min)
    {
        NetMgr                 *net;
        fpi::SvcUuid            peer;
        EpSvcHandle::pointer    eph;

        if (nd_eph != NULL)
        {
            goto out;
        }
        peer.svc_uuid = rs_uuid.uuid_get_val();
        net = NetMgr::ep_mgr_singleton();
        eph = net->svc_get_handle<fpi::PlatNetSvcClient>(peer, maj, min);

        /* TODO(Vy): wire up the common event plugin to handle error. */
        if (eph != NULL)
        {
            nd_eph     = eph;
            out:
            *handle    = nd_eph;
            nd_svc_rpc = nd_eph->svc_rpc<fpi::PlatNetSvcClient>();
            fds_verify(nd_svc_rpc != NULL);
            return nd_svc_rpc;
        }
        *handle = NULL;
        return NULL;
    }


    // node_om_request
    // ---------------
    //
    boost::shared_ptr<EPSvcRequest> NodeAgent::node_om_request()
    {
        fpi::SvcUuid    om_uuid;

        gl_OmUuid.uuid_assign(&om_uuid);
        return gSvcRequestPool->newEPSvcRequest(om_uuid);
    }

    // node_msg_request
    // ----------------
    //
    boost::shared_ptr<EPSvcRequest> NodeAgent::node_msg_request()
    {
        fpi::SvcUuid    uuid;

        rs_uuid.uuid_assign(&uuid);
        return gSvcRequestPool->newEPSvcRequest(uuid);
    }

    // Debug operator
    //
    std::ostream &operator << (std::ostream &os, const NodeAgent::pointer node)
    {
        os << " agent: " << node.get() << std::hex << " [" << node->get_uuid().uuid_get_val() <<
        "] " << std::dec;
        return os;
    }

    // agent_svc_fillin
    // ----------------
    //
    void NodeAgent::agent_svc_fillin(fpi::NodeSvcInfo    *out, const node_data_t   *ninfo,
                                     fpi::FDSP_MgrIdType type) const
    {
    }
}  // namespace fds
