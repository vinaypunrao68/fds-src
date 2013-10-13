
#include "orchMgr.h"
#include "OmLocDomain.h"

#include <iostream>  // NOLINT(*)
#include <string>
#include <vector>

namespace fds {

FdsLocalDomain::FdsLocalDomain(const std::string& om_prefix, fds_log* om_log)
  :parent_log(om_log)
{
//  const std::string catname = om_prefix + std::string("_volpolicy_cat.ldb");
//  policy_catalog = new Catalog(catname);

  dom_mutex = new fds_mutex("OrchMgrDomainMutex");

  for (int i = 0; i < MAX_OM_NODES; i++) {
    /*
     * TODO: Make this variable length rather
     * that statically allocated.
     */
    node_id_to_name[i] = "";
  }
  /*
   * Always hard code the DMT/DLT values.
   * TODO: We should read the info from disk
   * and not assume 0.
   */
  curDltVer = 0;
  curDmtVer = 0;
  // dltWidth = 16; /* Can address 2^16 SMs */
  dltWidth = 3; /* Can address 2^3 SMs */
  // dmtWidth = 16; /* Can address 2^16 DMs */
  dmtWidth = 3; /* Can address 2^3 DMs */
  dltDepth = 4;  /* Max 4 total SM replicas */
  dmtDepth = 4;  /* Max 4 total DM replicas */

  curDlt = new FdsDlt(dltWidth, dltDepth);
  curDmt = new FdsDmt(dmtWidth, dmtDepth);


  /*
   * TODO: Remove this or move it to a test mode eventually.
   * This creates a stock volume that the OM knows about so
   * that an OM volume create request isn't needed. Since
   * other nodes (SH/SM/DM) don't generally communicate
   * over the Ice config path, which is what contains the
   * CreateVol() rpc, this is easier.
   */
  VolumeInfo *new_vol = new VolumeInfo();
  new_vol->vol_name = "Test volume";
  new_vol->volUUID = 1;
  new_vol->properties = new VolumeDesc(new_vol->vol_name,
                                       new_vol->volUUID);
  new_vol->hv_nodes.push_back("localhost-sh");
  volumeMap[new_vol->volUUID] = new_vol;

  /*
   * per domain  admin control
   */

   admin_ctrl = new FdsAdminCtrl(om_prefix, om_log);
}

FdsLocalDomain::~FdsLocalDomain()
{
  delete curDlt;
  delete curDmt;
}


void FdsLocalDomain::roundRobinDlt(fds_placement_table* table,
                            const node_map_t& nodeMap,
                            fds_log* callerLog) {
  /*
   * Iterate over each bucket/column and add
   * a vector of size depth().
   */
  Error err(ERR_OK);
  std::vector<fds_nodeid_t> node_list;
  node_map_t::const_iterator rr_it = nodeMap.cbegin();

  for (fds_uint32_t i = 0; i < table->getNumBuckets(); i++) {
    node_list.clear();
    node_map_t::const_iterator nl_it = rr_it;
    if (nl_it == nodeMap.cend()) {
      continue;
    }

    for (fds_uint32_t j = 0; j < table->getMaxDepth(); j++) {
      node_list.push_back((nl_it->second).node_id);
      // FDS_PLOG(callerLog) << "Pushing into bucket " << i
      //                    << " entry " << i << " node "
      //                    << (nl_it->second).node_id;
      nl_it++;
      if (nl_it == nodeMap.cend()) {
        nl_it = nodeMap.cbegin();
      }
    }
    err = table->insert(i, node_list);
    assert(err == ERR_OK);

    /*
     * Move the starting point for the list
     * and reset it to the beginning if we've
     * looped around.
     */
    rr_it++;
    if (rr_it == nodeMap.cend()) {
      rr_it = nodeMap.cbegin();
    }
  }
}

/*
 * Should be called while holding the dlt lock
 */
void FdsLocalDomain::updateDltLocked() {
  /*
   * Calls a specific method to populate
   * the DLT.
   * TODO: For now just call a round robin.
   */
  FDS_PLOG(parent_log) << "Updating DLT";
  roundRobinDlt(static_cast<fds_placement_table *>(curDlt),
                currentSmMap,
                parent_log);
}

/*
 * Should be called while holding the dmt lock
 */
void FdsLocalDomain::updateDmtLocked() {
  /*
   * Calls a specific method to populate
   * the DLT.
   * TODO: For now just call a round robin.
   */
  FDS_PLOG(parent_log) << "Updating DMT";
  roundRobinDlt(static_cast<fds_placement_table *>(curDmt),
                currentDmMap,
                parent_log);
}

/*
 * Updates both the DLT and DMT
 */
void FdsLocalDomain::updateTables() {
  /*
   * Use a less coarse lock, like a
   * lock for just the tables.
   * We want to update both tables under
   * the same lock to ensure consistency.
   */
  dom_mutex->lock();
  updateDltLocked();
  updateDmtLocked();
  dom_mutex->unlock();
}

void FdsLocalDomain::loadNodesFromFile(const std::string& dltFileName,
                                 const std::string& dmtFileName) {
  /*
   * TODO: Move this over to stringstreams eventually
   */
  FILE *fp;
  size_t n_bytes = 0;
  char *line_ptr = 0;

  FDS_PLOG(parent_log) << "Loading cluster map from local files "
                   << dltFileName << " and " << dmtFileName
                   << std::endl;

  fp = fopen(dltFileName.c_str(), "r");
  assert(fp != NULL);

  fds_uint32_t row = 0;

  /*
   * Create some generic empty node to add to
   * the map since there isn't any actual node
   * info available.
   */
  NodeInfo genericNode("Generic node 0",
                       0, 0, 0, 0, 0, 0, 0,
                       0, 0, 0,0,
                       0, 0, 0,
                       FDS_ProtocolInterface::FDS_Node_Up);

  dom_mutex->lock();
  while (!feof(fp)) {
    int n_nodes = 0;
    char *curr_ptr = 0;
    int bytes_read = 0;
    int i;

    /*
     * Reset the maps because we're learning
     * all cluster state from this file.
     */
    currentSmMap.clear();
    currentDmMap.clear();
    currentShMap.clear();

    /*
     * Read line by line
     */
    getline(&line_ptr, &n_bytes, fp);
    sscanf(line_ptr, "%d%n", &n_nodes, &bytes_read);  // NOLINT(*)
    if (n_nodes == 0) {
      continue;
    }
    curr_ptr = line_ptr + bytes_read;

    for (i = 0; i < n_nodes; i++) {
      fds_nodeid_t  node_id = 0;
      sscanf(curr_ptr, "%d%n", &node_id, &bytes_read);  // NOLINT(*)
      curr_ptr += bytes_read;
      currentSmMap[std::to_string(node_id)] = genericNode;
      currentDmMap[std::to_string(node_id)] = genericNode;
      currentShMap[std::to_string(node_id)] = genericNode;
      FDS_PLOG(parent_log) << "Loaded node " << node_id
                       << " into cluster map";
    }

    /*
     * TODO: For now we're just exiting here since all we need
     * is the first row. This needs to be cleaned up.
     */
    fclose(fp);
    dom_mutex->unlock();
    return;

    row++;
    line_ptr[0] = 0;
  }

  dom_mutex->unlock();
  fclose(fp);
}

#if 0
// config path request  handler
OrchMgr::ReqCfgHandler::ReqCfgHandler(OrchMgr *oMgr) {
  this->orchMgr = oMgr;
}

OrchMgr::ReqCfgHandler::~ReqCfgHandler() {
}
#endif

void FdsLocalDomain::copyPropertiesToVolumeDesc(FdspVolDescPtr v_desc,
                                         VolumeDesc *pVol) {
  v_desc->vol_name = pVol->name;
  v_desc->volUUID = pVol->volUUID;
  v_desc->tennantId = pVol->tennantId;
  v_desc->localDomainId = pVol->localDomainId;
  v_desc->globDomainId = pVol->globDomainId;

  v_desc->capacity = pVol->capacity;
  v_desc->volType = pVol->volType;
  v_desc->maxQuota = pVol->maxQuota;
  v_desc->replicaCnt = pVol->replicaCnt;

  v_desc->consisProtocol = FDS_ProtocolInterface::
      FDSP_ConsisProtoType(pVol->consisProtocol);
  v_desc->appWorkload = pVol->appWorkload;

  v_desc->volPolicyId = pVol->volPolicyId;
  v_desc->iops_max = pVol->iops_max;
  v_desc->iops_min = pVol->iops_min;
  v_desc->rel_prio = pVol->relativePrio;
}

void FdsLocalDomain::initOMMsgHdr(const FdspMsgHdrPtr& msg_hdr) {
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

fds_int32_t FdsLocalDomain::getFreeNodeId(const std::string& node_name) {
  for (fds_uint32_t i = 0; i < MAX_OM_NODES; i++) {
    if (node_id_to_name[i] == "") {
      node_id_to_name[i] = node_name;
      return i;
    }
  }
  FDS_PLOG(parent_log) << "No id available to allocate to node " << node_name;
  return -1;
}

/*
 * Dump all existing SM/DM nodes info as a sequence
 * of NotifyNodeAdd ctrl messages to a newly registering node
 */
void FdsLocalDomain::sendMgrNodeListToFdsNode(const NodeInfo& n_info) {
  FdspMsgHdrPtr msg_hdr_ptr = new FDS_ProtocolInterface::FDSP_MsgHdrType;
  FDS_ProtocolInterface::FDSP_Node_Info_TypePtr node_info_ptr =
      new FDS_ProtocolInterface::FDSP_Node_Info_Type;

  initOMMsgHdr(msg_hdr_ptr);

  msg_hdr_ptr->msg_code = FDS_ProtocolInterface::FDSP_MSG_NOTIFY_NODE_ADD;
  msg_hdr_ptr->msg_id = 0;
  msg_hdr_ptr->tennant_id = 1;
  msg_hdr_ptr->local_domain_id = 1;

  ReqCtrlPrx OMClientAPI = n_info.cpPrx;

  for (int i = 0; i < 2; i++) {
    node_map_t& node_map = (i == 0) ? currentDmMap:currentSmMap;

    msg_hdr_ptr->dst_id = n_info.node_type;

    for (auto it = node_map.begin(); it != node_map.end(); ++it) {
      fds_node_name_t node_name = it->first;
      NodeInfo& next_node_info = it->second;
      if (node_name == n_info.node_name) {
        continue;
      }

      node_info_ptr->node_id = next_node_info.node_id;
      node_info_ptr->node_type = next_node_info.node_type;
      node_info_ptr->node_name = next_node_info.node_name;
      node_info_ptr->node_state = next_node_info.node_state;
      node_info_ptr->ip_lo_addr = next_node_info.node_ip_address;
      node_info_ptr->control_port = next_node_info.control_port;
      node_info_ptr->data_port = next_node_info.data_port;

      FDS_PLOG(parent_log) << "Sending node notification to node "
                       << n_info.node_name << " for node "
                       << node_name << " IP " << node_info_ptr->ip_lo_addr
                       << " port " << node_info_ptr->data_port << " state - "
                       << next_node_info.node_state;


      if (next_node_info.node_state == FDS_ProtocolInterface::FDS_Node_Up) {
        OMClientAPI->begin_NotifyNodeAdd(msg_hdr_ptr, node_info_ptr);
      } else {
	/*
         * Nothing to send about this node really. The new node does
         * not even know about this node.
         */
        // OMClientAPI->NotifyNodeRmv(msg_hdr_ptr, node_info_ptr);
      }
    }
  }
}

// Broadcast a node event to all existing DM/SM/HV nodes
void FdsLocalDomain::sendNodeEventToFdsNodes(const NodeInfo& nodeInfo,
                                      FDS_ProtocolInterface::FDSP_NodeState
                                      node_state) {
  FdspMsgHdrPtr msg_hdr_ptr = new FDS_ProtocolInterface::FDSP_MsgHdrType;
  FDS_ProtocolInterface::FDSP_Node_Info_TypePtr node_info_ptr =
      new FDS_ProtocolInterface::FDSP_Node_Info_Type;

  initOMMsgHdr(msg_hdr_ptr);

  msg_hdr_ptr->msg_code = FDS_ProtocolInterface::FDSP_MSG_NOTIFY_NODE_ADD;
  msg_hdr_ptr->msg_id = 0;
  msg_hdr_ptr->tennant_id = 1;
  msg_hdr_ptr->local_domain_id = 1;

  // node_info_ptr->node_id = nodeInfo.node_id;
  node_info_ptr->node_type = nodeInfo.node_type;
  node_info_ptr->node_name = nodeInfo.node_name;
  node_info_ptr->node_state = node_state;
  node_info_ptr->ip_lo_addr = nodeInfo.node_ip_address;
  node_info_ptr->control_port = nodeInfo.control_port;
  node_info_ptr->data_port = nodeInfo.data_port;

  for (int i = 0; i < 3; i++) {
    node_map_t& node_map = (i == 0) ? currentDmMap :
        ((i == 1) ? currentSmMap:currentShMap);

    msg_hdr_ptr->dst_id = (i == 0) ?
      FDS_ProtocolInterface::FDSP_DATA_MGR :
      ((i == 1)?FDS_ProtocolInterface::FDSP_STOR_MGR :
       FDS_ProtocolInterface::FDSP_STOR_HVISOR);

    for (auto it = node_map.begin(); it != node_map.end(); ++it) {
      fds_node_name_t node_name = it->first;
      NodeInfo& next_node_info = it->second;
      if (node_name == nodeInfo.node_name) {
        continue;
      }

      FDS_PLOG(parent_log) << "Sending node notification to node "
                       << node_name << " for node "
                       << nodeInfo.node_name << " state - "
                       << node_state;

      ReqCtrlPrx OMClientAPI = next_node_info.cpPrx;
      if (node_state == FDS_ProtocolInterface::FDS_Node_Up) {
        OMClientAPI->begin_NotifyNodeAdd(msg_hdr_ptr, node_info_ptr);
      } else {
        OMClientAPI->begin_NotifyNodeRmv(msg_hdr_ptr, node_info_ptr);
      }
    }
  }
}

// Broadcast DLT or DMT to all existing DM/SM/HV nodes
void FdsLocalDomain::sendNodeTableToFdsNodes(int table_type) {
  FdspMsgHdrPtr msg_hdr_ptr = new FDS_ProtocolInterface::FDSP_MsgHdrType;

  initOMMsgHdr(msg_hdr_ptr);

  msg_hdr_ptr->msg_code = (table_type == table_type_dlt) ?
      FDS_ProtocolInterface::FDSP_MSG_DLT_UPDATE :
      FDS_ProtocolInterface::FDSP_MSG_DMT_UPDATE;
  msg_hdr_ptr->msg_id = 0;
  msg_hdr_ptr->tennant_id = 1;
  msg_hdr_ptr->local_domain_id = 1;

  FDS_ProtocolInterface::FDSP_DLT_TypePtr dlt_info_ptr;
  FDS_ProtocolInterface::FDSP_DMT_TypePtr dmt_info_ptr;
      //  = new FDS_ProtocolInterface::FDSP_DMT_Type;
  if (table_type == table_type_dlt) {
    // dlt_info_ptr = new FDS_ProtocolInterface::FDSP_DLT_Type;
    // dlt_info_ptr->DLT_version = current_dlt_version;
    // dlt_info_ptr->DLT = current_dlt_table;
    dlt_info_ptr = curDlt->toIce();
  } else {
    //dmt_info_ptr = new FDS_ProtocolInterface::FDSP_DMT_Type;
    // dmt_info_ptr->DMT_version = current_dmt_version;
    // dmt_info_ptr->DMT = current_dmt_table;
    dmt_info_ptr = curDmt->toIce();
  }

  for (int i = 0; i < 3; i++) {
    node_map_t& node_map = (i == 0) ? currentDmMap :
        ((i == 1) ? currentSmMap : currentShMap);

    msg_hdr_ptr->dst_id = (i == 0) ?
      FDS_ProtocolInterface::FDSP_DATA_MGR :
      ((i == 1) ? FDS_ProtocolInterface::FDSP_STOR_MGR :
       FDS_ProtocolInterface::FDSP_STOR_HVISOR);

    for (auto it = node_map.begin(); it != node_map.end(); ++it) {
      fds_node_name_t node_name = it->first;
      NodeInfo& next_node_info = it->second;

      FDS_PLOG(parent_log) << "Sending "
                       << ((table_type == table_type_dlt) ? "DLT " : "DMT ")
                       <<  "version "
                       << ((table_type == table_type_dlt) ?
                           current_dlt_version : current_dmt_version)
                       << " to node " << node_name;

      ReqCtrlPrx OMClientAPI = next_node_info.cpPrx;
      if (table_type == table_type_dlt) {
        OMClientAPI->begin_NotifyDLTUpdate(msg_hdr_ptr, dlt_info_ptr);
      } else {
        OMClientAPI->begin_NotifyDMTUpdate(msg_hdr_ptr, dmt_info_ptr);
      }
    }
  }
}

/*
 * Dump all existing volumes (as a sequence of create vol ctrl
 * messages) to a newly registering SM/DM Node
 */
void FdsLocalDomain::sendAllVolumesToFdsMgrNode(NodeInfo node_info) {
  ReqCtrlPrx OMClientAPI = node_info.cpPrx;

  FdspMsgHdrPtr msg_hdr = new FDS_ProtocolInterface::FDSP_MsgHdrType;
  FdspNotVolPtr vol_msg = new FDS_ProtocolInterface::FDSP_NotifyVolType;
  vol_msg->vol_desc = new FDS_ProtocolInterface::FDSP_VolumeDescType();

  initOMMsgHdr(msg_hdr);
  vol_msg->type = FDS_ProtocolInterface::FDSP_NOTIFY_ADD_VOL;
  msg_hdr->dst_id = FDS_ProtocolInterface::FDSP_DATA_MGR;

  for (auto it = volumeMap.begin(); it != volumeMap.end(); ++it) {
    VolumeInfo *pVolInfo = it->second;
    VolumeDesc* pVolDesc = pVolInfo->properties;
    msg_hdr->glob_volume_id = pVolDesc->volUUID;
    vol_msg->vol_name = std::string(pVolDesc->name);
    copyPropertiesToVolumeDesc(vol_msg->vol_desc, pVolDesc);
    
    FDS_PLOG(parent_log) << "Sending create vol to node " << node_info.node_name << " for volume " << pVolInfo->volUUID;
    OMClientAPI->begin_NotifyAddVol(msg_hdr, vol_msg);
  }
}

// Broadcast create vol ctrl message to all DM/SM Nodes
void FdsLocalDomain::sendCreateVolToFdsNodes(VolumeInfo  *pVolInfo) {
  VolumeDesc* pVolDesc = (pVolInfo->properties);

  FdspMsgHdrPtr msg_hdr = new FDS_ProtocolInterface::FDSP_MsgHdrType;
  FdspNotVolPtr vol_msg = new FDS_ProtocolInterface::FDSP_NotifyVolType;
  vol_msg->vol_desc = new FDS_ProtocolInterface::FDSP_VolumeDescType();

  initOMMsgHdr(msg_hdr);
  msg_hdr->glob_volume_id = pVolDesc->volUUID;
  vol_msg->type = FDS_ProtocolInterface::FDSP_NOTIFY_ADD_VOL;
  vol_msg->vol_name = std::string(pVolDesc->name);
  copyPropertiesToVolumeDesc(vol_msg->vol_desc, pVolDesc);

  for (int i = 0; i < 2; i++) {
    node_map_t& node_map = (i == 0) ? currentDmMap:currentSmMap;
    msg_hdr->dst_id = (i == 0) ?
        FDS_ProtocolInterface::FDSP_DATA_MGR :
        FDS_ProtocolInterface::FDSP_STOR_MGR;

    for (auto it = node_map.begin(); it != node_map.end(); ++it) {
      fds_node_name_t node_name = it->first;
      NodeInfo& node_info = it->second;

      FDS_PLOG(parent_log) << "Sending create vol to node "
                       << node_name << " for volume "
                       << pVolInfo->volUUID;

      ReqCtrlPrx OMClientAPI = node_info.cpPrx;
      OMClientAPI->begin_NotifyAddVol(msg_hdr, vol_msg);
    }
  }
}

// Broadcast delete vol ctrl message to all DM/SM Nodes
void FdsLocalDomain::sendDeleteVolToFdsNodes(VolumeInfo *pVolInfo) {
  VolumeDesc *pVolDesc = (pVolInfo->properties);

  FdspMsgHdrPtr msg_hdr = new FDS_ProtocolInterface::FDSP_MsgHdrType;
  FdspNotVolPtr vol_msg = new FDS_ProtocolInterface::FDSP_NotifyVolType;
  vol_msg->vol_desc = new FDS_ProtocolInterface::FDSP_VolumeDescType();

  initOMMsgHdr(msg_hdr);
  msg_hdr->glob_volume_id = pVolDesc->volUUID;
  vol_msg->type = FDS_ProtocolInterface::FDSP_NOTIFY_RM_VOL;
  vol_msg->vol_name = std::string(pVolDesc->name);
  copyPropertiesToVolumeDesc(vol_msg->vol_desc, pVolDesc);

  for (int i = 0; i < 2; i++) {
    node_map_t& node_map = (i == 0) ? currentDmMap : currentSmMap;
    msg_hdr->dst_id = (i == 0) ?
        FDS_ProtocolInterface::FDSP_DATA_MGR :
        FDS_ProtocolInterface::FDSP_STOR_MGR;

    for (auto it = node_map.begin(); it != node_map.end(); ++it) {
      fds_node_name_t node_name = it->first;
      NodeInfo& node_info = it->second;

      FDS_PLOG(parent_log) << "Sending delete vol to node "
                       << node_name << " for volume "
                       << pVolInfo->volUUID;

      ReqCtrlPrx OMClientAPI = node_info.cpPrx;
      OMClientAPI->begin_NotifyRmVol(msg_hdr, vol_msg);
    }
  }
}

// Send attach vol ctrl message to a HV node
void FdsLocalDomain::sendAttachVolToHvNode(fds_node_name_t node_name,
                                    VolumeInfo *pVolInfo) {

  VolumeDesc *pVolDesc = (pVolInfo->properties);

  FdspMsgHdrPtr msg_hdr = new FDS_ProtocolInterface::FDSP_MsgHdrType;
  FdspAttVolPtr vol_msg = new FDS_ProtocolInterface::FDSP_AttachVolType;
  vol_msg->vol_desc = new FDS_ProtocolInterface::FDSP_VolumeDescType();

  initOMMsgHdr(msg_hdr);
  msg_hdr->msg_code = FDS_ProtocolInterface::FDSP_MSG_ATTACH_VOL_CTRL;
  msg_hdr->dst_id = FDS_ProtocolInterface::FDSP_STOR_HVISOR;
  msg_hdr->glob_volume_id = pVolDesc->volUUID;

  vol_msg->vol_name = std::string(pVolDesc->name);
  copyPropertiesToVolumeDesc(vol_msg->vol_desc, pVolDesc);

  NodeInfo& node_info = currentShMap[node_name];

  FDS_PLOG(parent_log) << "Sending attach vol to node " << node_name
                   << " for volume " << pVolInfo->volUUID;

  ReqCtrlPrx OMClientAPI = node_info.cpPrx;
  OMClientAPI->begin_AttachVol(msg_hdr, vol_msg);
}

// Send attach vol ctrl message to a HV node
void FdsLocalDomain::sendDetachVolToHvNode(fds_node_name_t node_name,
                                    VolumeInfo *pVolInfo) {

  VolumeDesc *pVolDesc = (pVolInfo->properties);

  FdspMsgHdrPtr msg_hdr = new FDS_ProtocolInterface::FDSP_MsgHdrType;
  FdspAttVolPtr vol_msg = new FDS_ProtocolInterface::FDSP_AttachVolType;
  vol_msg->vol_desc = new FDS_ProtocolInterface::FDSP_VolumeDescType();

  initOMMsgHdr(msg_hdr);
  msg_hdr->msg_code = FDS_ProtocolInterface::FDSP_MSG_DETACH_VOL_CTRL;
  msg_hdr->dst_id = FDS_ProtocolInterface::FDSP_STOR_HVISOR;
  msg_hdr->glob_volume_id = pVolDesc->volUUID;

  vol_msg->vol_name = std::string(pVolDesc->name);
  copyPropertiesToVolumeDesc(vol_msg->vol_desc, pVolDesc);

  NodeInfo& node_info = currentShMap[node_name];

  FDS_PLOG(parent_log) << "Sending detach vol to node " << node_name
                   << " for volume " << pVolInfo->volUUID;

  ReqCtrlPrx OMClientAPI = node_info.cpPrx;
  OMClientAPI->begin_DetachVol(msg_hdr, vol_msg);
}

/*
 * Dump all concerned volumes as a sequence of
 * attach vol ctrl messages to a HV node
 */
void FdsLocalDomain::sendAllVolumesToHvNode(fds_node_name_t node_name) {
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


} /* fds  namespace */
