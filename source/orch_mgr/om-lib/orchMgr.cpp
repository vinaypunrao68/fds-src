/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#include <orchMgr.h>

#include <iostream>  // NOLINT(*)
#include <string>
#include <vector>
#include <OmResources.h>

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
    if (policy_mgr)
        delete policy_mgr;
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

    /*
     * create a default domain. If the domains are NOT configured 
     * all the nodes will be in default domain
     */
    FdsLocalDomain *default_domain = new FdsLocalDomain(stor_prefix, om_log);
    localDomainInfo  *default_domain_info = new localDomainInfo();
    default_domain_info->loc_domain_name = "Default Domain";
    default_domain_info->loc_dom_id = 1;
    default_domain_info->glb_dom_id = 1;
    default_domain_info->domain_ptr = default_domain;
    locDomMap[default_domain_info->loc_dom_id] = default_domain_info;

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
                          100,
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
                          100,
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
    
    if (omc_server_session)
        omcp_session_tbl->listenServer(omc_server_session);
}

void OrchMgr::start_cfgpath_server()
{
    netSession* cfg_server_session = 
            cfg_session_tbl->getSession(my_node_name,
                                        FDS_ProtocolInterface::FDSP_CLI_MGR);
    
    if (cfg_server_session)
        cfg_session_tbl->listenServer(cfg_server_session);
}

void OrchMgr::interrupt_cb(int signum)
{
    FDS_PLOG(orchMgr->GetLog())
            << "OrchMgr: Shutting down communicator";

    omcp_session_tbl.reset();
    cfg_session_tbl.reset();
    exit(0);
}

fds_log* OrchMgr::GetLog() {
    return g_fdslog; // om_log;
}

int OrchMgr::CreateDomain(const FdspMsgHdrPtr& fdsp_msg,
                          const FdspCrtDomPtr& crt_dom_req) {
    FDS_PLOG_SEV(GetLog(), fds_log::notification)
            << "Received CreateDomain Req : "
            << crt_dom_req->domain_name
            << " :domain_id - " << crt_dom_req->domain_id;

    /* check  for duplicate domain id */
    om_mutex->lock();
    if (locDomMap.count(crt_dom_req->domain_id) == 0) {
        FDS_PLOG_SEV(om_log, fds_log::warning)
                << "Duplicate domain: " << crt_dom_req->domain_id;
        om_mutex->unlock();
        return -1;
    }

    FdsLocalDomain *new_domain = new FdsLocalDomain(stor_prefix, om_log);
    localDomainInfo  *domain_info = new localDomainInfo();
    domain_info->loc_domain_name = crt_dom_req->domain_name;
    domain_info->loc_dom_id = crt_dom_req->domain_id;
    domain_info->glb_dom_id = 1;
    domain_info->domain_ptr = new_domain;
    locDomMap[domain_info->loc_dom_id] = domain_info;

    om_mutex->unlock();
    return 0;
}

int OrchMgr::DeleteDomain(const FdspMsgHdrPtr& fdsp_msg,
                          const FdspCrtDomPtr& del_dom_req) {
    /* TBD*/
    return 0;
}

