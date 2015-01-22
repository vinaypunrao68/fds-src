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
#include <NetSession.h>
#include <OmDeploy.h>
#include <OmDmtDeploy.h>
#include <OmResources.h>
#include <OmDataPlacement.h>
#include <OmVolumePlacement.h>
#include <fds_process.h>

#define OM_WAIT_NODES_UP_SECONDS   (5*60)
#define OM_WAIT_START_SECONDS      1

// temp for testing
// #define TEST_DELTA_RSYNC

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

        template <class Event, class FSM> void on_entry(Event const &e, FSM &f) {
            LOGDEBUG << "DST_Start. Evt: " << e.logString();
        }
        template <class Event, class FSM> void on_exit(Event const &e, FSM &f) {
            LOGDEBUG << "DST_Start. Evt: " << e.logString();
        }
    };
    struct DST_Wait : public msm::front::state<>
    {
        DST_Wait() : waitTimer(new FdsTimer()),
                     waitTimerTask(new WaitTimerTask(*waitTimer)) {}

        ~DST_Wait() {
            waitTimer->destroy();
        }

        template <class Evt, class Fsm, class State>
        void operator()(Evt const &, Fsm &, State &) {}

        template <class Event, class FSM> void on_entry(Event const &e, FSM &f) {
            LOGDEBUG << "DST_Wait. Evt: " << e.logString();
        }
        template <class Event, class FSM> void on_exit(Event const &e, FSM &f) {
            LOGDEBUG << "DST_Wait. Evt: " << e.logString();
        }

        /**
         * timer to come out of this state
         */
        FdsTimerPtr waitTimer;
        FdsTimerTaskPtr waitTimerTask;
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

        template <class Event, class FSM> void on_entry(Event const &e, FSM &f) {
            LOGDEBUG << "DST_WaitNds. Evt: " << e.logString();
        }
        template <class Event, class FSM> void on_exit(Event const &e, FSM &f) {
            LOGDEBUG << "DST_WaitNds. Evt: " << e.logString();
        }

        NodeUuidSet sm_services;  // sm services we are waiting to come up
        NodeUuidSet sm_up;  // sm services that are already up
        NodeUuidSet dm_services;  // dm services we are waiting to come up
        NodeUuidSet dm_up;  // dm services that are already up

        /**
         * timer to come out of this state if we don't get all SMs up
         */
        FdsTimerPtr waitTimer;
        FdsTimerTaskPtr waitTimerTask;
    };
    struct DST_WaitDltDmt : public msm::front::state<>
    {
        DST_WaitDltDmt() {
            dlt_up = false;
            dmt_up = false;
        }
        template <class Evt, class Fsm, class State>
        void operator()(Evt const &, Fsm &, State &) {}

        template <class Event, class FSM> void on_entry(Event const &e, FSM &f) {
            LOGDEBUG << "DST_WaitDltDmt. Evt: " << e.logString();
        }
        template <class Event, class FSM> void on_exit(Event const &e, FSM &f) {
            LOGDEBUG << "DST_WaitDltDmt. Evt: " << e.logString();
        }

        bool dlt_up;
        bool dmt_up;
    };
    struct DST_DomainUp : public msm::front::state<>
    {
        typedef mpl::vector1<LocalDomainUp> flag_list;

        template <class Evt, class Fsm, class State>
        void operator()(Evt const &, Fsm &, State &) {}

        template <class Event, class FSM> void on_entry(Event const &e, FSM &f) {
            LOGDEBUG << "DST_DomainUp. Evt: " << e.logString();
        }
        template <class Event, class FSM> void on_exit(Event const &e, FSM &f) {
            LOGDEBUG << "DST_DomainUp. Evt: " << e.logString();
        }
    };

    /**
     * Define the initial state.
     */
    typedef DST_Start initial_state;
    struct MyInitEvent {
        std::string logString() const {
            return "MyInitEvent";
        }
    };
    typedef MyInitEvent initial_event;

    /**
     * Transition actions.
     */
    struct DACT_Wait
    {
        template <class Evt, class Fsm, class SrcST, class TgtST>
        void operator()(Evt const &, Fsm &, SrcST &, TgtST &);
    };
    struct DACT_WaitDone
    {
        template <class Evt, class Fsm, class SrcST, class TgtST>
        void operator()(Evt const &, Fsm &, SrcST &, TgtST &);
    };
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
    struct GRD_DltDmtUp
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
        msf::Row< DST_Start  , NoPersistEvt, DST_Wait    , DACT_Wait    , msf::none   >,
        // +-----------------+-------------+-------------+--------------+--------------+
        msf::Row< DST_Wait   , TimeoutEvt  , DST_DomainUp, DACT_WaitDone, msf::none   >,
        // +-----------------+-------------+-------------+--------------+--------------+
        msf::Row< DST_WaitNds, RegNodeEvt  ,DST_WaitDltDmt, DACT_NodesUp , GRD_NdsUp   >,  // NOLINT
        msf::Row< DST_WaitNds, TimeoutEvt  ,DST_WaitDltDmt, DACT_UpdDlt , GRD_EnoughNds>,  // NOLINT
        // +-----------------+-------------+------------+---------------+--------------+
        msf::Row<DST_WaitDltDmt, DltDmtUpEvt, DST_DomainUp, DACT_LoadVols, GRD_DltDmtUp >
        // +------------------+-------------+------------+---------------+-------------+
        >{};  // NOLINT

    template <class Event, class FSM> void no_transition(Event const &, FSM &, int);
};

