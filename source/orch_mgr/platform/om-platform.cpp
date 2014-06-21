/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <om-platform.h>
#include <fds_process.h>
#include <OmResources.h>
#include <net/net-service-tmpl.hpp>

namespace fds {

OmPlatform gl_OmPlatform;

/*
 * -----------------------------------------------------------------------------------
 * Endpoint Plugin
 * -----------------------------------------------------------------------------------
 */
OMEpPlugin::~OMEpPlugin() {}
OMEpPlugin::OMEpPlugin(OmPlatform *om_plat) : EpEvtPlugin(), om_plat_(om_plat) {}

// ep_connected
// ------------
//
void
OMEpPlugin::ep_connected()
{
}

// ep_done
// -------
//
void
OMEpPlugin::ep_down()
{
}

// svc_up
// ------
//
void
OMEpPlugin::svc_up(EpSvcHandle::pointer handle)
{
}

// svc_down
// --------
//
void
OMEpPlugin::svc_down(EpSvc::pointer svc, EpSvcHandle::pointer handle)
{
}

// -------------------------------------------------------------------------------------
// OM Specific Platform
// -------------------------------------------------------------------------------------
OmPlatform::OmPlatform()
    : Platform("OM-Platform",
               FDSP_ORCH_MGR,
               new OM_NodeContainer(),
               new DomainClusterMap("OM-Platform-ClusMap",
                                 NULL,
                                 new SmContainer(fpi::FDSP_STOR_MGR),
                                 new DmContainer(fpi::FDSP_DATA_MGR),
                                 new AmContainer(fpi::FDSP_STOR_HVISOR),
                                 new PmContainer(fpi::FDSP_PLATFORM),
                                 new OmContainer(fpi::FDSP_ORCH_MGR)),
               new DomainResources("OM-Resources"),
               NULL) {}

/**
 * The way AM and OM get their ports are different from SM/DM; therefore we can't use
 * the base class method to get OM & AM port numbers.
 */
int
OmPlatform::mod_init(SysParams const *const param)
{
    fds_uint32_t base;
    FdsConfigAccessor conf(g_fdsprocess->get_conf_helper());

    Platform::platf_assign_singleton(&gl_OmPlatform);
    plf_node_type = FDSP_ORCH_MGR;
    Platform::mod_init(param);

    plf_my_ip        = util::get_local_ip();
    plf_my_node_name = plf_my_ip;

    LOGNORMAL << "My ctrl port " << plf_get_my_ctrl_port()
        << ", data port " << plf_get_my_data_port() << ", OM ip: " << plf_om_ip_str;

    return 0;
}

void
OmPlatform::mod_startup()
{
    Platform::mod_startup();
#if 0
    om_recv   = bo::shared_ptr<OMSvcHandler>(new OMSvcHandler());
    om_plugin = new OMEpPlugin(this);
    om_ep     = new EndPoint<fpi::OMSvcClient, fpi::OMSvcProcessor>(
        Platform::platf_singleton()->plf_get_om_port(),
        *Platform::platf_singleton()->plf_get_my_svc_uuid(),
        NodeUuid(0ULL),
        bo::shared_ptr<fpi::OMSvcProcessor>(new fpi::OMSvcProcessor(om_recv)),
        om_plugin);

    LOGNORMAL << "Startup platform specific net svc, port "
              << Platform::platf_singleton()->plf_get_om_port();
#endif
}

// mod_enable_service
// ------------------
//
void
OmPlatform::mod_enable_service()
{
    // NetMgr::ep_mgr_singleton()->ep_register(om_ep, false);
    Platform::mod_enable_service();
}

void
OmPlatform::mod_shutdown()
{
    Platform::mod_shutdown();
}


// Factory methods required for OM RPC.
//
PlatRpcReqt *
OmPlatform::plat_creat_reqt_disp()
{
    return nullptr;
}

PlatRpcResp *
OmPlatform::plat_creat_resp_disp()
{
    return nullptr;
}

PlatDataPathResp *
OmPlatform::plat_creat_dpath_resp()
{
    return nullptr;
}

}  // namespace fds
