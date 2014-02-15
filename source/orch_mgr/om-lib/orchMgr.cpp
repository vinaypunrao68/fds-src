/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#include <orchMgr.h>

#include <iostream>  // NOLINT(*)
#include <string>
#include <vector>
#include <OmResources.h>
#include <lib/Catalog.h>
#include <lib/PerfStats.h>

namespace fds {

OrchMgr *orchMgr;

OrchMgr::OrchMgr(int argc, char *argv[],
                 const std::string& default_config_path,
                 const std::string& base_path)
    : Module("Orch Manager"),
      FdsProcess(argc, argv, default_config_path, base_path),
      conf_port_num(0),
      ctrl_port_num(0),
      test_mode(false),
      omcp_req_handler(new FDSP_OMControlPathReqHandler(this)),
      cp_resp_handler(new FDSP_ControlPathRespHandler(this)),
      cfg_req_handler(new FDSP_ConfigPathReqHandler(this)) {

    om_log  = g_fdslog;
    om_mutex = new fds_mutex("OrchMgrMutex");

    for (int i = 0; i < MAX_OM_NODES; i++) {
        /*
         * TODO: Make this variable length rather
         * that statically allocated.
         */
        node_id_to_name[i] = "";
    }

    /*
     * Testing code for loading test info from disk.
     */
    // loadNodesFromFile("dlt1.txt", "dmt1.txt");
    // updateTables();

    FDS_PLOG(om_log) << "Constructing the Orchestration  Manager";
}

OrchMgr::~OrchMgr() {
    FDS_PLOG(om_log) << "Destructing the Orchestration  Manager";

    cfg_session_tbl->endAllSessions();
    cfgserver_thread->join();

    delete om_log;
    if (policy_mgr) {
        delete policy_mgr;
    }
}

int OrchMgr::mod_init(SysParams const *const param) {
    Module::mod_init(param);
    return 0;
}

void OrchMgr::mod_startup() {
}

void OrchMgr::mod_shutdown() {
}

void OrchMgr::setup(int argc, char* argv[],
                    fds::Module **mod_vec)
{
    /* First invoke FdsProcess setup so that it
     * sets up the signal handler and executes
     * module vector
     */
    FdsProcess::setup(argc, argv, mod_vec);

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
        (fds_log::severity_level) (conf_helper_.get<int>("log_severity")));

    policy_mgr = new VolPolicyMgr(stor_prefix, om_log);
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

    FDS_PLOG_SEV(om_log, fds_log::notification)
            << "Orchestration Manager using config port " << config_portnum
            << " control port " << control_portnum;

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

