/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <dm-platform.h>
#include <fds_process.h>
#include <fdsp/DMSvc.h>
#include <DMSvcHandler.h>
#include <net/net_utils.h>
#include <net/net-service-tmpl.hpp>
#include <net/SvcRequestPool.h>
#include <platform/platform-lib.h>

namespace fds {

DmPlatform gl_DmPlatform;

/* DM specific flags */
DBG(DEFINE_FLAG(dm_drop_cat_queries));
DBG(DEFINE_FLAG(dm_drop_cat_updates));

/*
 * -----------------------------------------------------------------------------------
 * Endpoint Plugin
 * -----------------------------------------------------------------------------------
 */
DMEpPlugin::~DMEpPlugin() {}
DMEpPlugin::DMEpPlugin(DmPlatform *dm_plat) : EpEvtPlugin(), dm_plat_(dm_plat) {}

// ep_connected
// ------------
//
void
DMEpPlugin::ep_connected()
{
}

// ep_done
// -------
//
void
DMEpPlugin::ep_down()
{
}

// svc_up
// ------
//
void
DMEpPlugin::svc_up(EpSvcHandle::pointer handle)
{
}

// svc_down
// --------
//
void
DMEpPlugin::svc_down(EpSvc::pointer svc, EpSvcHandle::pointer handle)
{
}

// -------------------------------------------------------------------------------------
// DM Specific Platform
// -------------------------------------------------------------------------------------
DmPlatform::DmPlatform()
    : Platform("DM-Platform",
               fpi::FDSP_DATA_MGR,
               new DomainContainer("DM-Platform-NodeInv",
                                 NULL,
                                 new SmContainer(fpi::FDSP_STOR_MGR),
                                 new DmContainer(fpi::FDSP_DATA_MGR),
                                 new AmContainer(fpi::FDSP_STOR_HVISOR),
                                 new PmContainer(fpi::FDSP_PLATFORM),
                                 new OmContainer(fpi::FDSP_ORCH_MGR)),
               new DomainClusterMap("DM-Platform-ClusMap",
                                 NULL,
                                 new SmContainer(fpi::FDSP_STOR_MGR),
                                 new DmContainer(fpi::FDSP_DATA_MGR),
                                 new AmContainer(fpi::FDSP_STOR_HVISOR),
                                 new PmContainer(fpi::FDSP_PLATFORM),
                                 new OmContainer(fpi::FDSP_ORCH_MGR)),
               new DomainResources("DM-Resources"),
               NULL) {}

int
DmPlatform::mod_init(SysParams const *const param)
{
    FdsConfigAccessor conf(g_fdsprocess->get_conf_helper());

    Platform::platf_assign_singleton(&gl_DmPlatform);
    plf_node_type = fpi::FDSP_DATA_MGR;
    Platform::mod_init(param);

    plf_my_ip        = net::get_local_ip(conf.get_abs<std::string>("fds.nic_if"));
    plf_my_node_name = plf_my_ip;

    LOGNORMAL << "My ctrl port " << plf_get_my_ctrl_port()
        << ", data port " << plf_get_my_data_port() << ", OM ip: " << plf_om_ip_str
        << ", meta sync server port " << plf_get_my_metasync_port();

    return 0;
}

void
DmPlatform::mod_startup()
{
    NetPlatform *net;

    Platform::mod_startup();
    registerFlags();

    dm_recv   = bo::shared_ptr<DMSvcHandler>(new DMSvcHandler());
    dm_plugin = new DMEpPlugin(this);
    dm_ep     = new EndPoint<fpi::DMSvcClient, fpi::DMSvcProcessor>(
        Platform::platf_singleton()->plf_get_my_base_port(),
        *Platform::platf_singleton()->plf_get_my_svc_uuid(),
        NodeUuid(0ULL),
        bo::shared_ptr<fpi::DMSvcProcessor>(new fpi::DMSvcProcessor(dm_recv)),
        dm_plugin);

    net = NetPlatform::nplat_singleton();
    net->nplat_set_my_ep(dm_ep);

    LOGNORMAL << "Startup platform specific net svc, port "
              << Platform::platf_singleton()->plf_get_my_base_port();
}

// mod_enable_service
// ------------------
//
void
DmPlatform::mod_enable_service()
{
    NetMgr::ep_mgr_singleton()->ep_register(dm_ep, false);
    Platform::mod_enable_service();
}

void
DmPlatform::mod_shutdown()
{
    Platform::mod_shutdown();
}

// plf_reg_node_info
// -----------------
//
void
DmPlatform::plf_reg_node_info(const NodeUuid &uuid, const FdspNodeRegPtr msg)
{
    NodeAgent::pointer       new_node;
    DomainContainer::pointer local;

    local = plf_node_inventory();
    Error err = local->dc_register_node(uuid, msg, &new_node);
    if (err != ERR_OK) {
        fds_verify(0);
    }
    AgentContainer::pointer svc = local->dc_container_frm_msg(msg->node_type);
    svc->agent_handshake(plf_net_sess, new_node);
}

boost::shared_ptr<BaseAsyncSvcHandler>
DmPlatform::getBaseAsyncSvcHandler()
{
    return dm_recv;
}

/**
* @brief Register dm flags
*/
void DmPlatform::registerFlags()
{
    PlatformProcess::plf_manager()->\
        plf_get_flags_map().registerCommonFlags();
    /* DM specific flags */
    DBG(REGISTER_FLAG(PlatformProcess::plf_manager()->\
                  plf_get_flags_map(), dm_drop_cat_queries));
    DBG(REGISTER_FLAG(PlatformProcess::plf_manager()->\
                  plf_get_flags_map(), dm_drop_cat_updates));
}

}  // namespace fds