void NodeDomainFSM::WaitTimerTask::runTimerTask()
{
    OM_NodeDomainMod* domain = OM_NodeDomainMod::om_local_domain();
    LOGNOTIFY << "Timeout";
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
    LOGDEBUG << "Evt: " << evt.logString() << " NodeDomainFSM no transition";
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
    } else if (src.dm_services.count(regEvt.svc_uuid) > 0) {
        src.dm_up.insert(regEvt.svc_uuid);
    }

    if (src.sm_services.size() == src.sm_up.size() &&
        src.dm_services.size() == src.dm_up.size()) {
        b_ret = true;
    }

    LOGDEBUG << "GRD_NdsUp: register svc uuid " << std::hex
             << regEvt.svc_uuid.uuid_get_val() << std::dec << "; waiting for "
             << src.sm_services.size() << " Sms, already got "
             << src.sm_up.size() << " SMs; waiting for "
             << src.dm_services.size() << " Dms, already got "
             << src.dm_up.size() << " DMs; returning " << b_ret;

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
    OM_DMTMod *dmtMod = om->om_dmt_mod();
    ClusterMap *cm = om->om_clusmap_mod();

    LOGDEBUG << "NodeDomainFSM DACT_NodesUp";

    // cancel timeout timer
    src.waitTimer->cancel(src.waitTimerTask);

    // start DLT, DMT deploy from the point where we ready to commit

    // cluster map must not have pending nodes, but remember that
    // sm nodes in container may have pending nodes that are already in DLT
    fds_verify((cm->getAddedServices(fpi::FDSP_STOR_MGR)).size() == 0);
    fds_verify((cm->getRemovedServices(fpi::FDSP_STOR_MGR)).size() == 0);
    dltMod->dlt_deploy_event(DltLoadedDbEvt());

    // cluster map must not have pending nodes, but remember that
    // dm nodes in container may have pending nodes that are already in DMT
    fds_verify((cm->getAddedServices(fpi::FDSP_DATA_MGR)).size() == 0);
    fds_verify((cm->getRemovedServices(fpi::FDSP_DATA_MGR)).size() == 0);
    dmtMod->dmt_deploy_event(DmtLoadedDbEvt());

    // move nodes that are already in DLT from pending list
    // to cluster map (but make them not pending in cluster map
    // because they are already in DLT), so that we will not try to update DLT
    // with those nodes again
    OM_SmContainer::pointer smNodes = local->om_sm_nodes();
    NodeList addNodes, rmNodes;
    smNodes->om_splice_nodes_pend(&addNodes, &rmNodes, src.sm_up);
    cm->updateMap(fpi::FDSP_STOR_MGR, addNodes, rmNodes);
    // since updateMap keeps those nodes as pending, we tell cluster map that
    // they are not pending, since these nodes are already in the DLT
    cm->resetPendServices(fpi::FDSP_STOR_MGR);

    // move nodes that are already in DMT from pending list
    // to cluster map (but make them not pending in cluster map
    // because they are already in DMT), so that we will not try to update DMT
    // with those nodes again
    OM_DmContainer::pointer dmNodes = local->om_dm_nodes();
    addNodes.clear();
    rmNodes.clear();
    dmNodes->om_splice_nodes_pend(&addNodes, &rmNodes, src.dm_up);
    cm->updateMap(fpi::FDSP_DATA_MGR, addNodes, rmNodes);
    // since updateMap keeps those nodes as pending, we tell cluster map that
    // they are not pending, since these nodes are already in the DMT
    cm->resetPendServices(fpi::FDSP_DATA_MGR);
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

    // also send all known stream registrations
    OM_NodeContainer *local = OM_NodeDomainMod::om_loc_domain_ctrl();
    local->om_bcast_stream_register_cmd(0, true);
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
    dst.dm_services.swap(waitEvt.dm_services);

    LOGDEBUG << "NodeDomainFSM DACT_WaitNds: will wait for " << dst.sm_services.size()
             << " SM(s) to come up and " << dst.dm_services.size() << " << DM(s) to come up";

    // start timer to timeout in case services do not come up
    if (!dst.waitTimer->schedule(dst.waitTimerTask,
                                 std::chrono::seconds(OM_WAIT_NODES_UP_SECONDS))) {
        fds_verify(!"Failed to schedule timer");
        // couldn't start timer, so don't wait for services to come up
        LOGWARN << "DACT_WaitNds: failed to start timer -- will not wait for "
                << "services to come up";
        fsm.process_event(TimeoutEvt());
    }
}

/**
 * DACT_Wait
 * ----------
 * Action when there is no persistent state to bring up, but we will still
 * wait several min before we say domain is up, so that when several SMs
 * come up together in the beginning, DLT computation will happen for all of
 * them at once without any migration and syncing
 */