    omcp_session_tbl->createServerSession<netOMControlPathServerSession>(
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

    cfg_session_tbl->createServerSession<netConfigPathServerSession>(
        netSession::ipString2Addr(ip_address),
        config_portnum,
        my_node_name,
        FDS_ProtocolInterface::FDSP_CLI_MGR,
        cfg_req_handler);

    cfgserver_thread.reset(new std::thread(&OrchMgr::start_cfgpath_server, this));

    om_policy_srv = new Orch_VolPolicyServ();

    defaultS3BucketPolicy();
}

void OrchMgr::run()
{
    // run server to listen for OMControl messages from
    // SM, DM and SH
    netSession* omc_server_session =
            omcp_session_tbl->getSession(my_node_name,
                                         FDS_ProtocolInterface::FDSP_OMCLIENT_MGR);

    if (omc_server_session) {
        omcp_session_tbl->listenServer(omc_server_session);
    }
}

void OrchMgr::start_cfgpath_server()
{
    netSession* cfg_server_session =
            cfg_session_tbl->getSession(my_node_name,
                                        FDS_ProtocolInterface::FDSP_CLI_MGR);

    if (cfg_server_session) {
        cfg_session_tbl->listenServer(cfg_server_session);
    }
}

void OrchMgr::interrupt_cb(int signum)
{
    FDS_PLOG(orchMgr->GetLog()) << "OrchMgr: Shutting down communicator";

    omcp_session_tbl.reset();
    cfg_session_tbl.reset();
    exit(0);
}

fds_log* OrchMgr::GetLog() {
    return g_fdslog;  // om_log;
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

int OrchMgr::ApplyTierPolicy(::fpi::tier_pol_time_unitPtr& policy) {  // NOLINT
    om_policy_srv->serv_recvTierPolicyReq(policy);
    return 0;
}

int OrchMgr::AuditTierPolicy(::fpi::tier_pol_auditPtr& audit) {  // NOLINT
    om_policy_srv->serv_recvAuditTierPolicy(audit);
    return 0;
}

int OrchMgr::CreatePolicy(const FdspMsgHdrPtr& fdsp_msg,
                           const FdspCrtPolPtr& crt_pol_req)
{
    Error err(ERR_OK);
    int policy_id = (crt_pol_req->policy_info).policy_id;

    FDS_PLOG_SEV(GetLog(), fds_log::notification)
            << "Received CreatePolicy  Msg for policy "
            << (crt_pol_req->policy_info).policy_name
            << ", id" << policy_id;

    om_mutex->lock();
    err = policy_mgr->createPolicy(crt_pol_req->policy_info);
    om_mutex->unlock();
    return err.GetErrno();
}

int OrchMgr::DeletePolicy(const FdspMsgHdrPtr& fdsp_msg,
                           const FdspDelPolPtr& del_pol_req)
{
    Error err(ERR_OK);
    int policy_id = del_pol_req->policy_id;
    std::string policy_name = del_pol_req->policy_name;
    FDS_PLOG_SEV(GetLog(), fds::fds_log::notification)
            << "Received DeletePolicy  Msg for policy "
            << policy_name << ", id " << policy_id;

    om_mutex->lock();
    err = policy_mgr->deletePolicy(policy_id, policy_name);
    if (err.ok()) {
        /* removed policy from policy catalog or policy didn't exist 
         * TODO: what do we do with volumes that use policy we deleted ? */
    }
    om_mutex->unlock();
    return err.GetErrno();
}

int OrchMgr::ModifyPolicy(const FdspMsgHdrPtr& fdsp_msg,
                          const FdspModPolPtr& mod_pol_req)
{
    Error err(ERR_OK);
    int policy_id = (mod_pol_req->policy_info).policy_id;
    FDS_PLOG_SEV(GetLog(), fds_log::notification)
            << "Received ModifyPolicy  Msg for policy "
            << (mod_pol_req->policy_info).policy_name
            << ", id " << policy_id;

    om_mutex->lock();
    err = policy_mgr->modifyPolicy(mod_pol_req->policy_info);
    if (err.ok()) {
        /* modified policy in the policy catalog 
         * TODO: we probably should send modify volume messages to SH/DM, etc.  O */
    }
    om_mutex->unlock();
    return err.GetErrno();
}

void OrchMgr::NotifyQueueFull(const FDSP_MsgHdrTypePtr& fdsp_msg,
                              const FDSP_NotifyQueueStateTypePtr& queue_state_req) {
    // Use some simple logic for now to come up with the throttle level
    // based on the queue_depth for queues of various pririty

    om_mutex->lock();
    FDSP_QueueStateListType& que_st_list = queue_state_req->queue_state_list;
    int min_priority = que_st_list[0].priority;
    int min_p_q_depth = que_st_list[0].queue_depth;

    for (int i = 0; i < que_st_list.size(); i++) {
        FDS_PLOG_SEV(GetLog(), fds_log::notification)
                << "Received queue full for volume "
                << que_st_list[i].vol_uuid
                << ", priority - " << que_st_list[i].priority
                << " queue_depth - " << que_st_list[i].queue_depth;

        assert((que_st_list[i].priority >= 0) && (que_st_list[i].priority <= 10));
        assert((que_st_list[i].queue_depth >= 0.5) && (que_st_list[i].queue_depth <= 1));
        if (que_st_list[i].priority < min_priority) {
            min_priority = que_st_list[i].priority;
            min_p_q_depth = que_st_list[i].queue_depth;
        }
    }

    float throttle_level = static_cast<float>(min_priority) +
            static_cast<float>(1-min_p_q_depth)/0.5;

    // For now we will ignore if the calculated throttle level is greater than
    // the current throttle level. But it will have to be more complicated than this.
    // Every time we set a throttle level, we need to fire off a timer and
    // bring back to the normal throttle level (may be gradually) unless
    // we get more of these QueueFull messages, in which case, we will have to
    // extend the throttle period.
    om_mutex->unlock();

    OM_NodeContainer *local = OM_NodeDomainMod::om_loc_domain_ctrl();
    if (throttle_level < local->om_get_cur_throttle_level()) {
        local->om_set_throttle_lvl(throttle_level);
    } else {
        FDS_PLOG(GetLog()) << "Calculated throttle level " << throttle_level
                           << " less than current throttle level of "
                           << local->om_get_cur_throttle_level()
                           << ". Ignoring.";
    }
}

void OrchMgr::NotifyPerfstats(const FDSP_MsgHdrTypePtr& fdsp_msg,
                              const FDSP_PerfstatsTypePtr& perf_stats_msg)
{
    FDS_PLOG(GetLog())
            << "OM received perfstats from node of type: "
            << perf_stats_msg->node_type
            << " start ts "
            <<  perf_stats_msg->start_timestamp;

    /* Since we do not negotiate yet (should we?) the slot length of stats with AM and SM
     * the stat slot length in AM and SM should be FDS_STAT_DEFAULT_SLOT_LENGTH */
    fds_verify(perf_stats_msg->slot_len_sec == FDS_STAT_DEFAULT_SLOT_LENGTH);

    if (perf_stats_msg->node_type == FDS_ProtocolInterface::FDSP_STOR_HVISOR) {
        FDS_PLOG(GetLog())
                << "OM received perfstats from AM, start ts "
                << perf_stats_msg->start_timestamp;
        OM_NodeContainer *local = OM_NodeDomainMod::om_loc_domain_ctrl();
        local->om_handle_perfstats_from_am(perf_stats_msg->vol_hist_list,
                                           perf_stats_msg->start_timestamp);

        for (int i = 0; i < (perf_stats_msg->vol_hist_list).size(); ++i) {
            FDS_ProtocolInterface::FDSP_VolPerfHistType& vol_hist
                    = (perf_stats_msg->vol_hist_list)[i];
            FDS_PLOG(GetLog()) << "OM: received perfstat for vol " << vol_hist.vol_uuid;
            for (int j = 0; j < (vol_hist.stat_list).size(); ++j) {
                FDS_ProtocolInterface::FDSP_PerfStatType stat = (vol_hist.stat_list)[j];
                FDS_PLOG_SEV(GetLog(), fds::fds_log::debug)
                        << "OM: --- stat_type " << stat.stat_type
                        << " rel_secs " << stat.rel_seconds
                        << " iops " << stat.nios << " lat " << stat.ave_lat;
            }
        }
    } else if (perf_stats_msg->node_type == FDS_ProtocolInterface::FDSP_STOR_MGR) {
        FDS_PLOG(GetLog())
                << "OM received perfstats from SM, start ts "
                << perf_stats_msg->start_timestamp;
        /*
         * We need to decide whether we want to merge stats from multiple SMs from one
         * volume or have them separate. Should just mostly follow the code of handling
         * stats from AM but for now output debug msg to the log
         */
        for (int i = 0; i < (perf_stats_msg->vol_hist_list).size(); ++i) {
            FDS_ProtocolInterface::FDSP_VolPerfHistType& vol_hist
                    = (perf_stats_msg->vol_hist_list)[i];
            FDS_PLOG(GetLog()) << "OM: received perfstat for vol " << vol_hist.vol_uuid;
            for (int j = 0; j < (vol_hist.stat_list).size(); ++j) {
                FDS_ProtocolInterface::FDSP_PerfStatType& stat = (vol_hist.stat_list)[j];
                FDS_PLOG_SEV(GetLog(), fds_log::normal)
                        << "OM: --- stat_type " << stat.stat_type
                        << " rel_secs " << stat.rel_seconds
                        << " iops " << stat.nios << " lat " << stat.ave_lat;
            }
        }
    } else {
        FDS_PLOG_SEV(GetLog(), fds_log::warning)
                << "OM received perfstats from node of type "
                << perf_stats_msg->node_type
                << " which we don't need stats from (or we do now?)";
    }
}

void OrchMgr::defaultS3BucketPolicy()
{
    Error err(ERR_OK);

    FDS_ProtocolInterface::FDSP_PolicyInfoType policy_info;
    policy_info.policy_name = std::string("S3-Bucket_policy");
    policy_info.policy_id = 50;
    policy_info.iops_min = 400;
    policy_info.iops_max = 600;
    policy_info.rel_prio = 1;

    orchMgr->om_mutex->lock();
    err = orchMgr->policy_mgr->createPolicy(policy_info);
    orchMgr->om_mutex->unlock();

    FDS_PLOG_SEV(GetLog(), fds_log::normal) << "Created  default S3 policy ";
}

OrchMgr *gl_orch_mgr;
}  // namespace fds
