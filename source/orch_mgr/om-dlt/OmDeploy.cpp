/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <iostream>
#include <vector>
#include <boost/msm/front/state_machine_def.hpp>
#include <boost/msm/front/functor_row.hpp>
#include <OmDeploy.h>

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
     * OM DLT Deployment states.
     */
    struct DST_Idle : public msm::front::state<>
    {
        template <class Evt, class Fsm, class State>
        void operator()(Evt const &, Fsm &, State &) {}

        template <class Event, class FSM> void on_entry(Event const &, FSM &) {}
        template <class Event, class FSM> void on_exit(Event const &, FSM &) {}
    };
    struct DST_Rebal : public msm::front::state<>
    {
        typedef mpl::vector<DltCompRebalEvt> deferred_events;

        template <class Evt, class Fsm, class State>
        void operator()(Evt const &, Fsm &, State &) {}

        template <class Event, class FSM> void on_entry(Event const &, FSM &) {}
        template <class Event, class FSM> void on_exit(Event const &, FSM &) {}
    };
    struct DST_Commit : public msm::front::state<>
    {
        typedef mpl::vector<DltCompRebalEvt> deferred_events;

        DST_Commit() : acks_to_wait(0) {}

        template <class Evt, class Fsm, class State>
        void operator()(Evt const &, Fsm &, State &) {}

        template <class Event, class FSM> void on_entry(Event const &, FSM &) {}
        template <class Event, class FSM> void on_exit(Event const &, FSM &) {}

        fds_uint32_t acks_to_wait;
    };
    struct DST_Close : public msm::front::state<>
    {
        typedef mpl::vector<DltCompRebalEvt> deferred_events;

        DST_Close() : acks_to_wait(0) {}

        template <class Evt, class Fsm, class State>
        void operator()(Evt const &, Fsm &, State &) {}

        template <class Event, class FSM> void on_entry(Event const &, FSM &) {}
        template <class Event, class FSM> void on_exit(Event const &, FSM &) {}

        fds_uint32_t acks_to_wait;
    };

    /**
     * Define the initial state.
     */
    typedef DST_Idle initial_state;

    /**
     * Transition actions.
     */
    struct DACT_CompRebal
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
    /**
     * Guard conditions.
     */
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
    // +-----------------+----------------+------------+----------------+---------------+
    // | Start           | Event          | Next       | Action         | Guard         |
    // +-----------------+----------------+------------+----------------+---------------+
    msf::Row< DST_Idle   , DltCompRebalEvt, DST_Rebal  , DACT_CompRebal , msf::none     >,
    // +-----------------+----------------+------------+----------------+---------------+
    msf::Row< DST_Rebal  , DltRebalOkEvt  , DST_Commit , DACT_Commit    , GRD_DltRebal  >,
    msf::Row< DST_Rebal  , DltNoRebalEvt  , DST_Idle   , msf::none      , msf::none     >,
    // +-----------------+----------------+------------+----------------+---------------+
    msf::Row< DST_Commit , DltCommitOkEvt , DST_Close  , DACT_Close     , GRD_DltCommit >,
    // +-----------------+----------------+------------+----------------+---------------+
    msf::Row< DST_Close  , DltCloseOkEvt  , DST_Idle   , msf::none      , GRD_DltClose  >
    // +-----------------+----------------+------------+----------------+---------------+
    >{};  // NOLINT

    template <class Event, class FSM> void no_transition(Event const &, FSM &, int);
};

// ------------------------------------------------------------------------------------
// DLT Module Vector
// ------------------------------------------------------------------------------------
OM_DLTMod::OM_DLTMod(char const *const name)
    : Module(name)
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
OM_DLTMod::dlt_deploy_event(DltCompRebalEvt const &evt)
{
    dlt_dply_fsm->process_event(evt);
}

void
OM_DLTMod::dlt_deploy_event(DltNoRebalEvt const &evt)
{
    dlt_dply_fsm->process_event(evt);
}

void
OM_DLTMod::dlt_deploy_event(DltRebalOkEvt const &evt)
{
    dlt_dply_fsm->process_event(evt);
}

void
OM_DLTMod::dlt_deploy_event(DltCommitOkEvt const &evt)
{
    dlt_dply_fsm->process_event(evt);
}

void
OM_DLTMod::dlt_deploy_event(DltCloseOkEvt const &evt)
{
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

/* DACT_CompRebal
 * ------------
 * DLT computation + rebalance state. Updates cluster map based on
 * pending added/removed SM nodes. If there are changes
 * to cluster map, computes and stores a new DLT
 * based on the current cluster map.
 */
template <class Evt, class Fsm, class SrcST, class TgtST>
void
DltDplyFSM::DACT_CompRebal::operator()(Evt const &evt, Fsm &fsm, SrcST &src, TgtST &dst)
{
    FDS_PLOG_SEV(g_fdslog, fds_log::debug) << "FSM DACT_CompRebal";
    DltCompRebalEvt dltEvt = (DltCompRebalEvt)evt;
    DataPlacement *dp = dltEvt.ode_dp;
    ClusterMap *cm = dltEvt.ode_cm;
    OM_SmContainer::pointer smNodes = dltEvt.ode_sm_nodes;
    fds_verify(dp != NULL);

    // Get added and removed nodes from pending SM additions
    // and removals. We are updating the cluster map only in this
    // state, so that it couldn't be changed while in the process
    // of updating the DLT
    NodeList addNodes, rmNodes;
    FDS_PLOG_SEV(g_fdslog, fds_log::debug) << "DACT_CompRebal: Call cluster update map";
    smNodes->om_splice_nodes_pend(&addNodes, &rmNodes);
    cm->updateMap(addNodes, rmNodes);
    LOGDEBUG << "Added Nodes size: " << addNodes.size()
            << " rmNodes size: " << rmNodes.size();
    // Recompute the DLT. Once complete, the data placement's
    // current dlt will be updated to the new dlt version.
    if ((addNodes.size() != 0) || (rmNodes.size() != 0)) {
        FDS_PLOG_SEV(g_fdslog, fds_log::debug)
                << "DACT_CompRebal: compute DLT and rebalance";
        dp->computeDlt();
        // start rebalance
        Error err = dp->beginRebalance();
        fds_verify(err == ERR_OK);
    }
}

// DACT_Commit
// -----------
//
template <class Evt, class Fsm, class SrcST, class TgtST>
void
DltDplyFSM::DACT_Commit::operator()(Evt const &evt, Fsm &fsm, SrcST &src, TgtST &dst)
{
    FDS_PLOG_SEV(g_fdslog, fds_log::debug) << "FSM DACT_Commit";
    DltRebalOkEvt rebalOkEvt = (DltRebalOkEvt)evt;
    OM_NodeDomainMod *domain = OM_NodeDomainMod::om_local_domain();
    OM_NodeContainer* dom_ctrl = domain->om_loc_domain_ctrl();
    DataPlacement *dp = rebalOkEvt.ode_dp;
    ClusterMap* cm = rebalOkEvt.ode_clusmap;
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
        fsm.process_event(DltCommitOkEvt(dp->getLatestDltVersion()));
    } else {
        // lets wait for majority of acks
        dst.acks_to_wait = count/2 + 1;
        if (dst.acks_to_wait < 1) {
            dst.acks_to_wait = count;
        }
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
    dst.acks_to_wait = close_cnt / 2 + 1;
    if (dst.acks_to_wait == 0)
        dst.acks_to_wait = close_cnt;
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
