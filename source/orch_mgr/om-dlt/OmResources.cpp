/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <boost/msm/front/state_machine_def.hpp>
#include <boost/msm/front/functor_row.hpp>
#include <fds_timer.h>
#include <orch-mgr/om-service.h>
#include <OmDeploy.h>
#include <OmResources.h>
#include <OmDataPlacement.h>
#include <fds_process.h>

#define OM_WAIT_NODES_UP_SECONDS   5*60

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
     * Timer task to come out from waiting state
     */
    class WaitTimerTask : public FdsTimerTask {
      public:
        explicit WaitTimerTask(FdsTimer &timer)  // NOLINT
        : FdsTimerTask(timer) {}
        ~WaitTimerTask() {}

        virtual void runTimerTask() override;
    };

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
        DST_WaitNds() : waitTimer(new FdsTimer()),
                        waitTimerTask(new WaitTimerTask(*waitTimer)) {}

        ~DST_WaitNds() {
            waitTimer->destroy();
        }

        template <class Evt, class Fsm, class State>
        void operator()(Evt const &, Fsm &, State &) {}

        template <class Event, class FSM> void on_entry(Event const &, FSM &) {}
        template <class Event, class FSM> void on_exit(Event const &, FSM &) {}

        NodeUuidSet sm_services;  // services we are waiting to come up
        NodeUuidSet sm_up;  // services that are already up

        /**
         * timer to come out of this state if we don't get all SMs up
         */
        FdsTimerPtr waitTimer;
        FdsTimerTaskPtr waitTimerTask;
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
    struct DACT_WaitNds
    {
        template <class Evt, class Fsm, class SrcST, class TgtST>
        void operator()(Evt const &, Fsm &, SrcST &, TgtST &);
    };
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
    struct DACT_LoadVols
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
    struct GRD_EnoughNds
    {
        template <class Evt, class Fsm, class SrcST, class TgtST>
        bool operator()(Evt const &, Fsm &, SrcST &, TgtST &);
    };

    /**
     * Transition table for OM Node Domain
     */
    struct transition_table : mpl::vector<
        // +-----------------+-------------+-------------+--------------+-------------+
        // | Start           | Event       | Next        | Action       | Guard       |
        // +-----------------+-------------+-------------+--------------+-------------+
        msf::Row< DST_Start  , WaitNdsEvt  , DST_WaitNds , DACT_WaitNds , msf::none   >,
        msf::Row< DST_Start  , NoPersistEvt, DST_DomainUp, msf::none    , msf::none   >,
        // +-----------------+-------------+------------+---------------+--------------+
        msf::Row< DST_WaitNds, RegNodeEvt  , DST_WaitDlt , DACT_NodesUp , GRD_NdsUp   >,
        msf::Row< DST_WaitNds, TimeoutEvt  , DST_WaitDlt , DACT_UpdDlt , GRD_EnoughNds>,
        // +-----------------+-------------+------------+---------------+--------------+
        msf::Row< DST_WaitDlt, DltUpEvt    , DST_DomainUp, DACT_LoadVols, msf::none   >
        // +------------------+-------------+------------+---------------+-------------+
        >{};  // NOLINT

    template <class Event, class FSM> void no_transition(Event const &, FSM &, int);
};

void NodeDomainFSM::WaitTimerTask::runTimerTask()
{
    OM_NodeDomainMod* domain = OM_NodeDomainMod::om_local_domain();
    LOGNOTIFY << "Timeout waiting for SM(s) to come up";
    domain->local_domain_event(TimeoutEvt());
}

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
    fds_bool_t b_ret = false;
    RegNodeEvt regEvt = (RegNodeEvt)evt;

    // check if this is one of the services we are waiting for
    if (src.sm_services.count(regEvt.svc_uuid) > 0) {
        src.sm_up.insert(regEvt.svc_uuid);
        if (src.sm_services.size() == src.sm_up.size()) {
            b_ret = true;
        }
    }
    LOGDEBUG << "GRD_NdsUp: register svc uuid " << std::hex
             << regEvt.svc_uuid.uuid_get_val() << std::dec << "; waiting for "
             << src.sm_services.size() << " Sms, already got "
             << src.sm_up.size() << " SMs; returning " << b_ret;

    return b_ret;
}

/**
 * DACT_NodesUp
 * ------------
 * We got all SMs that we were waiting for up.
 * Commit the DLT we loaded from configDB.
 */
template <class Evt, class Fsm, class SrcST, class TgtST>
void
NodeDomainFSM::DACT_NodesUp::operator()(Evt const &evt, Fsm &fsm, SrcST &src, TgtST &dst)
{
    OM_Module *om = OM_Module::om_singleton();
    OM_NodeContainer* local = OM_NodeDomainMod::om_loc_domain_ctrl();
    OM_DLTMod *dltMod = om->om_dlt_mod();
    DataPlacement *dp = om->om_dataplace_mod();
    ClusterMap *cm = om->om_clusmap_mod();

    LOGDEBUG << "NodeDomainFSM DACT_NodesUp";

    // cancel timeout timer
    src.waitTimer->cancel(src.waitTimerTask);

    // start DLT deploy from the point where we ready to commit

    // cluster map must not have pending nodes, but remember that
    // sm nodes in container may have pending nodes that are already in DLT
    fds_verify((cm->getAddedNodes()).size() == 0);
    fds_verify((cm->getRemovedNodes()).size() == 0);
    dltMod->dlt_deploy_event(DltLoadedDbEvt());

    // remove nodes that are already in DLT from pending list
    // so that we will not try to update DLT with those nodes again
    OM_SmContainer::pointer smNodes = local->om_sm_nodes();
    smNodes->om_rm_nodes_added_pend(src.sm_up);
}

