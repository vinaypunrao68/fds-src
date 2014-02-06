/*                                                                                                   
 * Copyright 2013 Formation Data Systems, Inc.                                                       
 */
#include <orchMgr.h>
#include <OmLocDomain.h>

#include <iostream>  // NOLINT(*)
#include <string>
#include <vector>

namespace fds {

FdsLocalDomain::FdsLocalDomain(const std::string& om_prefix,
                               fds_log* om_log)
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

//    curDlt = new FdsDlt(dltWidth, dltDepth);
    curDmt = new FdsDmt(dmtWidth, dmtDepth);

    next_free_vol_id = 2;

    /*
     * per domain  admin control
     */

    admin_ctrl = new FdsAdminCtrl(om_prefix, om_log);

    /* cached stats that we receve from AM - we will keep a bit longer history
     * than AM does to account for async receive of stats from multiple AM
     * and then CLI query for them */
    am_stats = new PerfStats(om_prefix+"OM_from_AM", 5*FDS_STAT_DEFAULT_HIST_SLOTS);
    if (am_stats) {
        am_stats->enable(); /* stats are not enabled by default */
    }

    /* TEMP file */
    std::string fname = std::string("stats//") + om_prefix+"-example.json";
    json_file.open(fname.c_str(), std::ios::out | std::ios::app);
}

