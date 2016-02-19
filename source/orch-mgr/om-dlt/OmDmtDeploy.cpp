/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <iostream>
#include <vector>
#include <string>
#include <atomic>
#include <boost/msm/front/state_machine_def.hpp>
#include <boost/msm/front/functor_row.hpp>
#include <OmDmtDeploy.h>
#include <OmVolumePlacement.h>
#include <OmResources.h>
#include <fds_timer.h>
#include <orch-mgr/om-service.h>

namespace fds {

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
    class WaitingTimerTask: public FdsTimerTask {
      public:
        explicit WaitingTimerTask(FdsTimer &timer)  // NOLINT
                : FdsTimerTask(timer) {}
        ~WaitingTimerTask() {}

        virtual void runTimerTask() override;
    };
    /**
     * OM DMT Deployment states.
     */
    struct DST_Idle : public msm::front::state<>
    {
        template <class Evt, class Fsm, class State>
        void operator()(Evt const &, Fsm &, State &) {}

        template <class Event, class FSM> void on_entry(Event const &e, FSM &f) {
            LOGNOTIFY << "DST_Idle. Evt: " << e.logString() << " Ready for DM Migration";
        }
        template <class Event, class FSM> void on_exit(Event const &e, FSM &f) {
            LOGNOTIFY << "DST_Idle. Evt: " << e.logString();
        }
    };
    struct DST_Error
            : public msm::front::interrupt_state<mpl::vector<DmtEndErrorEvt, DmtRecoveryEvt>>
    {
        DST_Error() : abortMigrAcksToWait( 0 ),
                      commitDmtAcksToWait( 0 ) {}

        template <class Evt, class Fsm, class State>
        void operator()(Evt const &, Fsm &, State &) {}

        template <class Event, class FSM> void on_entry(Event const &e, FSM &f) {
            OM_Module* om = OM_Module::om_singleton();
            VolumePlacement* vp = om->om_volplace_mod();
            LOGNOTIFY << "DST_Error. Evt: " << e.logString() << " DM Migration encountered error"
                    << "(migrationid: " << vp->getCommittedDMTVersion() << ")";
        }
        template <class Event, class FSM> void on_exit(Event const &e, FSM &f) {
            LOGNOTIFY << "DST_Error. Evt: " << e.logString();
        }

        fds_uint32_t abortMigrAcksToWait;
        fds_uint32_t commitDmtAcksToWait;
    };
    struct DST_AllOk : public msm::front::state<>
    {
        DST_AllOk() : close_acks_to_wait( 0 ),
                      tryAgainTimer( new FdsTimer() ),
                      tryAgainTimerTask( new RetryTimerTask( *tryAgainTimer ) ) {}

        template <class Evt, class Fsm, class State>
        void operator()(Evt const &, Fsm &, State &) {}

        template <class Event, class FSM> void on_entry(Event const &e, FSM &f) {
            OM_Module* om = OM_Module::om_singleton();
            VolumePlacement* vp = om->om_volplace_mod();
            LOGNOTIFY << "DST_AllOk. Evt: " << e.logString() << " DM Migration error handling finished"
                    << "(migrationid: " << vp->getCommittedDMTVersion() << ")";
        }
        template <class Event, class FSM> void on_exit(Event const &e, FSM &f) {
            LOGNOTIFY << "DST_AllOk. Evt: " << e.logString();
        }


        fds_uint32_t close_acks_to_wait;

        /**
         * Timer to try to compute DMT again, in case new DMs joined or DMs
         * got removed while deploying current DMT
         */
        FdsTimerPtr tryAgainTimer;
        FdsTimerTaskPtr tryAgainTimerTask;
    };
    struct DST_Waiting : public msm::front::state<>
    {
        DST_Waiting() : waitingTimer(new FdsTimer()),
                        waitingTimerTask(
                                new WaitingTimerTask(*waitingTimer)) {}

        ~DST_Waiting() {
            waitingTimer->destroy();
        }

        template <class Evt, class Fsm, class State>
        void operator()(Evt const &, Fsm &, State &) {}

        template <class Event, class FSM> void on_entry(Event const &e, FSM &f) {
            LOGNOTIFY << "DST_Waiting. Evt: " << e.logString();
        }
        template <class Event, class FSM> void on_exit(Event const &e, FSM &f) {
            LOGNOTIFY << "DST_Waiting. Evt: " << e.logString();
        }

        /**
        * timer to retry re-compute DLT
        */
        FdsTimerPtr waitingTimer;
        FdsTimerTaskPtr waitingTimerTask;
    };
    struct DST_Rebalance : public msm::front::state<>
    {
        typedef mpl::vector<DmtPushMetaAckEvt> deferred_events;

        template <class Evt, class Fsm, class State>
        void operator()(Evt const &, Fsm &, State &) {}

        template <class Event, class FSM> void on_entry(Event const &e, FSM &f) {
            OM_Module* om = OM_Module::om_singleton();
            VolumePlacement* vp = om->om_volplace_mod();
            LOGNOTIFY << "DST_Rebalance. Evt: " << e.logString() << " DM Migration in progress "
                    << "(migrationid: " << vp->getTargetDMTVersion() << ")";
        }
        template <class Event, class FSM> void on_exit(Event const &e, FSM &f) {
            LOGNOTIFY << "DST_Rebalance. Evt: " << e.logString();
        }

        NodeUuidSet dms_to_ack;
    };
    struct DST_Commit : public msm::front::state<>
    {
        typedef mpl::vector<DmtCommitAckEvt> deferred_events;

        template <class Evt, class Fsm, class State>
        void operator()(Evt const &, Fsm &, State &) {}

        template <class Event, class FSM> void on_entry(Event const &e, FSM &f) {
            LOGNOTIFY << "DST_Commit. Evt: " << e.logString();
        }
        template <class Event, class FSM> void on_exit(Event const &e, FSM &f) {
            LOGNOTIFY << "DST_Commit. Evt: " << e.logString();
        }

