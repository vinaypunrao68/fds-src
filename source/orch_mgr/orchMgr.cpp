/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#include "orch_mgr/orchMgr.h"

#include <iostream>  // NOLINT(*)
#include <string>

using namespace FDS_ProtocolInterface;

namespace fds {

OrchMgr *orchMgr;

OrchMgr::OrchMgr()
    : port_num(0) {
  om_log = new fds_log("om", "logs");
  om_mutex = new fds_mutex("OrchMgrMutex");
  for (int i = 0; i < MAX_OM_NODES; i++) {
	node_id_to_name[i] = "";
  }
  FDS_PLOG(om_log) << "Constructing the Orchestration  Manager";
}

OrchMgr::~OrchMgr() {
  FDS_PLOG(om_log) << "Destructing the Orchestration  Manager";
  delete om_log;
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
    } else {
      std::cout << "Invalid argument " << argv[i] << std::endl;
      return -1;
    }
  }

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

void OrchMgr::copyVolumeInfoToProperties(FDS_Volume *pVol,
                                         FdspVolInfoPtr v_info) {
  pVol->vol_name = v_info->vol_name;
  pVol->volUUID = v_info->volUUID;
  pVol->capacity = v_info->capacity;
  pVol->volType = fds::FDS_VolType(v_info->volType);
  pVol->consisProtocol = fds::FDS_ConsisProtoType(v_info->consisProtocol);
  pVol->appWorkload = fds::FDS_AppWorkload(v_info->appWorkload);
}

void OrchMgr::copyPropertiesToVolumeInfo(FdspVolInfoPtr v_info,
                                         FDS_Volume *pVol) {
  v_info->vol_name = pVol->vol_name;
  v_info->volUUID = pVol->volUUID;
  v_info->capacity = pVol->capacity;
  v_info->volType = FDS_ProtocolInterface::FDSP_VolType(pVol->volType);
  v_info->consisProtocol = FDS_ProtocolInterface::
      FDSP_ConsisProtoType(pVol->consisProtocol);
  v_info->appWorkload = FDS_ProtocolInterface::
      FDSP_AppWorkload(pVol->appWorkload);
}

void OrchMgr::initOMMsgHdr(const FdspMsgHdrPtr& msg_hdr) {
  msg_hdr->minor_ver = 0;
  msg_hdr->msg_code = FDS_ProtocolInterface::FDSP_MSG_NOTIFY_VOL;
  msg_hdr->msg_id =  1;

  msg_hdr->major_ver = 0xa5;
  msg_hdr->minor_ver = 0x5a;

  msg_hdr->num_objects = 1;
  msg_hdr->frag_len = 0;
  msg_hdr->frag_num = 0;

  msg_hdr->tennant_id = 0;
  msg_hdr->local_domain_id = 0;

  msg_hdr->src_id = FDS_ProtocolInterface::FDSP_ORCH_MGR;
  msg_hdr->dst_id = FDS_ProtocolInterface::FDSP_DATA_MGR;

  msg_hdr->err_code = FDS_ProtocolInterface::FDSP_ERR_SM_NO_SPACE;
  msg_hdr->result = FDS_ProtocolInterface::FDSP_ERR_OK;
}

int OrchMgr::getFreeNodeId(std::string& node_name) {
	int i; 
  	for (i = 0; i < MAX_OM_NODES; i++) {
		if (node_id_to_name[i] == "") {
			node_id_to_name[i] = node_name;
			return i;
		}
  	}
	FDS_PLOG(om_log) << "No id available to allocate to node " << node_name;
 	return -1;
}


// Dump all existing SM/DM nodes info as a sequence of NotifyNodeAdd ctrl messages to a newly registering node
void OrchMgr::sendMgrNodeListToFdsNode(NodeInfo& n_info) {
	return;
} 


