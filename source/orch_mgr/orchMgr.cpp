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
    } else {
      std::cout << "Invalid argument " << argv[i] << std::endl;
      return -1;
    }
  }


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

  FDS_PLOG(om_log) << "Orchestration Manager using port " << orchMgrPortNum;

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

  adapter->activate();

  communicator()->waitForShutdown();

  return EXIT_SUCCESS;
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

  FDS_PLOG(GetLog()) << "Received CreateDomain Req : "
                     << crt_dom_req->domain_name  << " :domain_id - " << crt_dom_req->domain_id;

   /* check  for duplicate domain id */
  om_mutex->lock();
  if ( locDomMap.count(crt_dom_req->domain_id) == 0) {
    FDS_PLOG(om_log) << "Duplicate domain: " << crt_dom_req->domain_id;
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
int OrchMgr::CreateVol(const FdspMsgHdrPtr& fdsp_msg,
                        const FdspCrtVolPtr& crt_vol_req) {
  Error err(ERR_OK);
  int  vol_id = crt_vol_req->vol_info->volUUID;
  int  domain_id = crt_vol_req->vol_info->localDomainId;
  std::string vol_name = crt_vol_req->vol_info->vol_name;
  localDomainInfo  *currentDom;
  int returnCode;

  FDS_PLOG(GetLog()) << "Received CreateVol Req for volume "
                     << vol_name << " ; id - " << vol_id;

  /*
   * get the domain Id. If  Domain is not created  use  default domain 
   * for now use the default domain
   */
   currentDom  = locDomMap[DEFAULT_LOC_DOMAIN_ID];

   
  om_mutex->lock();

  if ( currentDom->domain_ptr->volumeMap.count(vol_id) != 0) {
    FDS_PLOG(om_log) << "Received CreateVol for existing volume " << vol_id;
    om_mutex->unlock();
    return -1;
  }
  VolumeInfo *new_vol = new VolumeInfo();
  new_vol->vol_name = vol_name;
  new_vol->volUUID = vol_id;
  new_vol->properties = new VolumeDesc(crt_vol_req->vol_info);
  err = policy_mgr->fillVolumeDescPolicy(new_vol->properties);
  if ( err == ERR_CAT_ENTRY_NOT_FOUND ) {
      /* TODO: policy not in the catalog , should we return error or use default policu */
      FDS_PLOG(GetLog()) << "Create volume " << vol_name 
                         << " Req requested unknown policy " << (crt_vol_req->vol_info)->volPolicyId;    
  }
  else if (!err.ok()) {
      FDS_PLOG(GetLog()) << "CreateVol: Failed to get policy info for volume " << vol_name;
      /* TODO: for now ignoring error */
  }
   /*
    * check the resource  availability 
    */

   if ((returnCode = currentDom->domain_ptr->admin_ctrl->volAdminControl(new_vol)) != 0) {
  	FDS_PLOG(GetLog()) << "Unable to create Volume \n";
	 delete new_vol;
         om_mutex->unlock();
	 return -1; 
    }

  currentDom->domain_ptr->volumeMap[vol_id] = new_vol;
  currentDom->domain_ptr->sendCreateVolToFdsNodes(new_vol);
 /* update the per domain disk resource  */
  om_mutex->unlock();
 return 0;
}

void OrchMgr::DeleteVol(const FdspMsgHdrPtr& fdsp_msg,
                        const FdspDelVolPtr& del_vol_req) {
  int  vol_id = del_vol_req->vol_uuid;
  std::string vol_name = del_vol_req->vol_name;
  VolumeInfo *cur_vol;
  localDomainInfo  *currentDom;

  FDS_PLOG(GetLog()) << "Received DeleteVol Req for volume "
                     << vol_name << " ; id - " << vol_id;

  /*
   * get the domain Id. If  Domain is not created  use  default domain 
   * for now  use the default domain
   */
   currentDom  = locDomMap[DEFAULT_LOC_DOMAIN_ID];

  om_mutex->lock();
  if ( currentDom->domain_ptr->volumeMap.count(vol_id) == 0) {
    FDS_PLOG(om_log) << "Received DeleteVol for non-existent volume " << vol_id;
    om_mutex->unlock();
    return;
  }
  VolumeInfo *del_vol =  currentDom->domain_ptr->volumeMap[vol_id];

  for (int i = 0; i < del_vol->hv_nodes.size(); i++) {
    if (currentDom->domain_ptr->currentShMap.count(del_vol->hv_nodes[i]) == 0) {
      FDS_PLOG(om_log) << "Inconsistent State Detected. "
                       << "HV node in volume's hvnode list but not in SH map";
      assert(0);
    }
    currentDom->domain_ptr->sendDetachVolToHvNode(del_vol->hv_nodes[i], del_vol);
  }

  currentDom->domain_ptr->sendDeleteVolToFdsNodes(del_vol);
 
   /*
    * update  admission control  class  to reflect the  delete Volume 
    */
   cur_vol= currentDom->domain_ptr->volumeMap[vol_id];
   currentDom->domain_ptr->admin_ctrl->updateAdminControlParams(cur_vol);

   currentDom->domain_ptr->volumeMap.erase(vol_id);
  om_mutex->unlock();

  delete del_vol;
}

void OrchMgr::ModifyVol(const FdspMsgHdrPtr& fdsp_msg,
                        const FdspModVolPtr& mod_vol_req) {
  FDS_PLOG(GetLog()) << "Received ModifyVol  Msg";
}

void OrchMgr::CreatePolicy(const FdspMsgHdrPtr& fdsp_msg,
                           const FdspCrtPolPtr& crt_pol_req) {
  Error err(ERR_OK);
  int policy_id = crt_pol_req->policy_info->policy_id;

  FDS_PLOG(GetLog()) << "Received CreatePolicy  Msg for policy "
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
  FDS_PLOG(GetLog()) << "Received DeletePolicy  Msg for policy "
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
  FDS_PLOG(GetLog()) << "Received ModifyPolicy  Msg for policy "
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
  int  vol_id = atc_vol_req->vol_uuid;
  std::string vol_name = atc_vol_req->vol_name;
  fds_node_name_t node_name = atc_vol_req->node_id;
  localDomainInfo  *currentDom;

  FDS_PLOG(GetLog()) << "Received Attach Vol Req for volume "
                     << vol_name << " ; id - " << vol_id
                     << " at node " << node_name;

  /*
   * get the domain Id. If  Domain is not created  use  default domain 
   * for now use default  domain 
   */
    currentDom  = locDomMap[DEFAULT_LOC_DOMAIN_ID];

  om_mutex->lock();
  if ( currentDom->domain_ptr->volumeMap.count(vol_id) == 0) {
    FDS_PLOG(om_log) << "Received Attach Vol for non-existent volume "
                     << vol_id;
    om_mutex->unlock();
    return;
  }
  VolumeInfo *this_vol =  currentDom->domain_ptr->volumeMap[vol_id];
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
      FDS_PLOG(om_log) << "Attach Vol req for volume " << vol_id
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
  int  vol_id = dtc_vol_req->vol_uuid;
  std::string vol_name = dtc_vol_req->vol_name;
  fds_node_name_t node_name = dtc_vol_req->node_id;
  fds_bool_t node_not_attached = true;
  localDomainInfo  *currentDom;

  FDS_PLOG(GetLog()) << "Received Detach Vol Req for volume "
                     << vol_name << " ; id - " << vol_id
                     << " at node " << node_name;

  currentDom  = locDomMap[DEFAULT_LOC_DOMAIN_ID];

  om_mutex->lock();
  if ( currentDom->domain_ptr->volumeMap.count(vol_id) == 0) {
    FDS_PLOG(om_log) << "Received Detach Vol for non-existent volume "
                     << vol_id;
    om_mutex->unlock();
    return;
  }
  VolumeInfo *this_vol =  currentDom->domain_ptr->volumeMap[vol_id];
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
    FDS_PLOG(om_log) << "Detach Vol req for volume " << vol_id
                     << " rejected because this volume is "
                     << "not attached at node " << node_name;
  } else {
    if (currentDom->domain_ptr->currentShMap.count(node_name) > 0) {
      currentDom->domain_ptr->sendDetachVolToHvNode(node_name, this_vol);
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

  FDS_PLOG(GetLog()) << "Received RegisterNode Msg"
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
    FDS_PLOG(GetLog()) << "Duplicate Node Registration for "
                       << reg_node_req->node_name;
  }

  fds_int32_t new_node_id = currentDom->domain_ptr->getFreeNodeId(reg_node_req->node_name);

  FDS_PLOG(GetLog()) << "Assigning node id " << new_node_id << " to node " << reg_node_req->node_name 
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


int OrchMgr::ReqCfgHandler::DeleteDomain(const FdspMsgHdrPtr& fdsp_msg,
                                       const FdspCrtDomPtr& del_dom_req,
                                       const Ice::Current&) {
  try {
    orchMgr->DeleteDomain(fdsp_msg, del_dom_req);
  }
  catch(...) {
    FDS_PLOG(orchMgr->GetLog()) << "Orch Mgr encountered exception while "
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
    FDS_PLOG(orchMgr->GetLog()) << "Orch Mgr encountered exception while "
                                << "processing create domain";
  }
}

int OrchMgr::ReqCfgHandler::CreateVol(const FdspMsgHdrPtr& fdsp_msg,
                                       const FdspCrtVolPtr& crt_vol_req,
                                       const Ice::Current&) {
  try {
    orchMgr->CreateVol(fdsp_msg, crt_vol_req);
  }
  catch(...) {
    FDS_PLOG(orchMgr->GetLog()) << "Orch Mgr encountered exception while "
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

void OrchMgr::ReqCfgHandler::RegisterNode(const FdspMsgHdrPtr &fdsp_msg,
                                          const FdspRegNodePtr &reg_node_req,
                                          const Ice::Current&) {
  orchMgr->RegisterNode(fdsp_msg, reg_node_req);
}

void OrchMgr::ReqCfgHandler::AssociateRespCallback(
    const Ice::Identity& ident,
    const Ice::Current& current) {
}

}  // namespace fds

int main(int argc, char *argv[]) {
  fds::orchMgr = new fds::OrchMgr();

  fds::orchMgr->main(argc, argv, "orch_mgr.conf");

  delete fds::orchMgr;
}

