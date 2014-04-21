/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <iostream>
#include <vector>
#include <atomic>
#include <boost/msm/front/state_machine_def.hpp>
#include <boost/msm/front/functor_row.hpp>
#include <fds_timer.h>
#include <OmDeploy.h>
#include <orch-mgr/om-service.h>

namespace fds {

OM_DLTMod                    gl_OMDltMod("OM-DLT");

namespace msm = boost::msm;
namespace mpl = boost::mpl;
namespace msf = msm::front;

/**
 * OM DLT deployment FSM structure.
 */
struct DltDplyFSM : public msm::front::state_machine_def<DltDplyFSM>
{
    template <class Event, class FSM> void on_entry(Event const &, FSM &);
    template <class Event, class FSM> void on_exit(Event const &, FSM &);

    /**
     * Timer task to retry re-compute DLT again if computation was locked
     * and we did not let the request to proceed
     */
    class RetryTimerTask : public FdsTimerTask {
      public:
        explicit RetryTimerTask(FdsTimer &timer)  // NOLINT 
        : FdsTimerTask(timer) {}
        ~RetryTimerTask() {}

        virtual void runTimerTask() override;
    };

    /**
     * OM DLT Deployment states.
     */
    struct DST_Idle : public msm::front::state<>
    {
        DST_Idle() : retryTimer(new FdsTimer()),
                     retryTimerTask(new RetryTimerTask(*retryTimer)) {}

        ~DST_Idle() {
            retryTimer->destroy();
        }

        template <class Evt, class Fsm, class State>
        void operator()(Evt const &, Fsm &, State &) {}

        template <class Event, class FSM> void on_entry(Event const &, FSM &) {}
        template <class Event, class FSM> void on_exit(Event const &, FSM &) {}

        /**
         * timer to retry re-compute DLT
         */
        FdsTimerPtr retryTimer;
        FdsTimerTaskPtr retryTimerTask;
    };
    struct DST_SendDlts : public msm::front::state<>
    {
        typedef mpl::vector<DltCommitOkEvt> deferred_events;

        template <class Evt, class Fsm, class State>
        void operator()(Evt const &, Fsm &, State &) {}

        template <class Event, class FSM> void on_entry(Event const &, FSM &) {}
        template <class Event, class FSM> void on_exit(Event const &, FSM &) {}

        std::atomic_flag lock = ATOMIC_FLAG_INIT;
    };
    struct DST_WaitSync : public msm::front::state<>
    {
        template <class Evt, class Fsm, class State>
        void operator()(Evt const &, Fsm &, State &) {}

        template <class Event, class FSM> void on_entry(Event const &, FSM &) {}
        template <class Event, class FSM> void on_exit(Event const &, FSM &) {}

        NodeUuidSet sm_to_wait;  // set of sms we are waiting to respond
    };
    struct DST_Rebal : public msm::front::state<>
    {
        template <class Evt, class Fsm, class State>
        void operator()(Evt const &, Fsm &, State &) {}

        template <class Event, class FSM> void on_entry(Event const &, FSM &) {}
        template <class Event, class FSM> void on_exit(Event const &, FSM &) {}
    };
    struct DST_Commit : public msm::front::state<>
    {
        /**
         * action when leaving this state sends dlt close ops
         * and close acks may come while we are still inside the
         * action method, so deferring till we leave this state and
         * go to next (DST_Close) state where we will process acks.
         */
        typedef mpl::vector<DltCloseOkEvt> deferred_events;

        DST_Commit() : acks_to_wait(0) {}

        template <class Evt, class Fsm, class State>
        void operator()(Evt const &, Fsm &, State &) {}

        template <class Event, class FSM> void on_entry(Event const &, FSM &) {}
        template <class Event, class FSM> void on_exit(Event const &, FSM &) {}

        fds_uint32_t acks_to_wait;
    };
    struct DST_Close : public msm::front::state<>
    {
        typedef mpl::vector<DltComputeEvt> deferred_events;

        DST_Close() : acks_to_wait(0),
                      tryAgainTimer(new FdsTimer()),
                      tryAgainTimerTask(new RetryTimerTask(*tryAgainTimer)) {}

        ~DST_Close() {
            tryAgainTimer->destroy();
        }

        template <class Evt, class Fsm, class State>
        void operator()(Evt const &, Fsm &, State &) {}