FdsLocalDomain::~FdsLocalDomain()
{
    delete curDlt;
    delete curDmt;
    if (am_stats) {
        am_stats->disable();
        delete am_stats;
    }
    if (json_file.is_open()) {
        json_file.close();
    }
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

    int total_num_nodes = nodeMap.size();
    int bucket_depth = table->getMaxDepth();
    if (bucket_depth > total_num_nodes) {
        bucket_depth = total_num_nodes;
    }

    for (fds_uint32_t i = 0; i < table->getNumBuckets(); i++) {
        node_list.clear();
        node_map_t::const_iterator nl_it = rr_it;
        if (nl_it == nodeMap.cend()) {
            continue;
        }

        for (fds_uint32_t j = 0; j < bucket_depth; j++) {
            node_list.push_back((nl_it->second).node_id);
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
    FDS_PLOG_SEV(parent_log, fds::fds_log::notification) << "Updating DLT";
// SAN    roundRobinDlt(static_cast<fds_placement_table *>(curDlt),
//                  currentSmMap,
//                  parent_log);
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
    FDS_PLOG_SEV(parent_log, fds::fds_log::notification) << "Updating DMT";
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

    FDS_PLOG_SEV(parent_log, fds::fds_log::notification)
            << "Loading cluster map from local files "
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
                         0, 0, 0, 0,
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
            sscanf(curr_ptr, "%llu%n", &node_id, &bytes_read);  // NOLINT(*)
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

void FdsLocalDomain::copyPropertiesToVolumeDesc(
    FDS_ProtocolInterface::FDSP_VolumeDescType& v_desc,
    VolumeDesc *pVol) {

    v_desc.vol_name = pVol->name;
    v_desc.volUUID = pVol->volUUID;
    v_desc.tennantId = pVol->tennantId;
    v_desc.localDomainId = pVol->localDomainId;
    v_desc.globDomainId = pVol->globDomainId;

    v_desc.capacity = pVol->capacity;
    v_desc.volType = pVol->volType;
    v_desc.maxQuota = pVol->maxQuota;
    v_desc.defReplicaCnt = pVol->replicaCnt;

    v_desc.defConsisProtocol = FDS_ProtocolInterface::
            FDSP_ConsisProtoType(pVol->consisProtocol);
    v_desc.appWorkload = pVol->appWorkload;

    v_desc.volPolicyId = pVol->volPolicyId;
    v_desc.iops_max = pVol->iops_max;
    v_desc.iops_min = pVol->iops_min;
    v_desc.rel_prio = pVol->relativePrio;
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
    msg_hdr->src_node_name = "";

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
    FDS_PLOG_SEV(parent_log, fds_log::error)
            << "No id available to allocate to node " << node_name;

    return -1;
}

fds_int32_t FdsLocalDomain::getNextFreeVolId() {
    if ((next_free_vol_id+1) == admin_vol_id)
        ++next_free_vol_id;

    return next_free_vol_id++;
}

/*
 * Dump all existing SM/DM nodes info as a sequence
 * of NotifyNodeAdd ctrl messages to a newly registering node
 */
void FdsLocalDomain::sendMgrNodeListToFdsNode(const NodeInfo& n_info) {
    FdspMsgHdrPtr msg_hdr_ptr(new FDS_ProtocolInterface::FDSP_MsgHdrType);
    FDS_ProtocolInterface::FDSP_Node_Info_TypePtr node_info_ptr(
        new FDS_ProtocolInterface::FDSP_Node_Info_Type);

    FDSP_ControlPathReqClientPtr ctrlAPI = n_info.getClient();

    initOMMsgHdr(msg_hdr_ptr);
    msg_hdr_ptr->session_uuid = n_info.getSessionId();
    msg_hdr_ptr->msg_code = FDS_ProtocolInterface::FDSP_MSG_NOTIFY_NODE_ADD;
    msg_hdr_ptr->msg_id = 0;
    msg_hdr_ptr->tennant_id = 1;
    msg_hdr_ptr->local_domain_id = 1;

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

            FDS_PLOG_SEV(parent_log, fds_log::notification)
                    << "Sending node notification to node "
                    << n_info.node_name << " for node "
                    << node_name << " IP " << node_info_ptr->ip_lo_addr
                    << " port " << node_info_ptr->data_port << " state - "
                    << next_node_info.node_state;


            if (next_node_info.node_state == FDS_ProtocolInterface::FDS_Node_Up) {
                ctrlAPI->NotifyNodeAdd(msg_hdr_ptr, node_info_ptr);
            } else {
                /*
                 * Nothing to send about this node really. The new node does
                 * not even know about this node.
                 */
                ctrlAPI->NotifyNodeRmv(msg_hdr_ptr, node_info_ptr);
            }
        }
    }
}

// Broadcast a node event to all existing DM/SM/HV nodes
void FdsLocalDomain::sendNodeEventToFdsNodes(const NodeInfo& nodeInfo,
                                             FDS_ProtocolInterface::FDSP_NodeState
                                             node_state) {
    FdspMsgHdrPtr msg_hdr_ptr(new FDS_ProtocolInterface::FDSP_MsgHdrType);
    FDS_ProtocolInterface::FDSP_Node_Info_TypePtr node_info_ptr(
        new FDS_ProtocolInterface::FDSP_Node_Info_Type);

    initOMMsgHdr(msg_hdr_ptr);

    msg_hdr_ptr->msg_code = FDS_ProtocolInterface::FDSP_MSG_NOTIFY_NODE_ADD;
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

            FDS_PLOG_SEV(parent_log, fds_log::notification)
                    << "Sending node notification to node "
                    << node_name << " for node "
                    << nodeInfo.node_name << " state - "
                    << node_state;

            msg_hdr_ptr->session_uuid = next_node_info.getSessionId();
            if (node_state == FDS_ProtocolInterface::FDS_Node_Up) {
                next_node_info.getClient()->NotifyNodeAdd(msg_hdr_ptr, node_info_ptr);
            } else {
                next_node_info.getClient()->NotifyNodeRmv(msg_hdr_ptr, node_info_ptr);
            }
        }
    }
}

