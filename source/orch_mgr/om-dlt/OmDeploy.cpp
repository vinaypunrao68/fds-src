/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <vector>
#include <boost/msm/front/state_machine_def.hpp>
#include <boost/msm/front/functor_row.hpp>
#include <OmDeploy.h>

namespace fds {

OM_DLTMod                    gl_OMDltMod("OM-DLT");

namespace msm = boost::msm;
namespace mpl = boost::mpl;
using namespace msm::front;  // NOLINT

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
        template <class Event, class FSM> void on_entry(Event const &, FSM &);
        template <class Event, class FSM> void on_exit(Event const &, FSM &);
    };
    struct DST_Comp : public msm::front::state<>
    {
        template <class Event, class FSM> void on_entry(Event const &, FSM &);
        template <class Event, class FSM> void on_exit(Event const &, FSM &);
    };
    struct DST_Update : public msm::front::state<>
    {
        template <class Event, class FSM> void on_entry(Event const &, FSM &);
        template <class Event, class FSM> void on_exit(Event const &, FSM &);
    };
    struct DST_Commit : public msm::front::state<>
    {
        template <class Event, class FSM> void on_entry(Event const &, FSM &);
        template <class Event, class FSM> void on_exit(Event const &, FSM &);
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
    struct DACT_Update
    {
        template <class Evt, class Fsm, class SrcST, class TgtST>
        void operator()(Evt const &, Fsm &, SrcST &, TgtST &);
    };
    struct DACT_Commit
    {
        template <class Evt, class Fsm, class SrcST, class TgtST>
        void operator()(Evt const &, Fsm &, SrcST &, TgtST &);
    };

    /**
     * Guard conditions.
     */
    struct GRD_DltUpdate
    {
        template <class Evt, class Fsm, class SrcST, class TgtST>
        bool operator()(Evt const &, Fsm &, SrcST &, TgtST &);
    };
    struct GRD_DltCommit
    {
        template <class Evt, class Fsm, class SrcST, class TgtST>
        bool operator()(Evt const &, Fsm &, SrcST &, TgtST &);
    };

    /**
     * Transition table for OM DLT deployment.
     */
    struct transition_table : mpl::vector<
    // +------------+----------------+------------+-----------------+-----------------+
    // | Start      | Event          | Next       | Action          | Guard           |
    // +------------+----------------+------------+-----------------+-----------------+
    Row< DST_Idle   , DltCompEvt     , DST_Comp   , DACT_Compute    , none            >,
    // +------------+----------------+------------+-----------------+-----------------+
    Row< DST_Comp   , DltUpdateEvt   , DST_Update , DACT_Update     , none            >,
    // +------------+----------------+------------+-----------------+-----------------+
    Row< DST_Update , DltUpdateOkEvt , DST_Update , none            , GRD_DltUpdate   >,
    Row< DST_Update , DltCommitEvt   , DST_Commit , DACT_Commit     , none            >,
    // +------------+----------------+------------+-----------------+-----------------+
    Row< DST_Commit , DltCommitOkEvt , DST_Idle   , none            , GRD_DltCommit   >
    // +------------+----------------+------------+-----------------+-----------------+
    >{};  // NOLINT
    template <class Event, class FSM> void no_transition(Event const &, FSM &, int);
};

// typedef msm::back::state_machine<DltDplyFSM> FSM_DplyDLT;

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
}

void
OM_DLTMod::mod_shutdown()
{
}

// --------------------------------------------------------------------------------------
// OM DLT Deployment FSM Implementation
// --------------------------------------------------------------------------------------
template <class Evt, class Fsm>
void
DltDplyFSM::on_entry(Evt const &evt, Fsm &fsm)
{
}

template <class Evt, class Fsm>
void
DltDplyFSM::on_exit(Evt const &evt, Fsm &fsm)
{
}

// no_transition
// -------------
//
template <class Evt, class Fsm>
void
DltDplyFSM::no_transition(Evt const &evt, Fsm &fsm, int state)
{
}

// DACT_Compute
// ------------
//
template <class Evt, class Fsm, class SrcST, class TgtST>
void
DltDplyFSM::DACT_Compute::operator()(Evt const &evt, Fsm &fsm, SrcST &src, TgtST &dst)
{
}

// DACT_Update
// -----------
//
template <class Evt, class Fsm, class SrcST, class TgtST>
void
DltDplyFSM::DACT_Update::operator()(Evt const &evt, Fsm &fsm, SrcST &src, TgtST &dst)
{
}

// DACT_Commit
// -----------
//
template <class Evt, class Fsm, class SrcST, class TgtST>
void
DltDplyFSM::DACT_Commit::operator()(Evt const &evt, Fsm &fsm, SrcST &src, TgtST &dst)
{
}

// GRD_DltUpdate
// -------------
//
template <class Evt, class Fsm, class SrcST, class TgtST>
bool
DltDplyFSM::GRD_DltUpdate::operator()(Evt const &evt, Fsm &fsm, SrcST &src, TgtST &dst)
{
    return true;
}

// GRD_DltCommit
// -------------
//
template <class Evt, class Fsm, class SrcST, class TgtST>
bool
DltDplyFSM::GRD_DltCommit::operator()(Evt const &evt, Fsm &fsm, SrcST &src, TgtST &dst)
{
    return true;
}

}  // namespace fds
