/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <iostream>
#include <vector>
#include <atomic>
#include <boost/msm/front/state_machine_def.hpp>
#include <boost/msm/front/functor_row.hpp>
#include <OmDmtDeploy.h>
#include <orch-mgr/om-service.h>

namespace fds {

OM_DMTMod                    gl_OMDmtMod("OM-DMT");

namespace msm = boost::msm;
namespace mpl = boost::mpl;
namespace msf = msm::front;

/**
 * OM DMT deployment FSM structure.
 */
struct DmtDplyFSM : public msm::front::state_machine_def<DmtDplyFSM>
{
    template <class Event, class FSM> void on_entry(Event const &, FSM &);
    template <class Event, class FSM> void on_exit(Event const &, FSM &);

    /**
     * OM DMT Deployment states.
     */
    struct DST_Idle : public msm::front::state<>
    {
        template <class Evt, class Fsm, class State>
        void operator()(Evt const &, Fsm &, State &) {}

        template <class Event, class FSM> void on_entry(Event const &, FSM &) {}
        template <class Event, class FSM> void on_exit(Event const &, FSM &) {}
    };
    struct DST_SendDmts : public msm::front::state<>
    {
        typedef mpl::vector<DmtComputeEvt> deferred_events;

        template <class Evt, class Fsm, class State>
        void operator()(Evt const &, Fsm &, State &) {}

        template <class Event, class FSM> void on_entry(Event const &, FSM &) {}
        template <class Event, class FSM> void on_exit(Event const &, FSM &) {}

        fds_uint32_t vol_acks_to_wait;
        std::atomic_flag lock = ATOMIC_FLAG_INIT;
    };
    struct DST_Close : public msm::front::state<>
    {
        typedef mpl::vector<DmtComputeEvt> deferred_events;

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
    struct DACT_Compute
    {
        template <class Evt, class Fsm, class SrcST, class TgtST>
        void operator()(Evt const &, Fsm &, SrcST &, TgtST &);
        // volume acks to trigger the DMT update
        // fds_uint32_t vol_acks_to_wait;
    };
    struct DACT_ComputeDb
    {
        template <class Evt, class Fsm, class SrcST, class TgtST>
        void operator()(Evt const &, Fsm &, SrcST &, TgtST &);
    };
    struct DACT_SendDmts
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
    struct GRD_DmtCompute
    {
        template <class Evt, class Fsm, class SrcST, class TgtST>
        bool operator()(Evt const &, Fsm &, SrcST &, TgtST &);
    };
    struct GRD_DmtClose
    {
        template <class Evt, class Fsm, class SrcST, class TgtST>
        bool operator()(Evt const &, Fsm &, SrcST &, TgtST &);
    };

    /**
     * Transition table for OM DMT deployment.
     */
    struct transition_table : mpl::vector<
    // +------------------+----------------+-------------+---------------+--------------+
    // | Start            | Event          | Next        | Action       | Guard        |
    // +------------------+----------------+-------------+---------------+--------------+
    msf::Row< DST_Idle    , DmtComputeEvt  , DST_SendDmts, DACT_Compute , msf::none    >,
    msf::Row< DST_Idle    , DmtLoadedDbEvt , DST_SendDmts, DACT_ComputeDb , msf::none  >,
    // +------------------+----------------+-------------+---------------+--------------+
    msf::Row< DST_SendDmts, DmtVolAckEvt   , DST_Close   , DACT_SendDmts, GRD_DmtCompute>,
    // +------------------+----------------+-------------+---------------+--------------+
    msf::Row< DST_Close   , DmtCloseOkEvt  , DST_Idle    , DACT_UpdDone , GRD_DmtClose >
    // +------------------+----------------+-------------+---------------+--------------+
    >{};  // NOLINT