int OrchMgr::RemoveNode(const FdspMsgHdrPtr& fdsp_msg,
                        const FdspRmNodePtr& rm_node_req) {
    FDS_PLOG_SEV(GetLog(), fds_log::normal)
            << "Received RemoveNode Req : "
            << rm_node_req->node_name;

    OM_NodeDomainMod *domain = OM_NodeDomainMod::om_local_domain();

    // TODO(Anna) remove this local domain
    localDomainInfo  *currentDom = locDomMap[DEFAULT_LOC_DOMAIN_ID];

    om_mutex->lock();

    // TODO(Anna) Still using old code before we completely move to new NodeAgent, etc.
    fds::FdsLocalDomain::node_map_t& sm_map = currentDom->domain_ptr->currentSmMap;

    // TODO(Anna) Only allow to remove SM node for now, add DM removal after
    // we move to new code
    if (sm_map.count(rm_node_req->node_name) == 0) {
        FDS_PLOG_SEV(GetLog(), fds_log::notification)
                << "Remove node request for non-SM node " 
                << rm_node_req->node_name << ". Not implemented!.";

        om_mutex->unlock();
        return -1;
    }

    // remove node from local domain map
    NodeUuid rm_node_uuid(fds_get_uuid64(rm_node_req->node_name));
    Error err = domain->om_del_node_info(rm_node_uuid, rm_node_req->node_name);
    if (!err.ok()) {
        FDS_PLOG_SEV(GetLog(), fds_log::error)
                << "RemoveNode: remove node info from local domain failed for node " 
                << rm_node_req->node_name << ", uuid " << std::hex << rm_node_uuid.uuid_get_val()
                << std::dec << ", result: " << err.GetErrstr();
        om_mutex->unlock();
        return -1;
    }

    // TODO(Anna) Fix this using new domain struct and node info
    sm_map[rm_node_req->node_name].node_state = FDS_ProtocolInterface::FDS_Node_Rmvd;
    // If this is a SM or a DM, let existing nodes know about this node removal
    if (sm_map[rm_node_req->node_name].node_type != FDS_ProtocolInterface::FDSP_STOR_HVISOR) {
        currentDom->domain_ptr->sendNodeEventToFdsNodes(sm_map[rm_node_req->node_name],
                                                        sm_map[rm_node_req->node_name].node_state);

        /* update the disk capabilities */
        currentDom->domain_ptr->admin_ctrl->removeDiskCapacity(sm_map[rm_node_req->node_name]);
    }
    sm_map.erase(rm_node_req->node_name);

    om_mutex->unlock();

    // TODO(Anna): For now, let's start the cluster update process
    // now. This should eventually be decoupled from removal
    domain->om_update_cluster();

    return 0;

}

int OrchMgr::GetDomainStats(const FdspMsgHdrPtr& fdsp_msg,
                            const FdspGetDomStatsPtr& get_stats_req)
{
    int domain_id = get_stats_req->domain_id;
    localDomainInfo  *currentDom = NULL;

    FDS_PLOG_SEV(GetLog(), fds_log::normal)
            << "Received GetDomainStats Req for domain "
            << domain_id;

    /*
     * Use default domain for now... 
     */
    FDS_PLOG_SEV(GetLog(), fds_log::warning)
            << "For now always returning stats of default domain. "
            << "Domain id in the request is ignored";

    currentDom  = locDomMap[DEFAULT_LOC_DOMAIN_ID];

    if (currentDom) {
        om_mutex->lock();
        currentDom->domain_ptr->sendBucketStats(
            5,
            fdsp_msg->src_node_name,
            fdsp_msg->req_cookie);

        /* if need to test printing to json file, call this func instead .. */
        //    currentDom->domain_ptr->printStatsToJsonFile();
        om_mutex->unlock();
    }
    return 0;
}

int OrchMgr::ApplyTierPolicy(::FDS_ProtocolInterface::tier_pol_time_unitPtr& policy) {
    om_policy_srv->serv_recvTierPolicyReq(policy);
    return 0;
}

int OrchMgr::AuditTierPolicy(::FDS_ProtocolInterface::tier_pol_auditPtr& audit) {
    om_policy_srv->serv_recvAuditTierPolicy(audit);
    return 0;
}