// Broadcast DLT or DMT to all existing DM/SM/HV nodes
void FdsLocalDomain::sendNodeTableToFdsNodes(int table_type) {
    FdspMsgHdrPtr msg_hdr_ptr(new FDS_ProtocolInterface::FDSP_MsgHdrType);
    initOMMsgHdr(msg_hdr_ptr);

    msg_hdr_ptr->msg_code = (table_type == table_type_dlt) ?
            FDS_ProtocolInterface::FDSP_MSG_DLT_UPDATE :
            FDS_ProtocolInterface::FDSP_MSG_DMT_UPDATE;
    msg_hdr_ptr->msg_id = 0;
    msg_hdr_ptr->tennant_id = 1;
    msg_hdr_ptr->local_domain_id = 1;

    FDS_ProtocolInterface::FDSP_DLT_Data_TypePtr dlt_info_ptr;
    FDS_ProtocolInterface::FDSP_DMT_TypePtr dmt_info_ptr;
    if (table_type == table_type_dlt) {
        // dlt_info_ptr = new FDS_ProtocolInterface::FDSP_DLT_Type;
        // dlt_info_ptr->DLT_version = current_dlt_version;
        // dlt_info_ptr->DLT = current_dlt_table;
//SAN         dlt_info_ptr = curDlt->toFdsp();
    } else {
        // dmt_info_ptr = new FDS_ProtocolInterface::FDSP_DMT_Type;
        // dmt_info_ptr->DMT_version = current_dmt_version;
        // dmt_info_ptr->DMT = current_dmt_table;
        dmt_info_ptr = curDmt->toFdsp();
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

            FDS_PLOG_SEV(parent_log, fds_log::notification)
                    << "Sending "
                    << ((table_type == table_type_dlt) ? "DLT " : "DMT ")
                    <<  "version "
                    << ((table_type == table_type_dlt) ?
                        current_dlt_version : current_dmt_version)
                    << " to node " << node_name;

            msg_hdr_ptr->session_uuid = next_node_info.getSessionId();
            if (table_type == table_type_dlt) {
                next_node_info.getClient()->NotifyDLTUpdate(msg_hdr_ptr, dlt_info_ptr);
            } else {
                next_node_info.getClient()->NotifyDMTUpdate(msg_hdr_ptr, dmt_info_ptr);
            }
        }
    }
}

/*
 * Dump all existing volumes (as a sequence of create vol ctrl
 * messages) to a newly registering SM/DM Node
 */
void FdsLocalDomain::sendAllVolumesToFdsMgrNode(NodeInfo node_info) {
    FdspMsgHdrPtr msg_hdr(new FDS_ProtocolInterface::FDSP_MsgHdrType);
    FdspNotVolPtr vol_msg(new FDS_ProtocolInterface::FDSP_NotifyVolType);

    initOMMsgHdr(msg_hdr);
    msg_hdr->session_uuid = node_info.getSessionId();
    vol_msg->type = FDS_ProtocolInterface::FDSP_NOTIFY_ADD_VOL;
    msg_hdr->dst_id = FDS_ProtocolInterface::FDSP_DATA_MGR;

    for (auto it = volumeMap.begin(); it != volumeMap.end(); ++it) {
        VolumeInfo *pVolInfo = it->second;
        VolumeDesc* pVolDesc = pVolInfo->properties;
        msg_hdr->glob_volume_id = pVolDesc->volUUID;
        vol_msg->vol_name = std::string(pVolDesc->name);
        copyPropertiesToVolumeDesc(vol_msg->vol_desc, pVolDesc);

        FDS_PLOG_SEV(parent_log, fds_log::notification)
                << "Sending create vol to node "
                << node_info.node_name
                << " for volume " << pVolInfo->volUUID;

        node_info.getClient()->NotifyAddVol(msg_hdr, vol_msg);
    }
}

/**
 * Sends registration result to a single node
 */
void
FdsLocalDomain::sendRegRespToNode(NodeInfo node_info,
                                  const Error &err) {
    FdspMsgHdrPtr msg_hdr(new FDS_ProtocolInterface::FDSP_MsgHdrType);
    FDS_ProtocolInterface::FDSP_RegisterNodeTypePtr reg_msg(
        new FDS_ProtocolInterface::FDSP_RegisterNodeType());

    initOMMsgHdr(msg_hdr);
    msg_hdr->result       = FDS_ProtocolInterface::FDSP_ERR_OK;
    msg_hdr->err_code     = err.GetErrno();
    msg_hdr->session_uuid = node_info.getSessionId();

    FDS_PLOG_SEV(parent_log, fds_log::notification)
            << "Sending registration result to node "
            << node_info.node_name;

    // TODO(Andrew): OM needs an interface to respond to these messages
    // node_info.getClient()->RegisterNodeResp(msg_hdr, reg_msg);
}