    template <class Event, class FSM> void no_transition(Event const &, FSM &, int);
};

// ------------------------------------------------------------------------------------
// DMT Module Vector
// ------------------------------------------------------------------------------------
OM_DMTMod::OM_DMTMod(char const *const name)
    : Module(name)
{
    dmt_dply_fsm = new FSM_DplyDMT();
}

OM_DMTMod::~OM_DMTMod()
{
    delete dmt_dply_fsm;
}

int
OM_DMTMod::mod_init(SysParams const *const param)
{
    Module::mod_init(param);

    return 0;
}

void
OM_DMTMod::mod_startup()
{
    dmt_dply_fsm->start();
}

void
OM_DMTMod::mod_shutdown()
{
}

// dmt_deploy_curr_state
// ---------------------
//
char const *const
OM_DMTMod::dmt_deploy_curr_state()
{
    static char const *const state_names[] = {
        "Idle", "Compute", "Commit"
    };
    return state_names[dmt_dply_fsm->current_state()[0]];
}

// dmt_deploy_event
// ----------------
//
void
OM_DMTMod::dmt_deploy_event(DmtComputeEvt const &evt)
{
    dmt_dply_fsm->process_event(evt);
}

void
OM_DMTMod::dmt_deploy_event(DmtCloseOkEvt const &evt)
{
    dmt_dply_fsm->process_event(evt);
}

void
OM_DMTMod::dmt_deploy_event(DmtVolAckEvt const &evt)
{
    dmt_dply_fsm->process_event(evt);
}

void
OM_DMTMod::dmt_deploy_event(DmtLoadedDbEvt const &evt)
{
    dmt_dply_fsm->process_event(evt);
}

// --------------------------------------------------------------------------------------
// OM DMT Deployment FSM Implementation
// --------------------------------------------------------------------------------------
template <class Evt, class Fsm>
void
DmtDplyFSM::on_entry(Evt const &evt, Fsm &fsm)
{
    FDS_PLOG_SEV(g_fdslog, fds_log::debug) << "DmtDplyFSM on entry";
}

template <class Evt, class Fsm>
void
DmtDplyFSM::on_exit(Evt const &evt, Fsm &fsm)
{
    FDS_PLOG_SEV(g_fdslog, fds_log::debug) << "DmtDplyFSM on exit";
}

// no_transition
// -------------
//
template <class Evt, class Fsm>
void
DmtDplyFSM::no_transition(Evt const &evt, Fsm &fsm, int state)
{
    FDS_PLOG_SEV(g_fdslog, fds_log::debug) << "DmtDplyFSM no trans";
}


// GRD_DmtCompute
template <class Evt, class Fsm, class SrcST, class TgtST>
bool
DmtDplyFSM::GRD_DmtCompute::operator()(Evt const &evt, Fsm &fsm, SrcST &src, TgtST &dst)
{
    // fds_verify(src.vol_acks_to_wait > 0);
    LOGDEBUG << "GRD_DmtCompute: Vol acks to wait " << src.vol_acks_to_wait;
    if (src.vol_acks_to_wait )
       src.vol_acks_to_wait--;

    bool bret = (src.vol_acks_to_wait == 0);
    LOGDEBUG << "GRD_DmtCompute: Vol acks to wait " << src.vol_acks_to_wait
             << " returning " << bret;

    return bret;
}

/* DACT_ComputeDb
 * ------------
 */
template <class Evt, class Fsm, class SrcST, class TgtST>
void
DmtDplyFSM::DACT_ComputeDb::operator()(Evt const &evt, Fsm &fsm, SrcST &src, TgtST &dst)
{
    FDS_PLOG_SEV(g_fdslog, fds_log::debug) << "FSM DACT_ComputeDb";
}
/* DACT_Compute
 * ------------
 */
template <class Evt, class Fsm, class SrcST, class TgtST>
void
DmtDplyFSM::DACT_Compute::operator()(Evt const &evt, Fsm &fsm, SrcST &src, TgtST &dst)
{
    LOGDEBUG << "FSM DACT_Compute";
    DmtComputeEvt computeEvt = (DmtComputeEvt)evt;

    dst.vol_acks_to_wait = computeEvt.numVols;
    LOGDEBUG << "FSM DACT_Compute" << dst.vol_acks_to_wait;
}

/* DACT_SendDmts
 * ------------
 * For added nodes, send currently commited DMT to them so when we
 * send migration msg with target DMT, they know which tokens to migrate.
 */
template <class Evt, class Fsm, class SrcST, class TgtST>
void
DmtDplyFSM::DACT_SendDmts::operator()(Evt const &evt, Fsm &fsm, SrcST &src, TgtST &dst)
{
    FDS_PLOG_SEV(g_fdslog, fds_log::debug) << "FSM DACT_SendDmts";
    OM_NodeContainer* loc_domain = OM_NodeDomainMod::om_loc_domain_ctrl();
    loc_domain->om_round_robin_dmt();
    fds_uint32_t count  = loc_domain->om_bcast_dmt_table();
    count = 0;
    if (count < 1) {
        dst.acks_to_wait = 1;
        fsm.process_event(DmtCloseOkEvt());
    } else {
        dst.acks_to_wait = count;
    }
}


// GRD_DmtClose
// -------------
//
template <class Evt, class Fsm, class SrcST, class TgtST>
bool
DmtDplyFSM::GRD_DmtClose::operator()(Evt const &evt, Fsm &fsm, SrcST &src, TgtST &dst)
{
    // for now assuming close is always a success
    fds_verify(src.acks_to_wait > 0);
    src.acks_to_wait--;

    bool bret = (src.acks_to_wait == 0);
    LOGDEBUG << "GRD_DmtClose: acks to wait " << src.acks_to_wait
             << " returning " << bret;

    return bret;
}

// DACT_UpdDone
// -----------
template <class Evt, class Fsm, class SrcST, class TgtST>
void
DmtDplyFSM::DACT_UpdDone::operator()(Evt const &evt, Fsm &fsm, SrcST &src, TgtST &dst)
{
    LOGDEBUG << "DmtFSM DACT_UpdDone";
    OM_NodeDomainMod *domain = OM_NodeDomainMod::om_local_domain();
    //   domain->om_dmt_update_cluster();
}

}  // namespace fds