int OrchMgr::CreateVol(const FdspMsgHdrPtr& fdsp_msg,
                       const FdspCrtVolPtr& crt_vol_req) {
    Error err(ERR_OK);
    int  domain_id = (crt_vol_req->vol_info).localDomainId;
    std::string vol_name = (crt_vol_req->vol_info).vol_name;
    localDomainInfo  *currentDom;

    FDS_PLOG_SEV(GetLog(), fds_log::notification)
            << "Received CreateVol Req for volume "
            << vol_name;

    /*
     * get the domain Id. If  Domain is not created  use  default domain 
     * for now use the default domain
     */
    currentDom  = locDomMap[DEFAULT_LOC_DOMAIN_ID];
    om_mutex->lock();

    if (currentDom->domain_ptr->volumeMap.count(vol_name) != 0) {
        FDS_PLOG_SEV(om_log, fds_log::warning)
                << "Received CreateVol for existing volume "
                << vol_name;
        om_mutex->unlock();
        return -1;
    }

    VolumeInfo *new_vol = new VolumeInfo();
    new_vol->vol_name = vol_name;
    new_vol->volUUID = currentDom->domain_ptr->getNextFreeVolId();
    FDS_PLOG_SEV(GetLog(), fds_log::normal)
            << " created volume ID " << new_vol->volUUID;

    new_vol->properties = new VolumeDesc(crt_vol_req->vol_info, new_vol->volUUID);
    err = policy_mgr->fillVolumeDescPolicy(new_vol->properties);
    if ( err == ERR_CAT_ENTRY_NOT_FOUND ) {
        /* TODO: policy not in the catalog,
           should we return error or use default policy? */
        FDS_PLOG_SEV(GetLog(), fds_log::warning)
                << "Create volume " << vol_name
                << " Req requested unknown policy "
                << (crt_vol_req->vol_info).volPolicyId;
    } else if (!err.ok()) {
        FDS_PLOG_SEV(GetLog(), fds_log::error)
                << "CreateVol: Failed to get policy info for volume "
                <<  vol_name;
        /* TODO: for now ignoring error */
    }
    /*
     * check the resource  availability 
     */

    err = currentDom->domain_ptr->admin_ctrl->volAdminControl(new_vol->properties);
    if ( !err.ok() ) {
        delete new_vol;
        om_mutex->unlock();
        FDS_PLOG_SEV(GetLog(), fds_log::error)
                << "Unable to create Volume " << vol_name
                << "; result" << err.GetErrstr();
        return -1;
    }

    currentDom->domain_ptr->volumeMap[vol_name] = new_vol;
    currentDom->domain_ptr->sendCreateVolToFdsNodes(new_vol);
    /* update the per domain disk resource  */
    om_mutex->unlock();
    return 0;
}

int OrchMgr::DeleteVol(const FdspMsgHdrPtr& fdsp_msg,
                        const FdspDelVolPtr& del_vol_req) {
    std::string vol_name = del_vol_req->vol_name;
    VolumeInfo *cur_vol;
    localDomainInfo  *currentDom;

    FDS_PLOG_SEV(GetLog(), fds_log::notification)
            << "Received DeleteVol Req for volume "
            << vol_name;

    /*
     * get the domain Id. If  Domain is not created  use  default domain 
     * for now  use the default domain
     */
    currentDom  = locDomMap[DEFAULT_LOC_DOMAIN_ID];

    om_mutex->lock();
    if (currentDom->domain_ptr->volumeMap.count(vol_name) == 0) {
        FDS_PLOG_SEV(om_log, fds_log::warning)
                << "Received DeleteVol for non-existent volume " << vol_name;
        om_mutex->unlock();
        return -1;
    }
    VolumeInfo *del_vol =  currentDom->domain_ptr->volumeMap[vol_name];

    for (int i = 0; i < del_vol->hv_nodes.size(); i++) {
        if (currentDom->domain_ptr->currentShMap.count(del_vol->hv_nodes[i]) == 0) {
            FDS_PLOG_SEV(om_log, fds_log::error)
                    << "Inconsistent State Detected. "
                    << "HV node in volume's hvnode list but not in SH map";
            assert(0);
        }
        currentDom->domain_ptr->sendDetachVolToHvNode(del_vol->hv_nodes[i], del_vol);
    }

    currentDom->domain_ptr->sendDeleteVolToFdsNodes(del_vol);

    /*
     * update  admission control  class  to reflect the  delete Volume 
     */
    cur_vol= currentDom->domain_ptr->volumeMap[vol_name];
    currentDom->domain_ptr->admin_ctrl->updateAdminControlParams(cur_vol->properties);

    currentDom->domain_ptr->volumeMap.erase(vol_name);
    om_mutex->unlock();

    delete del_vol;
    return 0;
}

