/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <string>
#include <net/SvcRequestPool.h>
#include <platform/platform-lib.h>
#include <platform/node-inv-shmem.h>
#include <platform/service-ep-lib.h>
#include <NetSession.h>

bool    gdb_plat = false;

namespace fds
{

    Platform    *gl_PlatformSvc;

    const int    Platform::plat_svc_types[] = {
        NET_SVC_CTRL, NET_SVC_CONFIG, NET_SVC_DATA, NET_SVC_MIGRATION, NET_SVC_META_SYNC
    };

    // plf_get_my_max_svc_ports
    // ------------------------
    //
    fds_uint32_t Platform::plf_get_my_max_svc_ports()
    {
        return sizeof(plat_svc_types) / sizeof(int);  // NOLINT
    }

    // -------------------------------------------------------------------------------------
    // Common Platform Services
    // -------------------------------------------------------------------------------------
    Platform::Platform(char const *const name, FDSP_MgrIdType node_type,
                       DomainContainer::pointer node_inv, DomainClusterMap::pointer cluster,
                       DomainResources::pointer resources,
                       OmAgent::pointer master) : Module(name), plf_node_type(node_type),
        plf_domain(NULL),
        plf_master(master), plf_node_inv(node_inv), plf_clus_map(cluster), plf_resources(resources)
    {
        plf_om_ctrl_port = 0;
        plf_net_sess     = NULL;
    }

    Platform::~Platform()
    {
    }

    // -----------------------------------------------------------------------------------
    // Platform node/cluster methods.
    // -----------------------------------------------------------------------------------

    // plf_reg_node_info
    // -----------------
    //
    void Platform::plf_reg_node_info(const NodeUuid &uuid, const FdspNodeRegPtr msg)
    {
        NodeAgent::pointer          new_node;
        DomainContainer::pointer    local;

        local = plf_node_inventory();

        Error                       err = local->dc_register_node(uuid, msg, &new_node);

        if (err != ERR_OK)
        {
            fds_verify(0);
        }
    }

    // plf_del_node_info
    // -----------------
    //
    void Platform::plf_del_node_info(const NodeUuid &uuid, const std::string &name)
    {
        DomainContainer::pointer    local;

        local = plf_node_inventory();
        Error                       err = local->dc_unregister_node(uuid, name);
    }

    // plf_update_cluster
    // ------------------
    //
    void Platform::plf_update_cluster()
    {
    }

    // prf_rpc_om_handshake
    // --------------------
    // Perform the handshake connection with OM.
    //
    void Platform::plf_rpc_om_handshake(fpi::FDSP_RegisterNodeTypePtr reg)
    {
        fds_verify(plf_master != NULL);

        plf_master->om_handshake(plf_net_sess, plf_om_ip_str, plf_om_ctrl_port);
        plf_master->init_node_reg_pkt(reg);
        plf_master->om_register_node(reg);
    }

    // plf_is_om_node
    // --------------
    // Return true if this is the node running the primary OM node.
    //
    bool Platform::plf_is_om_node()
    {
        // Do simple IP compare my IP with OM ip.
        // Until we can get rid of the old endpoint using plf_my_ctrl_port, use data
        // port for net service update protocols.
        // HACK(Vy) - we need better mechanism!
        //
        if ((*plf_get_om_ip() == "127.0.0.1") || (*plf_get_om_ip() == "localhost"))
        {
            /* Assume same node testing, use port info. */
            if (plf_get_om_svc_port() == plf_get_my_node_port())
            {
                return true;
            }
        }

        if ((*plf_get_my_ip() == *plf_get_om_ip()) &&
            (plf_get_om_svc_port() == plf_get_my_node_port()))
        {
            return true;
        }
        return false;
    }

