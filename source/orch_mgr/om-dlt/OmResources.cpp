/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <iostream>
#include <string>
#include <orch-mgr/om-service.h>
#include <OmDeploy.h>
#include <OmResources.h>
#include <OmDataPlacement.h>
#include <fds_process.h>

namespace fds {

OM_NodeDomainMod             gl_OMNodeDomainMod("OM-Node");

OM_NodeDomainMod::OM_NodeDomainMod(char const *const name) : Module(name)
{
    om_locDomain = new OM_NodeContainer();
}

OM_NodeDomainMod::~OM_NodeDomainMod()
{
    delete om_locDomain;
}

/**
 * Module functions
 */
int
OM_NodeDomainMod::mod_init(SysParams const *const param)
{
    Module::mod_init(param);

    FdsConfigAccessor conf_helper(g_fdsprocess->get_conf_helper());
    om_test_mode = conf_helper.get<bool>("test_mode");
    om_locDomain->om_init_domain();
    return 0;
}

void
OM_NodeDomainMod::mod_startup()
{
}

void
OM_NodeDomainMod::mod_shutdown()
{
}

// om_local_domain
// ---------------
//
OM_NodeDomainMod *
OM_NodeDomainMod::om_local_domain()
{
    return &gl_OMNodeDomainMod;
}

// om_reg_node_info
// ----------------
//
Error
OM_NodeDomainMod::om_reg_node_info(const NodeUuid&      uuid,
                                   const FdspNodeRegPtr msg)
{
    NodeAgent::pointer newNode;
    /* XXX: TODO (vy), remove this code once we have node FSM */
    OM_Module *om = OM_Module::om_singleton();

    Error err = om_locDomain->dc_register_node(uuid, msg, &newNode);
    if (err == ERR_OK) {
        fds_verify(newNode != NULL);
        om_locDomain->om_bcast_new_node(newNode, msg);

        // Let this new node know about exisiting node list.
        // TODO(Andrew): this should change into dissemination of the cur cluster map.
        //
        om_locDomain->om_add_capacity(newNode);
        om_locDomain->om_update_node_list(newNode, msg);
        om_locDomain->om_bcast_vol_list(newNode);

        // Let this new know about existing dlt.
        // TODO(Andrew): this should change into dissemination of the cur cluster map.
        DataPlacement *dp = om->om_dataplace_mod();
        OM_SmAgent::agt_cast_ptr(newNode)->om_send_dlt(dp->getCurDlt());

        // Send the DMT to DMs.
        om_locDomain->om_round_robin_dmt();
        om_locDomain->om_bcast_dmt_table();
    }
    ClusterMap *clus = om->om_clusmap_mod();

    if (clus->getNumMembers() == 0) {
        static int node_up_cnt = 0;

        node_up_cnt++;
        if (node_up_cnt < 1) {
            FDS_PLOG_SEV(g_fdslog, fds_log::normal)
                    << "OM_NBodeDomainMod: Batch up node up, cnt " << node_up_cnt;
            return err;
        }
    }

    // If this new node is an SM, update the DLT and bcast
    // TODO(Andrew): We should decouple registration from
    // cluster map addition eventually. We may want to add
    // the node to the inventory and then wait for a CLI
    // cmd to make the node a member.
    // TODO(Andrew): Today, cluster map only knows SM. It
    // should contain all members
    om_update_cluster_map();

    return err;
}

// om_del_node_info
// ----------------
//
Error
OM_NodeDomainMod::om_del_node_info(const NodeUuid& uuid,
                                   const std::string& node_name)
{
    Error err = om_locDomain->dc_unregister_node(uuid, node_name);

    om_update_cluster_map();
    return err;
}

// om_create_domain
// ----------------
//
int
OM_NodeDomainMod::om_create_domain(const FdspCrtDomPtr &crt_domain)
{
    return 0;
}

// om_delete_domain
// ----------------
//
int
OM_NodeDomainMod::om_delete_domain(const FdspCrtDomPtr &crt_domain)
{
    return 0;
}

// om_update_cluster_map
// ---------------------
//
void
OM_NodeDomainMod::om_update_cluster_map()
{
    OM_SmContainer::pointer smNodes = om_locDomain->om_sm_nodes();
    NodeList       addNodes, rmNodes;
    OM_Module     *om;
    OM_DLTMod     *dlt;
    ClusterMap    *clus;
    DataPlacement *dp;

    om   = OM_Module::om_singleton();
    dp   = om->om_dataplace_mod();
    dlt  = om->om_dlt_mod();
    clus = om->om_clusmap_mod();

    FDS_PLOG_SEV(g_fdslog, fds_log::debug) << "OM_NodeDomainMod: Call cluster update map";

    smNodes->om_splice_nodes_pend(&addNodes, &rmNodes);
    clus->updateMap(addNodes, rmNodes);
    // dlt->dlt_deploy_event(DltCompEvt(dp));
}

/**
 * Drives the DLT deployment state machine.
 */
void
OM_NodeDomainMod::om_update_cluster() {
    OM_Module *om = OM_Module::om_singleton();
    OM_DLTMod *dltMod = om->om_dlt_mod();
    DataPlacement *dp = om->om_dataplace_mod();

    // Recompute the DLT
    DltCompEvt computeEvent(dp);
    dltMod->dlt_deploy_event(computeEvent);

    // Start rebalancing/syncing data to prepare
    // for the new DLT
    DltRebalEvt rebalEvent(dp);
    dltMod->dlt_deploy_event(rebalEvent);
}

// Called when OM receives notification that the rebalance is
// done on node with uuid 'uuid'.
Error
OM_NodeDomainMod::om_recv_migration_done(const NodeUuid& uuid,
                                         fds_uint64_t dlt_version) {
    Error err(ERR_OK);
    OM_Module *om = OM_Module::om_singleton();
    OM_DLTMod *dltMod = om->om_dlt_mod();
    DataPlacement *dp = om->om_dataplace_mod();
    OM_SmAgent::pointer agent = om_sm_agent(uuid);
    if (agent == NULL) {
        FDS_PLOG_SEV(g_fdslog, fds_log::error)
                << "OM: Received migration done event from unknown node "
                << ": uuid " << uuid.uuid_get_val();
        err = Error(ERR_NOT_FOUND);
        return err;
    }

    // for now we shouldn't move to new dlt version until
    // we are done with current cluster update, so
    // expect to see migration done resp for current dlt version
    fds_uint64_t cur_dlt_ver = (dp->getCurDlt())->getVersion();
    fds_verify(cur_dlt_ver == dlt_version);

    // Set node's state to 'node_up'
    agent->set_node_state(FDS_ProtocolInterface::FDS_Node_Up);

    // update node's dlt version so we don't send this dlt again
    agent->set_node_dlt_version(dlt_version);

    // 'rebal ok' event, once all nodes sent migration done
    // notification, the state machine will commit the DLT
    // to other nodes.
    ClusterMap* cm = om->om_clusmap_mod();
    dltMod->dlt_deploy_event(DltRebalOkEvt(cm, dp));

    // in case no nodes need the new DLT (eg. we just added the
    // first node and it already got the new DLT when we sent
    // start migration event), the commit ok guard will check
    // if all nodes already have new DLT and move to the next state
    // otherwise, it will wait for dlt commit responses
    dltMod->dlt_deploy_event(DltCommitOkEvt(cm, cur_dlt_ver));

    return err;
}

//
// Called when OM receives response for DLT commit fron a node
//
Error
OM_NodeDomainMod::om_recv_dlt_commit_resp(const NodeUuid& uuid,
                                          fds_uint64_t dlt_version) {
    Error err(ERR_OK);
    OM_Module *om = OM_Module::om_singleton();
    OM_DLTMod *dltMod = om->om_dlt_mod();
    DataPlacement *dp = om->om_dataplace_mod();
    OM_SmAgent::pointer agent = om_sm_agent(uuid);
    if (agent == NULL) {
        FDS_PLOG_SEV(g_fdslog, fds_log::error)
                << "OM: Received DLT commit ack from unknown node "
                << ": uuid " << uuid.uuid_get_val();
        err = Error(ERR_NOT_FOUND);
        return err;
    }

    // for now we shouldn't move to new dlt version until
    // we are done with current cluster update, so
    // expect to see dlt commit resp for current dlt version
    fds_uint64_t cur_dlt_ver = (dp->getCurDlt())->getVersion();
    fds_verify(cur_dlt_ver == dlt_version);

    // set node's confirmed dlt version to this version
    agent->set_node_dlt_version(dlt_version);

    // commit ok event, will transition to next state when
    // when all 'up' nodes acked this dlt commit
    ClusterMap* cm = om->om_clusmap_mod();
    dltMod->dlt_deploy_event(DltCommitOkEvt(cm, cur_dlt_ver));

    return err;
}

// om_persist_node_info
// --------------------
//
void
OM_NodeDomainMod::om_persist_node_info(fds_uint32_t idx)
{
}

/**
 * Constructor for OM control path response handling
 */
OM_ControlRespHandler::OM_ControlRespHandler() {
}

void
OM_ControlRespHandler::NotifyAddVolResp(
    const FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
    const FDS_ProtocolInterface::FDSP_NotifyVolType& not_add_vol_resp) {
    // Don't do anything here. This stub is just to keep cpp compiler happy
}

void
OM_ControlRespHandler::NotifyAddVolResp(
    FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
    FDS_ProtocolInterface::FDSP_NotifyVolTypePtr& not_add_vol_resp) {
}

void
OM_ControlRespHandler::NotifyRmVolResp(
    const FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
    const FDS_ProtocolInterface::FDSP_NotifyVolType& not_rm_vol_resp) {
    // Don't do anything here. This stub is just to keep cpp compiler happy
}

void
OM_ControlRespHandler::NotifyRmVolResp(
    FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
    FDS_ProtocolInterface::FDSP_NotifyVolTypePtr& not_rm_vol_resp) {
}

void
OM_ControlRespHandler::NotifyModVolResp(
    const FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
    const FDS_ProtocolInterface::FDSP_NotifyVolType& not_mod_vol_resp) {
    // Don't do anything here. This stub is just to keep cpp compiler happy
}

void
OM_ControlRespHandler::NotifyModVolResp(
    FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
    FDS_ProtocolInterface::FDSP_NotifyVolTypePtr& not_mod_vol_resp) {
}

void
OM_ControlRespHandler::AttachVolResp(
    const FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
    const FDS_ProtocolInterface::FDSP_AttachVolType& atc_vol_resp) {
    // Don't do anything here. This stub is just to keep cpp compiler happy
}

void
OM_ControlRespHandler::AttachVolResp(
    FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
    FDS_ProtocolInterface::FDSP_AttachVolTypePtr& atc_vol_resp) {
}

void
OM_ControlRespHandler::DetachVolResp(
    const FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
    const FDS_ProtocolInterface::FDSP_AttachVolType& dtc_vol_resp) {
    // Don't do anything here. This stub is just to keep cpp compiler happy
}

void
OM_ControlRespHandler::DetachVolResp(
    FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
    FDS_ProtocolInterface::FDSP_AttachVolTypePtr& dtc_vol_resp) {
}

void
OM_ControlRespHandler::NotifyNodeAddResp(
    const FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
    const FDS_ProtocolInterface::FDSP_Node_Info_Type& node_info_resp) {
    // Don't do anything here. This stub is just to keep cpp compiler happy
}

void
OM_ControlRespHandler::NotifyNodeAddResp(
    FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
    FDS_ProtocolInterface::FDSP_Node_Info_TypePtr& node_info_resp) {
}

void
OM_ControlRespHandler::NotifyNodeRmvResp(
    const FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
    const FDS_ProtocolInterface::FDSP_Node_Info_Type& node_info_resp) {
    // Don't do anything here. This stub is just to keep cpp compiler happy
}

void
OM_ControlRespHandler::NotifyNodeRmvResp(
    FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
    FDS_ProtocolInterface::FDSP_Node_Info_TypePtr& node_info_resp) {
}

void
OM_ControlRespHandler::NotifyDLTUpdateResp(
    const FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
    const FDS_ProtocolInterface::FDSP_DLT_Resp_Type& dlt_resp) {
    // Don't do anything here. This stub is just to keep cpp compiler happy
}

void
OM_ControlRespHandler::NotifyDLTUpdateResp(
    FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
    FDS_ProtocolInterface::FDSP_DLT_Resp_TypePtr& dlt_resp) {
    FDS_PLOG_SEV(g_fdslog, fds_log::notification)
            << "OM received response for NotifyDltUpdate from node "
            << fdsp_msg->src_node_name
            << " for DLT version " << dlt_resp->DLT_version;

    OM_NodeDomainMod* domain = OM_NodeDomainMod::om_local_domain();
    // TODO(Anna) Should we use node names or node uuids directly in
    // fdsp messages? for now getting uuid from hashing the name
    NodeUuid node_uuid(fds_get_uuid64(fdsp_msg->src_node_name));
    domain->om_recv_dlt_commit_resp(node_uuid, dlt_resp->DLT_version);
}

void
OM_ControlRespHandler::NotifyDMTUpdateResp(
    const FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
    const FDS_ProtocolInterface::FDSP_DMT_Type& dmt_info_resp) {
    // Don't do anything here. This stub is just to keep cpp compiler happy
}

void
OM_ControlRespHandler::NotifyDMTUpdateResp(
    FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
    FDS_ProtocolInterface::FDSP_DMT_TypePtr& dmt_info_resp) {
}

}  // namespace fds
