/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <iostream>
#include <string>
#include <vector>
#include <boost/msm/front/state_machine_def.hpp>
#include <boost/msm/front/functor_row.hpp>
#include <orch-mgr/om-service.h>
#include <OmDeploy.h>
#include <OmResources.h>
#include <OmDataPlacement.h>
#include <fds_process.h>

namespace fds {

OM_NodeDomainMod             gl_OMNodeDomainMod("OM-Node");

//---------------------------------------------------------
// Node Domain state machine
//--------------------------------------------------------
namespace msm = boost::msm;
namespace mpl = boost::mpl;
namespace msf = msm::front;

/**
 * Flags -- allow info about a property of the current state
 */
struct LocalDomainUp {};

/**
 * OM Node Domain state machine structure
 */
struct NodeDomainFSM: public msm::front::state_machine_def<NodeDomainFSM>
{
    template <class Event, class FSM> void on_entry(Event const &, FSM &);
    template <class Event, class FSM> void on_exit(Event const &, FSM &);

    /**
     * Node Domain states
     */
    struct DST_Start : public msm::front::state<>
    {
        template <class Evt, class Fsm, class State>
        void operator()(Evt const &, Fsm &, State &) {}

        template <class Event, class FSM> void on_entry(Event const &, FSM &) {}
        template <class Event, class FSM> void on_exit(Event const &, FSM &) {}
    };
    struct DST_WaitNds : public msm::front::state<>
    {
        template <class Evt, class Fsm, class State>
        void operator()(Evt const &, Fsm &, State &) {}

        template <class Event, class FSM> void on_entry(Event const &, FSM &) {}
        template <class Event, class FSM> void on_exit(Event const &, FSM &) {}
    };
    struct DST_WaitDlt : public msm::front::state<>
    {
        template <class Evt, class Fsm, class State>
        void operator()(Evt const &, Fsm &, State &) {}

        template <class Event, class FSM> void on_entry(Event const &, FSM &) {}
        template <class Event, class FSM> void on_exit(Event const &, FSM &) {}
    };
    struct DST_DomainUp : public msm::front::state<>
    {
        typedef mpl::vector1<LocalDomainUp> flag_list;

        template <class Evt, class Fsm, class State>
        void operator()(Evt const &, Fsm &, State &) {}

        template <class Event, class FSM> void on_entry(Event const &, FSM &) {}
        template <class Event, class FSM> void on_exit(Event const &, FSM &) {}
    };

    /**
     * Define the initial state.
     */
    typedef DST_Start initial_state;

    /**
     * Transition actions.
     */
    struct DACT_NodesUp
    {
        template <class Evt, class Fsm, class SrcST, class TgtST>
        void operator()(Evt const &, Fsm &, SrcST &, TgtST &);
    };
    struct DACT_UpdDlt
    {
        template <class Evt, class Fsm, class SrcST, class TgtST>
        void operator()(Evt const &, Fsm &, SrcST &, TgtST &);
    };
    struct DACT_DomainUp
    {
        template <class Evt, class Fsm, class SrcST, class TgtST>
        void operator()(Evt const &, Fsm &, SrcST &, TgtST &);
    };

    /**
     * Guard conditions.
     */
    struct GRD_NdsUp
    {
        template <class Evt, class Fsm, class SrcST, class TgtST>
        bool operator()(Evt const &, Fsm &, SrcST &, TgtST &);
    };

    /**                                                                                                                         
     * Transition table for OM Node Domain
     */
    struct transition_table : mpl::vector<
        // +-------------------+-------------+-------------+--------------+------------+
        // | Start             | Event       | Next        | Action       | Guard      |
        // +-------------------+-------------+-------------+--------------+------------+
        msf::Row< DST_Start    , WaitNdsEvt  , DST_WaitNds , msf::none    , msf::none >,
        msf::Row< DST_Start    , DltUpEvt    , DST_DomainUp, DACT_NodesUp , msf::none >,
        // +-------------------+-------------+------------+---------------+------------+
        msf::Row< DST_WaitNds  , RegNdDoneEvt, DST_DomainUp, DACT_NodesUp , GRD_NdsUp >,
        msf::Row< DST_WaitNds  , TimeoutEvt  , DST_WaitDlt , DACT_UpdDlt  , msf::none >,
        // +-------------------+-------------+------------+---------------+------------+
        msf::Row< DST_WaitDlt  , DltUpEvt    , DST_DomainUp, DACT_NodesUp , msf::none >
        // +-------------------+-------------+------------+---------------+-------------+
        >{};  // NOLINT