    // plf_change_info
    // ---------------
    //
    void Platform::plf_change_info(const plat_node_data_t *ndata)
    {
        char            name[64];
        NodeUuid        uuid(ndata->nd_node_uuid);
        fds_uint32_t    base;

        if (ndata->nd_node_uuid == 0)
        {
            // TODO(Vy): we need to think about if AM needs to persist its own node/service
            // uuid or can it generate only the fly during its bootstrap process.
            // When we're here and we don't have uuid, this must be AM or running
            // in a test mode where there's no OM/platform.
            plf_my_uuid.uuid_set_type(fds_get_uuid64(get_uuid()), fpi::FDSP_PLATFORM);
        }else {
            plf_my_uuid.uuid_set_type(uuid.uuid_get_val(), fpi::FDSP_PLATFORM);
        }

        Platform::plf_svc_uuid_from_node(plf_my_uuid, &plf_my_svc_uuid, plf_node_type);

        base = ndata->nd_plat_port;

        if (base == 0)
        {
            base = plf_my_node_port;
        }

        plf_my_base_port = Platform::plf_svc_port_from_node(base, plf_node_type);

        // snprintf(name, sizeof(name), "node-%u", ndata->nd_node_number);
        // plf_my_node_name.assign(name);

        LOGNOTIFY << "My node port " << std::dec << plf_my_node_port
                  << "\nMy base port " << plf_my_base_port
                  << "\nMy ctrl port " << plf_get_my_ctrl_port()
                  << "\nMy conf port " << plf_get_my_conf_port()
                  << "\nMy data port " << plf_get_my_data_port()
                  << "\nMy migr port " << plf_get_my_migration_port()
                  << "\nMy metasync port " << plf_get_my_metasync_port()
                  << "\nMy node uuid " << std::hex << plf_my_uuid.uuid_get_val()
                  << "\nMy OM port   " << std::dec << plf_get_om_ctrl_port()
                  << "\nMy OM platform port " << plf_get_om_svc_port()
                  << "\nMy OM IP     " << plf_om_ip_str
                  << "\nMy IP        " << *plf_get_my_ip()
                  << "\nMy node port " << plf_get_my_node_port()
                  << std::endl;
    }

    // plf_svc_uuid_from_node
    // ----------------------
    // Simple formula to derrive service uuid from node uuid.
    //
    /* static */ void Platform::plf_svc_uuid_from_node(const NodeUuid      &node,
                                                       NodeUuid            *svc,
                                                       fpi::FDSP_MgrIdType type)
    {
        if (type == fpi::FDSP_PLATFORM)
        {
            svc->uuid_set_val(node.uuid_get_val());
            fds_assert(svc->uuid_get_type() == fpi::FDSP_PLATFORM);
        }else {
            svc->uuid_set_type(node.uuid_get_val(), type);
        }
    }

    // plf_svc_uuid_to_node
    // --------------------
    // The formular to convert back node uuid from a known service uuid.
    //
    /* static */ void Platform::plf_svc_uuid_to_node(NodeUuid             *node,
                                                     const NodeUuid       &svc,
                                                     fpi::FDSP_MgrIdType type)
    {
        if (type == fpi::FDSP_PLATFORM)
        {
            node->uuid_set_val(svc.uuid_get_val());
        }else {
            node->uuid_set_val(svc.uuid_get_val());
        }
        fds_assert(node->uuid_get_type() == fpi::FDSP_PLATFORM);
    }

    // plf_svc_port_from_node
    // ----------------------
    //
    /* static */ int Platform::plf_svc_port_from_node(int platform, fpi::FDSP_MgrIdType type)
    {
        if (type == fpi::FDSP_PLATFORM)
        {
            return platform;
        }
        return platform + (10 * (type + 1));
    }

