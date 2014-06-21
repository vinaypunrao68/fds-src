/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <sm-platform.h>
#include <fds_process.h>
#include <fdsp/SMSvc.h>
#include <SMSvcHandler.h>
#include <net/net-service-tmpl.hpp>
#include <net/RpcRequestPool.h>
#include <platform/platform-lib.h>

namespace fds {

SmPlatform gl_SmPlatform;

/* SM specific flags */
DBG(DEFINE_FLAG(sm_drop_gets));
DBG(DEFINE_FLAG(sm_drop_puts));


// -------------------------------------------------------------------------------------
// SM Platform Event Handler
// -------------------------------------------------------------------------------------
void
SmVolEvent::plat_evt_handler(const FDSP_MsgHdrTypePtr &hdr)
{
}

/*
 * -----------------------------------------------------------------------------------
 * Endpoint Plugin
 * -----------------------------------------------------------------------------------
 */
SMEpPlugin::~SMEpPlugin() {}
SMEpPlugin::SMEpPlugin(SmPlatform *sm_plat) : EpEvtPlugin(), sm_plat_(sm_plat) {}

// ep_connected
// ------------
//
void
SMEpPlugin::ep_connected()
{
}

// ep_done
// -------
//
void
SMEpPlugin::ep_down()
{
}

// svc_up
// ------
//
void
SMEpPlugin::svc_up(EpSvcHandle::pointer handle)
{
}

// svc_down
// --------
//
void
SMEpPlugin::svc_down(EpSvc::pointer svc, EpSvcHandle::pointer handle)
{
}
// -------------------------------------------------------------------------------------
// SM Specific Platform
// -------------------------------------------------------------------------------------
SmPlatform::SmPlatform()
    : Platform("SM-Platform",
               FDSP_STOR_MGR,
               new DomainNodeInv("SM-Platform-NodeInv",
                                 NULL,
                                 new SmContainer(FDSP_STOR_MGR),
                                 new DmContainer(FDSP_STOR_MGR),
                                 new AmContainer(FDSP_STOR_MGR),
                                 new PmContainer(FDSP_STOR_MGR),
                                 new OmContainer(FDSP_STOR_MGR)),
               new DomainClusterMap("SM-Platform-ClusMap",
                                 NULL,
                                 new SmContainer(FDSP_STOR_MGR),
                                 new DmContainer(FDSP_STOR_MGR),
                                 new AmContainer(FDSP_STOR_MGR),
                                 new PmContainer(FDSP_STOR_MGR),
                                 new OmContainer(FDSP_STOR_MGR)),
               new DomainResources("DM-Resources"),
               NULL) {}

int
SmPlatform::mod_init(SysParams const *const param)
{
    FdsConfigAccessor conf(g_fdsprocess->get_conf_helper());

    plf_node_type  = FDSP_STOR_MGR;
    Platform::platf_assign_singleton(&gl_SmPlatform);
    Platform::mod_init(param);

    plf_my_ip        = util::get_local_ip();
    plf_my_node_name = "my-sm-node";  // plf_my_ip;

    plf_vol_evt  = new SmVolEvent(plf_resources, plf_clus_map, this);
    plf_node_evt = new NodePlatEvent(plf_resources, plf_clus_map, this);

    return 0;
}

void
SmPlatform::mod_startup()
{
    Platform::mod_startup();
    registerFlags();
    gRpcRequestPool = new RpcRequestPool();

    sm_recv   = bo::shared_ptr<SMSvcHandler>(new SMSvcHandler());
    sm_plugin = new SMEpPlugin(this);
    sm_ep     = new EndPoint<fpi::SMSvcClient, fpi::SMSvcProcessor>(
        Platform::platf_singleton()->plf_get_my_base_port(),
        *Platform::platf_singleton()->plf_get_my_svc_uuid(),
        NodeUuid(0ULL),
        bo::shared_ptr<fpi::SMSvcProcessor>(new fpi::SMSvcProcessor(sm_recv)),
        sm_plugin);

    LOGNORMAL << " my_svc_uuid: " << *Platform::platf_singleton()->plf_get_my_svc_uuid()
        << " port: " << Platform::platf_singleton()->plf_get_my_base_port();
}

// mod_enable_service
// ------------------
//
void
SmPlatform::mod_enable_service()
{
    NetMgr::ep_mgr_singleton()->ep_register(sm_ep, false);
    Platform::mod_enable_service();
}

void
SmPlatform::mod_shutdown()
{
}

// plf_reg_node_info
// -----------------
//
void
SmPlatform::plf_reg_node_info(const NodeUuid &uuid, const FdspNodeRegPtr msg)
{
    NodeAgent::pointer     new_node;
    DomainNodeInv::pointer local;

    local = plf_node_inventory();
    Error err = local->dc_register_node(uuid, msg, &new_node);
    fds_verify(err == ERR_OK);

    AgentContainer::pointer svc = local->dc_container_frm_msg(msg->node_type);
    svc->agent_handshake(plf_net_sess, plf_dpath_resp, new_node);
}

PlatRpcReqt *
SmPlatform::plat_creat_reqt_disp()
{
    return NULL;
}

PlatRpcResp *
SmPlatform::plat_creat_resp_disp()
{
    return NULL;
}

PlatDataPathResp *
SmPlatform::plat_creat_dpath_resp()
{
    return NULL;
}

/**
* @brief Register sm flags
*/
void SmPlatform::registerFlags()
{
    PlatformProcess::plf_manager()->\
        plf_get_flags_map().registerCommonFlags();
    /* SM specific flags */
    DBG(REGISTER_FLAG(PlatformProcess::plf_manager()->\
                  plf_get_flags_map(), sm_drop_gets));
    DBG(REGISTER_FLAG(PlatformProcess::plf_manager()->\
                  plf_get_flags_map(), sm_drop_puts));
}

}  // namespace fds