/**
 * DACT_LoadVols
 * -------------
 * We are done with initial bringing up nodes, now load volumes.
 */
template <class Evt, class Fsm, class SrcST, class TgtST>
void
NodeDomainFSM::DACT_LoadVols::operator()(Evt const &evt, Fsm &fsm, SrcST &src, TgtST &dst)
{
    LOGDEBUG << "NodeDomainFSM DACT_LoadVols";
    OM_NodeDomainMod* domain = OM_NodeDomainMod::om_local_domain();
    domain->om_load_volumes();

    // if we are here, there was a time interval when OM was deferring
    // new SMs (that were not persisted) from being added to the cluster
    // (we did not add them to the DLT) -- they are in pending nodes in SM
    // container.
    // Start update cluster process to see if we have such deferred SMs
    domain->om_update_cluster();
}


/**
 * DACT_WaitNds
 * ------------
 * Action to wait for a set of nodes to come up. Just stores the nodes
 * to wait for in dst state. 
 */
template <class Evt, class Fsm, class SrcST, class TgtST>
void
NodeDomainFSM::DACT_WaitNds::operator()(Evt const &evt, Fsm &fsm, SrcST &src, TgtST &dst)
{
    WaitNdsEvt waitEvt = (WaitNdsEvt)evt;
    dst.sm_services.swap(waitEvt.sm_services);

    LOGDEBUG << "NodeDomainFSM DACT_WaitNds: will wait for " << dst.sm_services.size()
             << " SM(s) to come up";

    // start timer to timeout in case services do not come up
    if (!dst.waitTimer->schedule(dst.waitTimerTask,
                                 std::chrono::seconds(OM_WAIT_NODES_UP_SECONDS))) {
        // couldn't start timer, so don't wait for services to come up
        LOGWARN << "DACT_WaitNds: failed to start timer -- will not wait for "
                << "services to come up";
        fsm.process_event(TimeoutEvt());
    }
}

/**
 * GRD_EnoughNds
 * ------------
 * Checks if we have enough nodes up to proceed (updating DLT and bringing
 * up rest of OM).
 */