        template <class Event, class FSM> void on_entry(Event const &, FSM &) {}
        template <class Event, class FSM> void on_exit(Event const &, FSM &) {}

        fds_uint32_t acks_to_wait;

        /**
         * timer to try to compute DLT (in case new SMs joined while we were
         * deploying the current DLT)
         */
        FdsTimerPtr tryAgainTimer;
        FdsTimerTaskPtr tryAgainTimerTask;
    };

    /**
     * Define the initial state.
     */
    typedef DST_Idle initial_state;

    /**
     * Transition actions.
     */
    struct DACT_Compute
    {
        template <class Evt, class Fsm, class SrcST, class TgtST>
        void operator()(Evt const &, Fsm &, SrcST &, TgtST &);
    };
    struct DACT_SendDlts
    {
        template <class Evt, class Fsm, class SrcST, class TgtST>
        void operator()(Evt const &, Fsm &, SrcST &, TgtST &);
    };
    struct DACT_Rebalance
    {
        template <class Evt, class Fsm, class SrcST, class TgtST>
        void operator()(Evt const &, Fsm &, SrcST &, TgtST &);
    };
    struct DACT_Commit
    {
        template <class Evt, class Fsm, class SrcST, class TgtST>
        void operator()(Evt const &, Fsm &, SrcST &, TgtST &);
    };
    struct DACT_Close
    {
        template <class Evt, class Fsm, class SrcST, class TgtST>
        void operator()(Evt const &, Fsm &, SrcST &, TgtST &);
    };
    struct DACT_UpdDone
    {
        template <class Evt, class Fsm, class SrcST, class TgtST>
        void operator()(Evt const &, Fsm &, SrcST &, TgtST &);
    };
    /**
     * Guard conditions.
     */
    struct GRD_DltSync
    {
        template <class Evt, class Fsm, class SrcST, class TgtST>
        bool operator()(Evt const &, Fsm &, SrcST &, TgtST &);
    };
    struct GRD_DltCompute
    {
        template <class Evt, class Fsm, class SrcST, class TgtST>
        bool operator()(Evt const &, Fsm &, SrcST &, TgtST &);
    };
    struct GRD_DltRebal
    {
        template <class Evt, class Fsm, class SrcST, class TgtST>
        bool operator()(Evt const &, Fsm &, SrcST &, TgtST &);
    };
    struct GRD_DltCommit
    {
        template <class Evt, class Fsm, class SrcST, class TgtST>
        bool operator()(Evt const &, Fsm &, SrcST &, TgtST &);
    };
    struct GRD_DltClose
    {
        template <class Evt, class Fsm, class SrcST, class TgtST>
        bool operator()(Evt const &, Fsm &, SrcST &, TgtST &);
    };

    /**
     * Transition table for OM DLT deployment.
     */
    struct transition_table : mpl::vector<
    // +------------------+----------------+-------------+---------------+--------------+
    // | Start            | Event          | Next        | Action        | Guard        |
    // +------------------+----------------+-------------+---------------+--------------+
    msf::Row< DST_Idle    , DltComputeEvt  , DST_SendDlts, DACT_Compute , GRD_DltCompute>,
    msf::Row< DST_Idle    , DltLoadedDbEvt , DST_Commit  , DACT_Commit   , msf::none    >,
    // +------------------+----------------+-------------+---------------+--------------+
    msf::Row< DST_SendDlts, msf::none      , DST_WaitSync, DACT_SendDlts , msf::none    >,
    // +------------------+----------------+-------------+---------------+--------------+
    msf::Row< DST_WaitSync, DltCommitOkEvt , DST_Rebal   , DACT_Rebalance, GRD_DltSync  >,
    // +------------------+----------------+-------------+---------------+--------------+
    msf::Row< DST_Rebal   , DltRebalOkEvt  , DST_Commit  , DACT_Commit   , GRD_DltRebal >,
    // +------------------+----------------+-------------+---------------+--------------+
    msf::Row< DST_Commit  , DltCommitOkEvt , DST_Close   , DACT_Close    , GRD_DltCommit>,
    // +------------------+----------------+-------------+---------------+--------------+
    msf::Row< DST_Close   , DltCloseOkEvt  , DST_Idle    , DACT_UpdDone  , GRD_DltClose >
    // +------------------+----------------+-------------+---------------+--------------+
    >{};  // NOLINT