        // DMs that we send push_meta command and waiting for response from them
        NodeUuidSet pull_meta_dms;
    };
    struct DST_BcastAM : public msm::front::state<>
    {
        typedef mpl::vector<DmtCommitAckEvt> deferred_events;

        DST_BcastAM() : commit_acks_to_wait(0) {}

        template <class Evt, class Fsm, class State>
        void operator()(Evt const &, Fsm &, State &) {}

        template <class Event, class FSM> void on_entry(Event const &e, FSM &f) {
            LOGNOTIFY << "DST_BcastAM. Evt: " << e.logString();
        }
        template <class Event, class FSM> void on_exit(Event const &e, FSM &f) {
            LOGNOTIFY << "DST_BcastAM. Evt: " << e.logString();
        }

        fds_uint32_t commit_acks_to_wait;
    };
    struct DST_Close : public msm::front::state<>
    {
        typedef mpl::vector<DmtCloseOkEvt> deferred_events;

        DST_Close() : commit_acks_to_wait(0) {}

        template <class Evt, class Fsm, class State>
        void operator()(Evt const &, Fsm &, State &) {}

        template <class Event, class FSM> void on_entry(Event const &e, FSM &f) {
            LOGNOTIFY << "DST_Close. Evt: " << e.logString();
        }
        template <class Event, class FSM> void on_exit(Event const &e, FSM &f) {
            LOGNOTIFY << "DST_Close. Evt: " << e.logString();
        }

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

        template <class Event, class FSM> void on_entry(Event const &e, FSM &f) {
            LOGNOTIFY << "DST_Done. Evt: " << e.logString();
        }
        template <class Event, class FSM> void on_exit(Event const &e, FSM &f) {
            LOGNOTIFY << "DST_Done. Evt: " << e.logString();
        }

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
    typedef mpl::vector<DST_Idle, DST_AllOk> initial_state;
    struct MyInitEvent {
        std::string logString() const {
            return "MyInitEvent";
        }
    };
    typedef MyInitEvent initial_event;

    /**
     * Transition actions.
     */
    struct DACT_Start
    {
        template <class Evt, class Fsm, class SrcST, class TgtST>
        void operator()(Evt const &, Fsm &, SrcST &, TgtST &);
    };
    struct DACT_Error
    {
        template <class Evt, class Fsm, class SrcST, class TgtST>
        void operator()(Evt const &, Fsm &, SrcST &, TgtST &);
    };
    struct DACT_Recovered
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
    struct DACT_BcastAM
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
    struct DACT_Waiting
    {
        template <class Evt, class Fsm, class SrcST, class TgtST>
        void operator()(Evt const &, Fsm &, SrcST &, TgtST &);
    };
    struct DACT_EndError
    {
        template <class Evt, class Fsm, class SrcST, class TgtST>
        void operator()(Evt const &, Fsm &, SrcST &, TgtST &);
    };
    struct DACT_ChkEndErr
    {
        template <class Evt, class Fsm, class SrcST, class TgtST>
        void operator()(Evt const &, Fsm &, SrcST &, TgtST &);
    };
    struct DACT_RecoverDone
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
    struct GRD_DmtRebal
    {
        template <class Evt, class Fsm, class SrcST, class TgtST>
        bool operator()(Evt const &, Fsm &, SrcST &, TgtST &);
    };
    struct GRD_Commit
    {
        template <class Evt, class Fsm, class SrcST, class TgtST>
        bool operator()(Evt const &, Fsm &, SrcST &, TgtST &);
    };
    struct GRD_BcastAM
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
    struct GRD_ReRegister
    {
        template <class Evt, class Fsm, class SrcST, class TgtST>
        bool operator()(Evt const &, Fsm &, SrcST &, TgtST &);
    };

    /**
     * Transition table for OM DMT deployment.
     */
    struct transition_table : mpl::vector<
    // +------------------+----------------+-------------+---------------+--------------+
    // | Start            | Event          | Next        | Action        | Guard        |
    // +------------------+----------------+-------------+---------------+--------------+
    msf::Row< DST_Idle    , DmtDeployEvt   , DST_Waiting , DACT_Waiting  , GRD_DplyStart>,
    msf::Row< DST_Idle    , DmtLoadedDbEvt , DST_BcastAM , DACT_Commit   ,   msf::none  >,
    // +------------------+----------------+-------------+---------------+--------------+
    msf::Row< DST_Waiting , DmtTimeoutEvt  , DST_Rebalance, DACT_Start   , msf::none   >,
    msf::Row< DST_Waiting , DmtEndErrorEvt , DST_Idle    , DACT_Recovered, msf::none    >,
    // +------------------+----------------+-------------+---------------+--------------+
    msf::Row< DST_Rebalance, DmtVolAckEvt  , DST_Commit  , DACT_Rebalance,GRD_DmtRebal  >,
    msf::Row< DST_Rebalance, DmtEndErrorEvt, DST_Idle    , DACT_Recovered, msf::none    >,
    // +------------------+----------------+-------------+---------------+--------------+
    msf::Row< DST_Commit  , DmtPushMetaAckEvt, DST_BcastAM, DACT_Commit   , GRD_Commit  >,
    msf::Row< DST_Commit  , DmtEndErrorEvt , DST_Idle    , DACT_Recovered, msf::none    >,
    // +------------------+----------------+-------------+---------------+--------------+
    msf::Row< DST_BcastAM , DmtCommitAckEvt, DST_Close   , DACT_BcastAM  , GRD_BcastAM  >,
    msf::Row< DST_BcastAM , DmtEndErrorEvt , DST_Idle    , DACT_Recovered, msf::none    >,
    // +------------------+----------------+-------------+---------------+--------------+
    msf::Row< DST_Close   , DmtCommitAckEvt, DST_Done    , DACT_Close    , GRD_Close    >,
    msf::Row< DST_Close   , DmtEndErrorEvt , DST_Idle    , DACT_Recovered, msf::none    >,
    // +------------------+----------------+-------------+---------------+--------------+
    msf::Row< DST_Done    , DmtCloseOkEvt  , DST_Idle    , DACT_UpdDone  , GRD_Done     >,
    msf::Row< DST_Done    , DmtEndErrorEvt , DST_Idle    , DACT_Recovered, msf::none    >,
    // +------------------+----------------+-------------+---------------+--------------+
    msf::Row< DST_AllOk   ,DmtErrorFoundEvt, DST_Error   , DACT_Error    , msf::none    >,
    msf::Row< DST_AllOk   , DmtUpEvt   , DST_Error   , DACT_Error    , GRD_ReRegister>,
    // +------------------+----------------+-------------+---------------+--------------+
    msf::Row< DST_Error   , DmtEndErrorEvt , DST_AllOk   , DACT_EndError , msf::none    >,
    msf::Row< DST_Error   , DmtRecoveryEvt , DST_Error   , DACT_ChkEndErr, msf::none    >
    // +------------------+----------------+-------------+---------------+--------------+
    >{};  // NOLINT