template <class Evt, class Fsm, class SrcST, class TgtST>
bool
NodeDomainFSM::GRD_EnoughNds::operator()(Evt const &evt, Fsm &fsm, SrcST &src, TgtST &dst)
{
    fds_bool_t b_ret = false;

    // TODO(anna) proceed if more than half nodes are up
    // for now were are in waiting state until nodes are up
    // so only way out is for user register those nodes or
    // kill OM, clean persistent state and restart OM

    // first print out the nodes were are waiting for
    LOGWARN << "WARNING: OM IS NOT UP: "
            << "OM is waiting for the nodes to register!!!:";
    std::cout << std::endl << std::endl << "WARNING: OM IS NOT UP: OM is "
              << "waiting for the nodes to register: " << std::endl;
    for (NodeUuidSet::const_iterator cit = src.sm_services.cbegin();
         cit != src.sm_services.cend();
         ++cit) {
        if (src.sm_up.count(*cit) == 0) {
            std::cout << "   Node " << std::hex << (*cit).uuid_get_val()
                      << std::dec << std::endl;
            LOGWARN << "   Node " << std::hex << (*cit).uuid_get_val()
                    << std::dec;
        }
    }

    // restart timeout timer -- use 20x interval than initial
    if (!src.waitTimer->schedule(src.waitTimerTask,
                                 std::chrono::seconds(20*OM_WAIT_NODES_UP_SECONDS))) {
        // we are not going to pring warning messages anymore, will be in
        // wait state until all nodes are up
        LOGWARN << "GRD_EnoughNds: Failed to start timer -- bring up ALL nodes or "
                << "clean persistent state and restart OM";
    }

    return b_ret;
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
    OM_Module *om = OM_Module::om_singleton();
    OM_NodeDomainMod *domain = OM_NodeDomainMod::om_local_domain();
    OM_NodeContainer *dom_ctrl = domain->om_loc_domain_ctrl();
    DataPlacement *dp = om->om_dataplace_mod();
    ClusterMap *cm = om->om_clusmap_mod();

    LOGDEBUG << "NodeDomainFSM DACT_UpdDlt";

    // commit the DLT we got from config DB
    dp->commitDlt();

    // broadcast DLT to SMs only (so they know the diff when we update the DLT)
    dom_ctrl->om_bcast_dlt(dp->getCommitedDlt(), true);

    // remove nodes that are already in DLT from pending nodes in SM container
    OM_SmContainer::pointer smNodes = dom_ctrl->om_sm_nodes();
    smNodes->om_rm_nodes_added_pend(src.sm_up);

    // at this point there should be no pending add/rm nodes in cluster map
    fds_verify((cm->getAddedNodes()).size() == 0);
    fds_verify((cm->getRemovedNodes()).size() == 0);

    // set SMs that did not come up as 'delete pending' in cluster map
    // -- this will tell DP to remove those nodes from DLT
    for (NodeUuidSet::const_iterator cit = src.sm_services.cbegin();
         cit != src.sm_services.cend();
         ++cit) {
        if (src.sm_up.count(*cit) == 0) {
            cm->addPendingRmNode(*cit);
        }
    }

    // start cluster update process that will recompute DLT /rebalance /etc
    // so that we move to DLT that reflects actual nodes that came up
    domain->om_update_cluster();
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
void OM_NodeDomainMod::local_domain_event(RegNodeEvt const &evt) {
    domain_fsm->process_event(evt);
}
void OM_NodeDomainMod::local_domain_event(TimeoutEvt const &evt) {
    domain_fsm->process_event(evt);
}
void OM_NodeDomainMod::local_domain_event(NoPersistEvt const &evt) {
    domain_fsm->process_event(evt);
}

// om_load_state
// ------------------
//
Error
OM_NodeDomainMod::om_load_state(kvstore::ConfigDB* _configDB)
{
    Error err(ERR_OK);
    OM_Module *om = OM_Module::om_singleton();
    DataPlacement *dp = om->om_dataplace_mod();

    LOGNOTIFY << "Loading persistent state to local domain";
    configDB = _configDB;  // cache config db in local domain

    // get SMs that were up before and persistent in config db
    // if no SMs were up, even if platforms were running, we
    // don't need to wait for the platforms/DMs/AMs to come up
    // since we cannot admit any volumes anyway. So, we will go
    // directly to 'domain up' state if no SMs were running...
    NodeUuidSet sm_services;
    if (configDB) {
        NodeUuidSet nodes;  // actual nodes (platform)
        configDB->getNodeIds(nodes);

        // get set of SMs that were running on those nodes
        NodeUuidSet::const_iterator cit;
        for (cit = nodes.cbegin(); cit != nodes.cend(); ++cit) {
            NodeServices services;
            if (configDB->getNodeServices(*cit, services)) {
                if (services.sm.uuid_get_val() != 0) {
                    sm_services.insert(services.sm);
                    LOGDEBUG << "om_load_state: found SM on node "
                             << std::hex << (*cit).uuid_get_val() << " (SM "
                             << services.sm.uuid_get_val() << std::dec << ")";
                }
            }
        }

        // load DLT (and save as not commited) from config DB and
        // check if DLT matches the set of persisted nodes
        err = dp->loadDltsFromConfigDB(sm_services);
        if (!err.ok()) {
            LOGERROR << "Persistent state mismatch, error " << err.GetErrstr()
                     << std::endl << "OM will stay DOWN! Kill OM, cleanup "
                     << "persistent state and restart cluster again";
            return err;
        }
    }

    if (sm_services.size() > 0) {
        LOGNOTIFY << "Will wait for " << sm_services.size()
                  << " SMs to come up within next few minutes";
        local_domain_event(WaitNdsEvt(sm_services));
    } else {
        LOGNOTIFY << "We didn't persist any SMs or we couldn't load "
                  << "persistent state, so OM will come up in a moment.";
        local_domain_event(NoPersistEvt());
    }

    return err;
}

Error
OM_NodeDomainMod::om_load_volumes()
{
    Error err(ERR_OK);

    // load volumes for this domain
    // so far we are assuming this is domain with id 0
    int my_domainId = 0;

    std::vector<VolumeDesc> vecVolumes;
    std::vector<VolumeDesc>::const_iterator volumeIter;
    configDB->getVolumes(vecVolumes, my_domainId);
    if (vecVolumes.empty()) {
        LOGWARN << "no volumes found for domain "
                << "[" << my_domainId << "]";
    } else {
        LOGNORMAL << vecVolumes.size() << " volumes found for domain "
                  << "[" << my_domainId << "]";
    }

    // add each volume we found in configDB
    for (volumeIter = vecVolumes.begin();
         volumeIter != vecVolumes.end();
         ++volumeIter) {
        LOGDEBUG << "processing volume "
                 << "[" << volumeIter->volUUID << ":" << volumeIter->name << "]";

        if (!om_locDomain->addVolume(*volumeIter)) {
            LOGERROR << "unable to add volume "
                     << "[" << volumeIter->volUUID << ":" << volumeIter->name << "]";
        }
    }
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
                LOGERROR << "Can not find platform agent for node uuid "
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

        if (om_local_domain_up()) {
            om_update_cluster();
        } else {
            local_domain_event(RegNodeEvt(uuid, msg->node_type));
        }
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
    if (om_local_domain_up()) {
        om_update_cluster();
    }
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
