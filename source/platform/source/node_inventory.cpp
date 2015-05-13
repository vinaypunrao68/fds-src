/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#include <string>

#include "fds_process.h"
#include <platform/node_services.h>
#include "platform/platform.h"
#include <net/net-service-tmpl.hpp>
#include "platform/node_data.h"
#include "platform/node_shm_ctrl.h"
#include <util/Log.h>
#include <net/net_utils.h>

namespace fds
{
    std::ostream& operator<< (std::ostream &os, const NodeServices& node)
    {
        os << "["
        << " sm:" << node.sm
        << " dm:" << node.dm
        << " am:" << node.am
        << "]";
        return os;
    }

    // --------------------------------------------------------------------------------------
    // Node Inventory
    // --------------------------------------------------------------------------------------
    const ShmObjRO *NodeInventory::node_shm_ctrl() const
    {
        TRACEFUNC;
        if (node_svc_type == fpi::FDSP_ACCESS_MGR)
        {
            return NodeShmCtrl::shm_am_inventory();
        }
        return NodeShmCtrl::shm_node_inventory();
    }

    // node_get_shm_rec
    // ----------------
    // Return the copy of node data in shared memory.
    //
    void NodeInventory::node_get_shm_rec(node_data_t *ndata) const
    {
        TRACEFUNC;
        int               idx;
        NodeUuid          nd_uuid;
        fds_uint64_t      uuid;
        const ShmObjRO   *shm;

        rs_uuid.uuid_get_base_val(&nd_uuid);

        if (node_ro_idx == -1)
        {
            /* TODO(Vy): support legacy OM path. */
            Platform             *plat = Platform::platf_singleton();
            NodeAgent::pointer    na = plat->plf_find_node_agent(nd_uuid);

            fds_verify((na != NULL) && (na != this) && (na->node_ro_idx != -1));
            na->node_get_shm_rec(ndata);
            return;
        }

        shm  = NodeShmCtrl::shm_node_inventory();
        uuid = nd_uuid.uuid_get_val();
        idx  =
            shm->shm_lookup_rec(node_ro_idx, static_cast<const void *>(&uuid),
                                static_cast<void *>(ndata),
                                sizeof(*ndata));

        fds_verify(idx == node_ro_idx);
    }

    // node_info_frm_shm
    // -----------------
    //
    void NodeInventory::node_info_frm_shm(node_data_t *out) const
    {
        TRACEFUNC;
        int               idx;
        NodeUuid          uuid;
        fds_uint64_t      uid;
        const ShmObjRO   *shm;

        rs_uuid.uuid_get_base_val(&uuid);

        if (node_ro_idx == -1)
        {
            /* TODO(Vy): support legacy OM path. */
            Platform             *plat = Platform::platf_singleton();
            NodeAgent::pointer    na = plat->plf_find_node_agent(uuid);

            fds_verify((na != NULL) && (na != this) && (na->node_ro_idx != -1));
            na->node_info_frm_shm(out);
            return;
        }

        uid = uuid.uuid_get_val();
        shm = node_shm_ctrl();
        idx =
            shm->shm_lookup_rec(node_ro_idx, static_cast<const void *>(&uid),
                                static_cast<void *>(out),
                                sizeof(*out));

        fds_assert(idx == node_ro_idx);
    }

#if 0
    // get_node_name
    // -------------
    //
    std::string NodeInventory::get_node_name() const
    {
        node_data_t    ndata;

        node_get_shm_rec(&ndata);
        return std::string(ndata.nd_auto_name);
    }
#endif

    // get_ip_str
    // ----------
    //
    std::string NodeInventory::get_ip_str() const
    {
        TRACEFUNC;
        node_data_t    ndata;

        node_get_shm_rec(&ndata);
        return std::string(ndata.nd_ip_addr);
    }

    fds_uint32_t NodeInventory::node_base_port() const
    {
        TRACEFUNC;
        node_data_t    ndata;

        node_get_shm_rec(&ndata);
        return Platform::plf_svc_port_from_node(ndata.nd_base_port, node_svc_type);
    }

    // get_node_root
    // -------------
    //
    std::string NodeInventory::get_node_root() const
    {
        TRACEFUNC;
        return node_root;
    }

    // node_capability
    // ---------------
    //
    const node_stor_cap_t * NodeInventory::node_capability() const
    {
        TRACEFUNC;
        const ShmObjRO      *shm;
        const node_data_t   *ndata;

        shm   = node_shm_ctrl();

        if (node_ro_idx == -1)
        {
            /* TODO(Vy): support OM legacy path where we have 2 node agents. */
            NodeAgent::pointer    na;
            NodeUuid              uuid;
            Platform             *plat = Platform::platf_singleton();

            rs_uuid.uuid_get_base_val(&uuid);
            na = plat->plf_find_node_agent(uuid);
            fds_verify((na != NULL) && (na != this) && (na->node_ro_idx != -1));
            return na->node_capability();
        }

        ndata = shm->shm_get_rec<node_data_t>(node_ro_idx);
        fds_verify(ndata != NULL);
        return &ndata->nd_capability;
    }