// Broadcast a node event to all existing DM/SM/HV nodes
void OrchMgr::sendNodeEventToFdsNodes(NodeInfo& nodeInfo, FDS_ProtocolInterface::FDSP_NodeState node_state) {


  FdspMsgHdrPtr msg_hdr_ptr = new FDS_ProtocolInterface::FDSP_MsgHdrType;
  FDSP_Node_Info_TypePtr node_info_ptr = new FDSP_Node_Info_Type;

  initOMMsgHdr(msg_hdr_ptr);

  msg_hdr_ptr->msg_code = FDSP_MSG_NOTIFY_NODE_ADD;
  msg_hdr_ptr->msg_id = 0;
  msg_hdr_ptr->tennant_id = 1;
  msg_hdr_ptr->local_domain_id = 1;

  node_info_ptr->node_id = nodeInfo.node_id;
  node_info_ptr->node_type = nodeInfo.node_type;
  node_info_ptr->node_name = nodeInfo.node_name;
  node_info_ptr->node_state = node_state;
  node_info_ptr->ip_lo_addr = nodeInfo.node_ip_address;
  node_info_ptr->control_port = nodeInfo.control_port;
  node_info_ptr->data_port = nodeInfo.data_port;

  for (int i = 0; i < 3; i++) {
    node_map_t& node_map = (i == 0) ? currentDmMap:((i == 1)?currentSmMap:currentShMap);

    msg_hdr_ptr->dst_id = (i == 0) ?
      FDS_ProtocolInterface::FDSP_DATA_MGR :
      ((i == 1)?FDS_ProtocolInterface::FDSP_STOR_MGR:FDSP_STOR_HVISOR);

    for (auto it = node_map.begin(); it != node_map.end(); ++it) {

      fds_node_name_t node_name = it->first;
      NodeInfo& next_node_info = it->second;
      if (node_name == nodeInfo.node_name) {
	continue;
      }
      
      FDS_PLOG(om_log) << "Sending node notification to node " << node_name << " for node " << nodeInfo.node_name << " state - " << node_state;

      ReqCtrlPrx OMClientAPI = next_node_info.cpPrx;
      if (node_state == FDS_Node_Up) {
	  OMClientAPI->NotifyNodeAdd(msg_hdr_ptr, node_info_ptr);
      } else {
	  OMClientAPI->NotifyNodeRmv(msg_hdr_ptr, node_info_ptr);
      }
    }
  }
}

// Dump all existing volumes (as a sequence of create vol ctrl messages) to a newly registering SM/DM Node
void OrchMgr::sendAllVolumesToFdsMgrNode(NodeInfo node_info) {

  ReqCtrlPrx OMClientAPI = node_info.cpPrx;
   
  FdspMsgHdrPtr msg_hdr = new FDS_ProtocolInterface::FDSP_MsgHdrType;
  FdspNotVolPtr vol_msg = new FDS_ProtocolInterface::FDSP_NotifyVolType;
  vol_msg->vol_info = new FDS_ProtocolInterface::FDSP_VolumeInfoType();

  initOMMsgHdr(msg_hdr);
  vol_msg->type = FDS_ProtocolInterface::FDSP_NOTIFY_ADD_VOL;
  msg_hdr->dst_id = FDS_ProtocolInterface::FDSP_DATA_MGR;
  
  for (auto it = volumeMap.begin(); it != volumeMap.end(); ++it) {

    VolumeInfo *pVolInfo = it->second;
    FDS_Volume* pVol = &(pVolInfo->properties);
    msg_hdr->glob_volume_id = pVol->volUUID;
    vol_msg->vol_name = std::string(pVol->vol_name);
    copyPropertiesToVolumeInfo(vol_msg->vol_info, pVol);
    
    FDS_PLOG(om_log) << "Sending create vol to node " << node_info.node_name << " for volume " << pVolInfo->volUUID;
    OMClientAPI->NotifyAddVol(msg_hdr, vol_msg);
    
  }

}

