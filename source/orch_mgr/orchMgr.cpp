/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#include "orch_mgr/orchMgr.h"

#include <iostream>  // NOLINT(*)
#include <string>

namespace fds {

OrchMgr *orchMgr;

OrchMgr::OrchMgr()
    : port_num(0) {
  om_log = new fds_log("om", "logs");
  om_mutex = new fds_mutex("OrchMgrMutex");
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
      fds_node_id_t node_id = it->first;
      NodeInfo& node_info = it->second;

      ReqCtrlPrx OMClientAPI = node_info.cpPrx;
      OMClientAPI->NotifyAddVol(msg_hdr, vol_msg);
    }
  }
}

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
      fds_node_id_t node_id = it->first;
      NodeInfo& node_info = it->second;

      ReqCtrlPrx OMClientAPI = node_info.cpPrx;
      OMClientAPI->NotifyRmVol(msg_hdr, vol_msg);
    }
  }
}

void OrchMgr::sendAttachVolToHVNode(fds_node_id_t node_id, VolumeInfo *pVolInfo) {
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

  NodeInfo& node_info = currentShMap[node_id];

  ReqCtrlPrx OMClientAPI = node_info.cpPrx;
  OMClientAPI->AttachVol(msg_hdr, vol_msg);
}

void OrchMgr::sendDetachVolToHVNode(fds_node_id_t node_id,
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

  NodeInfo& node_info = currentShMap[node_id];

  ReqCtrlPrx OMClientAPI = node_info.cpPrx;
  OMClientAPI->DetachVol(msg_hdr, vol_msg);
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
  om_mutex->unlock();
  sendCreateVolToFdsNodes(new_vol);
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
  om_mutex->unlock();

  for (int i = 0; i < del_vol->hv_nodes.size(); i++) {
    if (currentShMap.count(del_vol->hv_nodes[i]) == 0) {
      FDS_PLOG(om_log) << "Inconsistent State Detected. "
                       << "HV node in volume's hvnode list but not in SH map";
      assert(0);
    }
    sendDetachVolToHVNode(del_vol->hv_nodes[i], del_vol);
  }

  sendDeleteVolToFdsNodes(del_vol);

  om_mutex->lock();
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
  fds_node_id_t node_id = atc_vol_req->node_id;

  FDS_PLOG(GetLog()) << "Received Attach Vol Req for volume "
                     << vol_name << " ; id - " << vol_id;
  om_mutex->lock();
  if (volumeMap.count(vol_id) == 0) {
    FDS_PLOG(om_log) << "Received Attach Vol for non-existent volume "
                     << vol_id;
    om_mutex->unlock();
    return;
  }
  VolumeInfo *this_vol = volumeMap[vol_id];

  if (currentShMap.count(node_id) == 0) {
    FDS_PLOG(om_log) << "Received Attach Vol for non-existent node " << node_id;
    om_mutex->unlock();
    return;
  }

  for (int i = 0; i < this_vol->hv_nodes.size(); i++) {
    if (this_vol->hv_nodes[i] == node_id) {
      FDS_PLOG(om_log) << "Attach Vol req for volume " << vol_id
                       << " rejected because this volume is "
                       << "already attached at node " << node_id;
      om_mutex->unlock();
      return;
    }
  }
  this_vol->hv_nodes.push_back(node_id);
  om_mutex->unlock();

  sendAttachVolToHVNode(node_id, this_vol);
}



void OrchMgr::DetachVol(const FdspMsgHdrPtr    &fdsp_msg,
                        const FdspAttVolCmdPtr &dtc_vol_req) {
  int  vol_id = dtc_vol_req->vol_uuid;
  std::string vol_name = dtc_vol_req->vol_name;
  fds_node_id_t node_id = dtc_vol_req->node_id;
  fds_bool_t node_not_attached = true;

  FDS_PLOG(GetLog()) << "Received Detach Vol Req for volume "
                     << vol_name << " ; id - " << vol_id;
  om_mutex->lock();
  if (volumeMap.count(vol_id) == 0) {
    FDS_PLOG(om_log) << "Received Detach Vol for non-existent volume "
                     << vol_id;
    om_mutex->unlock();
    return;
  }
  VolumeInfo *this_vol = volumeMap[vol_id];

  if (currentShMap.count(node_id) == 0) {
    FDS_PLOG(om_log) << "Received Detach Vol for non-existent node " << node_id;
    om_mutex->unlock();
    return;
  }

  for (int i = 0; i < this_vol->hv_nodes.size(); i++) {
    if (this_vol->hv_nodes[i] == node_id) {
      node_not_attached = false;
      this_vol->hv_nodes.erase(this_vol->hv_nodes.begin()+i);
      break;
    }
  }
  om_mutex->unlock();
  if (node_not_attached) {
    FDS_PLOG(om_log) << "Detach Vol req for volume " << vol_id
                     << " rejected because this volume is "
                     << "not attached at node " << node_id;
  }

  sendDetachVolToHVNode(node_id, this_vol);
}

void OrchMgr::RegisterNode(const FdspMsgHdrPtr  &fdsp_msg,
                           const FdspRegNodePtr &reg_node_req) {
  std::string ip_addr_str;
  Ice::Identity ident;
  std::ostringstream tcpProxyStr;

  FDS_PLOG(GetLog()) << "Received RegisterNode Msg"
                     << "  Node Id:" << reg_node_req->node_id
                     << "  Node IP:" << std::hex << reg_node_req->ip_lo_addr
                     << "  Node Type:" << std::dec << reg_node_req->node_type
                     << "  Control Port: " << reg_node_req->control_port
                     << "  Data Port: " << reg_node_req->data_port;

  ip_addr_str = ipv4_addr_to_str(reg_node_req->ip_lo_addr);
  
  // create a new  control  communication adaptor
  tcpProxyStr << "OrchMgrClient: tcp -h " << ip_addr_str
              << " -p  " << reg_node_req->control_port;

  // build the SM node map
  NodeInfo n_info(reg_node_req->node_id,
                  reg_node_req->ip_lo_addr,
                  reg_node_req->control_port,
                  n_info.data_port = reg_node_req->data_port,
                  n_info.node_state = FDSP_Types::FDS_Node_Up,
                  FDS_ProtocolInterface::
                  FDSP_ControlPathReqPrx::
                  checkedCast(communicator()->stringToProxy(tcpProxyStr.str())));

  switch (reg_node_req->node_type) {
    case FDS_ProtocolInterface::FDSP_STOR_MGR:
      currentSmMap[reg_node_req->node_id] = n_info;
      break;
    case FDS_ProtocolInterface::FDSP_DATA_MGR:
      currentDmMap[reg_node_req->node_id] = n_info;
      break;
    case FDS_ProtocolInterface::FDSP_STOR_HVISOR:
      currentShMap[reg_node_req->node_id] = n_info;
      break;
    default:
      FDS_PLOG(GetLog()) << "Unknown node type received";
  }
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