    // node_fill_shm_inv
    // -----------------
    //
    void NodeInventory::node_fill_shm_inv(const ShmObjRO *shm, int ro, int rw, FdspNodeType id)
    {
        TRACEFUNC;
        const node_data_t   *info;

        fds_assert(ro != -1);
        node_svc_type = id;

        info = shm->shm_get_rec<node_data_t>(ro);

        if (node_ro_idx == -1)
        {
            strncpy(rs_name, info->nd_auto_name, RS_NAME_MAX - 1);
        }else {
            NodeUuid    svc, node(info->nd_node_uuid);

            Platform::plf_svc_uuid_from_node(node, &svc, node_svc_type);
            fds_assert(rs_uuid == svc);
        }

        node_ro_idx = ro;
        node_rw_idx = rw;
    }

    // init_plat_info_msg
    // ------------------
    // Format the NodeInfoMsg that will be processed by functions above.  The data is taken
    // from the platform lib to represent the HW node running the SW.
    //
    void NodeInventory::init_plat_info_msg(fpi::NodeInfoMsg *msg) const
    {
        TRACEFUNC;
        fpi::UuidBindMsg     *msg_bind;
        Platform::ptr         plat = Platform::platf_const_singleton();
        EpSvcImpl::pointer    psvc = NetPlatform::nplat_singleton()->nplat_my_ep();

        msg_bind = &msg->node_loc;
        psvc->ep_fmt_uuid_binding(msg_bind, &msg->node_domain);

        msg->nd_base_port           = plat->plf_get_my_node_port();
        msg_bind->svc_type          = plat->plf_get_node_type();
        msg_bind->svc_auto_name     = *plat->plf_get_my_ip();
        msg_bind->svc_id.svc_name   = "";
        msg_bind->svc_node.svc_name = *plat->plf_get_my_name();
        msg_bind->svc_node.svc_uuid.svc_uuid = plat->plf_get_my_node_uuid()->uuid_get_val();
    }

    // init_om_info_msg
    // ----------------
    // Format the NodeInfoMsg with data about OM node.
    void NodeInventory::init_om_info_msg(fpi::NodeInfoMsg *msg)
    {
        TRACEFUNC;
        fpi::UuidBindMsg   *msg_bind;
        Platform::ptr       plat = Platform::platf_const_singleton();

        msg_bind                    = &msg->node_loc;
        msg->nd_base_port           = plat->plf_get_om_ctrl_port();
        msg_bind->svc_port          = plat->plf_get_om_ctrl_port();
        msg_bind->svc_addr          = *plat->plf_get_om_ip();
        msg_bind->svc_type          = fpi::FDSP_ORCH_MGR;
        msg_bind->svc_auto_name     = "OM";
        msg_bind->svc_id.svc_name   = "OM";
        msg_bind->svc_node.svc_name = *plat->plf_get_my_name();

        msg_bind->svc_id.svc_uuid.svc_uuid   = gl_OmUuid.uuid_get_val();
        msg_bind->svc_node.svc_uuid.svc_uuid = gl_OmUuid.uuid_get_val();
        memset(&msg->node_stor, 0, sizeof(msg->node_stor));
    }

    // init_om_pm_info_msg
    // -------------------
    // Format the NodeInfoMsg with data about OM Platform Services.
    void NodeInventory::init_om_pm_info_msg(fpi::NodeInfoMsg *msg)
    {
        TRACEFUNC;
        fpi::UuidBindMsg   *msg_bind;
        Platform::ptr       plat = Platform::platf_const_singleton();

        msg_bind                    = &msg->node_loc;
        msg->nd_base_port           = plat->plf_get_om_svc_port();
        msg_bind->svc_port          = plat->plf_get_om_ctrl_port();
        msg_bind->svc_addr          = *plat->plf_get_om_ip();
        msg_bind->svc_type          = fpi::FDSP_PLATFORM;
        msg_bind->svc_auto_name     = "OM-PM";
        msg_bind->svc_id.svc_name   = "OM-PM";
        msg_bind->svc_node.svc_name = *plat->plf_get_my_name();

        msg_bind->svc_id.svc_uuid.svc_uuid   = gl_OmPmUuid.uuid_get_val();
        msg_bind->svc_node.svc_uuid.svc_uuid = gl_OmPmUuid.uuid_get_val();
    }

    // init_node_info_msg
    // ------------------
    // Format the NodeInfoMsg with data from this node agent obj.
    void NodeInventory::init_node_info_msg(fpi::NodeInfoMsg *msg) const
    {
        TRACEFUNC;
    }