// Broadcast create vol ctrl message to all DM/SM Nodes
void OrchMgr::sendCreateVolToFdsNodes(VolumeInfo  *pVolInfo) {
  FDS_Volume* pVol = &(pVolInfo->properties);

  FdspMsgHdrPtr msg_hdr = new FDS_ProtocolInterface::FDSP_MsgHdrType;
  FdspNotVolPtr vol_msg = new FDS_ProtocolInterface::FDSP_NotifyVolType;
  vol_msg->vol_info = new FDS_ProtocolInterface::FDSP_VolumeInfoType();

  initOMMsgHdr(msg_hdr);
  msg_hdr->glob_volume_id = pVol->volUUID;
  vol_msg->type = FDS_ProtocolInterface::FDSP_NOTIFY_ADD_VOL;
  vol_msg->vol_name = std::string(pVol->vol_name);
  copyPropertiesToVolumeInfo(vol_msg->vol_info, pVol);

  for (int i = 0; i < 2; i++) {
    node_map_t& node_map = (i == 0) ? currentDmMap:currentSmMap;
    msg_hdr->dst_id = (i == 0) ?
        FDS_ProtocolInterface::FDSP_DATA_MGR :
        FDS_ProtocolInterface::FDSP_STOR_MGR;

    for (auto it = node_map.begin(); it != node_map.end(); ++it) {
      fds_node_name_t node_name = it->first;
      NodeInfo& node_info = it->second;

      FDS_PLOG(om_log) << "Sending create vol to node " << node_name << " for volume " << pVolInfo->volUUID;

      ReqCtrlPrx OMClientAPI = node_info.cpPrx;
      OMClientAPI->NotifyAddVol(msg_hdr, vol_msg);
    }
  }
}

// Broadcast delete vol ctrl message to all DM/SM Nodes
void OrchMgr::sendDeleteVolToFdsNodes(VolumeInfo *pVolInfo) {
  FDS_Volume *pVol = &(pVolInfo->properties);

  FdspMsgHdrPtr msg_hdr = new FDS_ProtocolInterface::FDSP_MsgHdrType;
  FdspNotVolPtr vol_msg = new FDS_ProtocolInterface::FDSP_NotifyVolType;
  vol_msg->vol_info = new FDS_ProtocolInterface::FDSP_VolumeInfoType();

  initOMMsgHdr(msg_hdr);
  msg_hdr->glob_volume_id = pVol->volUUID;
  vol_msg->type = FDS_ProtocolInterface::FDSP_NOTIFY_RM_VOL;
  vol_msg->vol_name = std::string(pVol->vol_name);
  copyPropertiesToVolumeInfo(vol_msg->vol_info, pVol);

  for (int i = 0; i < 2; i++) {
    node_map_t& node_map = (i == 0) ? currentDmMap:currentSmMap;
    msg_hdr->dst_id = (i == 0) ?
        FDS_ProtocolInterface::FDSP_DATA_MGR :
        FDS_ProtocolInterface::FDSP_STOR_MGR;

    for (auto it = node_map.begin(); it != node_map.end(); ++it) {
      fds_node_name_t node_name = it->first;
      NodeInfo& node_info = it->second;

      FDS_PLOG(om_log) << "Sending delete vol to node " << node_name << " for volume " << pVolInfo->volUUID;

      ReqCtrlPrx OMClientAPI = node_info.cpPrx;
      OMClientAPI->NotifyRmVol(msg_hdr, vol_msg);
    }
  }
}

