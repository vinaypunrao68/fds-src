/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <iostream>
#include <orch-mgr/om-service.h>
#include <OmDeploy.h>
#include <OmResources.h>
#include <OmDataPlacement.h>

namespace fds {

OM_NodeDomainMod             gl_OMNodeDomainMod("OM-Node");

OM_NodeDomainMod::OM_NodeDomainMod(char const *const name)
        : Module(name)
{
    omcpSessTbl = boost::shared_ptr<netSessionTbl>(
        new netSessionTbl(FDSP_ORCH_MGR));
    ctrlRspHndlr = boost::shared_ptr<OM_ControlRespHandler>(
        new OM_ControlRespHandler());
}

OM_NodeDomainMod::~OM_NodeDomainMod() {
    omcpSessTbl->endAllSessions();
}

int
OM_NodeDomainMod::mod_init(SysParams const *const param)
{
    Module::mod_init(param);

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
void
OM_NodeDomainMod::om_reg_node_info(const NodeUuid       *uuid,
                                   const FdspNodeRegPtr msg)
{
    fds_bool_t         add;
    NodeAgent::pointer agent;

    add   = false;
    agent = NULL;
    if (uuid != NULL) {
        agent = om_node_info(uuid);
    }
    if (agent == NULL) {
        add   = true;
        agent = om_new_node();
    }
    fds_verify(agent != NULL);
    agent->node_update_info(uuid, msg);
    if (add == true) {
        // Create an RPC endpoint to the node
        NodeAgentCpSessionPtr session(
            omcpSessTbl->startSession<netControlPathClientSession>(
                agent->get_ip_str(),
                agent->get_ctrl_port(),
                FDSP_STOR_MGR,  // TODO(Andrew): Should be just a node
                1,  // Just 1 channel for now...
                ctrlRspHndlr));

        // For now, ensure we can communicate to the node
        // before proceeding
        agent->setCpSession(session);

        // Add node the the node map
        om_activate_node(agent->node_index());
    }
    /* XXX: TODO (vy), remove this code once we have node FSM */
    OM_Module *om = OM_Module::om_singleton();
    ClusterMap *clus = om->om_clusmap_mod();

    if (clus->getNumMembers() == 0) {
        static int node_up_cnt = 0;

        node_up_cnt++;
        if (node_up_cnt < 1) {
            std::cout << "Batch up node up, cnt " << node_up_cnt << std::endl;
            return;
        }
    }

    // TODO(Andrew): We should decouple registration from
    // cluster map addition eventually. We may want to add
    // the node to the inventory and then wait for a CLI
    // cmd to make the node a member.
    om_update_cluster_map();
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
    DltRebalEvt rebalEvent(NULL);
    dltMod->dlt_deploy_event(rebalEvent);

    // TODO(Andrew): This state transition should not
    // be done here. It should be done when we receive
    // notification that the rebalance is done.
    dltMod->dlt_deploy_event(DltRebalOkEvt());

    // TODO(Andrew): This state transition should not
    // be done here. It should be done wherever we
    // transition to rebalance OK and want to send DLTs
    DltCommitEvt commitEvent(dp);
    dltMod->dlt_deploy_event(commitEvent);

    // TODO(Andrew): This state transition should not
    // be done here. It should be done when we receive
    // acks for the commit.
    dltMod->dlt_deploy_event(DltCommitOkEvt());
}

// om_del_node_info
// ----------------
//
void
OM_NodeDomainMod::om_del_node_info(const NodeUuid *uuid)
{
    NodeAgent::pointer agent;

    agent = om_node_info(uuid);
    if (agent != NULL) {
        om_deactivate_node(agent->node_index());
    }
    om_update_cluster_map();
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
    const FDS_ProtocolInterface::FDSP_DLT_Type& dlt_info_resp) {
    // Don't do anything here. This stub is just to keep cpp compiler happy
}

void
OM_ControlRespHandler::NotifyDLTUpdateResp(
    FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
    FDS_ProtocolInterface::FDSP_DLT_TypePtr& dlt_info_resp) {
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

// om_update_cluster_map
// ---------------------
//
void
OM_NodeDomainMod::om_update_cluster_map()
{
    NodeList       addNodes, rmNodes;
    OM_Module     *om;
    OM_DLTMod     *dlt;
    ClusterMap    *clus;
    DataPlacement *dp;

    node_mtx.lock();
    addNodes.splice(addNodes.begin(), node_up_pend);
    rmNodes.splice(rmNodes.begin(), node_down_pend);
    node_mtx.unlock();

    om   = OM_Module::om_singleton();
    dp   = static_cast<DataPlacement *>(om->om_dataplace_mod());
    dlt  = static_cast<OM_DLTMod *>(om->om_dlt_mod());
    clus = static_cast<ClusterMap *>(om->om_clusmap_mod());

    std::cout << "Call cluster update map" << std::endl;

    clus->updateMap(addNodes, rmNodes);
    // dlt->dlt_deploy_event(DltCompEvt(dp));
}

}  // namespace fds