    template <class Event, class FSM> void no_transition(Event const &, FSM &, int);
};

// ------------------------------------------------------------------------------------
// DMT Module Vector
// ------------------------------------------------------------------------------------
OM_DMTMod::OM_DMTMod(char const *const name)
    : Module(name),
      volume_grp_mode(false),
      waitingDMs(0)
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
    volume_grp_mode = bool(MODULEPROVIDER()->get_fds_config()->
                        get<bool>("fds.feature_toggle.common.enable_volumegrouping", false));

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
OM_DMTMod::dmt_deploy_event(DmtUpEvt const &evt)
{
    fds_mutex::scoped_lock l(fsm_lock);
    dmt_dply_fsm->process_event(evt);
}

void
OM_DMTMod::dmt_deploy_event(DmtTimeoutEvt const &evt)
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

void
OM_DMTMod::dmt_deploy_event(DmtErrorFoundEvt const &evt)
{
    fds_mutex::scoped_lock l(fsm_lock);
    dmt_dply_fsm->process_event(evt);
}

void
OM_DMTMod::dmt_deploy_event(DmtRecoveryEvt const &evt)
{
    fds_mutex::scoped_lock l(fsm_lock);
    dmt_dply_fsm->process_event(evt);
}

void
OM_DMTMod::addWaitingDMs() {
    auto nodeDomMod = OM_Module::om_singleton()->om_nodedomain_mod();
    if (!nodeDomMod->dmClusterPresent()) {
        ++waitingDMs;
    }
}
// --------------------------------------------------------------------------------------
// OM DMT Deployment FSM Implementation
// --------------------------------------------------------------------------------------
template <class Evt, class Fsm>
void
DmtDplyFSM::on_entry(Evt const &evt, Fsm &fsm)
{
    LOGDEBUG << "DmtDplyFSM on entry";
}

template <class Evt, class Fsm>
void
DmtDplyFSM::on_exit(Evt const &evt, Fsm &fsm)
{
    LOGDEBUG << "DmtDplyFSM on exit";
}

// no_transition
// -------------
//
template <class Evt, class Fsm>
void
DmtDplyFSM::no_transition(Evt const &evt, Fsm &fsm, int state)
{
    LOGDEBUG << "Evt: " << evt.logString() << " DmtDplyFSM no transition";
}

void DmtDplyFSM::RetryTimerTask::runTimerTask()
{
    OM_NodeDomainMod* domain = OM_NodeDomainMod::om_local_domain();
    if (!domain->isDomainShuttingDown()) {
        LOGNOTIFY << "Retry to re-compute DMT";
        domain->om_dmt_update_cluster();
    } else {
        LOGNOTIFY << "Will not recompute DMT since domain is shutting down or is down";
    }
}
void DmtDplyFSM::WaitingTimerTask::runTimerTask()
{
    OM_NodeDomainMod* domain = OM_NodeDomainMod::om_local_domain();
    LOGNOTIFY << "DmtWaitingFSM: moving from waiting state to compute state";
    domain->om_dmt_waiting_timeout();
}

/** 
 * GRD_DplyStart
 * Computes DMT and sets as target version if DMT is different from currently
 * commited DMT
 * @return true if DMT changes, otherwise returns false
 *         In pre-beta2 version, returns true if there are any pending added
 *         of removed DMs, otherwise false
 *         In GA version, with 2primary DMs consistency model, DMT may be recomputed
 *         if services failed/came back since last DMT computation. In that case,
 *         the method returns true.
 */
template <class Evt, class Fsm, class SrcST, class TgtST>
bool
DmtDplyFSM::GRD_DplyStart::operator()(Evt const &evt, Fsm &fsm, SrcST &src, TgtST &dst)
{
    fds_bool_t bret = false;
    NodeList addNodes, rmNodes, resyncNodes;
    OM_NodeContainer* loc_domain = OM_NodeDomainMod::om_loc_domain_ctrl();
    OM_Module* om = OM_Module::om_singleton();
    VolumePlacement* vp = om->om_volplace_mod();
    ClusterMap* cm = om->om_clusmap_mod();
    OM_DmContainer::pointer dmNodes = loc_domain->om_dm_nodes();

    // get pending DMs removal/additions
    dmNodes->om_splice_nodes_pend(&addNodes, &rmNodes, &resyncNodes);
    cm->updateMap(fpi::FDSP_DATA_MGR, addNodes, rmNodes, resyncNodes);
    fds_uint32_t added_nodes = (cm->getAddedServices(fpi::FDSP_DATA_MGR)).size();
    fds_uint32_t rm_nodes = (cm->getRemovedServices(fpi::FDSP_DATA_MGR)).size();
    fds_uint32_t resync_nodes = cm->getDmResyncServices().size();
    fds_uint32_t nonFailedDms = cm->getNumNonfailedMembers(fpi::FDSP_DATA_MGR);
    fds_uint32_t totalDms = cm->getNumMembers(fpi::FDSP_DATA_MGR);

    LOGNOTIFY << "Added DMs size: " << added_nodes
             << " Removed DMs size: " << rm_nodes
			 << " Resyncing DMs size: " << resync_nodes
             << " Total DMs: " << totalDms
             << " Non-failed DMs: " << nonFailedDms;

    LOGDEBUG << "Added nodes: ";
    for (auto cit : cm->getAddedServices(fpi::FDSP_DATA_MGR))
    {
    	LOGDEBUG << cit.uuid_get_val();
    }
    LOGDEBUG << "Removed nodes: ";
    for (auto cit : cm->getRemovedServices(fpi::FDSP_DATA_MGR))
    {
    	LOGDEBUG << cit.uuid_get_val();
    }
    LOGDEBUG << "Resyncing DM nodes: ";
    for (auto cit : cm->getDmResyncServices())
    {
    	LOGDEBUG << cit.uuid_get_val();
    }

    // For now, we're not supporting more than 1 resync nodes
    if (cm->getDmResyncServices().size() > 1) {
        // TODO - address it later
        LOGNORMAL << "We are not supporting more than one node down at a time";
        // bret = false;
        // fds_assert(!"ERROR: 2 nodes gone down and trying to resync");  // panic in lab mode
        // return bret;
    }

    // this method computes new DMT and sets as target if
    // 1. newly computed DMT is different from the current commited DMT
    // or
    // 2. We're in a DM Resync state, in which we bump the DMT version.
    Error err = vp->computeDMT(cm);
    bret = err.ok();
    if (err == ERR_INVALID_ARG) {
        // this should not happen if we don't have any removed DMs
        fds_verify(rm_nodes > 0);
        // Only happens with new consistency model (2primary)
        // TODO(Anna) need to handle this error. Should recompute
        // DMT without removing some of the DMs. TBD the approach
        // for now not commiting this DMT; If this is the case when
        // we are trying to remove just one DM, if a failed DM that
        // was primary together with the removed DM comes back, next recompute
        // will succeed; so we may still recover
    } else if (err == ERR_NOT_READY) {
        // this is ok -- no changes in the domain to recompute a DMT
        LOGDEBUG << "Not continuing DMT commit cycle since no changes in "
                 << " in the domain that cause DMT re-computation";
    } else if (err == ERR_NOT_FOUND) {
        LOGDEBUG << "No DMs joined yet";
    } else if (!err.ok()) {
        LOGERROR << "Unexpected error from computeDMT " << err
                 << " Not commiting new DMT and ignoring error "
                 << " FIX IT!";
    }

    /**
     * There is a possibility that a node was queued up during the previous
     * run of the meta-state machine (MSM).
     * As part of DST_Close, a timer task is started to retry the state maching from
     * the top so that we can catch nodes that happened in this scenario.
     * So we cannot depend on evt.dmResync as a guarantee for state.
     */
    if (!bret && resync_nodes) {
    	LOGNOTIFY << "DMT did not change, but dmResync requested";
    	bret = true;
    }

    LOGNORMAL << "Start DMT compute and deploying new DMT? " << bret;
    return bret;
}

template <class Evt, class Fsm, class SrcST, class TgtST>
bool
DmtDplyFSM::GRD_ReRegister::operator()(Evt const &evt, Fsm &fsm, SrcST &src, TgtST &dst)
{
    LOGNOTIFY << "DmtDplyFSM::GRD_ReRegister";
    fds_bool_t bret = false;
    NodeUuid nodeChk = evt.uuid;

    NodeList addNodes, rmNodes, resyncNodes;
    OM_NodeContainer* loc_domain = OM_NodeDomainMod::om_loc_domain_ctrl();
    OM_Module* om = OM_Module::om_singleton();
    VolumePlacement* vp = om->om_volplace_mod();
    ClusterMap* cm = om->om_clusmap_mod();

    if (cm->ongoingMigrationDMs.find(nodeChk) != cm->ongoingMigrationDMs.end()) {
        /**
         * The nodeChk UUID is found in a list of ongoing migrations
         * so it means that the node (executor) has crashed and reregistered.
         * We will fire the DACT_Error in this case
         */
        LOGNOTIFY << "Node " << nodeChk << " found to be already in syncing";
        bret = true;
    }

    return (bret);
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
    // if we have any volumes, send volumes to DMs that are added to cluster map
    OM_NodeContainer* loc_domain = OM_NodeDomainMod::om_loc_domain_ctrl();
    OM_Module* om = OM_Module::om_singleton();
    ClusterMap* cm = om->om_clusmap_mod();
    NodeUuidSet addDms = cm->getAddedServices(fpi::FDSP_DATA_MGR);
    fds_bool_t dmResync = cm->getDmResyncServices().size() ? true : false;

    dst.dms_to_ack.clear();

    if (!dmResync && !om->isInTestMode())  {
        for (NodeUuidSet::const_iterator cit = addDms.cbegin();
        		cit != addDms.cend(); ++cit) {
            OM_DmAgent::pointer dm_agent = loc_domain->om_dm_agent(*cit);
            LOGDEBUG << "bcasting vol on dmt deploy: ";
            // dm_agent->dump_agent_info();
            fds_uint32_t count = loc_domain->om_bcast_vol_list(dm_agent);
		    if (count > 0) {
			    dst.dms_to_ack.insert(*cit);
		    }
        }
        LOGNOTIFY << "DmtDplyFSM::DACT_Start, Will wait for " << dst.dms_to_ack.size()
				 << " DMs to acks volume notify";
    } else {
    	/*
    	 * In case of Resync, there's no volume management changes. The newly restarted
    	 * DM will have its service layer pull the volume descriptor, so no need to
    	 * waste resources broadcasting unchanged vol desc's to all the nodes.
    	 */
    	LOGNOTIFY << "DmtDplyFSM::DACT_Start, DM Resync OM DmtFSM. Will not broadcast volume descriptors";
    }
}

/* DACT_Waiting
 * ------------
 * Waiting to allow for additional nodes to join/leave before recomputing DMT.
 */
template <class Evt, class Fsm, class SrcST, class TgtST>
void
DmtDplyFSM::DACT_Waiting::operator()(Evt const &evt, Fsm &fsm, SrcST &src, TgtST &dst) {
    LOGNOTIFY << "DACT_Waiting: entering wait state.";
    if (!dst.waitingTimer->schedule(dst.waitingTimerTask,
            std::chrono::seconds(1))) {
        LOGWARN << "DACT_DmtWaiting: failed to start retry timer!!!"
                    << " DM additions/deletions may be pending for long time!";
    }
}

/** 
 * GRD_DmtRebal
 * @return true if we got all acks for volume notify, otherwise false
 */
template <class Evt, class Fsm, class SrcST, class TgtST>
bool
DmtDplyFSM::GRD_DmtRebal::operator()(Evt const &evt, Fsm &fsm, SrcST &src, TgtST &dst)
{
    DmtVolAckEvt volAckEvt = (DmtVolAckEvt)evt;
    if (volAckEvt.dm_uuid.uuid_get_val() > 0) {
        // ok if we receive vol ack for DM not in our wait list, because it
        // could be some other DM, etc
        if (src.dms_to_ack.count(volAckEvt.dm_uuid) > 0) {
            src.dms_to_ack.erase(evt.dm_uuid);
        }
    }

    bool bret = (src.dms_to_ack.size() == 0);
    LOGNOTIFY << "DmtDplyFSM::GRD_DmtRebal, DMs to wait " << src.dms_to_ack.size()
             << " for vol acks; returning " << bret;

    return bret;
}

/* DACT_Rebalance
 * ------------
 * Start rebalance metadata (DMs) due to DMT recomputation.
 * VolumePlacement has target DMT set to newly computed DMT
 */
template <class Evt, class Fsm, class SrcST, class TgtST>
void
DmtDplyFSM::DACT_Rebalance::operator()(Evt const &evt, Fsm &fsm, SrcST &src, TgtST &dst)
{
    Error err(ERR_OK);
    OM_Module* om = OM_Module::om_singleton();
    VolumePlacement* vp = om->om_volplace_mod();
    ClusterMap* cm = om->om_clusmap_mod();

    // if we are here, we must have target DMT, but this will be checked by
    // beginRebalance

    // send push meta messages to appropriate DMs
    dst.pull_meta_dms.clear();
    // This should be a clear set
    // fds_assert(cm->ongoingMigrationDMs.size() == 0);
    cm->ongoingMigrationDMs.clear();

    err = vp->beginRebalance(cm, &dst.pull_meta_dms);

    // Store a list of ongoing migrations so we can check if it's
    // a duplicate.
    for (auto cit : dst.pull_meta_dms) {
        auto pair = cm->ongoingMigrationDMs.insert(cit);
        fds_assert(pair.second); // we shouldn't have existing node in the set
    }

    if ( !err.ok() )
    {
        LOGERROR << "Begin Rebalance failed with " << err;
    }

    // it's possible that we didn't need to send push meta msg,
    // eg. there are no volumes or we removed a node and no-one got promoted
    if (dst.pull_meta_dms.size() < 1) {
        fsm.process_event(DmtPushMetaAckEvt(NodeUuid()));
    }
    LOGNOTIFY << "DmtDplyFSM::DACT_Rebalance, Will wait for " << dst.pull_meta_dms.size() << " push_meta acks";
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
        fds_verify(src.pull_meta_dms.count(evt.dm_uuid) > 0);
        src.pull_meta_dms.erase(evt.dm_uuid);
    }