    template <class Event, class FSM> void no_transition(Event const &, FSM &, int);
};

template <class Event, class Fsm>
void NodeDomainFSM::on_entry(Event const &evt, Fsm &fsm)
{
    LOGDEBUG << "NodeDomainFSM on entry";
}

template <class Event, class Fsm>
void NodeDomainFSM::on_exit(Event const &evt, Fsm &fsm)
{
    LOGDEBUG << "NodeDomainFSM on exit";
}

template <class Event, class Fsm>
void NodeDomainFSM::no_transition(Event const &evt, Fsm &fsm, int state)
{
    LOGDEBUG << "NodeDomainFSM no transition";
}

/**
 * GRD_NdsUp
 * ------------
 * Checks if all nodes that were up previously (known from config db) are again up
 */
template <class Evt, class Fsm, class SrcST, class TgtST>
bool
NodeDomainFSM::GRD_NdsUp::operator()(Evt const &evt, Fsm &fsm, SrcST &src, TgtST &dst)
{
    return true;
}

/**
 * DACT_NodesUp
 * ------------
 * Handle all nodes up. This is either: we previously did not have any nodes up (configdb
 * does not contain any nodes); all nodes that were previously up are up again; or
 * some (or none) of nodes that were previously up are up again but we already updated the
 * DLT accordingly. This method tries to bring all volumes from config db up.
 */
template <class Evt, class Fsm, class SrcST, class TgtST>
void
NodeDomainFSM::DACT_NodesUp::operator()(Evt const &evt, Fsm &fsm, SrcST &src, TgtST &dst)
{
    LOGDEBUG << "NodeDomainFSM DACT_NodesUp";
}

/**
 * DACT_UpdDlt
 * ------------
 * Handle the case when only some or none of nodes that were previously up (known from config db)
 * are up again, and we are not going to wait for the remaining nodes any longer. Will
 * recompute the DLT for the current nodes (as if nodes that did not come up were removed,
 * in order to minimize the amount of token migrations) and start migration.
 */
template <class Evt, class Fsm, class SrcST, class TgtST>
void
NodeDomainFSM::DACT_UpdDlt::operator()(Evt const &evt, Fsm &fsm, SrcST &src, TgtST &dst)
{
    LOGDEBUG << "NodeDomainFSM DACT_UpdDlt";
}



//--------------------------------------------------------------------
// OM Node Domain
//--------------------------------------------------------------------
OM_NodeDomainMod::OM_NodeDomainMod(char const *const name) : Module(name)
{
    om_locDomain = new OM_NodeContainer();
    domain_fsm = new FSM_NodeDomain();
}

OM_NodeDomainMod::~OM_NodeDomainMod()
{
    delete om_locDomain;
    delete domain_fsm;
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
    domain_fsm->start();
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

// om_local_domain_up
// ------------------
//
fds_bool_t
OM_NodeDomainMod::om_local_domain_up()
{
    return om_local_domain()->domain_fsm->is_flag_active<LocalDomainUp>();
}

// domain_event
// ------------
//
void OM_NodeDomainMod::local_domain_event(WaitNdsEvt const &evt) {
    domain_fsm->process_event(evt);
}
void OM_NodeDomainMod::local_domain_event(DltUpEvt const &evt) {
    domain_fsm->process_event(evt);
}
void OM_NodeDomainMod::local_domain_event(RegNdDoneEvt const &evt) {
    domain_fsm->process_event(evt);
}
void OM_NodeDomainMod::local_domain_event(TimeoutEvt const &evt) {
    domain_fsm->process_event(evt);
}

// om_load_state
// ------------------
//
Error
OM_NodeDomainMod::om_load_state(kvstore::ConfigDB* configDB)
{
    Error err(ERR_OK);
    LOGNOTIFY << "Loading persistent state to local domain";

    // TODO(anna) actually load from configDB, for now going to
    // domain up state
    local_domain_event(DltUpEvt());

    // TODO(anna) move to after we get nodes and DLT matched
    // in any case, below code does not work anyway (vols not
    // accepted by admission control
    /*
    std::vector<VolumeDesc> vecVolumes;
    std::vector<VolumeDesc>::const_iterator volumeIter;
    std::map<int, std::string>::const_iterator domainIter;
    for (domainIter = mapDomains.begin() ; domainIter != mapDomains.end() ; ++domainIter) { // NOLINT                                                      
        int domainId = domainIter->first;
        LOGNORMAL << "loading data for domain "
                  << "[" << domainId << ":" << domainIter->second << "]";

        OM_NodeContainer *domainCtrl = OM_NodeDomainMod::om_loc_domain_ctrl();

        vecVolumes.clear();
        configDB->getVolumes(vecVolumes, domainId);

        if (vecVolumes.empty()) {
            LOGWARN << "no volumes found for domain "
                    << "[" << domainId << ":" << domainIter->second << "]";
        } else {
            LOGNORMAL << vecVolumes.size() << " volumes found for domain "
                      << "[" << domainId << ":" << domainIter->second << "]";
        }

        for (volumeIter = vecVolumes.begin() ; volumeIter != vecVolumes.end() ; ++volumeIter) { //NOLINT                                                   
            LOGDEBUG << "processing volume "
                     << "[" << volumeIter->volUUID << ":" << volumeIter->name << "]";

            if (!domainCtrl->addVolume(*volumeIter)) {
                LOGERROR << "unable to add volume "
                         << "[" << volumeIter->volUUID << ":" << volumeIter->name << "]";
            }
        }
    }
    */

    // load the dlts
    /*
      if (!dp->loadDltsFromConfigDB()) {                                                                                                                        LOGWARN << "errors during loading dlts";
      }
    */

    return err;
}

// om_reg_node_info
// ----------------
//
Error
OM_NodeDomainMod::om_reg_node_info(const NodeUuid&      uuid,
                                   const FdspNodeRegPtr msg)
{
    NodeAgent::pointer      newNode;
    OM_PmContainer::pointer pmNodes;

    // TODO(anna) the below code would not work yet, because
    // register node message from SM/DM does not contain node
    // (platform) uuid yet.
    //
    pmNodes = om_locDomain->om_pm_nodes();
    fds_assert(pmNodes != NULL);

    if ((msg->node_type == fpi::FDSP_STOR_MGR) ||
        (msg->node_type == fpi::FDSP_DATA_MGR)) {
        // we must have a node (platform) that runs any service
        // registered with OM and node must be in active state
        if (!pmNodes->check_new_service((msg->node_uuid).uuid, msg->node_type)) {
            FDS_PLOG_SEV(g_fdslog, fds_log::error)
                    << "Error: cannot register service " << msg->node_name
                    << " on node with uuid " << std::hex << (msg->node_uuid).uuid
                    << std::dec << "; Check if Platform daemon is running";
            return Error(ERR_NODE_NOT_ACTIVE);
        }
    }

    Error err = om_locDomain->dc_register_node(uuid, msg, &newNode);
    if (err.ok() && (msg->node_type != fpi::FDSP_PLATFORM)) {
        fds_verify(newNode != NULL);

        // tell parent PM Agent about its new service
        newNode->set_node_state(fpi::FDS_Node_Up);

        if ((msg->node_uuid).uuid != 0) {
            err = pmNodes->handle_register_service((msg->node_uuid).uuid,
                                                   msg->node_type,
                                                   newNode);

            /* XXX: TODO: (bao) ignore err ERR_NODE_NOT_ACTIVE for now, for checker */
            if (err == ERR_NODE_NOT_ACTIVE) {
                err = ERR_OK;
            }
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
        if (msg->node_type == fpi::FDSP_STOR_MGR) {
            // Activate and account node capacity only when SM registers with OM.
            //
            NodeAgent::pointer pm = pmNodes->agent_info(NodeUuid(msg->node_uuid.uuid));
            if (pm != NULL) {
                om_locDomain->om_add_capacity(pm);
            } else {
                FDS_PLOG(g_fdslog) << "Can not find platform agant for node uuid "
                    << std::hex << msg->node_uuid.uuid << std::dec;
            }
        }
        om_locDomain->om_update_node_list(newNode, msg);
        om_locDomain->om_bcast_vol_list(newNode);

        // Let this new node know about existing dlt (if this is an SM, we are
        // sending commited DLT first and then new DLT to start migration)
        // TODO(Andrew): this should change into dissemination of the cur cluster map.
        OM_Module *om = OM_Module::om_singleton();
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

    return err;
}

//
// Called when OM receives response for DLT commit from a node
//
Error
OM_NodeDomainMod::om_recv_dlt_commit_resp(FdspNodeType node_type,
                                          const NodeUuid& uuid,
                                          fds_uint64_t dlt_version) {
    Error err(ERR_OK);
    OM_Module *om = OM_Module::om_singleton();
    OM_DLTMod *dltMod = om->om_dlt_mod();
    DataPlacement *dp = om->om_dataplace_mod();
    OM_NodeContainer *local = om_loc_domain_ctrl();
    AgentContainer::pointer agent_container = local->dc_container_frm_msg(node_type);
    NodeAgent::pointer agent = agent_container->agent_info(uuid);
    if (agent == NULL) {
        FDS_PLOG_SEV(g_fdslog, fds_log::error)
                << "OM: Received DLT commit ack from unknown node: uuid "
                << std::hex << uuid.uuid_get_val() << std::dec;
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
    dltMod->dlt_deploy_event(DltCommitOkEvt(cur_dlt_ver));

    return err;
}

//
// Called when OM receives response for DLT commit from a node
//
Error
OM_NodeDomainMod::om_recv_dlt_close_resp(const NodeUuid& uuid,
                                         fds_uint64_t dlt_version) {
    Error err(ERR_OK);
    OM_Module *om = OM_Module::om_singleton();
    OM_DLTMod *dltMod = om->om_dlt_mod();
    DataPlacement *dp = om->om_dataplace_mod();

    // we should only count acks for the current dlt version
    // TODO(anna) properly handle version mismatch
    // for now ignoring acks for old dlt version
    fds_uint64_t cur_dlt_ver = dp->getLatestDltVersion();
    if (cur_dlt_ver > dlt_version) {
        return err;
    }
    fds_verify(cur_dlt_ver == dlt_version);

    // tell state machine that we received ack for close
    dltMod->dlt_deploy_event(DltCloseOkEvt());

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
    LOGNOTIFY << "OM received response for NotifyAddVol from node "
              << fdsp_msg->src_node_name << " for volume "
              << "[" << not_add_vol_resp->vol_name << ":"
              << std::hex << not_add_vol_resp->vol_desc.volUUID << std::dec
              << "] Result: " << fdsp_msg->err_code;

    OM_NodeContainer *local = OM_NodeDomainMod::om_loc_domain_ctrl();
    VolumeContainer::pointer volumes = local->om_vol_mgr();
    volumes->om_notify_vol_resp(om_notify_vol_add,
                                fdsp_msg,
                                not_add_vol_resp->vol_name,
                                not_add_vol_resp->vol_desc.volUUID);
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
    LOGNOTIFY << "OM received response for NotifyRmVol (check only "
              << not_rm_vol_resp->check_only << ") from node "
              << fdsp_msg->src_node_name << " for volume "
              << "[" << not_rm_vol_resp->vol_name << ":"
              << std::hex << not_rm_vol_resp->vol_desc.volUUID << std::dec
              << "] Result: " << fdsp_msg->err_code;

    OM_NodeContainer *local = OM_NodeDomainMod::om_loc_domain_ctrl();
    VolumeContainer::pointer volumes = local->om_vol_mgr();
    om_vol_notify_t type = om_notify_vol_rm;
    if (not_rm_vol_resp->check_only)
        type = om_notify_vol_rm_chk;
    volumes->om_notify_vol_resp(type,
                                fdsp_msg,
                                not_rm_vol_resp->vol_name,
                                not_rm_vol_resp->vol_desc.volUUID);
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
    LOGNOTIFY << "OM received response for NotifyModVol from node "
              << fdsp_msg->src_node_name << " for volume "
              << "[" << not_mod_vol_resp->vol_name << ":"
              << std::hex << not_mod_vol_resp->vol_desc.volUUID << std::dec
              << "] Result: " << fdsp_msg->err_code;

    OM_NodeContainer *local = OM_NodeDomainMod::om_loc_domain_ctrl();
    VolumeContainer::pointer volumes = local->om_vol_mgr();
    volumes->om_notify_vol_resp(om_notify_vol_mod,
                                fdsp_msg,
                                not_mod_vol_resp->vol_name,
                                not_mod_vol_resp->vol_desc.volUUID);
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
    LOGNOTIFY << "OM received response for AttachVol from node "
              << fdsp_msg->src_node_name << " for volume "
              << "[" << atc_vol_resp->vol_name << ":"
              << std::hex << atc_vol_resp->vol_desc.volUUID << std::dec
              << "] Result: " << fdsp_msg->err_code;

    OM_NodeContainer *local = OM_NodeDomainMod::om_loc_domain_ctrl();
    VolumeContainer::pointer volumes = local->om_vol_mgr();
    volumes->om_notify_vol_resp(om_notify_vol_attach,
                                fdsp_msg,
                                atc_vol_resp->vol_name,
                                atc_vol_resp->vol_desc.volUUID);
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
    LOGNOTIFY << "OM received response for DetachVol from node "
              << fdsp_msg->src_node_name << " for volume "
              << "[" << dtc_vol_resp->vol_name << ":"
              << std::hex << dtc_vol_resp->vol_desc.volUUID << std::dec
              << "] Result: " << fdsp_msg->err_code;

    OM_NodeContainer *local = OM_NodeDomainMod::om_loc_domain_ctrl();
    VolumeContainer::pointer volumes = local->om_vol_mgr();
    volumes->om_notify_vol_resp(om_notify_vol_detach,
                                fdsp_msg,
                                dtc_vol_resp->vol_name,
                                dtc_vol_resp->vol_desc.volUUID);
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

    // notify DLT state machine
    OM_NodeDomainMod* domain = OM_NodeDomainMod::om_local_domain();
    NodeUuid node_uuid((fdsp_msg->src_service_uuid).uuid);
    domain->om_recv_dlt_commit_resp(fdsp_msg->src_id, node_uuid, dlt_resp->DLT_version);
}

void
OM_ControlRespHandler::NotifyDLTCloseResp(
    const FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
    const FDS_ProtocolInterface::FDSP_DLT_Resp_Type& dlt_resp) {
    // Don't do anything here. This stub is just to keep cpp compiler happy
}

void
OM_ControlRespHandler::NotifyDLTCloseResp(
    FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
    FDS_ProtocolInterface::FDSP_DLT_Resp_TypePtr& dlt_resp) {
    FDS_PLOG_SEV(g_fdslog, fds_log::notification)
            << "OM received response for NotifyDltClose from node "
            << fdsp_msg->src_node_name << ":"
            << std::hex << fdsp_msg->src_service_uuid.uuid << std::dec
            << " for DLT version " << dlt_resp->DLT_version;

    // notify DLT state machine
    OM_NodeDomainMod* domain = OM_NodeDomainMod::om_local_domain();
    NodeUuid node_uuid((fdsp_msg->src_service_uuid).uuid);
    domain->om_recv_dlt_close_resp(node_uuid, dlt_resp->DLT_version);
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
