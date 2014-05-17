/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <string>
#include <platform/platform-lib.h>
#include <NetSession.h>

namespace fds {

Platform *gl_PlatformSvc;

// -------------------------------------------------------------------------------------
// Common Platform Services
// -------------------------------------------------------------------------------------
Platform::Platform(char const *const         name,
                   FDSP_MgrIdType            node_type,
                   DomainNodeInv::pointer    node_inv,
                   DomainClusterMap::pointer cluster,
                   DomainResources::pointer  resources,
                   OmAgent::pointer          master)
    : Module(name), plf_node_type(node_type), plf_domain(NULL), plf_master(master),
      plf_node_inv(node_inv), plf_clus_map(cluster), plf_resources(resources)
{
    plf_node_evt         = NULL;
    plf_vol_evt          = NULL;
    plf_throttle_evt     = NULL;
    plf_migrate_evt      = NULL;
    plf_tier_evt         = NULL;
    plf_bucket_stats_evt = NULL;

    plf_om_ctrl_port     = 0;
    plf_my_ctrl_port     = 0;
    plf_my_conf_port     = 0;
    plf_my_data_port     = 0;
    plf_my_migr_port     = 0;

    plf_rpc_thrd  = NULL;
    plf_net_sess  = NULL;
    plf_rpc_reqt  = NULL;
    plf_my_sess   = NULL;
    plf_om_resp   = NULL;
}

Platform::~Platform()
{
    if (plf_my_sess != NULL) {
        plf_net_sess->endSession(plf_my_sess->getSessionTblKey());
    }
    if (plf_rpc_thrd != NULL) {
        plf_rpc_thrd->join();
    }
}

// -----------------------------------------------------------------------------------
// Platform node/cluster methods.
// -----------------------------------------------------------------------------------

// plf_reg_node_info
// -----------------
//
void
Platform::plf_reg_node_info(const NodeUuid &uuid, const FdspNodeRegPtr msg)
{
    NodeAgent::pointer     new_node;
    DomainNodeInv::pointer local;

    local = plf_node_inventory();
    Error err = local->dc_register_node(uuid, msg, &new_node);

    if (err != ERR_OK) {
        fds_verify(0);
    }
}

// plf_del_node_info
// -----------------
//
void
Platform::plf_del_node_info(const NodeUuid &uuid, const std::string &name)
{
    DomainNodeInv::pointer local;

    local = plf_node_inventory();
    Error err = local->dc_unregister_node(uuid, name);
}

// plf_update_cluster
// ------------------
//
void
Platform::plf_update_cluster()
{
}

// prf_rpc_om_handshake
// --------------------
// Perform the handshake connection with OM.
//
void
Platform::plf_rpc_om_handshake(fpi::FDSP_RegisterNodeTypePtr reg)
{
    if (plf_master == NULL) {
        fds_verify(plf_om_resp == NULL);

        plf_master  = new OmAgent(0);
        plf_om_resp = boost::shared_ptr<PlatRpcResp>(plat_creat_resp_disp());

        plf_master->om_handshake(plf_net_sess, plf_om_resp,
                                 plf_om_ip_str, plf_om_ctrl_port);
    }
    plf_master->init_node_reg_pkt(reg);
    plf_master->om_register_node(reg);
}

// plf_run_server
// --------------
//
void
Platform::plf_run_server(bool spawn_thr)
{
    if (spawn_thr == true) {
        plf_rpc_thrd = boost::shared_ptr<std::thread>(
               new std::thread(&Platform::plf_rpc_server_thread, this));
    } else {
        plf_rpc_thrd = NULL;
        plf_rpc_server_thread();
    }
}

// plf_is_om_node
// --------------
// Return true if this is the node running the primary OM node.
//
bool
Platform::plf_is_om_node()
{
    // Do simple IP compare my IP with OM ip.
    // Until we can get rid of the old endpoint using plf_my_ctrl_port, use data
    // port for net service update protocols.
    //
    if (/* (*plf_get_my_ip() == *plf_get_om_ip()) && */
        (plf_get_om_svc_port() == plf_get_my_data_port())) {
        return true;
    }
    return false;
}

// plf_rpc_server_thread
// ---------------------
//
void
Platform::plf_rpc_server_thread()
{
    // TODO(Rao): Ideally createServerSession should take a shared pointer for
    // plf_rpc_sess.  Make sure that happens; otherwise you end up with a pointer leak.
    //
    plf_my_sess = plf_net_sess->createServerSession<netControlPathServerSession>(
            netSession::ipString2Addr(netSession::getLocalIp()),
            plf_my_ctrl_port, plf_my_node_name,
            FDSP_ORCH_MGR, plf_rpc_reqt);

    plf_net_sess->listenServer(plf_my_sess);
}

// plf_change_info
// ---------------
//
void
Platform::plf_change_info(const plat_node_data_t *ndata)
{
    char         name[64];
    NodeUuid     uuid(ndata->nd_node_uuid);
    fds_uint32_t base;

    if (ndata->nd_node_uuid == 0) {
        // TODO(Vy): we need to think about if AM needs to persist its own node/service
        // uuid or can it generate only the fly during its bootstrap process.
        // When we're here and we don't have uuid, this must be AM or running
        // in a test mode where there's no OM/platform.
        plf_my_uuid = NodeUuid(fds_get_uuid64(get_uuid()));
    } else {
        plf_my_uuid = uuid;
    }
    Platform::plf_svc_uuid_from_node(plf_my_uuid, &plf_my_svc_uuid, plf_node_type);
    // snprintf(name, sizeof(name), "node-%u", ndata->nd_node_number);
    // plf_my_node_name.assign(name);

    base = PlatformProcess::
        plf_get_platform_port(ndata->nd_plat_port, ndata->nd_node_number);

    if (base == 0) {
        return;
    }
    switch (plf_node_type) {
        case FDSP_STOR_MGR:
            base = PlatformProcess::plf_get_sm_port(base);
            break;

        case FDSP_DATA_MGR:
            base = PlatformProcess::plf_get_dm_port(base);
            break;

        case FDSP_STOR_HVISOR:
            base = PlatformProcess::plf_get_am_port(base);
            break;

        case FDSP_ORCH_MGR:
            return;

        default:
            break;
    }
    plf_om_ctrl_port = ndata->nd_om_port;
    plf_my_ctrl_port = plf_ctrl_port(base);
    plf_my_conf_port = plf_conf_port(base);
    plf_my_data_port = plf_data_port(base);
    plf_my_migr_port = plf_migration_port(base);

    LOGNOTIFY << "My ctrl port " << std::dec << plf_my_ctrl_port << std::endl
              << "My conf port " << plf_my_conf_port << std::endl
              << "My data port " << plf_my_data_port << std::endl
              << "My migr port " << plf_my_migr_port << std::endl
              << "My node uuid " << std::hex << plf_my_uuid.uuid_get_val() << std::endl
              << "My OM port   " << std::dec << plf_om_ctrl_port << std::endl
              << "My OM IP     " << plf_om_ip_str << std::endl;
}

// plf_svc_uuid_from_node
// ----------------------
// Simple formula to derrive service uuid from node uuid.
//
void
Platform::plf_svc_uuid_from_node(const NodeUuid &node,
                                 NodeUuid       *svc,
                                 FDSP_MgrIdType  type)
{
    if (type == FDSP_PLATFORM) {
        svc->uuid_set_val(node.uuid_get_val());
    } else {
        svc->uuid_set_val(node.uuid_get_val() + 1 + type);
    }
}

// -----------------------------------------------------------------------------------
// Module methods
// -----------------------------------------------------------------------------------
int
Platform::mod_init(SysParams const *const param)
{
    Module::mod_init(param);

    plf_net_sess   = boost::shared_ptr<netSessionTbl>(new netSessionTbl(FDSP_PLATFORM));
    plf_rpc_reqt   = boost::shared_ptr<PlatRpcReqt>(plat_creat_reqt_disp());
    plf_dpath_resp = NodeAgentDpRespPtr(plat_creat_dpath_resp());

    FdsConfigAccessor conf(g_fdsprocess->get_conf_helper());
    plf_om_ip_str    = conf.get_abs<std::string>("fds.plat.om_ip");
    plf_om_ctrl_port = conf.get_abs<int>("fds.plat.om_port");
    plf_om_svc_port  = conf.get_abs<int>("fds.plat.om_svc_port");

    return 0;
}

void
Platform::mod_startup()
{
    Module::mod_startup();
}

void
Platform::mod_enable_service()
{
    Module::mod_enable_service();
}

void
Platform::mod_shutdown()
{
    Module::mod_shutdown();
}

namespace util {
/**
 * @return local ip
 */
std::string get_local_ip()
{
    struct ifaddrs *ifAddrStruct = NULL;
    struct ifaddrs *ifa          = NULL;
    void   *tmpAddrPtr           = NULL;
    std::string myIp;

    /*
     * Get the local IP of the host.  This is needed by the OM.
     */
    getifaddrs(&ifAddrStruct);
    for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr->sa_family == AF_INET) {  // IPv4
            if (strncmp(ifa->ifa_name, "lo", 2) != 0) {
                tmpAddrPtr = &((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;  // NOLINT
                char addrBuf[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, tmpAddrPtr, addrBuf, INET_ADDRSTRLEN);
                myIp = std::string(addrBuf);
                if (myIp.find("10.1") != std::string::npos) {
                    break; /*  TODO: more dynamic */
                }
            }
        }
    }
    if (ifAddrStruct != NULL) {
        freeifaddrs(ifAddrStruct);
    }
    return myIp;
}

}  // namespace util
}  // namespace fds