    bool bret = (src.pull_meta_dms.size() == 0);
    LOGNOTIFY << "DmtDplyFSM::GRD_Commit, Meta acks to wait " << src.pull_meta_dms.size()
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

    // We commit DMT as an 'official' version, but not yet persist it in
    // redis nor reset pending services in cluster map. We do that on
    // DMT close, just before moving back to idle state. If shutdown
    // happens between commit and close, we will redo the migration

    // commit DMT
    vp->commitDMT();

    // broadcast DMT to DMs first, once we receive acks, will broadcast
    // to AMs
    dst.commit_acks_to_wait = !om->isInTestMode() ?
              loc_domain->om_bcast_dmt(fpi::FDSP_DATA_MGR, vp->getCommittedDMT()) : 0;

    // there are must be nodes to which we send new DMT
    // unless all failed? -- in that case we should handle errors
    //    fds_verify(dst.commit_acks_to_wait > 0);

    LOGNOTIFY << "DmtDplyFSM::DACT_Commit, Committed DMT to DMs, will wait for "
              << dst.commit_acks_to_wait << " DMT commit acks";

    // Once we're in this state, it means all DMs ongoing staticMigrations have
    // completed. We need to clear the ongoingMigrations list at this point.
    cm->ongoingMigrationDMs.clear();

}