// Broadcast create vol ctrl message to all DM/SM Nodes
void FdsLocalDomain::sendCreateVolToFdsNodes(VolumeInfo  *pVolInfo) {
    VolumeDesc* pVolDesc = (pVolInfo->properties);

    FdspMsgHdrPtr msg_hdr(new FDS_ProtocolInterface::FDSP_MsgHdrType);
    FdspNotVolPtr vol_msg(new FDS_ProtocolInterface::FDSP_NotifyVolType);

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

            FDS_PLOG_SEV(parent_log, fds_log::notification)
                    << "Sending create vol to node "
                    << node_name << " for volume "
                    << pVolInfo->volUUID;

            msg_hdr->session_uuid = node_info.getSessionId();
            node_info.getClient()->NotifyAddVol(msg_hdr, vol_msg);
        }
    }
}

void FdsLocalDomain::sendModifyVolToFdsNodes(VolumeInfo* pVolInfo)
{
    VolumeDesc* pVolDesc = pVolInfo->properties;

    FdspMsgHdrPtr msg_hdr(new FDS_ProtocolInterface::FDSP_MsgHdrType);
    FdspNotVolPtr vol_msg(new FDS_ProtocolInterface::FDSP_NotifyVolType);

    initOMMsgHdr(msg_hdr);
    msg_hdr->glob_volume_id = pVolDesc->volUUID;
    vol_msg->type = FDS_ProtocolInterface::FDSP_NOTIFY_MOD_VOL;
    vol_msg->vol_name = std::string(pVolDesc->name);
    copyPropertiesToVolumeDesc(vol_msg->vol_desc, pVolDesc);

    /* Send to all DM/SM nodes */
    for (int i = 0; i < 2; i++) {
        node_map_t& node_map = (i == 0) ? currentDmMap:currentSmMap;
        msg_hdr->dst_id = (i == 0) ?
                FDS_ProtocolInterface::FDSP_DATA_MGR :
                FDS_ProtocolInterface::FDSP_STOR_MGR;

        for (auto it = node_map.begin(); it != node_map.end(); ++it) {
            fds_node_name_t node_name = it->first;
            NodeInfo& node_info = it->second;

            FDS_PLOG_SEV(parent_log, fds_log::notification)
                    << "Sending modify vol to node "
                    << node_name << " for volume "
                    << pVolInfo->vol_name << " UUID "
                    << pVolInfo->volUUID;

            msg_hdr->session_uuid = node_info.getSessionId();
            node_info.getClient()->NotifyModVol(msg_hdr, vol_msg);
        }
    }

    /* send to SH nodes to which this volume is attached to */
    msg_hdr->dst_id = FDS_ProtocolInterface::FDSP_STOR_HVISOR;
    for (int i = 0; i < pVolInfo->hv_nodes.size(); ++i)
    {
        if (currentShMap.count(pVolInfo->hv_nodes[i]) == 0) {
            FDS_PLOG_SEV(parent_log, fds_log::error)
                    << "Inconsistent state detected. "
                    << "AM node in volume's hvnode lis but not in AM map"
                    << " for volume " << pVolInfo->vol_name;
            fds_verify(false);
        }
        FDS_PLOG_SEV(parent_log, fds_log::notification)
                << "Sending modify vol to AM node "
                << pVolInfo->hv_nodes[i] << " for volume "
                << pVolInfo->vol_name << " UUID "
                << pVolInfo->volUUID;

        NodeInfo& node_info = currentShMap[pVolInfo->hv_nodes[i]];
        msg_hdr->session_uuid = node_info.getSessionId();
        node_info.getClient()->NotifyModVol(msg_hdr, vol_msg);
    }
}

