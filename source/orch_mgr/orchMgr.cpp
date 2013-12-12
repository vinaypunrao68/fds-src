/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#include "orchMgr.h"

#include <iostream>  // NOLINT(*)
#include <string>
#include <vector>

namespace fds {

OrchMgr *orchMgr;

OrchMgr::OrchMgr()
    : port_num(0),
      test_mode(false) {
  om_log = new fds_log("om", "logs");
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

  delete om_log;
  if (policy_mgr)
    delete policy_mgr;
}

int OrchMgr::run(int argc, char* argv[]) {
  /*
   * Process the cmdline args.
   */
  for (int i = 1; i < argc; i++) {
    if (strncmp(argv[i], "--port=", 7) == 0) {
      port_num = strtoul(argv[i] + 7, NULL, 0);
    } else if (strncmp(argv[i], "--prefix=", 9) == 0) {
      stor_prefix = argv[i] + 9;
    } else if (strncmp(argv[i], "--test", 7) == 0) {
      test_mode = true;
    }
  }

  GetLog()->setSeverityFilter((fds_log::severity_level) (getSysParams()->log_severity));

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

  Ice::PropertiesPtr props = communicator()->getProperties();

  /*
   * create a default policy ( ID = 50) for S3 bucket
   */

  /*
   * Set basic thread properties.
   */
  props->setProperty("OrchMgr.ThreadPool.Size", "50");
  props->setProperty("OrchMgr.ThreadPool.SizeMax", "100");
  props->setProperty("OrchMgr.ThreadPool.SizeWarn", "75");

  std::string orchMgrIPAddress;
  int orchMgrPortNum;

  FDS_PLOG(om_log) << " port num rx " << port_num;
  if (port_num != 0) {
    orchMgrPortNum = port_num;
  } else {
    orchMgrPortNum = props->getPropertyAsInt("OrchMgr.ConfigPort");
  }
  orchMgrIPAddress = props->getProperty("OrchMgr.IPAddress");

  FDS_PLOG_SEV(om_log, fds::fds_log::notification) << "Orchestration Manager using port " << orchMgrPortNum;

  callbackOnInterrupt();

  /*
   * setup CLI client adaptor interface  this also used for receiving the node up
   * messages from DM, SH and SM 
   */
  std::ostringstream tcpProxyStr;
  tcpProxyStr << "tcp -p " << orchMgrPortNum;
  Ice::ObjectAdapterPtr adapter =
      communicator()->createObjectAdapterWithEndpoints(
          "OrchMgr",
          tcpProxyStr.str());

  reqCfgHandlersrv = new ReqCfgHandler(this);
  adapter->add(reqCfgHandlersrv, communicator()->stringToIdentity("OrchMgr"));

  om_policy_srv = new Orch_VolPolicyServ();
  om_ice_proxy  = new Ice_VolPolicyServ(ORCH_MGR_POLICY_ID, *om_policy_srv);
  om_ice_proxy->serv_registerIceAdapter(communicator(), adapter);

  adapter->activate();

  defaultS3BucketPolicy();
  communicator()->waitForShutdown();

  return EXIT_SUCCESS;
}

void OrchMgr::setSysParams(SysParams *params) {
  sysParams = params;
}

SysParams* OrchMgr::getSysParams() {
  return sysParams;
}

fds_log* OrchMgr::GetLog() {
  return om_log;
}
 
void
OrchMgr::interruptCallback(int cb) {
  FDS_PLOG(orchMgr->GetLog()) << "Shutting down communicator";
  communicator()->shutdown();
}

// config path request  handler
OrchMgr::ReqCfgHandler::ReqCfgHandler(OrchMgr *oMgr) {
  this->orchMgr = oMgr;
}

OrchMgr::ReqCfgHandler::~ReqCfgHandler() {
}

int OrchMgr::CreateDomain(const FdspMsgHdrPtr& fdsp_msg,
                        const FdspCrtDomPtr& crt_dom_req) {

  FDS_PLOG_SEV(GetLog(), fds::fds_log::notification) << "Received CreateDomain Req : "
						     << crt_dom_req->domain_name  
						     << " :domain_id - " << crt_dom_req->domain_id;

   /* check  for duplicate domain id */
  om_mutex->lock();
  if ( locDomMap.count(crt_dom_req->domain_id) == 0) {
    FDS_PLOG_SEV(om_log, fds::fds_log::warning) << "Duplicate domain: " << crt_dom_req->domain_id;
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

void OrchMgr::GetDomainStats(const FdspMsgHdrPtr& fdsp_msg,
			     const FdspGetDomStatsPtr& get_stats_req)
{
  int domain_id = get_stats_req->domain_id;
  localDomainInfo  *currentDom = NULL;

  FDS_PLOG_SEV(GetLog(), fds::fds_log::normal) << "Received GetDomainStats Req for domain " 
					       << domain_id;

  /*
   * Use default domain for now... 
   */
  FDS_PLOG_SEV(GetLog(), fds::fds_log::warning) << "For now always returning stats of default domain. "
						<< "Domain id in the request is ignored";

  currentDom  = locDomMap[DEFAULT_LOC_DOMAIN_ID];

  if (currentDom) {
    om_mutex->lock();
    currentDom->domain_ptr->sendBucketStats(5, fdsp_msg->src_node_name, fdsp_msg->req_cookie);
    /* if need to test printing to json file, call this func instead .. */
    //    currentDom->domain_ptr->printStatsToJsonFile();
    om_mutex->unlock();
  }

}

int OrchMgr::CreateVol(const FdspMsgHdrPtr& fdsp_msg,
                        const FdspCrtVolPtr& crt_vol_req) {
  Error err(ERR_OK);
  // int  vol_id = crt_vol_req->vol_info->volUUID;
  int  domain_id = crt_vol_req->vol_info->localDomainId;
  std::string vol_name = crt_vol_req->vol_info->vol_name;
  localDomainInfo  *currentDom;

  FDS_PLOG_SEV(GetLog(), fds::fds_log::notification) << "Received CreateVol Req for volume "
						     << vol_name ;

  /*
   * get the domain Id. If  Domain is not created  use  default domain 
   * for now use the default domain
   */
   currentDom  = locDomMap[DEFAULT_LOC_DOMAIN_ID];

   
  om_mutex->lock();

  if ( currentDom->domain_ptr->volumeMap.count(vol_name) != 0) {
    FDS_PLOG_SEV(om_log, fds::fds_log::warning) << "Received CreateVol for existing volume " << vol_name;
    om_mutex->unlock();
    return -1;
  }
  VolumeInfo *new_vol = new VolumeInfo();
  new_vol->vol_name = vol_name;
  new_vol->volUUID = currentDom->domain_ptr->getNextFreeVolId();
  FDS_PLOG_SEV(GetLog(), fds::fds_log::normal) << " created volume ID " << new_vol->volUUID; 
  new_vol->properties = new VolumeDesc(crt_vol_req->vol_info, new_vol->volUUID);
  err = policy_mgr->fillVolumeDescPolicy(new_vol->properties);
  if ( err == ERR_CAT_ENTRY_NOT_FOUND ) {
      /* TODO: policy not in the catalog , should we return error or use default policu */
    FDS_PLOG_SEV(GetLog(), fds::fds_log::warning) << "Create volume " << vol_name 
						  << " Req requested unknown policy " << (crt_vol_req->vol_info)->volPolicyId;    
  }
  else if (!err.ok()) {
    FDS_PLOG_SEV(GetLog(), fds::fds_log::error) << "CreateVol: Failed to get policy info for volume " << vol_name;
      /* TODO: for now ignoring error */
  }
   /*
    * check the resource  availability 
    */

  err = currentDom->domain_ptr->admin_ctrl->volAdminControl(new_vol->properties);
  if ( !err.ok() ) {
    delete new_vol;
    om_mutex->unlock();
    FDS_PLOG_SEV(GetLog(), fds::fds_log::error) << "Unable to create Volume " << vol_name 
						<< "; result" << err.GetErrstr();
    return -1; 
  }

  currentDom->domain_ptr->volumeMap[vol_name] = new_vol;
  currentDom->domain_ptr->sendCreateVolToFdsNodes(new_vol);
 /* update the per domain disk resource  */
  om_mutex->unlock();
 return 0;
}

void OrchMgr::DeleteVol(const FdspMsgHdrPtr& fdsp_msg,
                        const FdspDelVolPtr& del_vol_req) {
  // int  vol_id = del_vol_req->vol_uuid;
  std::string vol_name = del_vol_req->vol_name;
  VolumeInfo *cur_vol;
  localDomainInfo  *currentDom;

  FDS_PLOG_SEV(GetLog(), fds::fds_log::notification) << "Received DeleteVol Req for volume "
						     << vol_name ;

  /*
   * get the domain Id. If  Domain is not created  use  default domain 
   * for now  use the default domain
   */
   currentDom  = locDomMap[DEFAULT_LOC_DOMAIN_ID];

  om_mutex->lock();
  if ( currentDom->domain_ptr->volumeMap.count(vol_name) == 0) {
    FDS_PLOG_SEV(om_log, fds::fds_log::warning) << "Received DeleteVol for non-existent volume " << vol_name;
    om_mutex->unlock();
    return;
  }
  VolumeInfo *del_vol =  currentDom->domain_ptr->volumeMap[vol_name];

  for (int i = 0; i < del_vol->hv_nodes.size(); i++) {
    if (currentDom->domain_ptr->currentShMap.count(del_vol->hv_nodes[i]) == 0) {
      FDS_PLOG_SEV(om_log, fds::fds_log::error) << "Inconsistent State Detected. "
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
}

void OrchMgr::ModifyVol(const FdspMsgHdrPtr& fdsp_msg,
                        const FdspModVolPtr& mod_vol_req) 
{
  Error err(ERR_OK);
  std::string vol_name = mod_vol_req->vol_desc->vol_name;
  localDomainInfo  *currentDom = NULL;
  VolumeDesc *new_voldesc = NULL;

  FDS_PLOG_SEV(GetLog(), fds::fds_log::notification) << "Received ModifyVol Msg for volume " << vol_name;

  /*
   * get the domain Id. If  Domain is not created  use  default domain 
   * for now  use the default domain
   */
   currentDom  = locDomMap[DEFAULT_LOC_DOMAIN_ID];

  om_mutex->lock();
  if ( currentDom->domain_ptr->volumeMap.count(vol_name) == 0) {
    FDS_PLOG_SEV(om_log, fds::fds_log::warning) << "Received ModifyVol for non-existent volume " << vol_name;
    om_mutex->unlock();
    return;
  }
  VolumeInfo *mod_vol =  currentDom->domain_ptr->volumeMap[vol_name];
  new_voldesc = new VolumeDesc(*(mod_vol->properties));

  /* we will not modify capacity for now, just policy id or min/max iops and priority */
  if (mod_vol_req->vol_desc->volPolicyId != 0) {
    /* change policy id and get its description from the catalog */
    new_voldesc->volPolicyId = mod_vol_req->vol_desc->volPolicyId;
    err = policy_mgr->fillVolumeDescPolicy(new_voldesc);
    if ( err == ERR_CAT_ENTRY_NOT_FOUND ) {
      /* policy not in the catalog , revert to old policy id and return */
      FDS_PLOG_SEV(GetLog(), fds::fds_log::warning) << "Modify volume " << vol_name 
						    << " requested unknown policy " << (mod_vol_req->vol_desc)->volPolicyId
						    << "; keeping original policy " << mod_vol->properties->volPolicyId;
    }
    else if (!err.ok()) {
      FDS_PLOG_SEV(GetLog(), fds::fds_log::error) << "ModifyVol: volume " << vol_name 
						  << " - Failed to get policy info for policy " << (mod_vol_req->vol_desc)->volPolicyId
						  << "; keeping original policy " << mod_vol->properties->volPolicyId;
    }

    /* cleanup and return if error */
    if (!err.ok()) {
      /* we did not copy anything to the actual volume desc yet, so just delete new_voldesc */
      delete new_voldesc;
      om_mutex->unlock();
      return;
    }
  }
  else {
    /* don't modify policy id, just min/max iops and priority in volume descriptor */
    new_voldesc->iops_min = mod_vol_req->vol_desc->iops_min;
    new_voldesc->iops_max = mod_vol_req->vol_desc->iops_max;
    new_voldesc->relativePrio = mod_vol_req->vol_desc->rel_prio;
    FDS_PLOG_SEV(GetLog(), fds::fds_log::notification) << "ModifyVol: volume " << vol_name 
						       << " - keeps policy id " << new_voldesc->volPolicyId
						       << " with new min_iops " << new_voldesc->iops_min
						       << " max_iops " << new_voldesc->iops_max
						       << " priority " << new_voldesc->relativePrio;
  }

  /* check if this volume can go through admission control with modified policy info */
  err = currentDom->domain_ptr->admin_ctrl->checkVolModify(mod_vol->properties, new_voldesc);
  if ( !err.ok() ) {
      /* we did not copy anything to the actual volume desc yet, so just delete new_voldesc */
    delete new_voldesc;
    om_mutex->unlock();
    FDS_PLOG_SEV(GetLog(), fds::fds_log::error) << "ModifyVol: volume " << vol_name 
						<< " -- cannot admit with new policy info, keeping the old policy";
    return; 
  }

  /* we admitted modified policy -- copy updated volume desc into volume info */
  mod_vol->properties->volPolicyId = new_voldesc->volPolicyId;
  mod_vol->properties->iops_min = new_voldesc->iops_min;
  mod_vol->properties->iops_max = new_voldesc->iops_max;
  mod_vol->properties->relativePrio = new_voldesc->relativePrio;
  delete new_voldesc;

  currentDom->domain_ptr->sendModifyVolToFdsNodes(mod_vol);

  om_mutex->unlock();
}

void OrchMgr::CreatePolicy(const FdspMsgHdrPtr& fdsp_msg,
                           const FdspCrtPolPtr& crt_pol_req) {
  Error err(ERR_OK);
  int policy_id = crt_pol_req->policy_info->policy_id;

  FDS_PLOG_SEV(GetLog(), fds::fds_log::notification) << "Received CreatePolicy  Msg for policy "
						     << crt_pol_req->policy_info->policy_name
						     << ", id" << policy_id;

  om_mutex->lock();
  err = policy_mgr->createPolicy(crt_pol_req->policy_info);
  om_mutex->unlock();
}

void OrchMgr::DeletePolicy(const FdspMsgHdrPtr& fdsp_msg,
                           const FdspDelPolPtr& del_pol_req) {
  Error err(ERR_OK);
  int policy_id = del_pol_req->policy_id;
  std::string policy_name = del_pol_req->policy_name;
  FDS_PLOG_SEV(GetLog(), fds::fds_log::notification) << "Received DeletePolicy  Msg for policy "
						     << policy_name << ", id " << policy_id;

  om_mutex->lock();
  err = policy_mgr->deletePolicy(policy_id, policy_name);
  if (err.ok()) {
    /* removed policy from policy catalog or policy didn't exist 
     * TODO: what do we do with volumes that use policy we deleted ? */
  }
  om_mutex->unlock();
}

void OrchMgr::ModifyPolicy(const FdspMsgHdrPtr& fdsp_msg,
                           const FdspModPolPtr& mod_pol_req) {
  Error err(ERR_OK);
  int policy_id = mod_pol_req->policy_info->policy_id;
  FDS_PLOG_SEV(GetLog(), fds::fds_log::notification) << "Received ModifyPolicy  Msg for policy "
						     << mod_pol_req->policy_info->policy_name
						     << ", id " << policy_id;

  om_mutex->lock();
  err = policy_mgr->modifyPolicy(mod_pol_req->policy_info);
  if (err.ok()) {
    /* modified policy in the policy catalog 
     * TODO: we probably should send modify volume messages to SH/DM, etc.  O */
  }
  om_mutex->unlock();
}

void OrchMgr::AttachVol(const FdspMsgHdrPtr &fdsp_msg,
                        const FdspAttVolCmdPtr &atc_vol_req) {
  // int  vol_id = atc_vol_req->vol_uuid;
  std::string vol_name = atc_vol_req->vol_name;
  fds_node_name_t node_name = fdsp_msg->src_node_name;
  localDomainInfo  *currentDom;

  FDS_PLOG_SEV(GetLog(), fds::fds_log::notification) << "Received Attach Vol Req for volume "
						     << vol_name
						     << " at node " << node_name;

  /*
   * get the domain Id. If  Domain is not created  use  default domain 
   * for now use default  domain 
   */
    currentDom  = locDomMap[DEFAULT_LOC_DOMAIN_ID];

  om_mutex->lock();
  if ( currentDom->domain_ptr->volumeMap.count(vol_name) == 0) {
    FDS_PLOG_SEV(om_log, fds::fds_log::warning) << "Received Attach Vol for non-existent volume "
						<< vol_name;
    om_mutex->unlock();
    return;
  }
  VolumeInfo *this_vol =  currentDom->domain_ptr->volumeMap[vol_name];
#if 0
  // Let's actually allow this, as a means of provisioning before HV is online
  if (currentDom->domain_ptr->currentShMap.count(node_id) == 0) {
    FDS_PLOG(om_log) << "Received Attach Vol for non-existent node " << node_id;
    om_mutex->unlock();
    return;
  }
#endif

  for (int i = 0; i < this_vol->hv_nodes.size(); i++) {
    if (this_vol->hv_nodes[i] == node_name) {
      FDS_PLOG_SEV(om_log, fds::fds_log::notification) << "Attach Vol req for volume " << vol_name
						       << " rejected because this volume is "
						       << "already attached at node " << node_name;
      om_mutex->unlock();
      return;
    }
  }
  this_vol->hv_nodes.push_back(node_name);
  if (currentDom->domain_ptr->currentShMap.count(node_name) > 0) {
    currentDom->domain_ptr->sendAttachVolToHvNode(node_name, this_vol);
  }
  om_mutex->unlock();
}



void OrchMgr::DetachVol(const FdspMsgHdrPtr    &fdsp_msg,
                        const FdspAttVolCmdPtr &dtc_vol_req) {
  // int  vol_id = dtc_vol_req->vol_uuid;
  std::string vol_name = dtc_vol_req->vol_name;
  fds_node_name_t node_name = dtc_vol_req->node_id;
  fds_bool_t node_not_attached = true;
  localDomainInfo  *currentDom;

  FDS_PLOG_SEV(GetLog(), fds::fds_log::notification) << "Received Detach Vol Req for volume "
						     << vol_name
						     << " at node " << node_name;

  currentDom  = locDomMap[DEFAULT_LOC_DOMAIN_ID];

  om_mutex->lock();
  if ( currentDom->domain_ptr->volumeMap.count(vol_name) == 0) {
    FDS_PLOG_SEV(om_log, fds::fds_log::warning) << "Received Detach Vol for non-existent volume "
						<< vol_name;
    om_mutex->unlock();
    return;
  }
  VolumeInfo *this_vol =  currentDom->domain_ptr->volumeMap[vol_name];
#if 0
  if (currentDom->domain_ptr->currentShMap.count(node_id) == 0) {
    FDS_PLOG(om_log) << "Received Detach Vol for non-existent node " << node_id;
    om_mutex->unlock();
    return;
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
    FDS_PLOG_SEV(om_log, fds::fds_log::notification) << "Detach Vol req for volume " << vol_name
						     << " rejected because this volume is "
						     << "not attached at node " << node_name;
  } else {
    if (currentDom->domain_ptr->currentShMap.count(node_name) > 0) {
      currentDom->domain_ptr->sendDetachVolToHvNode(node_name, this_vol);
    }
  }
  om_mutex->unlock();
}

void OrchMgr::TestBucket(const FdspMsgHdrPtr& fdsp_msg,
			 const FdspTestBucketPtr& test_buck_req)
{
  localDomainInfo  *currentDom;
  std::string bucket_name = test_buck_req->bucket_name;
  std::string source_node_name = fdsp_msg->src_node_name;
  FDS_PLOG_SEV(GetLog(), fds::fds_log::notification) << "OM received test bucket request for bucket "
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
  if ( currentDom->domain_ptr->volumeMap.count(bucket_name) == 0) {
    FDS_PLOG_SEV(om_log, fds::fds_log::notification) << "OM: TestBucket -- bucket " << bucket_name
						     << " does not exist, will sent error back to requesting node "
						     << source_node_name;

    if (currentDom->domain_ptr->currentShMap.count(source_node_name) > 0) {
      currentDom->domain_ptr->sendTestBucketResponseToHvNode(source_node_name, bucket_name, false);
    }
    else {
      FDS_PLOG_SEV(om_log, fds::fds_log::warning) << "OM: TestBucket -- OM does not know about requesting node "
						  << source_node_name;
    }
  }
  else if (test_buck_req->attach_vol_reqd == false) {
    /* OM was not requested to attach volume to node, so just returning success */
    FDS_PLOG_SEV(om_log, fds::fds_log::notification) << "OM: TestBucket -- bucket " << bucket_name
						     << " exists! OM sending success back to requesting node "
						     << source_node_name;

    if (currentDom->domain_ptr->currentShMap.count(source_node_name) > 0) {
      currentDom->domain_ptr->sendTestBucketResponseToHvNode(source_node_name, bucket_name, true);
    }
    else {
      FDS_PLOG_SEV(om_log, fds::fds_log::warning) << "OM: TestBucket -- OM does not know about requesting node "
						  << source_node_name;
    }
  }
  else {
    VolumeInfo *this_vol =  currentDom->domain_ptr->volumeMap[bucket_name];

    for (int i = 0; i < this_vol->hv_nodes.size(); i++) {
      if (this_vol->hv_nodes[i] == source_node_name) {
	FDS_PLOG_SEV(om_log, fds::fds_log::warning) << "OM: TestBucket - bucket " << bucket_name
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
  std::string ip_addr_str;
  Ice::Identity ident;
  std::ostringstream tcpProxyStr;
  localDomainInfo  *currentDom;

  FDS_PLOG_SEV(GetLog(), fds::fds_log::notification) << "Received RegisterNode Msg"
                     << "  Node name:" << reg_node_req->node_name
                     << "  Node IP:" << std::hex << reg_node_req->ip_lo_addr
                     << "  Node Type:" << std::dec << reg_node_req->node_type
                     << "  Control Port: " << reg_node_req->control_port
                     << "  Data Port: " << reg_node_req->data_port
                     << "  Disk iops Max : " << reg_node_req->disk_info->disk_iops_max
                     << "  Disk iops Min: " << reg_node_req->disk_info->disk_iops_min
                     << "  Disk capacity : " << reg_node_req->disk_info->disk_capacity
                     << "  Disk latency Max: " << reg_node_req->disk_info->disk_latency_max
                     << "  Disk latency Min: " << reg_node_req->disk_info->disk_latency_min
                     << "  Ssd iops Max: " << reg_node_req->disk_info->ssd_iops_max
                     << "  Ssd iops Min: " << reg_node_req->disk_info->ssd_iops_min
                     << "  Ssd capacity : " << reg_node_req->disk_info->ssd_capacity
                     << "  Ssd latency Max : " << reg_node_req->disk_info->ssd_latency_max
                     << "  Ssd latency Min: " << reg_node_req->disk_info->ssd_latency_min
                     << "  Disk Type : " << reg_node_req->disk_info->disk_type;
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

  ip_addr_str = ipv4_addr_to_str(reg_node_req->ip_lo_addr);

  // create a new  control  communication adaptor
  if (test_mode == true) {
    tcpProxyStr << "OrchMgrClient" << reg_node_req->control_port
                << ": tcp -h " << ip_addr_str
                << " -p  " << reg_node_req->control_port;
  } else {
    tcpProxyStr << "OrchMgrClient: tcp -h " << ip_addr_str
                << " -p  " << reg_node_req->control_port;
  }


  om_mutex->lock();

  if (node_map.count(reg_node_req->node_name) > 0) {
    FDS_PLOG_SEV(GetLog(),fds:: fds_log::notification) << "Duplicate Node Registration for "
						       << reg_node_req->node_name;
  }

  fds_int32_t new_node_id = currentDom->domain_ptr->getFreeNodeId(reg_node_req->node_name);

  FDS_PLOG_SEV(GetLog(), fds::fds_log::notification) << "Assigning node id " << new_node_id << " to node " << reg_node_req->node_name 
						     << ". Trying to connect at " << tcpProxyStr.str();


  /*
   * Build the node info structure and add it
   * to its map, based on type.
   */
  fds::NodeInfo n_info(new_node_id,
                  reg_node_req->node_name,
                  reg_node_req->node_type,
                  reg_node_req->ip_lo_addr,
                  reg_node_req->control_port,
                  n_info.data_port = reg_node_req->data_port,
		  reg_node_req->disk_info->disk_iops_max,
		  reg_node_req->disk_info->disk_iops_min,
		  reg_node_req->disk_info->disk_capacity,
		  reg_node_req->disk_info->disk_latency_max,
		  reg_node_req->disk_info->disk_latency_min,
		  reg_node_req->disk_info->ssd_iops_max,
		  reg_node_req->disk_info->ssd_iops_min,
		  reg_node_req->disk_info->ssd_capacity,
		  reg_node_req->disk_info->ssd_latency_max,
		  reg_node_req->disk_info->ssd_latency_min,
		  reg_node_req->disk_info->disk_type,
                  n_info.node_state = FDS_ProtocolInterface::FDS_Node_Up,
                  FDS_ProtocolInterface::
                  FDSP_ControlPathReqPrx::
                  checkedCast(communicator()->stringToProxy(
                      tcpProxyStr.str())));


  node_map[reg_node_req->node_name] = n_info;

  // If this is a SM or a DM, let existing nodes know about this node
  if (reg_node_req->node_type != FDS_ProtocolInterface::FDSP_STOR_HVISOR) {
    currentDom->domain_ptr->sendNodeEventToFdsNodes(n_info, n_info.node_state);

    /* update the disk capabilities */
    currentDom->domain_ptr->admin_ctrl->addDiskCapacity(n_info);
  }

  // Let this new node know about the existing node list
  currentDom->domain_ptr->sendMgrNodeListToFdsNode(n_info);

  // Let this new node know about the existing volumes.
  // If it's a HV node, send only the volumes it need to attach
  if (reg_node_req->node_type == FDS_ProtocolInterface::FDSP_STOR_HVISOR) {
    currentDom->domain_ptr->sendAllVolumesToHvNode(reg_node_req->node_name);
  } else {
    currentDom->domain_ptr->sendAllVolumesToFdsMgrNode(n_info);
  }

  om_mutex->unlock();

  /*
   * Recompute the DLT/DMT. This reacquires
   * the om_lock internally.
   */
  currentDom->domain_ptr->updateTables();

  /*
   * Send the DLT/DMT to the other nodes
   */
  currentDom->domain_ptr->sendNodeTableToFdsNodes(table_type_dlt);
  currentDom->domain_ptr->sendNodeTableToFdsNodes(table_type_dmt);
}

void OrchMgr::SetThrottleLevelForDomain(int domain_id, float throttle_level) {

  localDomainInfo  *currentDom;

  FDS_PLOG_SEV(GetLog(), fds::fds_log::notification) << "Setting throttle level for domain "
						     << domain_id << " at " << throttle_level;

  /*
   * get the domain Id. If  Domain is not created  use  default domain 
   * for now use the default domain
   */

  currentDom  = locDomMap[DEFAULT_LOC_DOMAIN_ID];
  currentDom->domain_ptr->current_throttle_level = throttle_level;
  currentDom->domain_ptr->sendThrottleLevelToHvNodes(throttle_level);

}

void OrchMgr::SetThrottleLevel(const FDSP_MsgHdrTypePtr& fdsp_msg, 
			       const FDSP_ThrottleMsgTypePtr& throttle_req) {

  om_mutex->lock();
  assert((throttle_req->throttle_level >= -10) && (throttle_req->throttle_level <= 10));
  SetThrottleLevelForDomain(throttle_req->domain_id, throttle_req->throttle_level);
  om_mutex->unlock();
  
}

void OrchMgr::NotifyQueueFull(const FDSP_MsgHdrTypePtr& fdsp_msg,
			      const FDSP_NotifyQueueStateTypePtr& queue_state_req) {

  // Use some simple logic for now to come up with the throttle level
  // based on the queue_depth for queues of various pririty

  om_mutex->lock();
  FDSP_QueueStateListType& que_st_list = queue_state_req->queue_state_list;
  int min_priority = que_st_list[0]->priority;
  int min_p_q_depth = que_st_list[0]->queue_depth;

  for (int i = 0; i < que_st_list.size(); i++) {
    FDSP_QueueStateTypePtr& que_st = que_st_list[i];
    FDS_PLOG_SEV(GetLog(), fds::fds_log::notification) << "Received queue full for volume "
						       << que_st->vol_uuid << ", priority - " << que_st->priority
						       << " queue_depth - " << que_st->queue_depth;

    assert((que_st->priority >= 0) && (que_st->priority <= 10));
    assert((que_st->queue_depth >= 0.5) && (que_st->queue_depth <= 1));
    if (que_st->priority < min_priority) {
      min_priority = que_st->priority;
      min_p_q_depth = que_st->queue_depth;
    }
  }

  float throttle_level = (float) min_priority + (float) (1-min_p_q_depth)/0.5;

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
  FDS_PLOG(GetLog()) << "OM received perfstats from node of type: " << perf_stats_msg->node_type
		     << " start ts " << perf_stats_msg->start_timestamp;

  /* Since we do not negotiate yet (should we?) the slot length of stats with AM and SM
   * the stat slot length in AM and SM should be FDS_STAT_DEFAULT_SLOT_LENGTH */
  fds_verify(perf_stats_msg->slot_len_sec == FDS_STAT_DEFAULT_SLOT_LENGTH); 

  localDomainInfo *currentDomain = locDomMap[DEFAULT_LOC_DOMAIN_ID];

  if (perf_stats_msg->node_type == FDS_ProtocolInterface::FDSP_STOR_HVISOR) {
    FDS_PLOG(GetLog()) << "OM received perfstats from AM, start ts " << perf_stats_msg->start_timestamp;
    currentDomain->domain_ptr->handlePerfStatsFromAM(perf_stats_msg->vol_hist_list, 
						     perf_stats_msg->start_timestamp);

    for (int i = 0; i < (perf_stats_msg->vol_hist_list).size(); ++i)
      {
	FDSP_VolPerfHistTypePtr vol_hist = (perf_stats_msg->vol_hist_list)[i];
	FDS_PLOG(GetLog()) << "OM: received perfstat for vol " << vol_hist->vol_uuid;
	for (int j = 0; j < (vol_hist->stat_list).size(); ++j) {
	  FDSP_PerfStatTypePtr stat = (vol_hist->stat_list)[j];
	  FDS_PLOG_SEV(GetLog(), fds::fds_log::debug) << "OM: --- stat_type " << stat->stat_type 
						       << " rel_secs " << stat->rel_seconds   	
						       << " iops " << stat->nios << " lat " << stat->ave_lat;
	}
      }

  }
  else if (perf_stats_msg->node_type == FDS_ProtocolInterface::FDSP_STOR_MGR) {
    FDS_PLOG(GetLog()) << "OM received perfstats from SM, start ts " << perf_stats_msg->start_timestamp;
    /* we need to decide whether we want to merge stats from multiple SMs from one volume
     * or have them separate. Should just mostly follow the code of handling stats from AM 
     * but for now output debug msg to the log */
    for (int i = 0; i < (perf_stats_msg->vol_hist_list).size(); ++i)
      {
	FDSP_VolPerfHistTypePtr vol_hist = (perf_stats_msg->vol_hist_list)[i];
	FDS_PLOG(GetLog()) << "OM: received perfstat for vol " << vol_hist->vol_uuid;
	for (int j = 0; j < (vol_hist->stat_list).size(); ++j) {
	  FDSP_PerfStatTypePtr stat = (vol_hist->stat_list)[j];
	  FDS_PLOG_SEV(GetLog(), fds::fds_log::normal) << "OM: --- stat_type " << stat->stat_type 
						       << " rel_secs " << stat->rel_seconds   	
						       << " iops " << stat->nios << " lat " << stat->ave_lat;
	}
      }
  }
  else {
    FDS_PLOG_SEV(GetLog(), fds::fds_log::warning) << "OM received perfstats from node of type " 
						  << perf_stats_msg->node_type
						  << " which we don't need stats from (or we do now?)"; 
  }
}


int OrchMgr::ReqCfgHandler::DeleteDomain(const FdspMsgHdrPtr& fdsp_msg,
                                       const FdspCrtDomPtr& del_dom_req,
                                       const Ice::Current&) {
  try {
    orchMgr->DeleteDomain(fdsp_msg, del_dom_req);
  }
  catch(...) {
    FDS_PLOG_SEV(orchMgr->GetLog(), fds::fds_log::error) << "Orch Mgr encountered exception while "
							 << "processing delete domain";
  }
}

int OrchMgr::ReqCfgHandler::CreateDomain(const FdspMsgHdrPtr& fdsp_msg,
                                       const FdspCrtDomPtr& crt_dom_req,
                                       const Ice::Current&) {
  try {
    orchMgr->CreateDomain(fdsp_msg, crt_dom_req);
  }
  catch(...) {
    FDS_PLOG_SEV(orchMgr->GetLog(), fds::fds_log::error) << "Orch Mgr encountered exception while "
							 << "processing create domain";
  }
}

void OrchMgr::ReqCfgHandler::GetDomainStats(const FdspMsgHdrPtr& fdsp_msg,
					    const FdspGetDomStatsPtr& get_stats_req,
					    const Ice::Current&)
{
  try {
    orchMgr->GetDomainStats(fdsp_msg, get_stats_req);
  }
  catch(...) {
    FDS_PLOG_SEV(orchMgr->GetLog(), fds::fds_log::error) << "Orch Mgr encountered exception while "
							 << "processing get domain stats";
  }
}

int OrchMgr::ReqCfgHandler::CreateVol(const FdspMsgHdrPtr& fdsp_msg,
                                       const FdspCrtVolPtr& crt_vol_req,
                                       const Ice::Current&) {
  try {
    orchMgr->CreateVol(fdsp_msg, crt_vol_req);
  }
  catch(...) {
    FDS_PLOG_SEV(orchMgr->GetLog(), fds::fds_log::error) << "Orch Mgr encountered exception while "
							 << "processing create volume";
  }
}

void OrchMgr::ReqCfgHandler::DeleteVol(const FdspMsgHdrPtr& fdsp_msg,
                                       const FdspDelVolPtr& del_vol_req,
                                       const Ice::Current&) {
  orchMgr->DeleteVol(fdsp_msg, del_vol_req);
}
void OrchMgr::ReqCfgHandler::ModifyVol(const FdspMsgHdrPtr& fdsp_msg,
                                       const FdspModVolPtr& mod_vol_req,
                                       const Ice::Current&) {
  orchMgr->ModifyVol(fdsp_msg, mod_vol_req);
}
void OrchMgr::ReqCfgHandler::CreatePolicy(const FdspMsgHdrPtr& fdsp_msg,
                                          const FdspCrtPolPtr& crt_pol_req,
                                          const Ice::Current&) {
  orchMgr->CreatePolicy(fdsp_msg, crt_pol_req);
}
void OrchMgr::ReqCfgHandler::DeletePolicy(const FdspMsgHdrPtr& fdsp_msg,
                                          const FdspDelPolPtr& del_pol_req,
                                          const Ice::Current&) {
  orchMgr->DeletePolicy(fdsp_msg, del_pol_req);
}
void OrchMgr::ReqCfgHandler::ModifyPolicy(const FdspMsgHdrPtr& fdsp_msg,
                                          const FdspModPolPtr& mod_pol_req,
                                          const Ice::Current&) {
  orchMgr->ModifyPolicy(fdsp_msg, mod_pol_req);
}

void OrchMgr::ReqCfgHandler::AttachVol(const FdspMsgHdrPtr& fdsp_msg,
                                       const FdspAttVolCmdPtr& atc_vol_req,
                                       const Ice::Current&) {
  orchMgr->AttachVol(fdsp_msg, atc_vol_req);
}

void OrchMgr::ReqCfgHandler::DetachVol(const FdspMsgHdrPtr& fdsp_msg,
                                       const FdspAttVolCmdPtr& dtc_vol_req,
                                       const Ice::Current&) {
  orchMgr->DetachVol(fdsp_msg, dtc_vol_req);
}

void OrchMgr::ReqCfgHandler::GetVolInfo(const ::FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg, 
		      const ::FDS_ProtocolInterface::FDSP_GetVolInfoReqTypePtr& get_vol_req, 
		const ::Ice::Current&) {
}

void OrchMgr::ReqCfgHandler::RegisterNode(const FdspMsgHdrPtr &fdsp_msg,
                                          const FdspRegNodePtr &reg_node_req,
                                          const Ice::Current&) {
  orchMgr->RegisterNode(fdsp_msg, reg_node_req);
}

void OrchMgr::ReqCfgHandler::TestBucket(const FdspMsgHdrPtr& fdsp_msg,
					const FdspTestBucketPtr& test_buck_req,
					const Ice::Current&) {
  orchMgr->TestBucket(fdsp_msg, test_buck_req);
}

void OrchMgr::ReqCfgHandler::SetThrottleLevel(const FDSP_MsgHdrTypePtr& fdsp_msg, 
		      const FDSP_ThrottleMsgTypePtr& throttle_req, 
		      const Ice::Current&) {
  orchMgr->SetThrottleLevel(fdsp_msg, throttle_req);
}

void OrchMgr::ReqCfgHandler::NotifyQueueFull(const FDSP_MsgHdrTypePtr& fdsp_msg,
		     const FDSP_NotifyQueueStateTypePtr& queue_state_req, 
		     const Ice::Current&) {
  orchMgr->NotifyQueueFull(fdsp_msg, queue_state_req);
}

void OrchMgr::ReqCfgHandler::NotifyPerfstats(const FDSP_MsgHdrTypePtr& fdsp_msg,
					     const FDSP_PerfstatsTypePtr& perf_stats_msg,
					     const Ice::Current&) {
  orchMgr->NotifyPerfstats(fdsp_msg, perf_stats_msg);
}

void OrchMgr::ReqCfgHandler::AssociateRespCallback(
    const Ice::Identity& ident,
    const Ice::Current& current) {
}

void OrchMgr::defaultS3BucketPolicy()
{
  Error err(ERR_OK);

 FDSP_PolicyInfoTypePtr policy_info = new FDSP_PolicyInfoType();

 policy_info->policy_name = std::string("S3-Bucket_policy");
 policy_info->policy_id = 50;
 policy_info->iops_min = 400;
 policy_info->iops_max = 600;
 policy_info->rel_prio = 1;

 orchMgr->om_mutex->lock();
 err = orchMgr->policy_mgr->createPolicy(policy_info);
 orchMgr->om_mutex->unlock();

 FDS_PLOG_SEV(GetLog(), fds::fds_log::normal) << "Created  default S3 policy "; 
}


OrchMgr *gl_orch_mgr;

}  // namespace fds

int main(int argc, char *argv[]) {
  fds::orchMgr = new fds::OrchMgr();

  fds::gl_orch_mgr = fds::orchMgr;

  fds::Module *io_dm_vec[] = {
    nullptr
  };
  fds::ModuleVector  io_dm(argc, argv, io_dm_vec);
  fds::orchMgr->setSysParams(io_dm.get_sys_params());

  fds::orchMgr->main(argc, argv, "orch_mgr.conf");

  delete fds::orchMgr;
  return 0;
}