// Send attach vol ctrl message to a HV node
void OrchMgr::sendAttachVolToHvNode(fds_node_name_t node_name,
				    VolumeInfo *pVolInfo) {

  FDS_Volume *pVol = &(pVolInfo->properties);

  FdspMsgHdrPtr msg_hdr = new FDS_ProtocolInterface::FDSP_MsgHdrType;
  FdspAttVolPtr vol_msg = new FDS_ProtocolInterface::FDSP_AttachVolType;
  vol_msg->vol_info = new FDS_ProtocolInterface::FDSP_VolumeInfoType();

  initOMMsgHdr(msg_hdr);
  msg_hdr->msg_code = FDS_ProtocolInterface::FDSP_MSG_ATTACH_VOL_CTRL;
  msg_hdr->dst_id = FDS_ProtocolInterface::FDSP_STOR_HVISOR;
  msg_hdr->glob_volume_id = pVol->volUUID;

  vol_msg->vol_name = std::string(pVol->vol_name);
  copyPropertiesToVolumeInfo(vol_msg->vol_info, pVol);

  NodeInfo& node_info = currentShMap[node_name];

  FDS_PLOG(om_log) << "Sending attach vol to node " << node_name << " for volume " << pVolInfo->volUUID;

  ReqCtrlPrx OMClientAPI = node_info.cpPrx;
  OMClientAPI->AttachVol(msg_hdr, vol_msg);
}

// Send attach vol ctrl message to a HV node
void OrchMgr::sendDetachVolToHvNode(fds_node_name_t node_name,
                                    VolumeInfo *pVolInfo) {

  FDS_Volume *pVol = &(pVolInfo->properties);

  FdspMsgHdrPtr msg_hdr = new FDS_ProtocolInterface::FDSP_MsgHdrType;
  FdspAttVolPtr vol_msg = new FDS_ProtocolInterface::FDSP_AttachVolType;
  vol_msg->vol_info = new FDS_ProtocolInterface::FDSP_VolumeInfoType();

  initOMMsgHdr(msg_hdr);
  msg_hdr->msg_code = FDS_ProtocolInterface::FDSP_MSG_DETACH_VOL_CTRL;
  msg_hdr->dst_id = FDS_ProtocolInterface::FDSP_STOR_HVISOR;
  msg_hdr->glob_volume_id = pVol->volUUID;

  vol_msg->vol_name = std::string(pVol->vol_name);
  copyPropertiesToVolumeInfo(vol_msg->vol_info, pVol);

  NodeInfo& node_info = currentShMap[node_name];

  FDS_PLOG(om_log) << "Sending detach vol to node " << node_name << " for volume " << pVolInfo->volUUID;

  ReqCtrlPrx OMClientAPI = node_info.cpPrx;
  OMClientAPI->DetachVol(msg_hdr, vol_msg);
}

// Dump all concerned volumes as a sequence of attach vol ctrl messages to a HV node
void OrchMgr::sendAllVolumesToHvNode(fds_node_name_t node_name) {

  for (auto it = volumeMap.begin(); it != volumeMap.end(); ++it) {

    VolumeInfo *pVolInfo = it->second;
    for (int i = 0; i < pVolInfo->hv_nodes.size(); i++) {
      if (pVolInfo->hv_nodes[i] == node_name) {
	sendAttachVolToHvNode(node_name, pVolInfo);
	break;
      }
    }
  }
    
}

void OrchMgr::CreateVol(const FdspMsgHdrPtr& fdsp_msg,
                        const FdspCrtVolPtr& crt_vol_req) {

  int  vol_id = crt_vol_req->vol_info->volUUID;
  std::string vol_name = crt_vol_req->vol_info->vol_name;

  FDS_PLOG(GetLog()) << "Received CreateVol Req for volume "
                     << vol_name << " ; id - " << vol_id;
  om_mutex->lock();
  if (volumeMap.count(vol_id) != 0) {
    FDS_PLOG(om_log) << "Received CreateVol for existing volume " << vol_id;
    om_mutex->unlock();
    return;
  }
  VolumeInfo *new_vol = new VolumeInfo();
  new_vol->vol_name = vol_name;
  new_vol->volUUID = vol_id;
  copyVolumeInfoToProperties(&(new_vol->properties), crt_vol_req->vol_info);
  volumeMap[vol_id] = new_vol;
  sendCreateVolToFdsNodes(new_vol);
  om_mutex->unlock();
 
}