void
FdsLocalDomain::sendTierPolicyToSMNodes(const FDSP_TierPolicyPtr &tier)
{
    node_map_t &node_map = currentSmMap;

    for (auto it = node_map.begin(); it != node_map.end(); it++) {
        fds_node_name_t node_name = it->first;
        NodeInfo &node_info = it->second;

        FDS_PLOG_SEV(parent_log, fds_log::notification)
                << "Sending tier policy to node "
                << node_name << " for volume " << tier->tier_vol_uuid;

        // TODO(thrift)
        // node_info.getClient()->TierPolicy(tier);
    }
}

void
FdsLocalDomain::sendTierAuditPolicyToSMNodes(const FDSP_TierPolicyAuditPtr &audit)
{
    node_map_t &node_map = currentSmMap;

    for (auto it = node_map.begin(); it != node_map.end(); it++) {
        fds_node_name_t node_name = it->first;
        NodeInfo &node_info = it->second;

        FDS_PLOG_SEV(parent_log, fds_log::notification)
                << "Sending tier audit policy to node "
                << node_name << " for volume " << audit->tier_vol_uuid;

        // TODO(thrift)
        // node_info.getClient()->TierPolicyAudit(audit);
    }
}

// Broadcast delete vol ctrl message to all DM/SM Nodes
void FdsLocalDomain::sendDeleteVolToFdsNodes(VolumeInfo *pVolInfo) {
    VolumeDesc *pVolDesc = (pVolInfo->properties);

    FdspMsgHdrPtr msg_hdr(new FDS_ProtocolInterface::FDSP_MsgHdrType);
    FdspNotVolPtr vol_msg(new FDS_ProtocolInterface::FDSP_NotifyVolType);

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

            FDS_PLOG_SEV(parent_log, fds_log::notification)
                    << "Sending delete vol to node "
                    << node_name << " for volume "
                    << pVolInfo->volUUID;

            msg_hdr->session_uuid = node_info.getSessionId();
            node_info.getClient()->NotifyRmVol(msg_hdr, vol_msg);
        }
    }
}


// Send attach vol ctrl message to a HV node
void FdsLocalDomain::sendAttachVolToHvNode(fds_node_name_t node_name,
                                           VolumeInfo *pVolInfo) {
    VolumeDesc *pVolDesc = (pVolInfo->properties);

    FdspMsgHdrPtr msg_hdr(new FDS_ProtocolInterface::FDSP_MsgHdrType);
    FdspAttVolPtr vol_msg(new FDS_ProtocolInterface::FDSP_AttachVolType);

    initOMMsgHdr(msg_hdr);
    msg_hdr->msg_code = FDS_ProtocolInterface::FDSP_MSG_ATTACH_VOL_CTRL;
    msg_hdr->dst_id = FDS_ProtocolInterface::FDSP_STOR_HVISOR;
    msg_hdr->glob_volume_id = pVolDesc->volUUID;

    vol_msg->vol_name = std::string(pVolDesc->name);
    copyPropertiesToVolumeDesc(vol_msg->vol_desc, pVolDesc);

    NodeInfo& node_info = currentShMap[node_name];

    FDS_PLOG_SEV(parent_log, fds_log::notification)
            << "Sending attach vol to node " << node_name
            << " node_id " << node_info.node_id;

    msg_hdr->session_uuid = node_info.getSessionId();
    node_info.getClient()->AttachVol(msg_hdr, vol_msg);
}