    template <class Event, class FSM> void no_transition(Event const &, FSM &, int);
};

// ------------------------------------------------------------------------------------
// DLT Module Vector
// ------------------------------------------------------------------------------------
OM_DLTMod::OM_DLTMod(char const *const name)
        : Module(name),
          fsm_lock("DLTMod lock")
{
    dlt_dply_fsm = new FSM_DplyDLT();
}

OM_DLTMod::~OM_DLTMod()
{
    delete dlt_dply_fsm;
}

int
OM_DLTMod::mod_init(SysParams const *const param)
{
    Module::mod_init(param);

    return 0;
}

void
OM_DLTMod::mod_startup()
{
    dlt_dply_fsm->start();
}

void
OM_DLTMod::mod_shutdown()
{
}

// dlt_deploy_curr_state
// ---------------------
//
char const *const
OM_DLTMod::dlt_deploy_curr_state()
{
    static char const *const state_names[] = {
        "Idle", "Compute", "Rebalance", "Commit"
    };
    return state_names[dlt_dply_fsm->current_state()[0]];
}

// dlt_deploy_event
// ----------------
//
void
OM_DLTMod::dlt_deploy_event(DltComputeEvt const &evt)
{
    fds_mutex::scoped_lock l(fsm_lock);
    dlt_dply_fsm->process_event(evt);
}

void
OM_DLTMod::dlt_deploy_event(DltRebalOkEvt const &evt)
{
    fds_mutex::scoped_lock l(fsm_lock);
    dlt_dply_fsm->process_event(evt);
}

void
OM_DLTMod::dlt_deploy_event(DltCommitOkEvt const &evt)
{
    fds_mutex::scoped_lock l(fsm_lock);
    dlt_dply_fsm->process_event(evt);
}

void
OM_DLTMod::dlt_deploy_event(DltCloseOkEvt const &evt)
{
    fds_mutex::scoped_lock l(fsm_lock);
    dlt_dply_fsm->process_event(evt);
}

void
OM_DLTMod::dlt_deploy_event(DltLoadedDbEvt const &evt)
{
    fds_mutex::scoped_lock l(fsm_lock);
    dlt_dply_fsm->process_event(evt);
}

// --------------------------------------------------------------------------------------
// OM DLT Deployment FSM Implementation
// --------------------------------------------------------------------------------------
template <class Evt, class Fsm>
void
DltDplyFSM::on_entry(Evt const &evt, Fsm &fsm)
{
    FDS_PLOG_SEV(g_fdslog, fds_log::debug) << "DltDplyFSM on entry";
}

template <class Evt, class Fsm>
void
DltDplyFSM::on_exit(Evt const &evt, Fsm &fsm)
{
    FDS_PLOG_SEV(g_fdslog, fds_log::debug) << "DltDplyFSM on exit";
}

// no_transition
// -------------
//
template <class Evt, class Fsm>
void
DltDplyFSM::no_transition(Evt const &evt, Fsm &fsm, int state)
{
    FDS_PLOG_SEV(g_fdslog, fds_log::debug) << "DltDplyFSM no trans";
}

void DltDplyFSM::RetryTimerTask::runTimerTask()
{
    OM_NodeDomainMod* domain = OM_NodeDomainMod::om_local_domain();
    LOGNOTIFY << "DltDplyFSM: retry to re-compute DLT";
    domain->om_dlt_update_cluster();
}

// GRD_DltCompute
// -------------
// Prevents more than one DltComputeEvt resulting in dlt computation,
// we only allow the first one to go in, and subsequent will be rejected
// until the current DLT computation/rebalance/commit finishes and then
// new/removed SMs that are pending will be taken into account in the next
// DLT update. Since we are not locking the state machine, this is a way
// to prevent races in DLT deployment.
// Also checks if there are any nodes added/removed and returns true if
// there are any and we need a DLT update.
//
template <class Evt, class Fsm, class SrcST, class TgtST>
bool
DltDplyFSM::GRD_DltCompute::operator()(Evt const &evt, Fsm &fsm, SrcST &src, TgtST &dst)
{
    fds_bool_t stop = dst.lock.test_and_set();
    LOGDEBUG << "GRD_DltCompute: proceed check if DLT meeds update? " << !stop;
    if (stop) {
        // since we don't want to lose this event, retry later just in case
        LOGDEBUG << "GRD_DltCompute: DLT re-compute already in progress, "
                 << " will try re-compute for next set of SMs later";
        if (!src.retryTimer->schedule(src.retryTimerTask,
                                      std::chrono::seconds(60))) {
            LOGWARN << "GRD_DltCompute: failed to start retry timer!!!"
                    << " SM additions/deletions may be pending for long time!";
        }
        return false;
    }

    OM_NodeDomainMod* domain = OM_NodeDomainMod::om_local_domain();
    OM_NodeContainer* local = OM_NodeDomainMod::om_loc_domain_ctrl();
    OM_Module *om = OM_Module::om_singleton();
    DataPlacement *dp = om->om_dataplace_mod();
    ClusterMap* cm = om->om_clusmap_mod();
    OM_SmContainer::pointer smNodes = local->om_sm_nodes();

    // Get added and removed nodes from pending SM additions
    // and removals. We are updating the cluster map only in this
    // state, so that it couldn't be changed while in the process
    // of updating the DLT
    NodeList addNodes, rmNodes;
    LOGDEBUG << "DACT_Compute: Call cluster map update";
    smNodes->om_splice_nodes_pend(&addNodes, &rmNodes);
    cm->updateMap(addNodes, rmNodes);
    LOGDEBUG << "Added Nodes size: " << (cm->getAddedNodes()).size()
             << " Removed Nodes size: " << (cm->getRemovedNodes()).size();

    // if there are no nodes added or removed, we don't need to re-compute
    // DLT, so return false to not proceed to compute state
    if (((cm->getAddedNodes()).size() == 0) && ((cm->getRemovedNodes()).size() == 0)) {
        // unlock since we are not updating the DLT
        LOGDEBUG << "GRD_DltCompute: cluster map is up to date";
        dst.lock.clear();
        return false;
    }

    return true;
}

/* DACT_Compute
 * ------------
 * If there are any added or removed nodes -- compute target DLT,
 * For added nodes, send currently commited DLT to them so when we
 * send migration msg with target DLT, they know which tokens to migrate.
 */
template <class Evt, class Fsm, class SrcST, class TgtST>
void
DltDplyFSM::DACT_Compute::operator()(Evt const &evt, Fsm &fsm, SrcST &src, TgtST &dst)
{
    FDS_PLOG_SEV(g_fdslog, fds_log::debug) << "FSM DACT_Compute";
    OM_Module *om = OM_Module::om_singleton();
    DataPlacement *dp = om->om_dataplace_mod();
    ClusterMap* cm = om->om_clusmap_mod();

    fds_verify(((cm->getAddedNodes()).size() != 0)
               || ((cm->getRemovedNodes()).size() != 0));

    LOGDEBUG << "DACT_Compute: compute target DLT";
    dp->computeDlt();
}

/* DACT_SendDlts
 * ------------
 * For added nodes, send currently commited DLT to them so when we
 * send migration msg with target DLT, they know which tokens to migrate.
 */
template <class Evt, class Fsm, class SrcST, class TgtST>
void
DltDplyFSM::DACT_SendDlts::operator()(Evt const &evt, Fsm &fsm, SrcST &src, TgtST &dst)
{
    FDS_PLOG_SEV(g_fdslog, fds_log::debug) << "FSM DACT_SendDlts";
    OM_NodeDomainMod* domain = OM_NodeDomainMod::om_local_domain();
    OM_Module *om = OM_Module::om_singleton();
    DataPlacement *dp = om->om_dataplace_mod();
    ClusterMap* cm = om->om_clusmap_mod();

    fds_verify(((cm->getAddedNodes()).size() != 0)
               || ((cm->getRemovedNodes()).size() != 0));

    // if there are added node, will send them currently commited DLT
    // so when we send them migration message with target DLT, then know
    // which tokens to migrate
    NodeUuidSet addedNodes = cm->getAddedNodes();
    if (addedNodes.size() > 0) {
            // send them commited DLT
        for (NodeUuidSet::const_iterator cit = addedNodes.cbegin();
             cit != addedNodes.cend();
             ++cit) {
            OM_SmAgent::pointer sm_agent = domain->om_sm_agent(*cit);
            Error ret = sm_agent->om_send_dlt(dp->getCommitedDlt());
            if (ret.ok()) {
                dst.sm_to_wait.insert(*cit);
                LOGDEBUG << "DACT_SendDlts: sent commited DLT to SM "
                         << sm_agent->get_node_name() << ":" << std::hex
                         << (*cit).uuid_get_val() << std::dec;
            }
        }
    }

    // ok to unlock here, because we are not in idle state anymore, and
    // other requests to start DLT update are queued till we go to idle state again
    src.lock.clear();
}

// GRD_DltSync
// -------------
// Guards waiting for Dlt update response from SMs that were just added and
// we sent them currently commited DLT (so that they compare target DLT and this
// on when they decide which tokens to migrate)
//
template <class Evt, class Fsm, class SrcST, class TgtST>
bool
DltDplyFSM::GRD_DltSync::operator()(Evt const &evt, Fsm &fsm, SrcST &src, TgtST &dst)
{
    if (src.sm_to_wait.count(evt.sm_uuid) > 0) {
        src.sm_to_wait.erase(evt.sm_uuid);
    }

    // for now assuming dlt update is always a success
    bool bret = (src.sm_to_wait.size() == 0);
    LOGDEBUG << "GRD_DltSync: acks to wait " << src.sm_to_wait.size()
             << " returning " << bret;

    return bret;
}

/* DACT_Rebalance
 * ------------
 * Start rebalance to converge to target DLT
 */
template <class Evt, class Fsm, class SrcST, class TgtST>
void
DltDplyFSM::DACT_Rebalance::operator()(Evt const &evt, Fsm &fsm, SrcST &src, TgtST &dst)
{
    FDS_PLOG_SEV(g_fdslog, fds_log::debug) << "FSM DACT_Rebalance";
    OM_Module *om = OM_Module::om_singleton();
    DataPlacement *dp = om->om_dataplace_mod();

    Error err = dp->beginRebalance();
    fds_verify(err == ERR_OK);
}

// DACT_Commit
// -----------
//
template <class Evt, class Fsm, class SrcST, class TgtST>
void
DltDplyFSM::DACT_Commit::operator()(Evt const &evt, Fsm &fsm, SrcST &src, TgtST &dst)
{
    FDS_PLOG_SEV(g_fdslog, fds_log::debug) << "FSM DACT_Commit";
    OM_Module *om = OM_Module::om_singleton();
    OM_NodeDomainMod *domain = OM_NodeDomainMod::om_local_domain();
    OM_NodeContainer* dom_ctrl = domain->om_loc_domain_ctrl();
    DataPlacement *dp = om->om_dataplace_mod();
    ClusterMap* cm = om->om_clusmap_mod();
    fds_verify((dp != NULL) && (cm != NULL));

    // commit as an 'official' version in the data placement engine
    dp->commitDlt();

    // reset pending nodes in cluster map, since they are already
    // present in the DLT
    cm->resetPendNodes();

    // Send new DLT to each node in the cluster map
    // the new DLT now is committed DLT
    fds_uint32_t count = dom_ctrl->om_bcast_dlt(dp->getCommitedDlt());
    if (count < 1) {
        dst.acks_to_wait = 1;
        fsm.process_event(DltCommitOkEvt(dp->getLatestDltVersion(), NodeUuid()));
    } else {
        dst.acks_to_wait = count;
    }
}

// DACT_Close
// -----------
// Send DLT close message to SM nodes to notify that all nodes
// know about the commited DLT
//
template <class Evt, class Fsm, class SrcST, class TgtST>
void
DltDplyFSM::DACT_Close::operator()(Evt const &evt, Fsm &fsm, SrcST &src, TgtST &dst)
{
    FDS_PLOG_SEV(g_fdslog, fds_log::debug) << "FSM DACT_Close";
    DltCommitOkEvt commitOkEvt = (DltCommitOkEvt)evt;
    OM_NodeDomainMod *domain = OM_NodeDomainMod::om_local_domain();
    OM_NodeContainer* dom_ctrl = domain->om_loc_domain_ctrl();

    // Send DLT close message to SM nodes
    fds_uint32_t close_cnt = dom_ctrl->om_bcast_dlt_close(commitOkEvt.cur_dlt_version);
    // if we are here, there must be at least one SM, so we should
    // not get 'close_cnt' == 0
    fds_verify(close_cnt > 0);
    // wait for all 'dlt close' acks to make sure token sync completes
    // on all SMs before we start updating DLT again
    // TODO(anna) implement a timeout so we don't get stuck if one of
    // those SMs fails or gets removed. In near future, we need to handle
    // SM removal case in the middle of DLT update.
    dst.acks_to_wait = close_cnt;

    // in case we are in domain bring up state, notify domain that current
    // DLT is up (we got quorum number of acks for DLT commit)
    domain->local_domain_event(DltUpEvt());
}

// GRD_DltClose
// -------------
//
template <class Evt, class Fsm, class SrcST, class TgtST>
bool
DltDplyFSM::GRD_DltClose::operator()(Evt const &evt, Fsm &fsm, SrcST &src, TgtST &dst)
{
    // for now assuming close is always a success
    fds_verify(src.acks_to_wait > 0);
    src.acks_to_wait--;

    bool bret = (src.acks_to_wait == 0);
    LOGDEBUG << "FGR_DltClose: acks to wait " << src.acks_to_wait
             << " returning " << bret;

    return bret;
}

// DACT_UpdDone
// -----------
// This DLT update is done. Since we may drop update dlt events while
// doing the current update, try to start DLT update here in case there are
// new/removed SMs since the start of the last DLT update.
//
template <class Evt, class Fsm, class SrcST, class TgtST>
void
DltDplyFSM::DACT_UpdDone::operator()(Evt const &evt, Fsm &fsm, SrcST &src, TgtST &dst)
{
    LOGDEBUG << "DltFSM DACT_UpdDone";
    OM_Module *om = OM_Module::om_singleton();
    ClusterMap* cm = om->om_clusmap_mod();
    fds_verify(cm != NULL);
    LOGNOTIFY << "OM deployed DLT with " << cm->getNumMembers() << " nodes";

    if (!src.tryAgainTimer->schedule(src.tryAgainTimerTask,
                                     std::chrono::seconds(1))) {
        LOGWARN << "DACT_UpdDone: failed to start try againtimer!!!"
                << " SM additions/deletions may be pending for long time!";
    }
}

// GRD_DltRebal
// -------------
//
template <class Evt, class Fsm, class SrcST, class TgtST>
bool
DltDplyFSM::GRD_DltRebal::operator()(Evt const &evt, Fsm &fsm, SrcST &src, TgtST &dst)
{
    OM_NodeDomainMod *domain = OM_NodeDomainMod::om_local_domain();
    DltRebalOkEvt rebalOkEvt = (DltRebalOkEvt)evt;
    DataPlacement *dp = rebalOkEvt.ode_dp;
    fds_verify(dp != NULL);

    // when all added nodes are in 'node up' state,
    // we are getting out of this state
    NodeUuidSet rebalNodes = dp->getRebalanceNodes();
    fds_bool_t all_up = true;
    for (std::unordered_set<NodeUuid, UuidHash>::const_iterator cit = rebalNodes.cbegin();
         cit != rebalNodes.cend();
         ++cit) {
        NodeAgent::pointer agent = domain->om_sm_agent(*cit);
        fds_verify(agent != NULL);
        FDS_PLOG_SEV(g_fdslog, fds_log::debug)
                << "GRD_DltRebal: Node " << agent->get_node_name()
                << " state " << agent->node_state();
        if (agent->node_state() != FDS_ProtocolInterface::FDS_Node_Up) {
            all_up = false;
            break;
        }
    }

    FDS_PLOG_SEV(g_fdslog, fds_log::debug)
            << "FSM GRD_DltRebal: was/is waiting for rebalance ok from "
            << rebalNodes.size() << " node(s), current result: " << all_up;

    return all_up;
}

// GRD_DltCommit
// -------------
//
template <class Evt, class Fsm, class SrcST, class TgtST>
bool
DltDplyFSM::GRD_DltCommit::operator()(Evt const &evt, Fsm &fsm, SrcST &src, TgtST &dst)
{
    fds_verify(src.acks_to_wait > 0);
    src.acks_to_wait--;
    fds_bool_t b_ret = (src.acks_to_wait == 0);

    LOGDEBUG << "GRD_DltCommit: waiting for " << src.acks_to_wait
             << " acks, result: " << b_ret;

    return b_ret;
}

}  // namespace fds