/**
 * GRD_BcastAM
 * -------------
 * @return true if we got all acks for DMT commit from DMs
 */
template <class Evt, class Fsm, class SrcST, class TgtST>
bool
DmtDplyFSM::GRD_BcastAM::operator()(Evt const &evt, Fsm &fsm, SrcST &src, TgtST &dst)
{
    OM_Module* om = OM_Module::om_singleton();
    VolumePlacement* vp = om->om_volplace_mod();
    fds_uint64_t committed_ver = vp->getCommittedDMTVersion();

    // Ignore acks from  non-DMs
    if (evt.svc_type != fpi::FDSP_DATA_MGR) {
        LOGWARN << "Ignoring dmt ack from svc_type: " << evt.svc_type;
        return false;
    }
    // we can possibly have a race condition, where a new non-DM service
    // gets registered while we are in the process of updating DMT
    // and moving to the next DMT version. That node may receive old DMT,
    // and now we have a different committed DMT. That node will also be
    // send a new DMT, but we can still receive a reply for the old version
    // we should ignore ack for the old version
    fds_verify(evt.dmt_version <= committed_ver);
    if (evt.dmt_version != committed_ver) {
        LOGNORMAL << "Received commit acks for old DMT version, acks to wait "
                  << src.commit_acks_to_wait;
        return false;
    }

    // for now assuming commit is always a success
    fds_verify(src.commit_acks_to_wait > 0);
    src.commit_acks_to_wait--;

    bool bret = (src.commit_acks_to_wait == 0);
    LOGNOTIFY << "DmtDplyFSM::GRD_BcastAM, Commit acks to wait from DMs: " << src.commit_acks_to_wait
             << ", returning " << bret;

    return bret;
}