// Send attach vol ctrl message to a HV node
void FdsLocalDomain::sendDetachVolToHvNode(fds_node_name_t node_name,
                                           VolumeInfo *pVolInfo) {
    VolumeDesc *pVolDesc = (pVolInfo->properties);

    FdspMsgHdrPtr msg_hdr(new FDS_ProtocolInterface::FDSP_MsgHdrType);
    FdspAttVolPtr vol_msg(new FDS_ProtocolInterface::FDSP_AttachVolType);

    initOMMsgHdr(msg_hdr);
    msg_hdr->msg_code = FDS_ProtocolInterface::FDSP_MSG_DETACH_VOL_CTRL;
    msg_hdr->dst_id = FDS_ProtocolInterface::FDSP_STOR_HVISOR;
    msg_hdr->glob_volume_id = pVolDesc->volUUID;

    vol_msg->vol_name = std::string(pVolDesc->name);
    copyPropertiesToVolumeDesc(vol_msg->vol_desc, pVolDesc);

    NodeInfo& node_info = currentShMap[node_name];

    FDS_PLOG_SEV(parent_log, fds_log::notification)
            << "Sending detach vol to node " << node_name
            << " for volume " << pVolInfo->volUUID;

    msg_hdr->session_uuid = node_info.getSessionId();
    node_info.getClient()->DetachVol(msg_hdr, vol_msg);
}