int OrchMgr::ModifyVol(const FdspMsgHdrPtr& fdsp_msg,
                       const FdspModVolPtr& mod_vol_req)
{
    Error err(ERR_OK);
    std::string vol_name = (mod_vol_req->vol_desc).vol_name;
    localDomainInfo  *currentDom = NULL;
    VolumeDesc *new_voldesc = NULL;

    FDS_PLOG_SEV(GetLog(), fds_log::notification)
            << "Received ModifyVol Msg for volume " << vol_name;

    /*
     * get the domain Id. If  Domain is not created  use  default domain 
     * for now  use the default domain
     */
    currentDom  = locDomMap[DEFAULT_LOC_DOMAIN_ID];

    om_mutex->lock();
    if (currentDom->domain_ptr->volumeMap.count(vol_name) == 0) {
        FDS_PLOG_SEV(om_log, fds_log::warning)
                << "Received ModifyVol for non-existent volume " << vol_name;
        om_mutex->unlock();
        return -1;
    }
    VolumeInfo *mod_vol =  currentDom->domain_ptr->volumeMap[vol_name];
    new_voldesc = new VolumeDesc(*(mod_vol->properties));

    /* we will not modify capacity for now, just policy id or min/max iops and priority */
    if ((mod_vol_req->vol_desc).volPolicyId != 0) {
        /* change policy id and get its description from the catalog */
        new_voldesc->volPolicyId = (mod_vol_req->vol_desc).volPolicyId;
        err = policy_mgr->fillVolumeDescPolicy(new_voldesc);
        if ( err == ERR_CAT_ENTRY_NOT_FOUND ) {
            /* policy not in the catalog , revert to old policy id and return */
            FDS_PLOG_SEV(GetLog(), fds_log::warning)
                    << "Modify volume " << vol_name
                    << " requested unknown policy "
                    << (mod_vol_req->vol_desc).volPolicyId
                    << "; keeping original policy "
                    <<  mod_vol->properties->volPolicyId;
        } else if (!err.ok()) {
            FDS_PLOG_SEV(GetLog(), fds_log::error)
                    << "ModifyVol: volume " << vol_name
                    << " - Failed to get policy info for policy "
                    << (mod_vol_req->vol_desc).volPolicyId
                    << "; keeping original policy "
                    << mod_vol->properties->volPolicyId;
        }

        /* cleanup and return if error */
        if (!err.ok()) {
            /* we did not copy anything to the actual volume desc yet,
             * so just delete new_voldesc */
            delete new_voldesc;
            om_mutex->unlock();
            return -1;
        }
    } else {
        /* don't modify policy id, just min/max iops and priority in volume descriptor */
        new_voldesc->iops_min = (mod_vol_req->vol_desc).iops_min;
        new_voldesc->iops_max = (mod_vol_req->vol_desc).iops_max;
        new_voldesc->relativePrio = (mod_vol_req->vol_desc).rel_prio;
        FDS_PLOG_SEV(GetLog(), fds_log::notification)
                << "ModifyVol: volume " << vol_name
                << " - keeps policy id " << new_voldesc->volPolicyId
                << " with new min_iops " << new_voldesc->iops_min
                << " max_iops " << new_voldesc->iops_max
                << " priority " << new_voldesc->relativePrio;
    }

    /* check if this volume can go through admission control with modified policy info */
    err = currentDom->domain_ptr->admin_ctrl->checkVolModify(
        mod_vol->properties,
        new_voldesc);
    if ( !err.ok() ) {
        /* we did not copy anything to the actual volume desc yet,
         * so just delete new_voldesc */
        delete new_voldesc;
        om_mutex->unlock();
        FDS_PLOG_SEV(GetLog(), fds_log::error)
                << "ModifyVol: volume " << vol_name
                << " -- cannot admit with new policy info, keeping the old policy";
        return -1;
    }

    /* we admitted modified policy -- copy updated volume desc into volume info */
    mod_vol->properties->volPolicyId = new_voldesc->volPolicyId;
    mod_vol->properties->iops_min = new_voldesc->iops_min;
    mod_vol->properties->iops_max = new_voldesc->iops_max;
    mod_vol->properties->relativePrio = new_voldesc->relativePrio;
    delete new_voldesc;

    currentDom->domain_ptr->sendModifyVolToFdsNodes(mod_vol);

    om_mutex->unlock();
    return 0;
}