    // -----------------------------------------------------------------------------------
    // Module methods
    // -----------------------------------------------------------------------------------
    int Platform::mod_init(SysParams const *const param)
    {
        Module::mod_init(param);

        plf_net_sess   = boost::shared_ptr<netSessionTbl>(new netSessionTbl(FDSP_PLATFORM));

        FdsConfigAccessor    conf(g_fdsprocess->get_conf_helper());
        plf_my_node_port = conf.get_abs<int>("fds.plat.platform_port");
        plf_my_base_port = Platform::plf_svc_port_from_node(plf_my_node_port, plf_node_type);

        plf_om_ip_str    = conf.get_abs<std::string>("fds.plat.om_ip");
        plf_om_ctrl_port = conf.get_abs<int>("fds.plat.om_port");
        plf_om_svc_port  = conf.get_abs<int>("fds.plat.om_platform_port");

        LOGNOTIFY << "om_ip: " << plf_om_ip_str << " ctrl port: " << plf_om_ctrl_port <<
        " svc port: " << plf_om_svc_port;
        return 0;
    }

    void Platform::mod_startup()
    {
        gSvcRequestPool = new SvcRequestPool();
        Module::mod_startup();
    }

    void Platform::mod_enable_service()
    {
        NodeShmCtrl::shm_ctrl_singleton()->shm_start_consumer_thr(plf_node_type);
        Module::mod_enable_service();
    }

    void Platform::mod_shutdown()
    {
        Module::mod_shutdown();
    }

    /**
     * ---------------------------------------------------------------------------------
     * OM Agent Event Handlers
     * ---------------------------------------------------------------------------------
     */
    OmNodeAgentEvt::~OmNodeAgentEvt()
    {
    }

    OmNodeAgentEvt::OmNodeAgentEvt(NodeAgent::pointer agent) : NodeAgentEvt(agent)
    {
    }

    // ep_connected
    // ------------
    //
    void OmNodeAgentEvt::ep_connected()
    {
        LOGDEBUG << "[Plat] OM agent is connected";
    }

    // ep_down
    // -------
    //
    void OmNodeAgentEvt::ep_down()
    {
    }

    // svc_up
    // ------
    //
    void OmNodeAgentEvt::svc_up(EpSvcHandle::pointer eph)
    {
    }

    // svc_down
    // --------
    //
    void OmNodeAgentEvt::svc_down(EpSvc::pointer svc, EpSvcHandle::pointer eph)
    {
    }

    /* *
     * ---------------------------------------------------------------------------------
     * Common factory methods.
     * ---------------------------------------------------------------------------------
     */

    // plat_new_pm_svc
    // ---------------
    //
    PmSvcEp::pointer Platform::plat_new_pm_svc(NodeAgent::pointer agt, fds_uint32_t maj,
                                               fds_uint32_t min)
    {
        return new PmSvcEp(agt, maj, min, new NodeAgentEvt(agt));
    }

    // plat_new_om_svc
    // ---------------
    //
    OmSvcEp::pointer Platform::plat_new_om_svc(NodeAgent::pointer agt, fds_uint32_t maj,
                                               fds_uint32_t min)
    {
        return new OmSvcEp(agt, maj, min, new OmNodeAgentEvt(agt));
    }

    // plat_new_sm_svc
    // ---------------
    //
    SmSvcEp::pointer Platform::plat_new_sm_svc(NodeAgent::pointer agt, fds_uint32_t maj,
                                               fds_uint32_t min)
    {
        return new SmSvcEp(agt, maj, min, new NodeAgentEvt(agt));
    }

    // plat_new_dm_svc
    // ---------------
    //
    DmSvcEp::pointer Platform::plat_new_dm_svc(NodeAgent::pointer agt, fds_uint32_t maj,
                                               fds_uint32_t min)
    {
        return new DmSvcEp(agt, maj, min, new NodeAgentEvt(agt));
    }

    // plat_new_am_svc
    // ---------------
    //
    AmSvcEp::pointer Platform::plat_new_am_svc(NodeAgent::pointer agt, fds_uint32_t maj,
                                               fds_uint32_t min)
    {
        return new AmSvcEp(agt, maj, min, new NodeAgentEvt(agt));
    }
}  // namespace fds