/* Send response for TestBucket if we are not sending attach volume message */
void FdsLocalDomain::sendTestBucketResponseToHvNode(fds_node_name_t node_name,
                                                    const std::string& bucket_name,
                                                    fds_bool_t vol_exists)
{
    // HACK! -- sending attach volume message with error code

    FdspMsgHdrPtr msg_hdr(new FDS_ProtocolInterface::FDSP_MsgHdrType);
    FdspAttVolPtr vol_msg(new FDS_ProtocolInterface::FDSP_AttachVolType);

    initOMMsgHdr(msg_hdr);
    msg_hdr->msg_code = FDS_ProtocolInterface::FDSP_MSG_ATTACH_VOL_CTRL;
    msg_hdr->dst_id = FDS_ProtocolInterface::FDSP_STOR_HVISOR;
    msg_hdr->glob_volume_id = 9876;

    msg_hdr->result = (vol_exists) ?
            FDSP_ERR_VOLUME_EXISTS : FDSP_ERR_VOLUME_DOES_NOT_EXIST;
    msg_hdr->err_msg = (vol_exists) ?
            std::string("Bucket exists") : std::string("Bucket does not exist");

    vol_msg->vol_name = bucket_name;
    /* initialize to something, even though it's ignored, but otherwise ice not happy */
    (vol_msg->vol_desc).vol_name = vol_msg->vol_name;
    (vol_msg->vol_desc).volUUID = 9876;
    (vol_msg->vol_desc).tennantId = 0;
    (vol_msg->vol_desc).localDomainId = 0;
    (vol_msg->vol_desc).capacity = 1000000;
    (vol_msg->vol_desc).volType = FDS_ProtocolInterface::FDSP_VOL_S3_TYPE;
    (vol_msg->vol_desc).maxQuota = 0;
    (vol_msg->vol_desc).defReplicaCnt = 1;
    (vol_msg->vol_desc).defConsisProtocol = FDS_ProtocolInterface::FDSP_CONS_PROTO_STRONG;
    (vol_msg->vol_desc).appWorkload = FDS_ProtocolInterface::FDSP_APP_S3_OBJS;

    NodeInfo& node_info = currentShMap[node_name];

    FDS_PLOG_SEV(parent_log, fds_log::notification)
            << "Sending TestBucket response (attach vol with error code) to node "
            << node_name
            << " for bucket " << vol_msg->vol_name
            << " node_info-> node-id = " << node_info.node_id;

    msg_hdr->session_uuid = node_info.getSessionId();
    node_info.getClient()->AttachVol(msg_hdr, vol_msg);
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

// Broadcast SetThrottleLevel message to all SH Nodes
void FdsLocalDomain::sendThrottleLevelToHvNodes(float throttle_level) {
    FdspMsgHdrPtr msg_hdr(new FDS_ProtocolInterface::FDSP_MsgHdrType);
    FDSP_ThrottleMsgTypePtr throttle_msg(new FDS_ProtocolInterface::FDSP_ThrottleMsgType);

    initOMMsgHdr(msg_hdr);
    throttle_msg->domain_id = DEFAULT_LOC_DOMAIN_ID;
    throttle_msg->throttle_level = throttle_level;

    for (auto it = currentShMap.begin(); it != currentShMap.end(); ++it) {
        fds_node_name_t node_name = it->first;
        NodeInfo& node_info = it->second;

        FDS_PLOG_SEV(parent_log, fds_log::notification)
                << "Sending throttle msg to node "
                << node_name << " for throttle level "
                << throttle_level;

        msg_hdr->session_uuid = node_info.getSessionId();
        node_info.getClient()->SetThrottleLevel(msg_hdr, throttle_msg);
    }
}

/* handle receiving performance stats from AM */
void FdsLocalDomain::handlePerfStatsFromAM(const FDSP_VolPerfHistListType& hist_list,
                                           const std::string start_timestamp)
{
    /* here is an assumption that a volume can only be attached to one AM, 
     * need to revisit if this is not the case anymore */
    for (int i = 0; i < hist_list.size(); ++i)
    {
        double vol_uuid = hist_list[i].vol_uuid;
        for (int j = 0; j < (hist_list[i].stat_list).size(); ++j) {
            FDS_PLOG_SEV(parent_log, fds_log::debug)
                    << "OM: handle perfstat from AM for volume "
                    << hist_list[i].vol_uuid;

            am_stats->setStatFromFdsp((fds_uint32_t)hist_list[i].vol_uuid,
                                      start_timestamp,
                                      (hist_list[i].stat_list)[j]);
        }
    }
}

/* get recent perf stats for all existing volumes and send them to the requesting node */
void FdsLocalDomain::sendBucketStats(fds_uint32_t perf_time_interval,
                                     fds_node_name_t dest_node_name,
                                     fds_uint32_t req_cookie)
{
    FdspMsgHdrPtr msg_hdr(new FDS_ProtocolInterface::FDSP_MsgHdrType);
    FDSP_BucketStatsRespTypePtr buck_stats_rsp(new FDSP_BucketStatsRespType());

    initOMMsgHdr(msg_hdr);
    msg_hdr->msg_code = FDS_ProtocolInterface::FDSP_MSG_GET_BUCKET_STATS_RSP;
    msg_hdr->dst_id = FDS_ProtocolInterface::FDSP_STOR_HVISOR;
    msg_hdr->req_cookie = req_cookie; /* copying from msg header that requested stats */
    msg_hdr->glob_volume_id = 0; /* should ignore */

    boost::posix_time::ptime ts = boost::posix_time::second_clock::universal_time();
    /* here are some hardcored values assuming that AMs send stats every 
     * (default slots num - 2)-second intervals, and OM receives 
     * (default slots num - 1)-second interval worth of stats;
     * OM keeps 3*(default slots num) number of slots of stats. When we get 
     * stats, we ignore last (default slot num) of slots in case OM did not
     * get the latest set of stats from all AMs, and calculate average performance
     * over a 'perf_time_interval' in seconds time interval. */

    /* ignore most recent slots because OM may not receive latest stats from all AMs */
    ts -= boost::posix_time::seconds(am_stats->getSecondsInSlot() *
                                     FDS_STAT_DEFAULT_HIST_SLOTS);
    std::string temp_str = to_iso_extended_string(ts);
    std::size_t i = temp_str.find_first_of(",");
    std::string ts_str("");
    if (i != std::string::npos) {
        ts_str = temp_str.substr(0, i);
    } else {
        ts_str = temp_str;
    }

    /* the timestamp is the most recent timestamp of the interval we are getting the average perf
     * i.e., performance is average iops of interval [timestamp - interval_length... timestamp] */
    buck_stats_rsp->timestamp = ts_str;

    /* for each volume, append perf info */
    for (auto it = volumeMap.begin(); it != volumeMap.end(); ++it) {
        VolumeInfo *pVolInfo = it->second;
        if (!pVolInfo && !pVolInfo->properties)
            continue;

        FDSP_BucketStatType stat;
        stat.vol_name = pVolInfo->vol_name;
        stat.sla = pVolInfo->properties->iops_min;
        stat.limit = pVolInfo->properties->iops_max;
        stat.performance = am_stats->getAverageIOPS(pVolInfo->volUUID,
                                                    ts,
                                                    perf_time_interval);
        stat.rel_prio = pVolInfo->properties->relativePrio;
        (buck_stats_rsp->bucket_stats_list).push_back(stat);

        FDS_PLOG_SEV(parent_log, fds_log::notification)
                << "sendBucketStats: will send stats for volume "
                << pVolInfo->vol_name
                << " perf " << stat.performance;
    }

    NodeInfo& node_info = currentShMap[dest_node_name];

    FDS_PLOG_SEV(parent_log, fds_log::normal)
            << "Sending GetBucketStats response to node "
            << dest_node_name
            << " node_info-> node-id = " << node_info.node_id;

    msg_hdr->session_uuid = node_info.getSessionId();
    node_info.getClient()->NotifyBucketStats(msg_hdr, buck_stats_rsp);
}

/* temp function to print recent perf stats of all existing volumes to json file */
void FdsLocalDomain::printStatsToJsonFile(void)
{
    int count = 0;
    boost::posix_time::ptime ts = boost::posix_time::second_clock::universal_time();
    /* here are some hardcored values assuming that AMs send stats every 
     * (default slots num - 2)-second intervals, and OM receives 
     * (default slots num - 1)-second interval worth of stats;
     * OM keeps 2*(default slots num) number of slots of stats. When we get 
     * stats, we ignore last (default slot num) of slots in case OM did not
     * get the latest set of stats from all AMs, and calculate average performance
     * of 5 seconds that come before the last default number of slots worth of stats */

    /* ignore most recent slots because OM may not receive latest stats from all AMs */
    ts -= boost::posix_time::seconds(am_stats->getSecondsInSlot() *
                                     FDS_STAT_DEFAULT_HIST_SLOTS);
    std::string temp_str = to_iso_extended_string(ts);
    std::size_t i = temp_str.find_first_of(",");
    std::string ts_str("");
    if (i != std::string::npos) {
        ts_str = temp_str.substr(0, i);
    } else {
        ts_str = temp_str;
    }

    json_file << "{" << std::endl;
    json_file << "\"time\": \"" << ts_str << "\"" << std::endl;
    json_file << "\"volumes\":" << std::endl;
    json_file << "  [" << std::endl;

    /* for each volume, append perf info */
    for (auto it = volumeMap.begin(); it != volumeMap.end(); ++it) {
        VolumeInfo *pVolInfo = it->second;
        if (!pVolInfo && !pVolInfo->properties)
            continue;

        FDS_PLOG(parent_log) << "printing stats for volume " << pVolInfo->vol_name;
        if (count > 0) {
            json_file << "," << std::endl;
        }

        /* TODO: the UI wants perf to be value between 0 - 100, so here we assume 
         * performance no higher than 3200 IOPS; check again what's expected */
        int sla = static_cast<int>(pVolInfo->properties->iops_min / 32);
        int limit = static_cast<int>(pVolInfo->properties->iops_max / 32);
        int perf = static_cast<int>(
            am_stats->getAverageIOPS(pVolInfo->volUUID, ts, 5) / 32);

        /* note we hardcoded IOPS average of 5 seconds, 
         * we should probably have that as a parameter */
        json_file << "    {"
                  << "\"id\": " << pVolInfo->volUUID << ", "
                  << "\"priority\": " << pVolInfo->properties->relativePrio << ", "
                  << "\"performance\": " << perf << ", "
                  << "\"sla\": " << sla << ", "
                  << "\"limit\": " << limit
                  << "}";

        ++count;
    }

    json_file << std::endl;
    json_file << "  ]" << std::endl;
    json_file << "}," << std::endl;
    json_file.flush();
}


}  // namespace fds
