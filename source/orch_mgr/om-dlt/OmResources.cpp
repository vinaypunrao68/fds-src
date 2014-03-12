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
    Error err(ERR_OK);
    NodeAgent::pointer newNode;
    /* XXX: TODO (vy), remove this code once we have node FSM */
    OM_Module *om = OM_Module::om_singleton();

    // TODO(anna) the below code would not work yet, because
    // register node message from SM/DM does not contain node
    // (platform) uuid yet.
    if ((msg->node_type == FDS_ProtocolInterface::FDSP_STOR_MGR) ||
        (msg->node_type == FDS_ProtocolInterface::FDSP_DATA_MGR)) {
        // we must have a node (platform) that runs any service
        // registered with OM and node must be in active state
        OM_PmContainer::pointer pmNodes = om_locDomain->om_pm_nodes();
        if (!pmNodes->check_new_service((msg->node_uuid).uuid, msg->node_type)) {
            FDS_PLOG_SEV(g_fdslog, fds_log::error)
                    << "Error: cannot register service " << msg->node_name
                    << " on node with uuid " << std::hex << (msg->node_uuid).uuid
                    << std::dec << "; Check if Platform daemon is running";
            return Error(ERR_NODE_NOT_ACTIVE);
        }
    }

    err = om_locDomain->dc_register_node(uuid, msg, &newNode);
    if (err.ok() && (msg->node_type == FDS_ProtocolInterface::FDSP_PLATFORM)) {
        FDS_PLOG(g_fdslog) << "om_reg_node: Registered Platform";
    } else if (err.ok()) {
        fds_verify(newNode != NULL);

        // tell parent PM Agent about its new service
        newNode->set_node_state(FDS_ProtocolInterface::FDS_Node_Up);

        if ((msg->node_uuid).uuid != 0) {
            OM_PmContainer::pointer pmNodes = om_locDomain->om_pm_nodes();
            err = pmNodes->handle_register_service((msg->node_uuid).uuid,
                                                   msg->node_type,
                                                   newNode);
        }

        FDS_PLOG(g_fdslog) << "OM recv reg node uuid " << std::hex
                           << msg->node_uuid.uuid << ", svc uuid " << uuid.uuid_get_val()
                           << std::dec << ", type " << msg->node_type;

        // since we already checked above that we could add service, verify error ok
        fds_verify(err.ok());

        om_locDomain->om_bcast_new_node(newNode, msg);

        // Let this new node know about exisiting node list.
        // TODO(Andrew): this should change into dissemination of the cur cluster map.
        //
        om_locDomain->om_add_capacity(newNode);
        om_locDomain->om_update_node_list(newNode, msg);
        om_locDomain->om_bcast_vol_list(newNode);

        // Let this new node know about existing dlt (if this is an SM, we are
        // sending commited DLT first and then new DLT to start migration)
        // TODO(Andrew): this should change into dissemination of the cur cluster map.
        DataPlacement *dp = om->om_dataplace_mod();
        OM_SmAgent::agt_cast_ptr(newNode)->om_send_dlt(dp->getCommitedDlt());

        // Send the DMT to DMs.
        om_locDomain->om_round_robin_dmt();
        om_locDomain->om_bcast_dmt_table();
    }

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
    OM_PmContainer::pointer pmNodes = om_locDomain->om_pm_nodes();
    // make sure that platform agents do not hold references to this node
    pmNodes->handle_unregister_service(uuid);
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

/**
 * Drives the DLT deployment state machine.
 */
void
OM_NodeDomainMod::om_update_cluster() {
    OM_Module *om = OM_Module::om_singleton();
    OM_DLTMod *dltMod = om->om_dlt_mod();
    DataPlacement *dp = om->om_dataplace_mod();
    ClusterMap* cm = om->om_clusmap_mod();
    OM_SmContainer::pointer smNodes = om_locDomain->om_sm_nodes();

    // Recompute the DLT
    DltCompRebalEvt computeEvent(cm, dp, smNodes);
    dltMod->dlt_deploy_event(computeEvent);

    // ClusterMap only contains SM nodes.
    // If there are not changes in SM nodes, we did not start
    // rebalance, so go back to idle state
    if (((cm->getAddedNodes()).size() == 0) &&
        ((cm->getRemovedNodes()).size() == 0)) {
        FDS_PLOG_SEV(g_fdslog, fds_log::debug)
                << "om_update_cluster: cluster map up to date";
        dltMod->dlt_deploy_event(DltNoRebalEvt());
        return;
    }
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
    fds_uint64_t cur_dlt_ver = dp->getLatestDltVersion();
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
// Called when OM receives response for DLT commit from an SM node
//
Error
OM_NodeDomainMod::om_recv_sm_dlt_commit_resp(const NodeUuid& uuid,
                                             fds_uint64_t dlt_version) {
    Error err(ERR_OK);
    OM_Module *om = OM_Module::om_singleton();
    OM_DLTMod *dltMod = om->om_dlt_mod();
    DataPlacement *dp = om->om_dataplace_mod();
    OM_SmAgent::pointer agent = om_sm_agent(uuid);
    if (agent == NULL) {
        FDS_PLOG_SEV(g_fdslog, fds_log::error)
                << "OM: Received DLT commit ack from unknown/or not SM node "
                << ": uuid " << uuid.uuid_get_val();
        err = Error(ERR_NOT_FOUND);
        return err;
    }

    // for now we shouldn't move to new dlt version until
    // we are done with current cluster update, so
    // expect to see dlt commit resp for current dlt version
    fds_uint64_t cur_dlt_ver = dp->getLatestDltVersion();
    if (cur_dlt_ver > dlt_version) {
        return err;
    }
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
OM_ControlRespHandler::NotifyNodeActiveResp(
    const FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
    const FDS_ProtocolInterface::FDSP_Node_Info_Type& node_info_resp) {
    // Don't do anything here. This stub is just to keep cpp compiler happy
}

void
OM_ControlRespHandler::NotifyNodeActiveResp(
    FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
    FDS_ProtocolInterface::FDSP_Node_Info_TypePtr& node_info_resp) {
    FDS_PLOG_SEV(g_fdslog, fds_log::notification)
            << "OM received response for NotifyNodeActive from node "
            << fdsp_msg->src_node_name;
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
            << fdsp_msg->src_node_name << ":"
            << std::hex << fdsp_msg->src_service_uuid.uuid << std::dec
            << " for DLT version " << dlt_resp->DLT_version;

    // if we got response from non-SM node, nothing to do
    if (fdsp_msg->src_id != FDSP_STOR_MGR) {
        return;
    }

    // if this is from SM, notify DLT state machine
    OM_NodeDomainMod* domain = OM_NodeDomainMod::om_local_domain();
    NodeUuid node_uuid((fdsp_msg->src_service_uuid).uuid);
    domain->om_recv_sm_dlt_commit_resp(node_uuid, dlt_resp->DLT_version);
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