    // svc_info_frm_shm
    // ----------------
    //
    void NodeInventory::svc_info_frm_shm(fpi::SvcInfo *svc) const
    {
        TRACEFUNC;
        node_data_t    ninfo;

        node_info_frm_shm(&ninfo);
        svc->svc_id.svc_uuid.svc_uuid = ninfo.nd_node_uuid;

        svc->svc_id.svc_name.assign(rs_name);
        svc->svc_auto_name.assign(ninfo.nd_auto_name);
        svc->svc_type   = node_svc_type;
        svc->svc_status = fpi::SVC_STATUS_ACTIVE;
    }

    // node_fill_inventory
    // -------------------
    // TODO(Vy): depricate this function.
    //
    void NodeInventory::node_fill_inventory(const FdspNodeRegPtr msg)
    {
        TRACEFUNC;
        std::string    ip;

        nd_gbyte_cap = msg->disk_info.ssd_capacity;
        nd_node_name = msg->node_name;
        node_root    = msg->node_root;

        ip = net::ipAddr2String(msg->ip_lo_addr);
        strncpy(rs_name, ip.c_str(), RS_NAME_MAX - 1);
    }

    // set_node_state
    // --------------
    //
    void NodeInventory::set_node_state(FdspNodeState state)
    {
        TRACEFUNC;
        nd_my_node_state = state;
    }

    void NodeInventory::set_node_dlt_version(fds_uint64_t dlt_version)
    {
        TRACEFUNC;
        // TODO(Vy): do this in platform side.
        nd_my_dlt_version = dlt_version;
    }

    // node_update_inventory
    // ---------------------
    //
    void NodeInventory::node_update_inventory(const FdspNodeRegPtr msg)
    {
        TRACEFUNC;
    }

    // init_msg_hdr
    // ------------
    //
    void NodeInventory::init_msg_hdr(fpi::FDSP_MsgHdrTypePtr msgHdr) const
    {
        TRACEFUNC;
        Platform::ptr    plat = Platform::platf_const_singleton();

        msgHdr->minor_ver = 0;
        msgHdr->msg_id    = 1;
        msgHdr->major_ver = 0xa5;
        msgHdr->minor_ver = 0x5a;

        msgHdr->num_objects = 1;
        msgHdr->frag_len    = 0;
        msgHdr->frag_num    = 0;

        msgHdr->tennant_id      = 0;
        msgHdr->local_domain_id = 0;
        msgHdr->err_code        = ERR_OK;
        msgHdr->result          = FDS_ProtocolInterface::FDSP_ERR_OK;

        if (plat == NULL)
        {
            /* We'll get here if this is OM */
            msgHdr->src_id        = fpi::FDSP_ORCH_MGR;
            msgHdr->src_node_name = "";
        }else {
            msgHdr->src_id        = plat->plf_get_node_type();
            msgHdr->src_node_name = *plat->plf_get_my_name();
        }
    }

    // init_node_info_pkt
    // ------------------
    //
    void NodeInventory::init_node_info_pkt(fpi::FDSP_Node_Info_TypePtr pkt) const
    {
        TRACEFUNC;
        Platform       *plat = Platform::platf_singleton();
        fds_uint32_t    base = node_base_port();

        pkt->node_id        = 0;
        pkt->node_uuid      = Platform::plf_get_my_node_uuid()->uuid_get_val();
        pkt->service_uuid   = get_uuid().uuid_get_val();
        pkt->ip_hi_addr     = 0;
        pkt->ip_lo_addr     = str_to_ipv4_addr(get_ip_str());
        pkt->node_type      = node_get_svc_type();
        pkt->node_name      = get_node_name();
        pkt->node_state     = node_state();
        pkt->control_port   = plat->plf_get_my_ctrl_port(base);
        pkt->data_port      = plat->plf_get_my_data_port(base);
        pkt->migration_port = plat->plf_get_my_migration_port(base);
        pkt->metasync_port  = plat->plf_get_my_metasync_port(base);
        pkt->node_root      = get_node_root();
    }

    // init_node_reg_pkt
    // -----------------
    //
    void NodeInventory::init_node_reg_pkt(fpi::FDSP_RegisterNodeTypePtr pkt) const
    {
        TRACEFUNC;
        Platform::ptr    plat = Platform::platf_const_singleton();
        pkt->node_type     = plat->plf_get_node_type();
        pkt->node_name     = *plat->plf_get_my_name();
        pkt->ip_hi_addr    = 0;
        pkt->ip_lo_addr    = str_to_ipv4_addr(*plat->plf_get_my_ip());
        pkt->control_port  = plat->plf_get_my_ctrl_port();
        pkt->node_root     = g_fdsprocess->proc_fdsroot()->dir_fdsroot();
    }

    const NodeUuid    gl_OmPmUuid(0xcac0);
    const NodeUuid    gl_OmUuid(0xcac0 | fpi::FDSP_ORCH_MGR);

}  // namespace fds
