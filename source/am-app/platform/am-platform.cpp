/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <string>
#include <net/net_utils.h>
#include <am-platform.h>
#include <net/PlatNetSvcHandler.h>
#include <net/net-service-tmpl.hpp>
#include <net/SvcRequestPool.h>
#include <AMSvcHandler.h>

namespace fds {

AmPlatform gl_AmPlatform;

/*
 * -----------------------------------------------------------------------------------
 * Endpoint Plugin
 * -----------------------------------------------------------------------------------
 */
AMEpPlugin::~AMEpPlugin() {}
AMEpPlugin::AMEpPlugin(AmPlatform *am_plat) : EpEvtPlugin(), am_plat_(am_plat) {}

// ep_connected
// ------------
//
void
AMEpPlugin::ep_connected()
{
}

// ep_done
// -------
//
void
AMEpPlugin::ep_down()
{
}

// svc_up
// ------
//
void
AMEpPlugin::svc_up(EpSvcHandle::pointer handle)
{
}

// svc_down
// --------
//
void
AMEpPlugin::svc_down(EpSvc::pointer svc, EpSvcHandle::pointer handle)
{
}
// -------------------------------------------------------------------------------------
// DM Specific Platform
// -------------------------------------------------------------------------------------
AmPlatform::AmPlatform()
    : Platform("DM-Platform",
               fpi::FDSP_STOR_HVISOR,
               new DomainContainer("AM-Platform-NodeInv",
                                 NULL,
                                 new SmContainer(fpi::FDSP_STOR_MGR),
                                 new DmContainer(fpi::FDSP_DATA_MGR),
                                 new AmContainer(fpi::FDSP_STOR_HVISOR),
                                 new PmContainer(fpi::FDSP_PLATFORM),
                                 new OmContainer(fpi::FDSP_ORCH_MGR)),
               new DomainClusterMap("AM-Platform-ClusMap",
                                 NULL,
                                 new SmContainer(fpi::FDSP_STOR_MGR),
                                 new DmContainer(fpi::FDSP_DATA_MGR),
                                 new AmContainer(fpi::FDSP_STOR_HVISOR),
                                 new PmContainer(fpi::FDSP_PLATFORM),
                                 new OmContainer(fpi::FDSP_ORCH_MGR)),
               new DomainResources("AM-Resources"),
               NULL) {}

// mod_init
// --------
// Values from configuration file for platform library linked with AM typed apps where
// we may not have shared memory with the platform daemon.  AM typed apps are stand
// alone unit tests or probe type, checker...
//
int
AmPlatform::mod_init(SysParams const *const param)
{
    fds_uint32_t base;

    Platform::platf_assign_singleton(&gl_AmPlatform);
    plf_node_type = fpi::FDSP_STOR_HVISOR;

    FdsConfigAccessor conf(g_fdsprocess->get_conf_helper());
    Platform::mod_init(param);

    plf_my_ip        = net::get_local_ip(conf.get_abs<std::string>("fds.nic_if"));
    plf_my_node_name = plf_my_ip;

    return 0;
}

void
AmPlatform::mod_startup()
{
    NetPlatform *net;

    Platform::mod_startup();
    gSvcRequestPool = new SvcRequestPool();

    am_recv   = bo::shared_ptr<AMSvcHandler>(new AMSvcHandler());
    am_plugin = new AMEpPlugin(this);
    am_ep     = new EndPoint<fpi::AMSvcClient, fpi::AMSvcProcessor>(
        Platform::platf_singleton()->plf_get_my_base_port(),
        *Platform::platf_singleton()->plf_get_my_svc_uuid(),
        NodeUuid(0ULL),
        bo::shared_ptr<fpi::AMSvcProcessor>(new fpi::AMSvcProcessor(am_recv)),
        am_plugin);

    net = NetPlatform::nplat_singleton();
    net->nplat_set_my_ep(am_ep);

    LOGNORMAL << " my_svc_uuid: " << *Platform::platf_singleton()->plf_get_my_svc_uuid()
        << " port: " << Platform::platf_singleton()->plf_get_my_base_port();
}

// mod_enable_service
// ------------------
//
void
AmPlatform::mod_enable_service()
{
    NetMgr::ep_mgr_singleton()->ep_register(am_ep, false);
    Platform::mod_enable_service();
}

void
AmPlatform::mod_shutdown()
{
}

boost::shared_ptr<BaseAsyncSvcHandler>
AmPlatform::getBaseAsyncSvcHandler()
{
    return am_recv;
}

}  // namespace fds
