/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <string>
#include <vector>
#include <map>
#include <ep-map.h>
#include <fds_process.h>
#include <net/net-service-tmpl.hpp>
#include <platform/platform-lib.h>
#include <platform/node-inv-shmem.h>
#include <net-plat-shared.h>

namespace fds {

NetPlatSvc                   gl_NetPlatform("NetPlatform");
NetPlatform                 *gl_NetPlatSvc = &gl_NetPlatform;

/*
 * -----------------------------------------------------------------------------------
 * Exported module
 * -----------------------------------------------------------------------------------
 */
NetPlatform::NetPlatform(const char *name) : Module(name)
{
    static Module *net_plat_deps[] = {
        NULL
    };
    netmgr     = NULL;
    plat_lib   = NULL;
    mod_intern = net_plat_deps;
}

NetPlatSvc::~NetPlatSvc() {}
NetPlatSvc::NetPlatSvc(const char *name) : NetPlatform(name)
{
    plat_ep        = NULL;
    plat_ep_plugin = NULL;
    plat_ep_hdler  = NULL;
    plat_agent     = NULL;
}

int
NetPlatSvc::mod_init(SysParams const *const p)
{
    netmgr   = NetMgr::ep_mgr_singleton();
    plat_lib = Platform::platf_singleton();
    return Module::mod_init(p);
}

void
NetPlatSvc::mod_startup()
{
    Module::mod_startup();

    // Create the agent and register the ep
    plat_ep_plugin = new PlatNetPlugin(this);
    plat_agent     = new DomainAgent(plat_lib->plf_my_node_uuid());
    plat_ep_hdler  = bo::shared_ptr<NetPlatHandler>(new NetPlatHandler(this));
    plat_ep        = new PlatNetEp(
            plat_lib->plf_get_my_nsvc_port(),
            *plat_lib->plf_get_my_plf_svc_uuid(), /* bind to my platform lib svc  */
            NodeUuid(0ULL),                       /* pure server mode */
            bo::shared_ptr<fpi::PlatNetSvcProcessor>(
                new fpi::PlatNetSvcProcessor(plat_ep_hdler)),
            plat_ep_plugin);

    LOGNORMAL << "startup shared platform net svc, port "
              << plat_lib->plf_get_my_nsvc_port() << std::hex
              << plat_lib->plf_get_my_plf_svc_uuid()->uuid_get_val();
}

void
NetPlatSvc::mod_enable_service()
{
    Module::mod_enable_service();

    // Regiser my node endpoint.
    netmgr->ep_register(plat_ep, false);
    if (!plat_lib->plf_is_om_node()) {
        plat_agent->pda_connect_domain(fpi::DomainID());
    }
}

void
NetPlatSvc::mod_shutdown()
{
    Module::mod_shutdown();
}

// nplat_domain_rpc
// ----------------
// Get the RPC handles needed to contact the master platform services.
//
EpSvcHandle::pointer
NetPlatSvc::nplat_domain_rpc(const fpi::DomainID &id)
{
    return plat_agent->pda_rpc_handle();
}

// nplat_my_ep
// -----------
//
EpSvc::pointer
NetPlatSvc::nplat_my_ep()
{
    return plat_ep;
}

// nplat_register_node
// -------------------
// Platform lib which linked to FDS daemon registers node inventory based on RO data
// from the shared memory segment.
//
void
NetPlatSvc::nplat_register_node(const fpi::NodeInfoMsg *msg)
{
    int                     idx;
    node_data_t             rec;
    ep_map_rec_t            map;
    NodeAgent::pointer      agent;
    DomainNodeInv::pointer  local;

    NodeInventory::node_info_msg_to_shm(msg, &rec);
    EpPlatLibMod::ep_node_info_to_mapping(&rec, &map);

    /* Search through shm for matching key, return back the index */
    idx = 0;
    NetMgr::ep_mgr_singleton()->ep_register_binding(&map, idx);

    local = Platform::platf_singleton()->plf_node_inventory();
    // local->dc_register_node(shm, &agent, idx, -1);
}

/*
 * -----------------------------------------------------------------------------------
 * Domain Agent
 *
 * This obj is also platform agent (DmAgent) but it has special methods to communicate
 * with "the" domain master.  In the future, domain master can move to other node but
 * the interface from this obj remains unchanged.  It always knows how to talk to "the"
 * domain master.
 * -----------------------------------------------------------------------------------
 */
DomainAgent::DomainAgent(const NodeUuid &uuid, bool alloc_plugin)
    : PmAgent(uuid), agt_domain_evt(NULL), agt_domain_ep(NULL)
{
    if (alloc_plugin == true) {
        agt_domain_evt = new DomainAgentPlugin(this);
    }
}

/**
 * pda_connect_domain
 * ------------------
 * Establish connection to the domain master.  Null domain implies local one.
 */
void
DomainAgent::pda_connect_domain(const fpi::DomainID &id)
{
    int                port;
    NetPlatform       *net;
    PlatNetEpPtr       eptr;

    fds_verify(agt_domain_evt != NULL);
    if (agt_domain_ep != NULL) {
        return;
    }
    net  = NetPlatform::nplat_singleton();
    eptr = ep_cast_ptr<PlatNetEp>(net->nplat_my_ep());
    fds_verify(eptr != NULL);

    std::string const *const om_ip = net->nplat_domain_master(&port);
    eptr->ep_new_handle(eptr, port, *om_ip, &agt_domain_ep, agt_domain_evt);
    LOGNORMAL << "Domain master ip " << *om_ip << ", port " << port;
}

/**
 * ep_connected
 * ------------
 * This is called when the domain agent establihed connection to the domain master.
 */
void
DomainAgentPlugin::ep_connected()
{
}

/**
 * ep_down
 * -------
 */
void
DomainAgentPlugin::ep_down()
{
}

/**
 * svc_up
 * ------
 */
void
DomainAgentPlugin::svc_up(EpSvcHandle::pointer handle)
{
}

/**
 * svc_down
 * --------
 */
void
DomainAgentPlugin::svc_down(EpSvc::pointer svc, EpSvcHandle::pointer handle)
{
}

/*
 * -----------------------------------------------------------------------------------
 * Endpoint Plugin
 * -----------------------------------------------------------------------------------
 */
PlatNetPlugin::PlatNetPlugin(NetPlatSvc *svc) : plat_svc(svc) {}

void
PlatNetPlugin::ep_connected()
{
}

void
PlatNetPlugin::ep_down()
{
}

void
PlatNetPlugin::svc_up(EpSvcHandle::pointer handle)
{
}

void
PlatNetPlugin::svc_down(EpSvc::pointer svc, EpSvcHandle::pointer handle)
{
}

/*
 * -----------------------------------------------------------------------------------
 * RPC Handlers
 * -----------------------------------------------------------------------------------
 */
void
NetPlatHandler::allUuidBinding(std::vector<fpi::UuidBindMsg>    &ret,
                               bo::shared_ptr<fpi::UuidBindMsg> &msg,
                               bo::shared_ptr<bool>             &all_list)
{
    std::cout << "all uuidBind there" << std::endl;
}

void
NetPlatHandler::notifyNodeInfo(std::vector<fpi::NodeInfoMsg>    &ret,
                               bo::shared_ptr<fpi::NodeInfoMsg> &inf)
{
}

void
NetPlatHandler::notifyNodeUp(fpi::RespHdr &ret, bo::shared_ptr<fpi::NodeInfoMsg> &info)
{
}

/**
 * Returns the status of the service
 * @param nullarg
 * @return
 */
fpi::ServiceStatus NetPlatHandler::
getStatus(boost::shared_ptr<int32_t>& nullarg)  // NOLINT
{
    if (!g_fdsprocess)
        return fpi::SVC_STATUS_INVALID;
    return fpi::SVC_STATUS_ACTIVE;
}

/**
 * Returns the stats associated with counters identified by id
 * @param _return
 * @param id
 */
void NetPlatHandler::getCounters(std::map<std::string, int64_t> & _return,
        boost::shared_ptr<std::string>& id)
{
    if (!g_fdsprocess) return;

    auto cntrs = g_fdsprocess->get_cntrs_mgr()->get_counters(*id);
    if (cntrs == nullptr) {
        return;
    }
    cntrs->toMap(_return);
}

/**
 * For setting a flag dynamically
 * @param id
 * @param val
 */
void NetPlatHandler::
setConfigVal(boost::shared_ptr<std::string>& id,  // NOLINT
            boost::shared_ptr<int64_t>& val)
{
    if (!g_fdsprocess) return;

    try {
        g_fdsprocess->get_fds_config()->set(*id, *val);
    } catch(...) {
        // TODO(Rao): Only ignore SettingNotFound exception
        /* Ignore the error */
    }
}

}  // namespace fds
