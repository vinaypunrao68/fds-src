/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <iostream>
#include <vector>
#include <atomic>
#include <boost/msm/front/state_machine_def.hpp>
#include <boost/msm/front/functor_row.hpp>
#include <OmDmtDeploy.h>
#include <OmVolumePlacement.h>
#include <OmResources.h>
#include <fds_timer.h>
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
     * Timer task to retry recompute DMT
     */
    class RetryTimerTask: public FdsTimerTask {
      public:
        explicit RetryTimerTask(FdsTimer &timer)  // NOLINT
        : FdsTimerTask(timer) {}
        ~RetryTimerTask() {}

        virtual void runTimerTask() override;
    };

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
    struct DST_Compute : public msm::front::state<>
    {
        typedef mpl::vector<DmtPushMetaAckEvt> deferred_events;

        template <class Evt, class Fsm, class State>
        void operator()(Evt const &, Fsm &, State &) {}

        template <class Event, class FSM> void on_entry(Event const &, FSM &) {}
        template <class Event, class FSM> void on_exit(Event const &, FSM &) {}

        fds_uint32_t vol_acks_to_wait;
    };
    struct DST_Commit : public msm::front::state<>
    {
        template <class Evt, class Fsm, class State>
        void operator()(Evt const &, Fsm &, State &) {}

        template <class Event, class FSM> void on_entry(Event const &, FSM &) {}
        template <class Event, class FSM> void on_exit(Event const &, FSM &) {}

        // DMs that we send push_meta command and waiting for response from them
        NodeUuidSet push_meta_dms;
    };
    struct DST_Close : public msm::front::state<>
    {
        DST_Close() : commit_acks_to_wait(0) {}

        template <class Evt, class Fsm, class State>
        void operator()(Evt const &, Fsm &, State &) {}

        template <class Event, class FSM> void on_entry(Event const &, FSM &) {}
        template <class Event, class FSM> void on_exit(Event const &, FSM &) {}

        fds_uint32_t commit_acks_to_wait;
    };
    struct DST_Done : public msm::front::state<>
    {
        DST_Done() : close_acks_to_wait(0),
                     tryAgainTimer(new FdsTimer()),
                     tryAgainTimerTask(new RetryTimerTask(*tryAgainTimer)) {}

        ~DST_Done() {
            tryAgainTimer->destroy();
        }

        template <class Evt, class Fsm, class State>
        void operator()(Evt const &, Fsm &, State &) {}

        template <class Event, class FSM> void on_entry(Event const &, FSM &) {}
        template <class Event, class FSM> void on_exit(Event const &, FSM &) {}

        fds_uint32_t close_acks_to_wait;

        /**
         * Timer to try to compute DMT again, in case new DMs joined or DMs
         * got removed while deploying current DMT
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
    struct DACT_Start
    {
        template <class Evt, class Fsm, class SrcST, class TgtST>
        void operator()(Evt const &, Fsm &, SrcST &, TgtST &);
    };
    struct DACT_Compute
    {
        template <class Evt, class Fsm, class SrcST, class TgtST>
        void operator()(Evt const &, Fsm &, SrcST &, TgtST &);
    };
    struct DACT_ComputeDb
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
    struct GRD_DplyStart
    {
        template <class Evt, class Fsm, class SrcST, class TgtST>
        bool operator()(Evt const &, Fsm &, SrcST &, TgtST &);
    };
    struct GRD_DmtCompute
    {
        template <class Evt, class Fsm, class SrcST, class TgtST>
        bool operator()(Evt const &, Fsm &, SrcST &, TgtST &);
    };
    struct GRD_Commit
    {
        template <class Evt, class Fsm, class SrcST, class TgtST>
        bool operator()(Evt const &, Fsm &, SrcST &, TgtST &);
    };
    struct GRD_Close
    {
        template <class Evt, class Fsm, class SrcST, class TgtST>
        bool operator()(Evt const &, Fsm &, SrcST &, TgtST &);
    };
    struct GRD_Done
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
    msf::Row< DST_Idle    , DmtDeployEvt   , DST_Compute , DACT_Start  , GRD_DplyStart >,
    msf::Row< DST_Idle    , DmtLoadedDbEvt , DST_Commit  , DACT_ComputeDb , msf::none  >,
    // +------------------+----------------+-------------+---------------+--------------+
    msf::Row< DST_Compute , DmtVolAckEvt   , DST_Commit  , DACT_Compute, GRD_DmtCompute >,
    // +------------------+----------------+-------------+---------------+--------------+
    msf::Row< DST_Commit , DmtPushMetaAckEvt, DST_Close  , DACT_Commit   ,  GRD_Commit  >,
    // +------------------+----------------+-------------+---------------+--------------+
    msf::Row< DST_Close   , DmtCommitAckEvt, DST_Done    , DACT_Close    , GRD_Close    >,
    // +------------------+----------------+-------------+---------------+--------------+
    msf::Row< DST_Done    , DmtCloseOkEvt  , DST_Idle    , DACT_UpdDone  , GRD_Done     >
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
OM_DMTMod::dmt_deploy_event(DmtDeployEvt const &evt)
{
    fds_mutex::scoped_lock l(fsm_lock);
    dmt_dply_fsm->process_event(evt);
}

void
OM_DMTMod::dmt_deploy_event(DmtCloseOkEvt const &evt)
{
    fds_mutex::scoped_lock l(fsm_lock);
    dmt_dply_fsm->process_event(evt);
}

void
OM_DMTMod::dmt_deploy_event(DmtPushMetaAckEvt const &evt)
{
    fds_mutex::scoped_lock l(fsm_lock);
    dmt_dply_fsm->process_event(evt);
}

void
OM_DMTMod::dmt_deploy_event(DmtCommitAckEvt const &evt)
{
    fds_mutex::scoped_lock l(fsm_lock);
    dmt_dply_fsm->process_event(evt);
}

void
OM_DMTMod::dmt_deploy_event(DmtVolAckEvt const &evt)
{
    fds_mutex::scoped_lock l(fsm_lock);
    dmt_dply_fsm->process_event(evt);
}

void
OM_DMTMod::dmt_deploy_event(DmtLoadedDbEvt const &evt)
{
    fds_mutex::scoped_lock l(fsm_lock);
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

void DmtDplyFSM::RetryTimerTask::runTimerTask()
{
    OM_NodeDomainMod* domain = OM_NodeDomainMod::om_local_domain();
    LOGNOTIFY << "Retry to re-compute DMT";
    domain->om_dmt_update_cluster(0);
}

/** 
 * GRD_DplyStart
 * @return true if there are any pending added or removed DMs, otherwise false
 */
template <class Evt, class Fsm, class SrcST, class TgtST>
bool
DmtDplyFSM::GRD_DplyStart::operator()(Evt const &evt, Fsm &fsm, SrcST &src, TgtST &dst)
{
    fds_bool_t bret = false;
    NodeList addNodes, rmNodes;
    OM_NodeContainer* loc_domain = OM_NodeDomainMod::om_loc_domain_ctrl();
    OM_Module* om = OM_Module::om_singleton();
    VolumePlacement* vp = om->om_volplace_mod();
    ClusterMap* cm = om->om_clusmap_mod();
    OM_DmContainer::pointer dmNodes = loc_domain->om_dm_nodes();

    // get pending DMs removal/additions
    dmNodes->om_splice_nodes_pend(&addNodes, &rmNodes);
    cm->updateMap(fpi::FDSP_DATA_MGR, addNodes, rmNodes);
    fds_uint32_t added_nodes = (cm->getAddedServices(fpi::FDSP_DATA_MGR)).size();
    fds_uint32_t rm_nodes = (cm->getRemovedServices(fpi::FDSP_DATA_MGR)).size();

    LOGDEBUG << "Added DMs size: " << added_nodes
             << " Removed DMs size: " << rm_nodes;

    bret = ((added_nodes > 0) || (rm_nodes > 0));
    LOGNORMAL << "Start DMT compute and deploying new DMT? " << bret;
    return bret;
}

/* DACT_Start
 * ------------
 * Gets how many acks for volume notify we need to wait for, before
 * transitioning to the next state
 */
template <class Evt, class Fsm, class SrcST, class TgtST>
void
DmtDplyFSM::DACT_Start::operator()(Evt const &evt, Fsm &fsm, SrcST &src, TgtST &dst)
{
    DmtDeployEvt dplyEvt = (DmtDeployEvt)evt;
    dst.vol_acks_to_wait = dplyEvt.numVols;
    LOGDEBUG << "FSM DACT_Start" << dst.vol_acks_to_wait;
}

/** 
 * GRD_DmtCompute
 * @return true if we got all acks for volume notify, otherwise false
 */
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
 * Re-compute DMT and send Push Meta message to DMs that need
 * to push meta to other DMs (that take over some volumes).
 */
template <class Evt, class Fsm, class SrcST, class TgtST>
void
DmtDplyFSM::DACT_Compute::operator()(Evt const &evt, Fsm &fsm, SrcST &src, TgtST &dst)
{
    Error err(ERR_OK);
    OM_Module* om = OM_Module::om_singleton();
    VolumePlacement* vp = om->om_volplace_mod();
    ClusterMap* cm = om->om_clusmap_mod();
    fds_uint32_t added_nodes = (cm->getAddedServices(fpi::FDSP_DATA_MGR)).size();
    fds_uint32_t rm_nodes = (cm->getRemovedServices(fpi::FDSP_DATA_MGR)).size();

    // if we are here, we must have at least one added or removed DM
    fds_verify((added_nodes > 0) || (rm_nodes > 0));

    // compute DMT
    vp->computeDMT(cm);

    // send push meta messages to appropriate DMs
    dst.push_meta_dms.clear();
    err = vp->beginRebalance(cm, &dst.push_meta_dms);
    // TODO(xxx) need to handle this error
    fds_verify(err.ok());

    // it's possible that we didn't need to send push meta msg,
    // eg. there are no volumes or we removed a node and no-one got promoted
    if (dst.push_meta_dms.size() < 1) {
        fsm.process_event(DmtPushMetaAckEvt(NodeUuid()));
    }
    LOGDEBUG << "Will wait for " << dst.push_meta_dms.size() << " push_meta acks";
}

/**
 *  GRD_Commit
 * -------------
 * @return true if OM received all acks for push_meta command
 */
template <class Evt, class Fsm, class SrcST, class TgtST>
bool
DmtDplyFSM::GRD_Commit::operator()(Evt const &evt, Fsm &fsm, SrcST &src, TgtST &dst)
{
    // for now assuming commit is always a success
    if (evt.dm_uuid.uuid_get_val() > 0) {
        // we must never receive ack for push meta from node we didn't send
        // push_meta command to
        fds_verify(src.push_meta_dms.count(evt.dm_uuid) > 0);
        src.push_meta_dms.erase(evt.dm_uuid);
    }

    bool bret = (src.push_meta_dms.size() == 0);
    LOGDEBUG << "Meta acks to wait " << src.push_meta_dms.size()
             << "; returning " << bret;
    return bret;
}

/* DACT_Commit
 * ------------
 * We got all acks for push_meta. Commit DMT and broadcast it to all
 * AMs and DMs.
 */
template <class Evt, class Fsm, class SrcST, class TgtST>
void
DmtDplyFSM::DACT_Commit::operator()(Evt const &evt, Fsm &fsm, SrcST &src, TgtST &dst)
{
    OM_NodeContainer* loc_domain = OM_NodeDomainMod::om_loc_domain_ctrl();
    OM_Module* om = OM_Module::om_singleton();
    VolumePlacement* vp = om->om_volplace_mod();
    ClusterMap* cm = om->om_clusmap_mod();

    // commit DMT
    vp->commitDMT();

    // since we accounted for added/removed nodes in DMT, reset pending nodes in
    // cluster map
    cm->resetPendServices(fpi::FDSP_DATA_MGR);

    // broadcast DMT
    dst.commit_acks_to_wait = loc_domain->om_bcast_dmt(vp->getCommittedDMT());
    // there are must be nodes to which we send new DMT
    // unless all failed? -- in that case we should handle errors
    fds_verify(dst.commit_acks_to_wait > 0);

    LOGDEBUG << "Committed DMT, will wait for " << dst.commit_acks_to_wait
             << " DMT commit acks";
}

/**
 * GRD_Close
 * -------------
 * @return true if we got all acks for DMT commit
 */
template <class Evt, class Fsm, class SrcST, class TgtST>
bool
DmtDplyFSM::GRD_Close::operator()(Evt const &evt, Fsm &fsm, SrcST &src, TgtST &dst)
{
    OM_Module* om = OM_Module::om_singleton();
    VolumePlacement* vp = om->om_volplace_mod();
    fds_uint64_t committed_ver = vp->getCommittedDMTVersion();

    // we can possibly have a race condition, where a new non-DM service
    // gets registered while we are in the process of updating DMT
    // and moving to the next DMT version. That node may receive old DMT,
    // and now we have a different committed DMT. That node will also be
    // send a new DMT, but we can still receive a reply for the old version
    // we should ignore ack for the old version
    fds_verify(evt.dmt_version <= committed_ver);
    if (evt.dmt_version != committed_ver) {
        LOGDEBUG << "Received commit acks for old DMT version, acks to wait "
                 << src.commit_acks_to_wait;
        return false;
    }

    // for now assuming close is always a success
    fds_verify(src.commit_acks_to_wait > 0);
    src.commit_acks_to_wait--;

    bool bret = (src.commit_acks_to_wait == 0);
    LOGDEBUG << "Commit acks to wait " << src.commit_acks_to_wait
             << ", returning " << bret;

    return bret;
}

/*
 * DACT_Close
 * -----------
 * We received all acks for DMT commit, broadcast DMT close msg to all DMs
 */
template <class Evt, class Fsm, class SrcST, class TgtST>
void
DmtDplyFSM::DACT_Close::operator()(Evt const &evt, Fsm &fsm, SrcST &src, TgtST &dst)
{
    OM_NodeContainer* loc_domain = OM_NodeDomainMod::om_loc_domain_ctrl();
    OM_Module* om = OM_Module::om_singleton();
    VolumePlacement* vp = om->om_volplace_mod();
    fds_uint64_t committed_ver = vp->getCommittedDMTVersion();

    // broadcast DMT close
    dst.close_acks_to_wait = loc_domain->om_bcast_dmt_close(committed_ver);
    // ok if there are no DMs to send close = no DMs at all
    // TODO(xxx) we should distinguish between that we got an error when
    // sending messages or there are no DMs at all
    if (dst.close_acks_to_wait < 1) {
        fsm.process_event(DmtCloseOkEvt(committed_ver));
    }

    LOGDEBUG << "Will wait for " << dst.close_acks_to_wait << " DMT close acks";
}

/**
 * GRD_Done
 * -------------
 * @return true if we got all acks for DMT close
 */
template <class Evt, class Fsm, class SrcST, class TgtST>
bool
DmtDplyFSM::GRD_Done::operator()(Evt const &evt, Fsm &fsm, SrcST &src, TgtST &dst)
{
    OM_Module* om = OM_Module::om_singleton();
    VolumePlacement* vp = om->om_volplace_mod();
    fds_uint64_t committed_ver = vp->getCommittedDMTVersion();

    // do we ever have an ok case when we get close ack for other than
    // committed DMT version? For now cannot think of any
    fds_verify(evt.dmt_version == committed_ver);

    // for now assuming close is always a success
    if (src.close_acks_to_wait > 0) {
        src.close_acks_to_wait--;
    }

    bool bret = (src.close_acks_to_wait == 0);
    LOGDEBUG << "Close acks to wait " << src.close_acks_to_wait
             << ", returning " << bret;

    return bret;
}

// DACT_UpdDone
// -----------
template <class Evt, class Fsm, class SrcST, class TgtST>
void
DmtDplyFSM::DACT_UpdDone::operator()(Evt const &evt, Fsm &fsm, SrcST &src, TgtST &dst)
{
    OM_Module* om = OM_Module::om_singleton();
    VolumePlacement* vp = om->om_volplace_mod();
    ClusterMap* cm = om->om_clusmap_mod();

    LOGNOTIFY << "OM deployed DMT with "
              << cm->getNumMembers(fpi::FDSP_DATA_MGR) << " DMs";

    // In case new DMs got added or DMs got removed while we were
    // deploying current DMT, start timer to try deploy a DMT again
    if (!src.tryAgainTimer->schedule(src.tryAgainTimerTask,
                                     std::chrono::seconds(1))) {
        LOGWARN << "Failed to start try again timer!!!"
                << " DM additions/deletions may be pending for long time";
    }
}

}  // namespace fds