void OrchMgr::DeleteVol(const FdspMsgHdrPtr& fdsp_msg,
                        const FdspDelVolPtr& del_vol_req) {
  int  vol_id = del_vol_req->vol_uuid;
  std::string vol_name = del_vol_req->vol_name;

  FDS_PLOG(GetLog()) << "Received DeleteVol Req for volume "
                     << vol_name << " ; id - " << vol_id;
  om_mutex->lock();
  if (volumeMap.count(vol_id) == 0) {
    FDS_PLOG(om_log) << "Received DeleteVol for non-existent volume " << vol_id;
    om_mutex->unlock();
    return;
  }
  VolumeInfo *del_vol = volumeMap[vol_id];
 
  for (int i = 0; i < del_vol->hv_nodes.size(); i++) {
    if (currentShMap.count(del_vol->hv_nodes[i]) == 0) {
      FDS_PLOG(om_log) << "Inconsistent State Detected. "
                       << "HV node in volume's hvnode list but not in SH map";
      assert(0);
    }
    sendDetachVolToHvNode(del_vol->hv_nodes[i], del_vol);
  }

  sendDeleteVolToFdsNodes(del_vol);

  volumeMap.erase(vol_id);
  om_mutex->unlock();

  delete del_vol;
}

void OrchMgr::ModifyVol(const FdspMsgHdrPtr& fdsp_msg,
                        const FdspModVolPtr& mod_vol_req) {
  FDS_PLOG(GetLog()) << "Received ModifyVol  Msg";
}

void OrchMgr::CreatePolicy(const FdspMsgHdrPtr& fdsp_msg,
                           const FdspCrtPolPtr& crt_pol_req) {
  FDS_PLOG(GetLog()) << "Received CreatePolicy  Msg";
}

void OrchMgr::DeletePolicy(const FdspMsgHdrPtr& fdsp_msg,
                           const FdspDelPolPtr& del_pol_req) {
  FDS_PLOG(GetLog()) << "Received CreatePolicy  Msg";
}

void OrchMgr::ModifyPolicy(const FdspMsgHdrPtr& fdsp_msg,
                           const FdspModPolPtr& mod_pol_req) {
  FDS_PLOG(GetLog()) << "Received ModifyPolicy  Msg";
}