/* DACT_BcastAM
 * ------------
 * We got all acks for commit DMT from DMs. Broadcast DMT to all AMs
 */
template <class Evt, class Fsm, class SrcST, class TgtST>
void
DmtDplyFSM::DACT_BcastAM::operator()(Evt const &evt, Fsm &fsm, SrcST &src, TgtST &dst)
{
    OM_NodeContainer* loc_domain = OM_NodeDomainMod::om_loc_domain_ctrl();
    OM_Module* om = OM_Module::om_singleton();
    VolumePlacement* vp = om->om_volplace_mod();

    // broadcast DMT to AMs
    dst.commit_acks_to_wait = loc_domain->om_bcast_dmt(fpi::FDSP_ACCESS_MGR,
                                                       vp->getCommittedDMT());
    LOGDEBUG << "dst.commit_acts_to_wait should be: " << dst.commit_acks_to_wait
                << " from AMs";
    // broadcast DMT to SMs
    dst.commit_acks_to_wait += loc_domain->om_bcast_dmt(fpi::FDSP_STOR_MGR,
                                                       vp->getCommittedDMT());
    LOGDEBUG << "dst.commit_acts_to_wait should be: " << dst.commit_acks_to_wait
                << " from SMs";

    // ok if there are not AMs to broadcast
    if (dst.commit_acks_to_wait == 0) {
        LOGDEBUG << "Not waiting for any acks, no AMs found...";
        fds_uint64_t committed_ver = vp->getCommittedDMTVersion();
        fsm.process_event(DmtCommitAckEvt(committed_ver, fpi::FDSP_ACCESS_MGR));
    }

    LOGNOTIFY << "Sent DMT to all AMs, will wait for " << dst.commit_acks_to_wait
             << " DMT commit acks";
}


/**
 * GRD_Close
 * -------------
 * @return true if we got all acks for DMT commit to AMs
 */
template <class Evt, class Fsm, class SrcST, class TgtST>
bool
DmtDplyFSM::GRD_Close::operator()(Evt const &evt, Fsm &fsm, SrcST &src, TgtST &dst)
{
    OM_Module* om = OM_Module::om_singleton();
    VolumePlacement* vp = om->om_volplace_mod();
    fds_uint64_t committed_ver = vp->getCommittedDMTVersion();

    LOGNOTIFY << "DMT Ack from svc type: " << evt.svc_type;

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
    // otherwise the ack must be from AMs only
//    fds_verify(evt.svc_type == fpi::FDSP_ACCESS_MGR || evt.svc_type == fpi::FDSP_STOR_MGR);

    // for now assuming commit is always a success
    if (src.commit_acks_to_wait > 0) {
        src.commit_acks_to_wait--;
    }

    bool bret = (src.commit_acks_to_wait == 0);
    LOGNOTIFY << "Commit acks to wait from AM/SM: " << src.commit_acks_to_wait
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
    OM_NodeDomainMod *domain = OM_NodeDomainMod::om_local_domain();
    OM_NodeContainer* loc_domain = OM_NodeDomainMod::om_loc_domain_ctrl();
    OM_Module* om = OM_Module::om_singleton();
    VolumePlacement* vp = om->om_volplace_mod();
    VolumeContainer::pointer volumes = loc_domain->om_vol_mgr();
    fds_uint64_t committed_ver = vp->getCommittedDMTVersion();

    // tell VolumePlacement that AM received all acks, that will set flag
    // 'not rebalancing' which will unblock the volume creation for volumes
    // whose DMT columns were in rebalancing state
    vp->notifyEndOfRebalancing();

    // notify all inactive (delayed) volumes to continue with create/delete process
    volumes->continueCreateDeleteVolumes();

    // broadcast DMT close
    dst.close_acks_to_wait = loc_domain->om_bcast_dmt_close(committed_ver);
    // ok if there are no DMs to send close = no DMs at all
    // TODO(xxx) we should distinguish between that we got an error when
    // sending messages or there are no DMs at all
    if (dst.close_acks_to_wait < 1) {
        fsm.process_event(DmtCloseOkEvt(committed_ver));
    }

    LOGDEBUG << "Sending dmt up event";
    domain->local_domain_event(DltDmtUpEvt(fpi::FDSP_DATA_MGR));

    LOGNOTIFY << "DmtDplyFSM::DACT_Close, Will wait for " << dst.close_acks_to_wait << " DMT close acks";
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
    LOGNOTIFY << "DmtDplyFSM::GRD_Done, Close acks to wait " << src.close_acks_to_wait
              << ", returning " << bret;

    return bret;
}