template <class Evt, class Fsm, class SrcST, class TgtST>
void
NodeDomainFSM::DACT_Wait::operator()(Evt const &evt, Fsm &fsm, SrcST &src, TgtST &dst)
{
    LOGDEBUG << "NodeDomainFSM DACT_Wait: initial wait for " << OM_WAIT_START_SECONDS
             << " seconds before start DLT compute for new SMs";

    // start timer to timeout in case services do not come up
    if (!dst.waitTimer->schedule(dst.waitTimerTask,
                                 std::chrono::seconds(OM_WAIT_START_SECONDS))) {
        // couldn't start timer, so don't wait for services to come up
        LOGWARN << "DACT_WaitNds: failed to start timer -- will not wait before "
                << " before admitting new SMs into DLT";
        fsm.process_event(TimeoutEvt());
    }
}

/**
 * DACT_WaitDone
 * --------------
 * When there is no persistent state to bring up, but we still
 * wait several min before we say domain is up. This action handles timeout
 * that we are done waiting and try to update cluster (compute DLT if any
 * SMs joines so far).
 */
template <class Evt, class Fsm, class SrcST, class TgtST>
void
NodeDomainFSM::DACT_WaitDone::operator()(Evt const &evt, Fsm &fsm, SrcST &src, TgtST &dst)
{
    LOGDEBUG << "NodeDomainFSM DACT_WaitDone: will try to compute DLT if any SMs joined";

    // start cluster update process that will recompute DLT /rebalance /etc
    // so that we move to DLT that reflects actual nodes that came up
    OM_NodeDomainMod *domain = OM_NodeDomainMod::om_local_domain();
    domain->om_dlt_update_cluster();
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
    // We expect the same nodes to come up that were up prior to restart.
    // Until proper restart is implemented we will not proceed when only
    // some of the nodes comeup.
    fds_verify(!"Timer expired while wating for nodes to come up in restart");

    // proceed if more than half nodes are up
    fds_uint32_t total_sms = src.sm_services.size();
    fds_uint32_t up_sms = src.sm_up.size();
    fds_verify(total_sms > up_sms);
    fds_uint32_t wait_more = total_sms - up_sms;
    if ((total_sms > 2) && (wait_more < total_sms/2)) {
        wait_more = 0;
    }

    if (wait_more == 0) {
        LOGNOTIFY << "GRD_EnoughNds: " << up_sms << " SMs out of " << total_sms
                  << " SMs are up, will proceed without waiting for remaining SMs";
        return true;
    }

    // first print out the nodes were are waiting for
    LOGWARN << "WARNING: OM IS NOT UP: OM is waiting for at least "
            << wait_more << " more nodes to register (!!!) out of the following:";
    for (NodeUuidSet::const_iterator cit = src.sm_services.cbegin();
         cit != src.sm_services.cend();
         ++cit) {
        if (src.sm_up.count(*cit) == 0) {
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

    return false;
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

    // at this point there should be no pending add/rm nodes in cluster map
    fds_verify((cm->getAddedServices(fpi::FDSP_STOR_MGR)).size() == 0);
    fds_verify((cm->getRemovedServices(fpi::FDSP_STOR_MGR)).size() == 0);

    // move nodes that came up (which are already in DLT) from pending nodes in
    // SM container to cluster map
    OM_SmContainer::pointer smNodes = dom_ctrl->om_sm_nodes();
    NodeList addNodes, rmNodes;
    smNodes->om_splice_nodes_pend(&addNodes, &rmNodes, src.sm_up);
    cm->updateMap(fpi::FDSP_STOR_MGR, addNodes, rmNodes);
    // since updateMap keeps those nodes as pending, we tell cluster map that
    // they are not pending, since these nodes are already in the DLT
    cm->resetPendServices(fpi::FDSP_STOR_MGR);

    // set SMs that did not come up as 'delete pending' in cluster map
    // -- this will tell DP to remove those nodes from DLT
    for (NodeUuidSet::const_iterator cit = src.sm_services.cbegin();
         cit != src.sm_services.cend();
         ++cit) {
        if (src.sm_up.count(*cit) == 0) {
            fds_assert(!"Shouldn't happen");
            LOGDEBUG << "DACT_UpdDlt: will remove node " << std::hex
                     << (*cit).uuid_get_val() << std::dec << " from DLT";
            cm->addPendingRmService(fpi::FDSP_STOR_MGR, *cit);

            // also remove that node from configDB
            domain->om_rm_sm_configDB(*cit);
        }
    }
    fds_verify((cm->getRemovedServices(fpi::FDSP_STOR_MGR)).size() > 0);

    // start cluster update process that will recompute DLT /rebalance /etc
    // so that we move to DLT that reflects actual nodes that came up
    domain->om_dlt_update_cluster();
}


/**
 * GRD_DltDmtUp
 * ------------
 * Checks if dlt and dmt are propagated to services
 */
template <class Evt, class Fsm, class SrcST, class TgtST>
bool
NodeDomainFSM::GRD_DltDmtUp::operator()(Evt const &evt, Fsm &fsm, SrcST &src, TgtST &dst)
{
    fds_bool_t b_ret = false;
    if (evt.svc_type == fpi::FDSP_STOR_MGR) {
        fds_assert(src.dlt_up == false);
        src.dlt_up = true;
    } else if (evt.svc_type == fpi::FDSP_DATA_MGR) {
        fds_assert(src.dmt_up == false);
        src.dmt_up = true;
    }
    LOGDEBUG << "GRD_DltDmtUp dlt up: " << src.dlt_up << " dmt up: " << src.dmt_up;

    if (src.dlt_up && src.dmt_up) {
        return true;
    } else {
        return false;
    }
}
//--------------------------------------------------------------------
// OM Node Domain
//--------------------------------------------------------------------
OM_NodeDomainMod::OM_NodeDomainMod(char const *const name)
        : Module(name),
          fsm_lock("OM_NodeDomainMod fsm lock")
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
    fds_mutex::scoped_lock l(om_local_domain()->fsm_lock);
    return om_local_domain()->domain_fsm->is_flag_active<LocalDomainUp>();
}

// domain_event
// ------------
//
void OM_NodeDomainMod::local_domain_event(WaitNdsEvt const &evt) {
    fds_mutex::scoped_lock l(fsm_lock);
    domain_fsm->process_event(evt);
}
void OM_NodeDomainMod::local_domain_event(DltDmtUpEvt const &evt) {
    fds_mutex::scoped_lock l(fsm_lock);
    domain_fsm->process_event(evt);
}
void OM_NodeDomainMod::local_domain_event(RegNodeEvt const &evt) {
    fds_mutex::scoped_lock l(fsm_lock);
    domain_fsm->process_event(evt);
}
void OM_NodeDomainMod::local_domain_event(TimeoutEvt const &evt) {
    fds_mutex::scoped_lock l(fsm_lock);
    domain_fsm->process_event(evt);
}
void OM_NodeDomainMod::local_domain_event(NoPersistEvt const &evt) {
    fds_mutex::scoped_lock l(fsm_lock);
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
    VolumePlacement* vp = om->om_volplace_mod();

    LOGNOTIFY << "Loading persistent state to local domain";
    configDB = _configDB;  // cache config db in local domain

    // get SMs that were up before and persistent in config db
    // if no SMs were up, even if platforms were running, we
    // don't need to wait for the platforms/DMs/AMs to come up
    // since we cannot admit any volumes anyway. So, we will go
    // directly to 'domain up' state if no SMs were running...
    NodeUuidSet sm_services;
    NodeUuidSet dm_services;
    if (configDB) {
        NodeUuidSet nodes;  // actual nodes (platform)
        configDB->getNodeIds(nodes);

        // get set of SMs and DMs that were running on those nodes
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
                if (services.dm.uuid_get_val() != 0) {
                    dm_services.insert(services.dm);
                    LOGDEBUG << "om_load_state: found DM on node "
                             << std::hex << (*cit).uuid_get_val() << " (DM "
                             << services.dm.uuid_get_val() << std::dec << ")";
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

        // Load DMT
        err = vp->loadDmtsFromConfigDB(dm_services);
        if (!err.ok()) {
            LOGERROR << "Persistent state mismatch, error " << err.GetErrstr()
                     << std::endl << "OM will stay DOWN! Kill OM, cleanup "
                     << "persistent state and restart cluster again";
            return err;
        }
        fds_verify((sm_services.size() == 0 && dm_services.size() == 0) ||
                   (sm_services.size() > 0 && dm_services.size() > 0));
    }

    if (sm_services.size() > 0) {
        LOGNOTIFY << "Will wait for " << sm_services.size()
                  << " SMs and " << dm_services.size()
                  << " DMs to come up within next few minutes";
        local_domain_event(WaitNdsEvt(sm_services, dm_services));
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
    for (auto& volume : vecVolumes) {
        LOGDEBUG << "processing volume "
                 << "[" << volume.volUUID << ":" << volume.name << "]";

        if (volume.isStateDeleted()) {
            LOGWARN << "not loading deleted volume : "
                    << "[" << volume.volUUID << ":" << volume.name << "]";
        } else if (!om_locDomain->addVolume(volume)) {
            LOGERROR << "unable to add volume "
                     << "[" << volume.volUUID << ":" << volume.name << "]";
        }
    }

    // load snapshots
    std::vector<fpi::Snapshot> vecSnapshots;
    VolumeContainer::pointer volContainer = om_locDomain->om_vol_mgr();
    for (const auto& volumeDesc : vecVolumes) {
        vecSnapshots.clear();
        configDB->listSnapshots(vecSnapshots, volumeDesc.volUUID);
        LOGDEBUG << "loading [" << vecSnapshots.size()
                 <<"] snapshots for volume:" << volumeDesc.volUUID;
        for (const auto& snapshot : vecSnapshots) {
            volContainer->addSnapshot(snapshot);
        }
    }

    return err;
}

fds_bool_t
OM_NodeDomainMod::om_rm_sm_configDB(const NodeUuid& uuid)
{
    fds_bool_t found = false;
    NodeUuid plat_uuid;

    // this is a bit painful because we need to read all the nodes
    // from configDB and find which node this SM belongs to
    // but ok because this operation happens very rarely
    std::unordered_set<NodeUuid, UuidHash> nodes;
    if (!configDB->getNodeIds(nodes)) {
        LOGWARN << "Failed to get nodes' infos from configDB, will not remove SM "
                << std::hex << uuid.uuid_get_val() << std::dec << " from configDB";
        return false;
    }

    NodeServices services;
    for (std::unordered_set<NodeUuid, UuidHash>::const_iterator cit = nodes.cbegin();
         cit != nodes.cend();
         ++cit) {
        if (configDB->getNodeServices(*cit, services)) {
            if (services.sm == uuid) {
                found = true;
                plat_uuid = *cit;
                break;
            }
        }
    }

    if (found) {
        services.sm = NodeUuid();
        configDB->setNodeServices(plat_uuid, services);
    } else {
        LOGWARN << "Failed to find platform for SM " << std::hex << uuid.uuid_get_val()
                << std::dec << "; will not remove SM in configDB";
        return false;
    }

    return true;
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

    LOGNORMAL << "OM recv reg node uuid " << std::hex
        << msg->node_uuid.uuid << ", svc uuid " << msg->service_uuid.uuid
        << std::dec << ", type " << msg->node_type << ", ip "
        << netSession::ipAddr2String(msg->ip_lo_addr) << ", ctrl port "
        << msg->control_port;

    if ((msg->node_type == fpi::FDSP_STOR_MGR) ||
        (msg->node_type == fpi::FDSP_DATA_MGR)) {
        // we must have a node (platform) that runs any service
        // registered with OM and node must be in active state
        if (!pmNodes->check_new_service((msg->node_uuid).uuid, msg->node_type)) {
            LOGERROR << "Error: cannot register service " << msg->node_name
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
            if (!err.ok()) {
                LOGWARN << "handler_register_service returned error: " << err
                    << " type:" << msg->node_type;
            }
        }


        // since we already checked above that we could add service, verify error ok
        // Vy: we could get duplicate if the agent already reigstered by platform lib.
        // fds_verify(err.ok());

        om_locDomain->om_bcast_new_node(newNode, msg);

        // Let this new node know about exisiting node list.
        // TODO(Andrew): this should change into dissemination of the cur cluster map.
        //
        if (msg->node_type == fpi::FDSP_STOR_MGR) {
            // Activate and account node capacity only when SM registers with OM.
            //
            NodeAgent::pointer pm = pmNodes->agent_info(NodeUuid(msg->node_uuid.uuid));
            if (pm != NULL) {
                om_locDomain->om_update_capacity(pm, true);
            } else {
                LOGERROR << "Can not find platform agent for node uuid "
                         << std::hex << msg->node_uuid.uuid << std::dec;
            }
        } else if (msg->node_type == fpi::FDSP_DATA_MGR) {
            om_locDomain->om_bcast_stream_reg_list(newNode);
        }
        om_locDomain->om_update_node_list(newNode, msg);

        if (msg->node_type != fpi::FDSP_DATA_MGR) {
            om_locDomain->om_bcast_vol_list(newNode);
            // for new DMs, we send volume list as part of DMT deploy state machine
        }

        // send qos related info to this node
        om_locDomain->om_send_me_qosinfo(newNode);

        // Let this new node know about existing dlt if this is not SM node
        // DLT deploy state machine will take care of SMs
        // TODO(Andrew): this should change into dissemination of the cur cluster map.
        if (msg->node_type != fpi::FDSP_STOR_MGR) {
            OM_Module *om = OM_Module::om_singleton();
            DataPlacement *dp = om->om_dataplace_mod();
            OM_SmAgent::agt_cast_ptr(newNode)->om_send_dlt(dp->getCommitedDlt());
        }


        if (om_local_domain_up()) {
            if (msg->node_type == fpi::FDSP_STOR_MGR) {
                om_dlt_update_cluster();
            }
            // Send the DMT to DMs.
            if (msg->node_type == fpi::FDSP_DATA_MGR) {
                om_dmt_update_cluster();
            } else {
                OM_Module *om = OM_Module::om_singleton();
                VolumePlacement* vp = om->om_volplace_mod();
                if (vp->hasCommittedDMT()) {
                    OM_NodeAgent::agt_cast_ptr(newNode)->om_send_dmt(vp->getCommittedDMT());
                } else {
                    LOGWARN << "Not sending DMT to new node, because no "
                        << " committed DMT yet";
                }
            }
        } else {
            local_domain_event(RegNodeEvt(uuid, msg->node_type));
        }
    }
    return err;
}

// om_del_services
// ----------------
//
Error
OM_NodeDomainMod::om_del_services(const NodeUuid& node_uuid,
                                  const std::string& node_name,
                                  fds_bool_t remove_sm,
                                  fds_bool_t remove_dm,
                                  fds_bool_t remove_am)
{
    Error err(ERR_OK);
    OM_PmContainer::pointer pmNodes = om_locDomain->om_pm_nodes();
    // make sure that platform agents do not hold references to this node
    // and unregister service resources
    NodeUuid uuid;
    if (remove_sm) {
        uuid = pmNodes->
            handle_unregister_service(node_uuid, node_name, fpi::FDSP_STOR_MGR);
        if (uuid.uuid_get_val() != 0) {
            // dc_unregister_service requires node name and checks if uuid matches service
            // name, however handle_unregister_service returns svc uuid only if there is
            // one service with the same name, so ok if we get name first
            OM_SmAgent::pointer smAgent = om_sm_agent(uuid);

            // remove this SM's capacity from total capacity
            NodeAgent::pointer pm = pmNodes->agent_info(node_uuid);
            if (pm != NULL) {
                om_locDomain->om_update_capacity(pm, false);
            } else {
                LOGERROR << "Can not find platform agent for node uuid "
                         << std::hex << node_uuid.uuid_get_val() << std::dec
                         << ", will not update total domain capacity on SM removal";
            }

            // unregister SM
            err = om_locDomain->dc_unregister_node(uuid, smAgent->get_node_name());
            LOGDEBUG << "Unregistered SM service for node " << node_name
                     << ":" << std::dec << node_uuid.uuid_get_val() << std::hex
                     << " result: " << err.GetErrstr();
        }
        if (om_local_domain_up()) {
            om_dlt_update_cluster();
        }
    }
    if (err.ok() && remove_dm) {
        uuid = pmNodes->
            handle_unregister_service(node_uuid, node_name, fpi::FDSP_DATA_MGR);
        if (uuid.uuid_get_val() != 0) {
            OM_DmAgent::pointer dmAgent = om_dm_agent(uuid);
            err = om_locDomain->dc_unregister_node(uuid, dmAgent->get_node_name());
            LOGDEBUG << "Unregistered DM service for node " << node_name
                     << ":" << std::dec << node_uuid.uuid_get_val() << std::hex
                     << " result: " << err.GetErrstr();
        }
        om_dmt_update_cluster();
    }
    if (err.ok() && remove_am) {
        uuid = pmNodes->
            handle_unregister_service(node_uuid, node_name, fpi::FDSP_STOR_HVISOR);
        if (uuid.uuid_get_val() != 0) {
            OM_AmAgent::pointer amAgent = om_am_agent(uuid);
            err = om_locDomain->dc_unregister_node(uuid, amAgent->get_node_name());
            LOGDEBUG << "Unregistered AM service for node " << node_name
                     << ":" << std::dec << node_uuid.uuid_get_val() << std::hex
                     << " result " << err.GetErrstr();
        }
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

void
OM_NodeDomainMod::om_dmt_update_cluster() {
    OM_Module *om = OM_Module::om_singleton();
    OM_DMTMod *dmtMod = om->om_dmt_mod();

    dmtMod->dmt_deploy_event(DmtDeployEvt());

    // in case there are no vol acks to wait
    dmtMod->dmt_deploy_event(DmtVolAckEvt(NodeUuid()));
}
void
OM_NodeDomainMod::om_dmt_waiting_timeout() {
    OM_Module *om = OM_Module::om_singleton();
    OM_DMTMod *dmtMod = om->om_dmt_mod();

    dmtMod->dmt_deploy_event(DmtTimeoutEvt());
    // in case there are no vol acks to wait
    dmtMod->dmt_deploy_event(DmtVolAckEvt(NodeUuid()));
}


/**
 * Drives the DLT deployment state machine.
 */
void
OM_NodeDomainMod::om_dlt_update_cluster() {
    OM_Module *om = OM_Module::om_singleton();
    OM_DLTMod *dltMod = om->om_dlt_mod();
    DataPlacement *dp = om->om_dataplace_mod();

    // this will check if we need to compute DLT
    dltMod->dlt_deploy_event(DltComputeEvt());

    // in case there was no DLT to send and we can
    // go to rebalances state, send event to check that
    const DLT* dlt = dp->getCommitedDlt();
    fds_uint64_t dlt_version = (dlt == NULL) ? 0 : dlt->getVersion();
    dltMod->dlt_deploy_event(DltCommitOkEvt(dlt_version, NodeUuid()));
}

// Called when DLT state machine waiting ends
void
OM_NodeDomainMod::om_dlt_waiting_timeout() {
    OM_Module *om = OM_Module::om_singleton();
    OM_DLTMod *dltMod = om->om_dlt_mod();
    DataPlacement *dp = om->om_dataplace_mod();
    dltMod->dlt_deploy_event(DltTimeoutEvt());

    const DLT* dlt = dp->getCommitedDlt();
    fds_uint64_t dlt_version = (dlt == NULL) ? 0 : dlt->getVersion();
    dltMod->dlt_deploy_event(DltCommitOkEvt(dlt_version, NodeUuid()));
}

void
OM_NodeDomainMod::om_service_down(const Error& error,
                                  const NodeUuid& svcUuid) {
    OM_Module *om = OM_Module::om_singleton();
    OM_DLTMod *dltMod = om->om_dlt_mod();
    OM_DMTMod *dmtMod = om->om_dmt_mod();
    dltMod->dlt_deploy_event(DltErrorFoundEvt(svcUuid, error));
    dmtMod->dmt_deploy_event(DmtErrorFoundEvt(svcUuid, error));
}

// Called when OM receives notification that the rebalance is
// done on node with uuid 'uuid'.
Error
OM_NodeDomainMod::om_recv_migration_done(const NodeUuid& uuid,
                                         fds_uint64_t dlt_version,
                                         const Error& migrError) {
    LOGDEBUG << "Receiving migration done...";
    Error err(ERR_OK);
    OM_Module *om = OM_Module::om_singleton();
    OM_DLTMod *dltMod = om->om_dlt_mod();
    DataPlacement *dp = om->om_dataplace_mod();
    OM_SmAgent::pointer agent = om_sm_agent(uuid);
    if (agent == NULL) {
        LOGERROR << "OM: Received migration done event from unknown node "
                 << ": uuid " << uuid.uuid_get_val();
        return ERR_NOT_FOUND;
    }

    // for now we shouldn't move to new dlt version until
    // we are done with current cluster update, so
    // expect to see migration done resp for current dlt version
    fds_uint64_t cur_dlt_ver = dp->getLatestDltVersion();
    fds_verify(cur_dlt_ver == dlt_version);

    if (migrError.ok()) {
        // Set node's state to 'node_up'
        agent->set_node_state(FDS_ProtocolInterface::FDS_Node_Up);

        // update node's dlt version so we don't send this dlt again
        agent->set_node_dlt_version(dlt_version);

        // 'rebal ok' event, once all nodes sent migration done
        // notification, the state machine will commit the DLT
        // to other nodes.
        ClusterMap* cm = om->om_clusmap_mod();
        dltMod->dlt_deploy_event(DltRebalOkEvt(cm, dp));
    } else {
        LOGNOTIFY << "Received migration error, will notify DLT state machine";
        dltMod->dlt_deploy_event(DltErrorFoundEvt(uuid, migrError));
    }

    return err;
}


//
// Called when OM received push meta response from DM service
//
Error
OM_NodeDomainMod::om_recv_push_meta_resp(const NodeUuid& uuid) {
    Error err(ERR_OK);
    OM_Module *om = OM_Module::om_singleton();
    OM_DMTMod *dmtMod = om->om_dmt_mod();

#ifdef TEST_DELTA_RSYNC
    LOGNOTIFY << "TEST: Will sleep before processing push meta ack";
    sleep(45);
    LOGNOTIFY << "TEST: Finished sleeping, will process push meta ack";
#endif

    dmtMod->dmt_deploy_event(DmtPushMetaAckEvt(uuid));
    return err;
}

//
// Called when OM receives response for DMT commit from a node
//
Error
OM_NodeDomainMod::om_recv_dmt_commit_resp(FdspNodeType node_type,
                                          const NodeUuid& uuid,
                                          fds_uint32_t dmt_version) {
    Error err(ERR_OK);
    OM_Module *om = OM_Module::om_singleton();
    OM_DMTMod *dmtMod = om->om_dmt_mod();
    OM_NodeContainer *local = om_loc_domain_ctrl();
    AgentContainer::pointer agent_container = local->dc_container_frm_msg(node_type);
    NodeAgent::pointer agent = agent_container->agent_info(uuid);
    if (agent == NULL) {
        FDS_PLOG_SEV(g_fdslog, fds_log::error)
                << "OM: Received DMT commit ack from unknown node: uuid "
                << std::hex << uuid.uuid_get_val() << std::dec;
        err = Error(ERR_NOT_FOUND);
        return err;
    }

    dmtMod->dmt_deploy_event(DmtCommitAckEvt(dmt_version, node_type));

    return err;
}

//
// Called when OM receives response for DMT close from DM
//
Error
OM_NodeDomainMod::om_recv_dmt_close_resp(const NodeUuid& uuid,
                                         fds_uint64_t dmt_version) {
    Error err(ERR_OK);
    OM_Module *om = OM_Module::om_singleton();
    OM_DMTMod *dmtMod = om->om_dmt_mod();

    // tell state machine that we received ack for close
    dmtMod->dmt_deploy_event(DmtCloseOkEvt(dmt_version));

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
    if (cur_dlt_ver > (dlt_version+1)) {
        LOGWARN << "Received DLT commit for unexpected version:" << dlt_version;
        return err;
    }

    // set node's confirmed dlt version to this version
    agent->set_node_dlt_version(dlt_version);

    // commit ok event, will transition to next state when
    // when all 'up' nodes acked this dlt commit
    dltMod->dlt_deploy_event(DltCommitOkEvt(dlt_version, uuid));

    return err;
}

//
// Called when OM receives response for DLT close from a node
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
OM_ControlRespHandler::NotifySnapVolResp(
    const FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
    const FDS_ProtocolInterface::FDSP_NotifyVolType& not_snap_vol_resp) {
    // Don't do anything here. This stub is just to keep cpp compiler happy
}

void
OM_ControlRespHandler::NotifySnapVolResp(
    FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
    FDS_ProtocolInterface::FDSP_NotifyVolTypePtr& not_snap_vol_resp) {
#if 0
    LOGNOTIFY << "OM received response for NotifySnapVol from node "
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
#endif
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
    fds_bool_t check_only = (not_rm_vol_resp->flag == fpi::FDSP_NOTIFY_VOL_CHECK_ONLY);
    LOGNOTIFY << "OM received response for NotifyRmVol (check only "
              << check_only << ") from node "
              << fdsp_msg->src_node_name << " for volume "
              << "[" << not_rm_vol_resp->vol_name << ":"
              << std::hex << not_rm_vol_resp->vol_desc.volUUID << std::dec
              << "] Result: " << fdsp_msg->err_code;

    OM_NodeContainer *local = OM_NodeDomainMod::om_loc_domain_ctrl();
    VolumeContainer::pointer volumes = local->om_vol_mgr();
    om_vol_notify_t type = check_only ? om_notify_vol_rm_chk : om_notify_vol_rm;
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
#if 0
    FDS_PLOG_SEV(g_fdslog, fds_log::notification)
            << "OM received response for NotifyDltUpdate from node "
            << fdsp_msg->src_node_name << ":"
            << std::hex << fdsp_msg->src_service_uuid.uuid << std::dec
            << " for DLT version " << dlt_resp->DLT_version;

    // notify DLT state machine
    OM_NodeDomainMod* domain = OM_NodeDomainMod::om_local_domain();
    NodeUuid node_uuid((fdsp_msg->src_service_uuid).uuid);
    domain->om_recv_dlt_commit_resp(fdsp_msg->src_id, node_uuid, dlt_resp->DLT_version);
#endif
}

void
OM_ControlRespHandler::NotifyDMTCloseResp(
    const FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
    const FDS_ProtocolInterface::FDSP_DMT_Resp_Type& dmt_resp) {
    // Don't do anything here. This stub is just to keep cpp compiler happy
}

void
OM_ControlRespHandler::NotifyDMTCloseResp(
    FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
    FDS_ProtocolInterface::FDSP_DMT_Resp_TypePtr& dmt_resp) {

    LOGNOTIFY << "OM received response for NotifyDMTClose from node "
            << fdsp_msg->src_node_name << ":"
            << std::hex << fdsp_msg->src_service_uuid.uuid << std::dec
            << " for DMT version " << dmt_resp->DMT_version;

    fds_verify(fdsp_msg->src_id == fpi::FDSP_DATA_MGR);

    // notify DMT state machine
    OM_NodeDomainMod* domain = OM_NodeDomainMod::om_local_domain();
    NodeUuid node_uuid((fdsp_msg->src_service_uuid).uuid);
    domain->om_recv_dmt_close_resp(node_uuid, dmt_resp->DMT_version);
}

void
OM_ControlRespHandler::PushMetaDMTResp(
    const FDS_ProtocolInterface::FDSP_MsgHdrType& fdsp_msg,
    const FDS_ProtocolInterface::FDSP_PushMeta& push_meta_resp) {
    // Don't do anything here. This stub is just to keep cpp compiler happy
}

void
OM_ControlRespHandler::PushMetaDMTResp(
    FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
    FDS_ProtocolInterface::FDSP_PushMetaPtr& push_meta_resp) {
    LOGNOTIFY << "Received PushMeta response from node "
            << fdsp_msg->src_node_name << ":"
            << std::hex << fdsp_msg->src_service_uuid.uuid << std::dec;

    fds_verify(fdsp_msg->src_id == fpi::FDSP_DATA_MGR);

    // notify DMT state machine
    OM_NodeDomainMod* domain = OM_NodeDomainMod::om_local_domain();
    NodeUuid node_uuid((fdsp_msg->src_service_uuid).uuid);
    domain->om_recv_push_meta_resp(node_uuid);
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
    const FDS_ProtocolInterface::FDSP_DMT_Resp_Type& dmt_info_resp) {
    // Don't do anything here. This stub is just to keep cpp compiler happy
}

void
OM_ControlRespHandler::NotifyDMTUpdateResp(
    FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
    FDS_ProtocolInterface::FDSP_DMT_Resp_TypePtr& dmt_resp) {

    FDS_PLOG_SEV(g_fdslog, fds_log::notification)
            << "OM received response for NotifyDmtUpdate from node "
            << fdsp_msg->src_node_name << ":"
            << std::hex << fdsp_msg->src_service_uuid.uuid << std::dec
            << " for DLT version " << dmt_resp->DMT_version;

    // notify DLT state machine
    OM_NodeDomainMod* domain = OM_NodeDomainMod::om_local_domain();
    NodeUuid node_uuid((fdsp_msg->src_service_uuid).uuid);
    domain->om_recv_dmt_commit_resp(fdsp_msg->src_id, node_uuid, dmt_resp->DMT_version);
}

}  // namespace fds