int OrchMgr::CreatePolicy(const FdspMsgHdrPtr& fdsp_msg,
                           const FdspCrtPolPtr& crt_pol_req) {
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
                           const FdspDelPolPtr& del_pol_req) {
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
                          const FdspModPolPtr& mod_pol_req) {
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

int OrchMgr::AttachVol(const FdspMsgHdrPtr &fdsp_msg,
                       const FdspAttVolCmdPtr &atc_vol_req) {
    std::string vol_name = atc_vol_req->vol_name;
    fds_node_name_t node_name = fdsp_msg->src_node_name;
    localDomainInfo  *currentDom;

    FDS_PLOG_SEV(GetLog(), fds_log::notification)
            << "Received Attach Vol Req for volume "
            << vol_name
            << " at node " << node_name;

    /*
     * get the domain Id. If  Domain is not created  use  default domain 
     * for now use default  domain 
     */
    currentDom  = locDomMap[DEFAULT_LOC_DOMAIN_ID];

    om_mutex->lock();
    if (currentDom->domain_ptr->volumeMap.count(vol_name) == 0) {
        FDS_PLOG_SEV(om_log, fds_log::warning)
                << "Received Attach Vol for non-existent volume "
                << vol_name;
        om_mutex->unlock();
        return -1;
    }
    VolumeInfo *this_vol =  currentDom->domain_ptr->volumeMap[vol_name];
#if 0
    // Let's actually allow this, as a means of provisioning before HV is online
    if (currentDom->domain_ptr->currentShMap.count(node_id) == 0) {
        FDS_PLOG(om_log) << "Received Attach Vol for non-existent node " << node_id;
        om_mutex->unlock();
        return -1;
    }
#endif

    for (int i = 0; i < this_vol->hv_nodes.size(); i++) {
        if (this_vol->hv_nodes[i] == node_name) {
            FDS_PLOG_SEV(om_log, fds_log::notification)
                    << "Attach Vol req for volume " << vol_name
                    << " rejected because this volume is "
                    << "already attached at node " << node_name;
            om_mutex->unlock();
            return -1;
        }
    }
    this_vol->hv_nodes.push_back(node_name);
    if (currentDom->domain_ptr->currentShMap.count(node_name) > 0) {
        currentDom->domain_ptr->sendAttachVolToHvNode(node_name, this_vol);
    }
    om_mutex->unlock();
    return 0;
}

int OrchMgr::DetachVol(const FdspMsgHdrPtr    &fdsp_msg,
                       const FdspAttVolCmdPtr &dtc_vol_req) {
    std::string vol_name = dtc_vol_req->vol_name;
    fds_node_name_t node_name = dtc_vol_req->node_id;
    fds_bool_t node_not_attached = true;
    localDomainInfo  *currentDom;

    FDS_PLOG_SEV(GetLog(), fds_log::notification)
            << "Received Detach Vol Req for volume "
            << vol_name
            << " at node " << node_name;

    currentDom  = locDomMap[DEFAULT_LOC_DOMAIN_ID];

    om_mutex->lock();
    if (currentDom->domain_ptr->volumeMap.count(vol_name) == 0) {
        FDS_PLOG_SEV(om_log, fds_log::warning)
                << "Received Detach Vol for non-existent volume "
                << vol_name;
        om_mutex->unlock();
        return -1;
    }
    VolumeInfo *this_vol =  currentDom->domain_ptr->volumeMap[vol_name];
#if 0
    if (currentDom->domain_ptr->currentShMap.count(node_id) == 0) {
        FDS_PLOG(om_log) << "Received Detach Vol for non-existent node " << node_id;
        om_mutex->unlock();
        return -1;
    }
#endif

    for (int i = 0; i < this_vol->hv_nodes.size(); i++) {
        if (this_vol->hv_nodes[i] == node_name) {
            node_not_attached = false;
            this_vol->hv_nodes.erase(this_vol->hv_nodes.begin()+i);
            break;
        }
    }
    if (node_not_attached) {
        FDS_PLOG_SEV(om_log, fds_log::notification)
                << "Detach Vol req for volume " << vol_name
                << " rejected because this volume is "
                << "not attached at node " << node_name;
    } else {
        if (currentDom->domain_ptr->currentShMap.count(node_name) > 0) {
            currentDom->domain_ptr->sendDetachVolToHvNode(node_name, this_vol);
        }
    }
    om_mutex->unlock();
    return 0;
}

void OrchMgr::TestBucket(const FdspMsgHdrPtr& fdsp_msg,
                         const FdspTestBucketPtr& test_buck_req)
{
    localDomainInfo  *currentDom;
    std::string bucket_name = test_buck_req->bucket_name;
    std::string source_node_name = fdsp_msg->src_node_name;
    FDS_PLOG_SEV(GetLog(), fds_log::notification)
            << "OM received test bucket request for bucket "
            << bucket_name
            << " attach_vol_reqd = "
            << test_buck_req->attach_vol_reqd;

    /*
     * get the domain Id. If  Domain is not created  use  default domain 
     * for now use the default domain
     */
    currentDom  = locDomMap[DEFAULT_LOC_DOMAIN_ID];

    /* check if volume exists */
    om_mutex->lock();
    if (currentDom->domain_ptr->volumeMap.count(bucket_name) == 0) {
        FDS_PLOG_SEV(om_log, fds_log::notification)
                << "OM: TestBucket -- bucket " << bucket_name
                << " does not exist, will sent error back to requesting node "
                << source_node_name;

        if (currentDom->domain_ptr->currentShMap.count(source_node_name) > 0) {
            currentDom->domain_ptr->sendTestBucketResponseToHvNode(
                source_node_name, bucket_name, false);
        } else {
            FDS_PLOG_SEV(om_log, fds_log::warning)
                    << "OM: TestBucket -- OM does not know about requesting node "
                    << source_node_name;
        }
    } else if (test_buck_req->attach_vol_reqd == false) {
        /* OM was not requested to attach volume to node, so just returning success */
        FDS_PLOG_SEV(om_log, fds_log::notification)
                << "OM: TestBucket -- bucket " << bucket_name
                << " exists! OM sending success back to requesting node "
                << source_node_name;

        if (currentDom->domain_ptr->currentShMap.count(source_node_name) > 0) {
            currentDom->domain_ptr->sendTestBucketResponseToHvNode(
                source_node_name, bucket_name, true);
        } else {
            FDS_PLOG_SEV(om_log, fds_log::warning)
                    << "OM: TestBucket -- OM does not know about requesting node "
                    << source_node_name;
        }
    } else {
        VolumeInfo *this_vol =  currentDom->domain_ptr->volumeMap[bucket_name];

        for (int i = 0; i < this_vol->hv_nodes.size(); i++) {
            if (this_vol->hv_nodes[i] == source_node_name) {
                FDS_PLOG_SEV(om_log, fds_log::warning)
                        << "OM: TestBucket - bucket " << bucket_name
                        << " is already attached to the requesting node "
                        << source_node_name << " so nothing to do";
                om_mutex->unlock();
                return;
            }
        }
        this_vol->hv_nodes.push_back(source_node_name);
        if (currentDom->domain_ptr->currentShMap.count(source_node_name) > 0) {
            currentDom->domain_ptr->sendAttachVolToHvNode(source_node_name, this_vol);
        }
    }

    om_mutex->unlock();
}

void OrchMgr::RegisterNode(const FdspMsgHdrPtr  &fdsp_msg,
                           const FdspRegNodePtr &reg_node_req) {
    localDomainInfo  *currentDom;

    FDS_PLOG_SEV(GetLog(), fds_log::notification)
            << "Received RegisterNode Msg"
            << "  Node name:" << reg_node_req->node_name
            << "  Node IP:" << std::hex << reg_node_req->ip_lo_addr
            << "  Node Type:" << std::dec << reg_node_req->node_type
            << "  Control Port: " << reg_node_req->control_port
            << "  Data Port: " << reg_node_req->data_port
            << "  Disk iops Max : " << (reg_node_req->disk_info).disk_iops_max
            << "  Disk iops Min: " << (reg_node_req->disk_info).disk_iops_min
            << "  Disk capacity : " << (reg_node_req->disk_info).disk_capacity
            << "  Disk latency Max: " << (reg_node_req->disk_info).disk_latency_max
            << "  Disk latency Min: " << (reg_node_req->disk_info).disk_latency_min
            << "  Ssd iops Max: " << (reg_node_req->disk_info).ssd_iops_max
            << "  Ssd iops Min: " << (reg_node_req->disk_info).ssd_iops_min
            << "  Ssd capacity : " << (reg_node_req->disk_info).ssd_capacity
            << "  Ssd latency Max : " << (reg_node_req->disk_info).ssd_latency_max
            << "  Ssd latency Min: " << (reg_node_req->disk_info).ssd_latency_min
            << "  Disk Type : " << (reg_node_req->disk_info).disk_type;
    /*
     * get the domain Id. If  Domain is not created  use  default domain 
     * for now use  default  domain 
     */
    currentDom  = locDomMap[DEFAULT_LOC_DOMAIN_ID];

    fds::FdsLocalDomain::node_map_t& node_map =
            (reg_node_req->node_type == FDS_ProtocolInterface::FDSP_STOR_MGR) ?
            currentDom->domain_ptr->currentSmMap :
            ((reg_node_req->node_type == FDS_ProtocolInterface::FDSP_DATA_MGR) ?
             currentDom->domain_ptr->currentDmMap : currentDom->domain_ptr->currentShMap);

    om_mutex->lock();

    if (node_map.count(reg_node_req->node_name) > 0) {
        FDS_PLOG_SEV(GetLog(), fds_log::notification)
                << "Duplicate Node Registration for "
                << reg_node_req->node_name;
    }

    fds_int32_t new_node_id =
            currentDom->domain_ptr->getFreeNodeId(reg_node_req->node_name);

    /*
     * Build the node info structure and add it
     * to its map, based on type.
     */
    OM_NodeDomainMod *domain = OM_NodeDomainMod::om_local_domain();
    NodeUuid new_node_uuid(fds_get_uuid64(reg_node_req->node_name));
    Error err = domain->om_reg_node_info(new_node_uuid, reg_node_req);
    if (!err.ok()) {
        FDS_PLOG_SEV(GetLog(), fds_log::error)
                << "Node Registration failed for "
                << reg_node_req->node_name << ", result: "
                << err.GetErrstr();
        return;
    }

    fds::NodeInfo n_info(new_node_id,
                         reg_node_req->node_name,
                         reg_node_req->node_type,
                         reg_node_req->ip_lo_addr,
                         reg_node_req->control_port,
                         n_info.data_port = reg_node_req->data_port,
                         (reg_node_req->disk_info).disk_iops_max,
                         (reg_node_req->disk_info).disk_iops_min,
                         (reg_node_req->disk_info).disk_capacity,
                         (reg_node_req->disk_info).disk_latency_max,
                         (reg_node_req->disk_info).disk_latency_min,
                         (reg_node_req->disk_info).ssd_iops_max,
                         (reg_node_req->disk_info).ssd_iops_min,
                         (reg_node_req->disk_info).ssd_capacity,
                         (reg_node_req->disk_info).ssd_latency_max,
                         (reg_node_req->disk_info).ssd_latency_min,
                         (reg_node_req->disk_info).disk_type,
                         n_info.node_state = FDS_ProtocolInterface::FDS_Node_Up,
                         omcp_session_tbl,
                         cp_resp_handler);

    node_map[reg_node_req->node_name] = n_info;

    // If this is a SM or a DM, let existing nodes know about this node
    if (reg_node_req->node_type != FDS_ProtocolInterface::FDSP_STOR_HVISOR) {
        currentDom->domain_ptr->sendNodeEventToFdsNodes(n_info, n_info.node_state);

        /* update the disk capabilities */
        currentDom->domain_ptr->admin_ctrl->addDiskCapacity(n_info);
    }

    // Let this new node know about the existing node list
    // TODO(Andrew): This should change into dissemination of
    // the current cluster map
    currentDom->domain_ptr->sendMgrNodeListToFdsNode(n_info);

    // Let this new node know about the existing volumes.
    // If it's a HV node, send only the volumes it need to attach
    if (reg_node_req->node_type == FDS_ProtocolInterface::FDSP_STOR_HVISOR) {
        currentDom->domain_ptr->sendAllVolumesToHvNode(reg_node_req->node_name);
    } else {
        currentDom->domain_ptr->sendAllVolumesToFdsMgrNode(n_info);
    }

    om_mutex->unlock();

    // TODO(Andrew): Return registration success
    // This is replying on the OM's global config interface,
    // not the per-node channel that's established during
    // registration/bootstrap
    currentDom->domain_ptr->sendRegRespToNode(n_info, ERR_OK);

    // TODO(Andrew): For now, let's start the cluster update process
    // now. This should eventually be decoupled from registration.
    domain->om_update_cluster();
}

void OrchMgr::SetThrottleLevelForDomain(int domain_id, float throttle_level) {
    localDomainInfo  *currentDom;

    FDS_PLOG_SEV(GetLog(), fds_log::notification)
            << "Setting throttle level for domain "
            << domain_id << " at " << throttle_level;

    /*
     * get the domain Id. If  Domain is not created  use  default domain 
     * for now use the default domain
     */

    currentDom  = locDomMap[DEFAULT_LOC_DOMAIN_ID];
    currentDom->domain_ptr->current_throttle_level = throttle_level;
    currentDom->domain_ptr->sendThrottleLevelToHvNodes(throttle_level);
}

int OrchMgr::SetThrottleLevel(const FDSP_MsgHdrTypePtr& fdsp_msg,
                              const FDSP_ThrottleMsgTypePtr& throttle_req) {
    om_mutex->lock();
    assert((throttle_req->throttle_level >= -10) && (throttle_req->throttle_level <= 10));
    SetThrottleLevelForDomain(throttle_req->domain_id, throttle_req->throttle_level);
    om_mutex->unlock();
    return 0;
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

    localDomainInfo  *currentDom = locDomMap[DEFAULT_LOC_DOMAIN_ID];

    // For now we will ignore if the calculated throttle level is greater than
    // the current throttle level. But it will have to be more complicated than this.
    // Every time we set a throttle level, we need to fire off a timer and
    // bring back to the normal throttle level (may be gradually) unless
    // we get more of these QueueFull messages, in which case, we will have to
    // extend the throttle period.

    if (throttle_level < currentDom->domain_ptr->current_throttle_level) {
        SetThrottleLevelForDomain(DEFAULT_LOC_DOMAIN_ID, throttle_level);
    } else {
        FDS_PLOG(GetLog()) << "Calculated throttle level " << throttle_level
                           << " less than current throttle level of "
                           << currentDom->domain_ptr->current_throttle_level
                           << ". Ignoring.";
    }
    om_mutex->unlock();
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

    localDomainInfo *currentDomain = locDomMap[DEFAULT_LOC_DOMAIN_ID];

    if (perf_stats_msg->node_type == FDS_ProtocolInterface::FDSP_STOR_HVISOR) {
        FDS_PLOG(GetLog())
                << "OM received perfstats from AM, start ts "
                << perf_stats_msg->start_timestamp;
        currentDomain->domain_ptr->handlePerfStatsFromAM(perf_stats_msg->vol_hist_list,
                                                         perf_stats_msg->start_timestamp);

        for (int i = 0; i < (perf_stats_msg->vol_hist_list).size(); ++i)
        {
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
        /* we need to decide whether we want to merge stats from multiple SMs from one volume
         * or have them separate. Should just mostly follow the code of handling stats from AM 
         * but for now output debug msg to the log */
        for (int i = 0; i < (perf_stats_msg->vol_hist_list).size(); ++i)
        {
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