// DACT_UpdDone
// -----------
template <class Evt, class Fsm, class SrcST, class TgtST>
void
DmtDplyFSM::DACT_UpdDone::operator()(Evt const &evt, Fsm &fsm, SrcST &src, TgtST &dst)
{
    OM_NodeDomainMod *domain = OM_NodeDomainMod::om_local_domain();
    OM_Module* om = OM_Module::om_singleton();
    OM_NodeContainer* loc_domain = OM_NodeDomainMod::om_loc_domain_ctrl();
    VolumePlacement* vp = om->om_volplace_mod();
    ClusterMap* cm = om->om_clusmap_mod();

    // persist commited DMT
    vp->persistCommitedTargetDmt();
    vp->markSuccess();

    // set all added DMs to ACTIVE state
    NodeUuidSet addDms = cm->getAddedServices(fpi::FDSP_DATA_MGR);
    for (auto uuid : addDms) {
        OM_DmAgent::pointer dm_agent = loc_domain->om_dm_agent(uuid);
        dm_agent->handle_service_deployed();
    }

    NodeUuidSet removedDms = cm->getRemovedServices(fpi::FDSP_DATA_MGR);

    for (auto uuid : removedDms) {
        domain->removeNodeComplete(uuid);
    }

    // since we accounted for added/removed nodes in DMT, reset pending nodes in
    // cluster map
    cm->resetPendServices(fpi::FDSP_DATA_MGR);

    LOGNOTIFY << "OM deployed DMT with "
              << cm->getNumMembers(fpi::FDSP_DATA_MGR) << " DMs";

    /**
      * TODO(Neil) - FS-3956
      * We need to have ability to know whether or not DM's SvcUUID
      * in the clustermap is really currently undergoing migration.
      * To do this, we need to start caring for incarnation numbers. If
      * the DM undergoing migration has a newer incarnation number, then that
      * means the DM undergoing migration has crashed and we can throw an error.
      * Otherwise, this should be a no-op.
      * For now, this is conflicting with the DmtDeployEvt that setupNewNode is
      * causing, so disable for now. setupNewNode should be throwing the deploy
      * event fine and dependably.
      */
    // In case new DMs got added or DMs got removed while we were
    // deploying current DMT, start timer to try deploy a DMT again
//    if (!src.tryAgainTimer->schedule(src.tryAgainTimerTask,
//                                     std::chrono::seconds(1))) {
//        LOGWARN << "Failed to start try again timer!!!"
//                << " DM additions/deletions may be pending for long time";
//    }

    LOGNOTIFY << "DM Migration Completed" << "(migrationid: " << vp->getCommittedDMTVersion() << ")";
}

// DACT_Error
// -----------
template <class Evt, class Fsm, class SrcST, class TgtST>
void
DmtDplyFSM::DACT_Error::operator()(Evt const &evt, Fsm &fsm, SrcST &src, TgtST &dst)
{
    LOGNOTIFY << "DACT_Error fired.";
    OM_Module* om = OM_Module::om_singleton();
    VolumePlacement* vp = om->om_volplace_mod();
    fds_bool_t am_dm_needs_dmt_rollback = false;
    if (vp->hasNoTargetDmt()) {
        // if we did not even have target DMT computed, nothing to recover
        // got back to all ok /IDLE state
        LOGNORMAL << "No target DMT computed/commited, nothing to recover";
        fsm.process_event(DmtEndErrorEvt());
    } else {
        OM_NodeDomainMod* domain = OM_NodeDomainMod::om_local_domain();
        OM_NodeContainer* dom_ctrl = domain->om_loc_domain_ctrl();

        fds_uint64_t targetDmtVersion = vp->getTargetDMTVersion();

        if ( OmExtUtilApi::getInstance()->isDMAbortAfterRestartTrue() ) {
            targetDmtVersion = OmExtUtilApi::getInstance()->getDMTargetVersionForAbort();
            LOGDEBUG << "Setting target DMT version for abort to:" << targetDmtVersion;
        }

        // We already computed target DMT, so most likely sent start migration msg
        // Send abort migration to DMs first, so that we can restart migration later
        // (otherwise DMs will think they are still migrating)
        LOGWARN << "Already computed or commited target DMT, will send abort msg "
                << " for target DMT version " << targetDmtVersion;

        // This flag is set only if OM came up after a restart and found it
        // was interrupted during a DMT computation. In this case, we do not
        // want to do undoTarget. TargetDmt/committedDmt values
        // are set correctly though om_load_state( ::loadDmtsFromConfigDb , ::commitDmt).
        // We prevented targetVersion in DMTmgr from being cleared out to enter this
        // error mode, checks in undoTarget will falsely assume the target has been committed
        // and take action. Target version will be explicitly cleared out in the end of error mode
        if ( !OmExtUtilApi::getInstance()->isDMAbortAfterRestartTrue() ) {
            // Revert to previously committed DMT locally in OM
            am_dm_needs_dmt_rollback = vp->undoTargetDmtCommit();
        }

        fds_uint32_t abortCnt = dom_ctrl->om_bcast_dm_migration_abort(vp->getCommittedDMTVersion());
        dst.abortMigrAcksToWait = 0;
        if (abortCnt > 0) {
            dst.abortMigrAcksToWait = abortCnt;
        }

        // we are going to wait for acks so that we do not go to idle/all ok state too soon
        // but we are going to ignore acks with errors/ timeouts
        // We will revert volume placement and persistent state right now without waiting
        // for abort migration acks and send DMT commit for previously committed DMT to AMs

        /**
         * TODO(Neil) FS-3600 part II - in case of network error, the abort migration to DMT may fail.
         * In this case, the DM that didn't receive the abort may think that everyone else is on the
         * previously targeted DMT, and the cluster will be in a split brain issue.
         * Need to think about how to solve this later.
         */

        // send DMT commit to AMs and SMs if target was committed
        fds_uint32_t commitCnt = 0;
        if (am_dm_needs_dmt_rollback) {
            LOGDEBUG << "AM and SM has already committed target DMT. Need to roll them back.";
            // has target DMT (see the first if) and it is commited
            commitCnt = dom_ctrl->om_bcast_dmt(fpi::FDSP_ACCESS_MGR,
                                               vp->getCommittedDMT());
            commitCnt += dom_ctrl->om_bcast_dmt(fpi::FDSP_STOR_MGR,
                                                vp->getCommittedDMT());
            dst.commitDmtAcksToWait = 0;
            if (commitCnt > 0) {
                dst.commitDmtAcksToWait = commitCnt;
            }
            LOGNORMAL << "Sent DMT commit to " << commitCnt << " nodes, will wait for resp";
        }

        // see if we already recovered or need to wait for acks
        if ((abortCnt < 1) && (commitCnt < 1)) {
            // we already recovered
            LOGNORMAL << "No services that need abort migration or previous DMT";
            fsm.process_event(DmtEndErrorEvt());
        }
    }
}

