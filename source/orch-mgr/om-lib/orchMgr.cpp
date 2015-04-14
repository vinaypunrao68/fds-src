/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#include <orchMgr.h>

#include <iostream>  // NOLINT(*)
#include <string>
#include <vector>
#include <OmResources.h>
#include <lib/Catalog.h>
#include <map>
#include <util/Log.h>
#include <NetSession.h>
#include <OmDataPlacement.h>
#include <OmVolumePlacement.h>
#include <orch-mgr/om-service.h>
#include <fdsp/OMSvc.h>
#include <om-svc-handler.h>
#include <net/SvcMgr.h>

namespace fds {

OrchMgr *orchMgr;

OrchMgr::OrchMgr(int argc, char *argv[], OM_Module *omModule)
    : conf_port_num(0),
      ctrl_port_num(0),
      test_mode(false),
      omcp_req_handler(new FDSP_OMControlPathReqHandler(this)),
      cfg_req_handler(new FDSP_ConfigPathReqHandler(this)),
      deleteScheduler(this),
      enableSnapshotSchedule(true)
{
    om_mutex = new fds_mutex("OrchMgrMutex");
    fds::gl_orch_mgr = this;

    static fds::Module *omVec[] = {
        omModule,
        NULL
    };

    for (int i = 0; i < MAX_OM_NODES; i++) {
        /*
         * TODO: Make this variable length rather
         * that statically allocated.
         */
        node_id_to_name[i] = "";
    }

    init<fds::OmSvcHandler, fpi::OMSvcProcessor>(argc, argv, "platform.conf",
                                                 "fds.om.", "om.log", omVec);

    enableSnapshotSchedule = MODULEPROVIDER()->get_fds_config()->get<bool>(
            "fds.om.enable_snapshot_schedule", true);
    if (enableSnapshotSchedule) {
        snapshotMgr.reset(new fds::snapshot::Manager(this));
    }

    /*
     * Testing code for loading test info from disk.
     */
    LOGNORMAL << "Constructing the Orchestration  Manager";
}

OrchMgr::~OrchMgr()
{
    LOGNORMAL << "Destructing the Orchestration  Manager";

    cfg_session_tbl->endAllSessions();
    cfgserver_thread->join();

    if (policy_mgr) {
        delete policy_mgr;
    }
    fds::gl_orch_mgr =  nullptr;
}

void OrchMgr::proc_pre_startup()
{
    int    argc;
    char **argv;

    SvcProcess::proc_pre_startup();

    argv = mod_vectors_->mod_argv(&argc);

    /*
     * Process the cmdline args.
     */
    for (int i = 1; i < argc; i++) {
        if (strncmp(argv[i], "--port=", 7) == 0) {
            conf_port_num = strtoul(argv[i] + 7, NULL, 0);
        } else if (strncmp(argv[i], "--cport=", 8) == 0) {
            ctrl_port_num = strtoul(argv[i] + 8, NULL, 0);
        } else if (strncmp(argv[i], "--prefix=", 9) == 0) {
            stor_prefix = argv[i] + 9;
        } else if (strncmp(argv[i], "--test", 7) == 0) {
            test_mode = true;
        }
    }

    GetLog()->setSeverityFilter(
        fds_log::getLevelFromName(conf_helper_.get<std::string>("log_severity")));

    my_node_name = stor_prefix + std::string("OrchMgr");

    std::string ip_address;
    int config_portnum;
    int control_portnum;
    if (conf_port_num != 0) {
        config_portnum = conf_port_num;
    } else {
        config_portnum = conf_helper_.get<int>("config_port");
    }
    if (ctrl_port_num != 0) {
        control_portnum = ctrl_port_num;
    } else {
        control_portnum = conf_helper_.get<int>("control_port");
    }
    ip_address = conf_helper_.get<std::string>("ip_address");

    LOGNOTIFY << "Orchestration Manager using config port " << config_portnum
              << " control port " << control_portnum;

    // check the config db port (default is 0 for now)
    // if the port is NOT set explicitly , do not use it .
    // reason : mutiple folks might use the same instance.
    // whoever decides to use it will have to set the port
    // properly
    // But we still need to instantiate as the object might be used .

    configDB = new kvstore::ConfigDB(
        conf_helper_.get<std::string>("configdb.host", "localhost"),
        conf_helper_.get<int>("configdb.port", 0),
        conf_helper_.get<int>("configdb.poolsize", 10));

    policy_mgr = new VolPolicyMgr(configDB, GetLog());

    defaultS3BucketPolicy();

    /*
     * Setup server session to listen to OMControl path messages from
     * DM, SM, and SH
     */
    omcp_session_tbl = boost::shared_ptr<netSessionTbl>(
        new netSessionTbl(my_node_name,
                          netSession::ipString2Addr(ip_address),
                          control_portnum,
                          10,
                          FDS_ProtocolInterface::FDSP_ORCH_MGR));

    omc_server_session = omcp_session_tbl->\
            createServerSession<netOMControlPathServerSession>(
                netSession::ipString2Addr(ip_address),
                control_portnum,
                my_node_name,
                FDS_ProtocolInterface::FDSP_OMCLIENT_MGR,
                omcp_req_handler);

    /*
     * Setup server session to listen to config path messages from fdscli
     */
    cfg_session_tbl = boost::shared_ptr<netSessionTbl>(
        new netSessionTbl(my_node_name,
                          netSession::ipString2Addr(ip_address),
                          config_portnum,
                          10,
                          FDS_ProtocolInterface::FDSP_ORCH_MGR));

    cfg_server_session = cfg_session_tbl->\
            createServerSession<netConfigPathServerSession>(
                netSession::ipString2Addr(ip_address),
                config_portnum,
                my_node_name,
                FDS_ProtocolInterface::FDSP_CLI_MGR,
                cfg_req_handler);

    cfgserver_thread.reset(new std::thread(&OrchMgr::start_cfgpath_server, this));
}

void OrchMgr::proc_pre_service()
{
    if (enableSnapshotSchedule) {
        snapshotMgr->init();
    }
    fds_bool_t config_db_up = loadFromConfigDB();
    // load persistent state to local domain
    OM_NodeDomainMod* local_domain = OM_NodeDomainMod::om_local_domain();
    local_domain->om_load_state(config_db_up ? configDB : NULL);
}

void OrchMgr::setupSvcInfo_()
{
    SvcProcess::setupSvcInfo_();

    auto config = MODULEPROVIDER()->get_conf_helper();
    svcInfo_.ip = config.get_abs<std::string>("fds.common.om_ip_list");
    svcInfo_.svc_port = config.get_abs<int>("fds.common.om_port");
    svcInfo_.svc_id.svc_uuid.svc_uuid = static_cast<int64_t>(
        config.get_abs<fds_uint64_t>("fds.common.om_uuid"));

    LOGNOTIFY << "Service info(After overriding): " << fds::logString(svcInfo_);
}

void OrchMgr::registerSvcProcess()
{
    LOGNOTIFY << "register service process";

    /* Add om information to service map */
    svcMgr_->updateSvcMap({svcInfo_});
}

int OrchMgr::run()
{
    // run server to listen for OMControl messages from
    // SM, DM and SH
    runConfigService(this);
    if (omc_server_session) {
        omcp_session_tbl->listenServer(omc_server_session);
    }
    return 0;
}

void OrchMgr::start_cfgpath_server()
{
    if (cfg_server_session) {
        cfg_session_tbl->listenServer(cfg_server_session);
    }
}

void OrchMgr::interrupt_cb(int signum)
{
    LOGNORMAL << "OrchMgr: Shutting down communicator";

    omcp_session_tbl.reset();
    cfg_session_tbl.reset();
    exit(0);
}

VolPolicyMgr *
OrchMgr::om_policy_mgr()
{
    return orchMgr->policy_mgr;
}

const std::string &
OrchMgr::om_stor_prefix()
{
    return orchMgr->stor_prefix;
}

void OrchMgr::defaultS3BucketPolicy()
{
    Error err(ERR_OK);

    FDS_ProtocolInterface::FDSP_PolicyInfoType policy_info;
    policy_info.policy_name = std::string("FDS Default/Stock Policy");
    policy_info.policy_id = 50;
    policy_info.iops_min = conf_helper_.get<int>("default_iops_min");;
    policy_info.iops_max = conf_helper_.get<int>("default_iops_max");;
    policy_info.rel_prio = 1;

    orchMgr->om_mutex->lock();
    err = orchMgr->policy_mgr->createPolicy(policy_info);
    orchMgr->om_mutex->unlock();

    LOGNORMAL << "Created  default S3 policy ";
}

bool OrchMgr::loadFromConfigDB() {
    LOGNORMAL << "loading data from configdb...";

    // check connection
    if (!configDB->isConnected()) {
        LOGCRITICAL << "unable to talk to config db ";
        return false;
    }

    // get global domain info
    std::string globalDomain = configDB->getGlobalDomain();
    if (globalDomain.empty()) {
        LOGWARN << "global.domain not configured.. setting a default [fds]";
        configDB->setGlobalDomain("fds");
    }

    // get local domains
    std::vector<fds::apis::LocalDomain> localDomains;
    configDB->listLocalDomains(localDomains);

    if (localDomains.empty())  {
        LOGWARN << "No Local Domains stored in the system. "
                << "Setting a default Local Domain.";
        int64_t id = configDB->createLocalDomain();
        if (id <= 0) {
            LOGERROR << "Some issue in Local Domain creation. ";
            return false;
        } else {
            LOGNOTIFY << "Default Local Domain creation succeded. ID: " << id;
        }
        configDB->listLocalDomains(localDomains);
    }

    if (localDomains.empty()) {
        LOGCRITICAL << "Something wrong with the configdb. "
                    << " -- not loading data.";
        return false;
    }

    // keep the pointer in data placement module
    DataPlacement *dp = OM_Module::om_singleton()->om_dataplace_mod();
    dp->setConfigDB(configDB);

    OM_Module::om_singleton()->om_volplace_mod()->setConfigDB(configDB);

    // load the snapshot policies
    if (enableSnapshotSchedule) {
        snapshotMgr->loadFromConfigDB();
    }

    return true;
}

DmtColumnPtr OrchMgr::getDMTNodesForVolume(fds_volid_t volId) {
    DMTPtr dmt = OM_Module::om_singleton()->om_volplace_mod()->getCommittedDMT();
    return dmt->getNodeGroup(volId);
}

kvstore::ConfigDB* OrchMgr::getConfigDB() {
    return configDB;
}

OrchMgr *gl_orch_mgr;
}  // namespace fds