void OrchMgr::AttachVol(const FdspMsgHdrPtr &fdsp_msg,
                        const FdspAttVolCmdPtr &atc_vol_req) {
  int  vol_id = atc_vol_req->vol_uuid;
  std::string vol_name = atc_vol_req->vol_name;
  fds_node_name_t node_name = atc_vol_req->node_id;

  FDS_PLOG(GetLog()) << "Received Attach Vol Req for volume "
                     << vol_name << " ; id - " << vol_id << " at node " << node_name;
  om_mutex->lock();
  if (volumeMap.count(vol_id) == 0) {
    FDS_PLOG(om_log) << "Received Attach Vol for non-existent volume "
                     << vol_id;
    om_mutex->unlock();
    return;
  }
  VolumeInfo *this_vol = volumeMap[vol_id];
#if 0
  // Let's actually allow this, as a means of provisioning before HV is online
  if (currentShMap.count(node_id) == 0) {
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
  if (currentShMap.count(node_name) > 0) {
    sendAttachVolToHvNode(node_name, this_vol);
  }
  om_mutex->unlock();
}



void OrchMgr::DetachVol(const FdspMsgHdrPtr    &fdsp_msg,
                        const FdspAttVolCmdPtr &dtc_vol_req) {
  int  vol_id = dtc_vol_req->vol_uuid;
  std::string vol_name = dtc_vol_req->vol_name;
  fds_node_name_t node_name = dtc_vol_req->node_id;
  fds_bool_t node_not_attached = true;

  FDS_PLOG(GetLog()) << "Received Detach Vol Req for volume "
                     << vol_name << " ; id - " << vol_id << " at node " << node_name;
  om_mutex->lock();
  if (volumeMap.count(vol_id) == 0) {
    FDS_PLOG(om_log) << "Received Detach Vol for non-existent volume "
                     << vol_id;
    om_mutex->unlock();
    return;
  }
  VolumeInfo *this_vol = volumeMap[vol_id];
#if 0
  if (currentShMap.count(node_id) == 0) {
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
    if (currentShMap.count(node_name) > 0) {
      sendDetachVolToHvNode(node_name, this_vol);
    }
  }
  om_mutex->unlock();

}

void OrchMgr::RegisterNode(const FdspMsgHdrPtr  &fdsp_msg,
                           const FdspRegNodePtr &reg_node_req) {
  std::string ip_addr_str;
  Ice::Identity ident;
  std::ostringstream tcpProxyStr;

  FDS_PLOG(GetLog()) << "Received RegisterNode Msg"
                     << "  Node name:" << reg_node_req->node_name
                     << "  Node IP:" << std::hex << reg_node_req->ip_lo_addr
                     << "  Node Type:" << std::dec << reg_node_req->node_type
                     << "  Control Port: " << reg_node_req->control_port
                     << "  Data Port: " << reg_node_req->data_port;

  node_map_t& node_map = (reg_node_req->node_type == FDSP_STOR_MGR)? currentSmMap:(
										   (reg_node_req->node_type == FDSP_DATA_MGR)? currentDmMap: currentShMap);

  ip_addr_str = ipv4_addr_to_str(reg_node_req->ip_lo_addr);
  
  // create a new  control  communication adaptor
  tcpProxyStr << "OrchMgrClient: tcp -h " << ip_addr_str
              << " -p  " << reg_node_req->control_port;



  om_mutex->lock();

  if (node_map.count(reg_node_req->node_name) > 0) {
    FDS_PLOG(GetLog()) << "Duplicate Node Registration for " << reg_node_req->node_name;
    return;
  }

  int new_node_id = getFreeNodeId(reg_node_req->node_name);

  // build the SM node map
  NodeInfo n_info(new_node_id, 
		  reg_node_req->node_name,
		  reg_node_req->node_type,
                  reg_node_req->ip_lo_addr,
                  reg_node_req->control_port,
                  n_info.data_port = reg_node_req->data_port,
                  n_info.node_state = FDS_ProtocolInterface::FDS_Node_Up,
                  FDS_ProtocolInterface::
                  FDSP_ControlPathReqPrx::
                  checkedCast(communicator()->stringToProxy(tcpProxyStr.str())));
  
  node_map[reg_node_req->node_name] = n_info;

  // If this is a SM or a DM, let existing nodes know about this node
  if (reg_node_req->node_type != FDSP_STOR_HVISOR)
    sendNodeEventToFdsNodes(n_info, n_info.node_state);

  // Let this new node know about the existing node list
  sendMgrNodeListToFdsNode(n_info);

  // Let this new node know about the existing volumes.
  // If it's a HV node, send only the volumes it need to attach
  if (reg_node_req->node_type == FDSP_STOR_HVISOR) {
    sendAllVolumesToHvNode(reg_node_req->node_name);
  } else {
    sendAllVolumesToFdsMgrNode(n_info);
  }

  // TODO: Update DLT, DMT and republish those here. 

  om_mutex->unlock();

}

void OrchMgr::ReqCfgHandler::CreateVol(const FdspMsgHdrPtr& fdsp_msg,
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

void OrchMgr::ReqCfgHandler::AssociateRespCallback(const Ice::Identity& ident,
                                                   const Ice::Current& current) {
}

}  // namespace fds

int main(int argc, char *argv[]) {
  fds::orchMgr = new fds::OrchMgr();

  fds::orchMgr->main(argc, argv, "orch_mgr.conf");

  delete fds::orchMgr;
}