// DACT_EndError
// --------------
// End of error state
template <class Evt, class Fsm, class SrcST, class TgtST>
void
DmtDplyFSM::DACT_EndError::operator()(Evt const &evt, Fsm &fsm, SrcST &src, TgtST &dst)
{
    LOGNOTIFY << "DACT_EndError";
    // End of error handling for FSM. Not balancing volume anymore so turn it off.
    OM_Module* om = OM_Module::om_singleton();

    VolumePlacement* vp = om->om_volplace_mod();
    vp->notifyEndOfRebalancing();
    vp->markFailure();

    ClusterMap* cm = om->om_clusmap_mod();
    cm->ongoingMigrationDMs.clear();

    if ( OmExtUtilApi::getInstance()->isDMAbortAfterRestartTrue() ) {
        vp->clearTargetDmt();

        OmExtUtilApi::getInstance()->clearDMAbortParams();
    }

    if (vp->canRetryMigration()) {
        LOGNOTIFY << "Migration has failed " << vp->failedAttempts() << " times.";
        if (!dst.tryAgainTimer->schedule(dst.tryAgainTimerTask,
            std::chrono::seconds(1))) {
            LOGWARN << "DACT_EndError: failed to start retry timer!!!"
                    << " DM migration may need manual intervention!";
        }
    } else {
        LOGERROR << "Migration has failed too many times. Manually inspect and "
                << " remove failed DMs and re-add them to initiate another migration";
    }
}

// DACT_ChkEndErr
// --------------
// Start recover from error state
//
template <class Evt, class Fsm, class SrcST, class TgtST>
void
DmtDplyFSM::DACT_ChkEndErr::operator()(Evt const &evt, Fsm &fsm, SrcST &src, TgtST &dst)
{
    DmtRecoveryEvt recoverAckEvt = (DmtRecoveryEvt)evt;
    FdspNodeType node_type = recoverAckEvt.svcUuid.uuid_get_type();
    LOGNOTIFY << "DACT_EndError ack for abort migration? " << recoverAckEvt.ackForAbort
             << " node type " << node_type << " " << recoverAckEvt.ackError;

    // if we got SL timeout for one of the nodes we were trying to add to DMT
    // most likely that node is down.. for now mark as down
    if ((node_type == fpi::FDSP_DATA_MGR) &&
        ((recoverAckEvt.ackError == ERR_SVC_REQUEST_TIMEOUT) ||
         (recoverAckEvt.ackError == ERR_SVC_REQUEST_INVOCATION))) {
        OM_Module *om = OM_Module::om_singleton();
        ClusterMap* cm = om->om_clusmap_mod();
        NodeUuidSet addedDms = cm->getAddedServices(fpi::FDSP_DATA_MGR);
        NodeUuidSet resyncDMs = cm->getDmResyncServices();
        LOGNORMAL << "DM timeout in SL, node uuid " << std::hex
                  << recoverAckEvt.svcUuid.uuid_get_val() << std::dec
                  << " ; we had " << addedDms.size() << " added DMs";
        for (NodeUuidSet::const_iterator cit = addedDms.cbegin();
             cit != addedDms.cend();
             ++cit) {
            if (*cit == recoverAckEvt.svcUuid) {
                LOGWARN << "Looks like DM that we tried to add to DMT is down, "
                        << " setting it's state to down: node uuid " << std::hex
                        << recoverAckEvt.svcUuid.uuid_get_val() << std::dec;
                OM_NodeDomainMod* domain = OM_NodeDomainMod::om_local_domain();
                OM_SmAgent::pointer dm_agent = domain->om_dm_agent(recoverAckEvt.svcUuid);
                dm_agent->set_node_state(fpi::FDS_Node_Down);
                cm->rmPendingAddedService(fpi::FDSP_DATA_MGR, recoverAckEvt.svcUuid);
                break;
            }
        }
        for (NodeUuidSet::const_iterator cit = resyncDMs.cbegin();
                cit != resyncDMs.cend(); ++cit) {
             if (*cit == recoverAckEvt.svcUuid) {
                LOGWARN << "Looks like DM that we tried to resync to DMT is down, "
                        << " setting it's state to down: node uuid " << std::hex
                        << recoverAckEvt.svcUuid.uuid_get_val() << std::dec;
                OM_NodeDomainMod* domain = OM_NodeDomainMod::om_local_domain();
                OM_SmAgent::pointer dm_agent = domain->om_dm_agent(recoverAckEvt.svcUuid);
                dm_agent->set_node_state(fpi::FDS_Node_Down);
                cm->rmPendingAddedService(fpi::FDSP_DATA_MGR, recoverAckEvt.svcUuid);
                break;
             }
        }
    }

    if (recoverAckEvt.ackForAbort) {
        fds_verify(src.abortMigrAcksToWait > 0);
        --src.abortMigrAcksToWait;
    } else {
        if (src.commitDmtAcksToWait > 0) {
            --src.commitDmtAcksToWait;
        }
    }

    if (src.commitDmtAcksToWait == 0 && src.abortMigrAcksToWait == 0) {
        LOGNOTIFY << "Receivied all acks for abort migration and revert DMT";
        fsm.process_event(DmtEndErrorEvt());
    }
}


// DACT_Recovered
// --------------
// Finished recovering from error state
template <class Evt, class Fsm, class SrcST, class TgtST>
void
DmtDplyFSM::DACT_Recovered::operator()(Evt const &evt, Fsm &fsm, SrcST &src, TgtST &dst)
{
    LOGNOTIFY << "DACT_Recovered";
}

}  // namespace fds
