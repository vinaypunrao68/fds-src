/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#include "platform/domain_container.h"
#include "platform/platform.h"
#include "plat_agent.h"
#include "node_shm_rw_ctrl.h"
#include "disk_plat_module.h"

namespace fds
{
    /*
     * -------------------------------------------------------------------------------------
     * Platform Node Agent
     * -------------------------------------------------------------------------------------
     */
    PlatAgent::PlatAgent(const NodeUuid &uuid) : DomainAgent(uuid, false)
    {
    }

    // init_stor_cap_msg
    // -----------------
    //
    void PlatAgent::init_stor_cap_msg(fpi::StorCapMsg *msg) const
    {
        DiskPlatModule   *disk;

        std::cout << "Platform agent fill in storage cap msg" << std::endl;
        disk = DiskPlatModule::dsk_plat_singleton();
        // if we are in perf test mode, over-write some node capabilities
        // from config
        FdsConfigAccessor    conf(g_fdsprocess->get_fds_config(), "fds.plat.testing.");
        NodeAgent::init_stor_cap_msg(msg);

        if (conf.get<bool>("manual_nodecap"))
        {
            msg->disk_iops_min = conf.get<int>("disk_iops_min");
            msg->disk_iops_max = conf.get<int>("disk_iops_max");
            LOGNORMAL << "Over-writing node perf capability from config ";
        }
    }

    // pda_register
    // ------------
    //
    void PlatAgent::pda_register()
    {
        int                         idx;
        node_data_t                 rec;
        fpi::NodeInfoMsg            msg;
        ShmObjRWKeyUint64          *shm;
        NodeAgent::pointer          agent;
        DomainContainer::pointer    local;

        this->init_plat_info_msg(&msg);
        this->node_info_msg_to_shm(&msg, &rec);

        shm = NodeShmRWCtrl::shm_node_rw_inv();
        idx = shm->shm_insert_rec(static_cast<void *>(&rec.nd_node_uuid),
                                  static_cast<void *>(&rec), sizeof(rec));
        fds_verify(idx != -1);

        agent = this;
        local = Platform::platf_singleton()->plf_node_inventory();
        local->dc_register_node(shm, &agent, idx, idx, NODE_DO_PROXY_ALL_SVCS);
    }

    // agent_publish_ep
    // ----------------
    // Called by the platform daemon side to publish uuid binding info to shared memory.
    //
    void PlatAgent::agent_publish_ep()
    {
        int               idx, base_port;
        node_data_t       ninfo;
        ep_map_rec_t      rec;
        Platform         *plat;
        EpPlatformdMod   *ep_map;

        plat   = Platform::platf_singleton();
        ep_map = EpPlatformdMod::ep_shm_singleton();
        node_info_frm_shm(&ninfo);
        LOGDEBUG << "Platform agent uuid " << std::hex << ninfo.nd_node_uuid;

        if (rs_uuid == gl_OmPmUuid)
        {
            /* Record both OM binding and local binding. */
            base_port          = ninfo.nd_base_port;
            ninfo.nd_node_uuid = gl_OmPmUuid.uuid_get_val();
            EpPlatLibMod::ep_node_info_to_mapping(&ninfo, &rec);
            idx = ep_map->ep_map_record(&rec);

            ninfo.nd_node_uuid = gl_OmUuid.uuid_get_val();
            ninfo.nd_base_port = plat->plf_get_om_ctrl_port();
            EpPlatLibMod::ep_node_info_to_mapping(&ninfo, &rec);
            idx = ep_map->ep_map_record(&rec);

            /* TODO(Vy): hack, record OM binding for the control path. */
            ninfo.nd_base_port = plat->plf_get_my_data_port(base_port);
            EpPlatLibMod::ep_node_info_to_mapping(&ninfo, &rec);
            rec.rmp_major = 0;
            rec.rmp_minor = NET_SVC_CTRL;
            idx = ep_map->ep_map_record(&rec);

            LOGDEBUG << "Platform daemon binds OM " << std::hex
                     << ninfo.nd_node_uuid << "@" << ninfo.nd_ip_addr << ":" << std::dec
                     << ninfo.nd_base_port << ", idx " << idx;
        }else {
            agent_bind_svc(ep_map, &ninfo, fpi::FDSP_STOR_MGR);
            agent_bind_svc(ep_map, &ninfo, fpi::FDSP_DATA_MGR);
            agent_bind_svc(ep_map, &ninfo, fpi::FDSP_STOR_HVISOR);
            agent_bind_svc(ep_map, &ninfo, fpi::FDSP_ORCH_MGR);
        }
    }

    // agent_bind_svc
    // --------------
    //
    void PlatAgent::agent_bind_svc(EpPlatformdMod *map, node_data_t *ninfo, fpi::FDSP_MgrIdType t)
    {
        int             i, idx, base_port, max_port, saved_port;
        NodeUuid        node, svc;
        fds_uint64_t    saved_uuid;
        ep_map_rec_t    rec;

        saved_port = ninfo->nd_base_port;
        saved_uuid = ninfo->nd_node_uuid;

        node.uuid_set_val(saved_uuid);
        Platform::plf_svc_uuid_from_node(node, &svc, t);

        ninfo->nd_node_uuid = svc.uuid_get_val();
        ninfo->nd_base_port = Platform::plf_svc_port_from_node(saved_port, t);
        EpPlatLibMod::ep_node_info_to_mapping(ninfo, &rec);

        /* Register services sharing the same uuid but using different port, encode in maj. */
        max_port  = Platform::plf_get_my_max_svc_ports();
        base_port = ninfo->nd_base_port;

        for (i = 0; i < max_port; i++)
        {
            idx = map->ep_map_record(&rec);
            LOGDEBUG << "Platform daemon binds " << t << ":" << std::hex << svc.uuid_get_val() <<
            "@" << ninfo->nd_ip_addr << ":" << std::dec << rec.rmp_major << ":" <<
            rec.rmp_minor <<
            " idx " << idx;

            rec.rmp_minor = Platform::plat_svc_types[i];

            if ((t == fpi::FDSP_STOR_HVISOR) && (rec.rmp_minor == NET_SVC_CONFIG))
            {
                /* Hard-coded to bind to Java endpoint in AM. */
                ep_map_set_port_info(&rec, 8999);
            }else {
                ep_map_set_port_info(&rec, base_port + rec.rmp_minor);
            }
        }

        /* Restore everything back to ninfo for the next loop */
        ninfo->nd_base_port = saved_port;
        ninfo->nd_node_uuid = saved_uuid;
    }
}  // namespace fds
