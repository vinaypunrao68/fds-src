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
#include <orchMgr.h>
#include <OmDeploy.h>
#include <OmDmtDeploy.h>
#include <OmResources.h>
#include <OmDataPlacement.h>
#include <OmVolumePlacement.h>
#include <omutils.h>
#include <fds_process.h>
#include <net/net_utils.h>
#include <net/SvcMgr.h>
#include <unistd.h>

#include "util/process.h"

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
struct LocalDomainDown {};

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
    struct DST_WaitActNds : public msm::front::state<>
    {
        DST_WaitActNds() : waitTimer(new FdsTimer()),
                           waitTimerTask(new WaitTimerTask(*waitTimer)) {}

        ~DST_WaitActNds() {
            waitTimer->destroy();
        }

        template <class Evt, class Fsm, class State>
        void operator()(Evt const &, Fsm &, State &) {}

        template <class Event, class FSM> void on_entry(Event const &e, FSM &f) {
            LOGDEBUG << "DST_WaitActNds. Evt: " << e.logString();
        }
        template <class Event, class FSM> void on_exit(Event const &e, FSM &f) {
            LOGDEBUG << "DST_WaitActNds. Evt: " << e.logString();
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
        typedef mpl::vector<RegNodeEvt> deferred_events;

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
    struct DST_DomainShutdown : public msm::front::state<>
    {
        typedef mpl::vector1<LocalDomainDown> flag_list;

        template <class Evt, class Fsm, class State>
        void operator()(Evt const &, Fsm &, State &) {}

        template <class Event, class FSM> void on_entry(Event const &e, FSM &f) {
            LOGDEBUG << "DST_DomainShutdown. Evt: " << e.logString();
        }
        template <class Event, class FSM> void on_exit(Event const &e, FSM &f) {
            LOGDEBUG << "DST_DomainShutdown. Evt: " << e.logString();
        }
    };
    struct DST_WaitShutAm : public msm::front::state<>
    {
        DST_WaitShutAm() : am_acks_to_wait(0) {}

        template <class Evt, class Fsm, class State>
        void operator()(Evt const &, Fsm &, State &) {}

        template <class Event, class FSM> void on_entry(Event const &e, FSM &f) {
            LOGDEBUG << "DST_WaitShutAm. Evt: " << e.logString();
        }
        template <class Event, class FSM> void on_exit(Event const &e, FSM &f) {
            LOGDEBUG << "DST_WaitShutAm. Evt: " << e.logString();
        }

        fds_uint32_t am_acks_to_wait;
    };
    struct DST_WaitShutDmSm : public msm::front::state<>
    {
        DST_WaitShutDmSm() : sm_acks_to_wait(0), dm_acks_to_wait(0) {}

        template <class Evt, class Fsm, class State>
        void operator()(Evt const &, Fsm &, State &) {}

        template <class Event, class FSM> void on_entry(Event const &e, FSM &f) {
            LOGDEBUG << "DST_WaitShutDmSm. Evt: " << e.logString();
        }
        template <class Event, class FSM> void on_exit(Event const &e, FSM &f) {
            LOGDEBUG << "DST_WaitShutDmSm. Evt: " << e.logString();
        }

        fds_uint32_t sm_acks_to_wait;
        fds_uint32_t dm_acks_to_wait;
    };
    struct DST_WaitDeact : public msm::front::state<>
    {
        DST_WaitDeact() : acks_to_wait(0) {}

        template <class Evt, class Fsm, class State>
        void operator()(Evt const &, Fsm &, State &) {}

        template <class Event, class FSM> void on_entry(Event const &e, FSM &f) {
            LOGDEBUG << "DST_WaitDeact. Evt: " << e.logString();
        }
        template <class Event, class FSM> void on_exit(Event const &e, FSM &f) {
            LOGDEBUG << "DST_WaitDeact. Evt: " << e.logString();
        }

        fds_uint32_t acks_to_wait;
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
    struct DACT_SendDltDmt
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
    struct DACT_SvcActive
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
    struct DACT_Shutdown
    {
        template <class Evt, class Fsm, class SrcST, class TgtST>
        void operator()(Evt const &, Fsm &, SrcST &, TgtST &);
    };
    struct DACT_ShutAm
    {
        template <class Evt, class Fsm, class SrcST, class TgtST>
        void operator()(Evt const &, Fsm &, SrcST &, TgtST &);
    };
    struct DACT_ShutDmSm
    {
        template <class Evt, class Fsm, class SrcST, class TgtST>
        void operator()(Evt const &, Fsm &, SrcST &, TgtST &);
    };
    struct DACT_DeactSvc
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
    struct GRD_AmShut
    {
        template <class Evt, class Fsm, class SrcST, class TgtST>
        bool operator()(Evt const &, Fsm &, SrcST &, TgtST &);
    };
    struct GRD_DmSmShut
    {
        template <class Evt, class Fsm, class SrcST, class TgtST>
        bool operator()(Evt const &, Fsm &, SrcST &, TgtST &);
    };
    struct GRD_DeactSvc
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
        msf::Row<DST_WaitDltDmt, DltDmtUpEvt, DST_DomainUp, DACT_LoadVols, GRD_DltDmtUp>,
        // +-----------------+-------------+------------+---------------+--------------+
        msf::Row<DST_DomainUp, ShutdownEvt , DST_WaitShutAm, DACT_ShutAm, msf::none >,
        msf::Row<DST_DomainUp, RegNodeEvt  ,DST_DomainUp,DACT_SendDltDmt,  msf::none   >,
        // +-----------------+-------------+------------+---------------+--------------+
        msf::Row<DST_WaitShutAm, ShutAckEvt, DST_WaitShutDmSm, DACT_ShutDmSm, GRD_AmShut >,
        msf::Row<DST_WaitShutDmSm, ShutAckEvt, DST_WaitDeact, DACT_DeactSvc, GRD_DmSmShut >,
        msf::Row<DST_WaitDeact, DeactAckEvt, DST_DomainShutdown, DACT_Shutdown, GRD_DeactSvc >,
        // +-----------------+-------------+------------+---------------+--------------+
        msf::Row<DST_DomainShutdown, WaitNdsEvt, DST_WaitActNds, DACT_WaitNds, msf::none>,
        msf::Row<DST_DomainShutdown, NoPersistEvt, DST_Wait,   DACT_Wait,   msf::none>,
        // +-----------------+-------------+------------+---------------+--------------+
        msf::Row< DST_WaitActNds, RegNodeEvt, DST_DomainUp, DACT_SvcActive, GRD_NdsUp> ,  // NOLINT
        msf::Row< DST_WaitActNds, TimeoutEvt, DST_DomainShutdown, msf::none, msf::none>   // NOLINT
        // +-----------------+-------------+------------+---------------+--------------+
        >{};  // NOLINT

    template <class Event, class FSM> void no_transition(Event const &, FSM &, int);
};

void NodeDomainFSM::WaitTimerTask::runTimerTask()
{
    OM_NodeDomainMod* domain = OM_NodeDomainMod::om_local_domain();
    LOGNOTIFY << "Timeout waiting for local domain to activate.";
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
* DACT_SendDltDmt
* ------------
* Send DLT/DMT to node that recently came online.
*/
template <class Evt, class Fsm, class SrcST, class TgtST>
void
NodeDomainFSM::DACT_SendDltDmt::operator()(Evt const &evt, Fsm &fsm, SrcST &src, TgtST &dst) {
    LOGDEBUG << "Sending DLT/DMT to recently onlined node "
                << std::hex << evt.svc_uuid.uuid_get_val()
                << std::dec;

	try {
        OM_Module *om = OM_Module::om_singleton();
        OM_NodeContainer *local = OM_NodeDomainMod::om_loc_domain_ctrl();

        // Get a NodeAgent::ptr to the new node
        NodeAgent::pointer newNode = local->dc_find_node_agent(evt.svc_uuid);

        // Send DLT to node
        DataPlacement *dp = om->om_dataplace_mod();
        OM_SmAgent::agt_cast_ptr(newNode)->om_send_dlt(dp->getCommitedDlt());


        // Send DMT to node
        VolumePlacement* vp = om->om_volplace_mod();
        if (vp->hasCommittedDMT()) {
            OM_NodeAgent::agt_cast_ptr(newNode)->om_send_dmt(vp->getCommittedDMT());
        } else {
            LOGWARN << "Not sending DMT to new node, because no "
                    << " committed DMT yet";
        }
    } catch(std::exception& e) {
        LOGERROR << "Orch Manager encountered exception while "
                    << "processing FSM DACT_SendDltDmt :: " << e.what();
    }
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
    LOGDEBUG << "NodeDomainFSM DACT_NodesUp";
    
    try {
        OM_Module *om = OM_Module::om_singleton();
        OM_NodeContainer* local = OM_NodeDomainMod::om_loc_domain_ctrl();
        OM_DLTMod *dltMod = om->om_dlt_mod();
        OM_DMTMod *dmtMod = om->om_dmt_mod();
        ClusterMap *cm = om->om_clusmap_mod();

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
        // we may get services registered that are not in the DLT, so only
        // reset pending services in the map which we actually waited for
        for (auto sm_uuid: src.sm_services) {
            // since updateMap keeps those nodes as pending, we tell cluster map that
            // they are not pending, since these nodes are already in the DLT
            cm->resetPendingAddedService(fpi::FDSP_STOR_MGR, sm_uuid);
        }

        // move nodes that are already in DMT from pending list
        // to cluster map (but make them not pending in cluster map
        // because they are already in DMT), so that we will not try to update DMT
        // with those nodes again
        OM_DmContainer::pointer dmNodes = local->om_dm_nodes();
        addNodes.clear();
        rmNodes.clear();
        dmNodes->om_splice_nodes_pend(&addNodes, &rmNodes, src.dm_up);
        cm->updateMap(fpi::FDSP_DATA_MGR, addNodes, rmNodes);
        // we may get services registered that are not in the DMT, so only
        // reset pending services in the map which we actually waited for
        for (auto dm_uuid: src.dm_services) {
            // since updateMap keeps those nodes as pending, we tell cluster map that
            // they are not pending, since these nodes are already in the DMT
            cm->resetPendingAddedService(fpi::FDSP_DATA_MGR, dm_uuid);
        }
    } catch(std::exception& e) {
        LOGERROR << "Orch Manager encountered exception while "
                    << "processing FSM DACT_NodeUp :: " << e.what();
    }
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
    
    try {
        OM_NodeDomainMod* domain = OM_NodeDomainMod::om_local_domain();
        domain->om_load_volumes();

        // also send all known stream registrations
        OM_NodeContainer *local = OM_NodeDomainMod::om_loc_domain_ctrl();
        local->om_bcast_stream_register_cmd(0, true);
    } catch(std::exception& e) {
        LOGERROR << "Orch Manager encountered exception while "
                    << "processing FSM DACT_LoadVols :: " << e.what();
    }
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
    LOGDEBUG << "NodeDomainFSM DACT_WaitNds";
        
    try {
        WaitNdsEvt waitEvt = (WaitNdsEvt)evt;
        dst.sm_services.swap(waitEvt.sm_services);
        dst.dm_services.swap(waitEvt.dm_services);

        LOGDEBUG << "Domain will wait for " << dst.sm_services.size()
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
    } catch(std::exception& e) {
        LOGERROR << "Orch Manager encountered exception while "
                    << "processing FSM DACT_WaitNds :: " << e.what();
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

    try {
        // start timer to timeout in case services do not come up
        if (!dst.waitTimer->schedule(dst.waitTimerTask,
                                     std::chrono::seconds(OM_WAIT_START_SECONDS))) {
            // couldn't start timer, so don't wait for services to come up
            LOGWARN << "DACT_WaitNds: failed to start timer -- will not wait before "
                    << " before admitting new SMs into DLT";
            fsm.process_event(TimeoutEvt());
        }
    
    } catch(std::exception& e) {
        LOGERROR << "Orch Manager encountered exception while "
                    << "processing FSM DACT_Wait :: " << e.what();
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

    try {
        // start cluster update process that will recompute DLT /rebalance /etc
        // so that we move to DLT that reflects actual nodes that came up
        OM_NodeDomainMod *domain = OM_NodeDomainMod::om_local_domain();
        domain->om_dlt_update_cluster();
    } catch(std::exception& e) {
        LOGERROR << "Orch Manager encountered exception while "
                    << "processing FSM DACT_WaitDone :: " << e.what();
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
    try {
        // We expect the same nodes to come up that were up prior to restart.
        // Until proper restart is implemented we will not proceed when only
        // some of the nodes come up.
        
        /*
         * FS-1814 -- Removing abort in debug mode 
         * 
         * fds_verify(!"Timer expired while waiting for nodes to come up in restart");
         */      
        
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
            // we are not going to print warning messages anymore, will be in
            // wait state until all nodes are up
            LOGWARN << "GRD_EnoughNds: Failed to start timer -- bring up ALL nodes or "
                    << "clean persistent state and restart OM";
        }
    } catch(std::exception& e) {
        LOGERROR << "Orch Manager encountered exception while "
                    << "processing FSM DACT_WaitDone :: " << e.what();
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
    LOGDEBUG << "NodeDomainFSM DACT_UpdDlt";
    
    try {
        OM_Module *om = OM_Module::om_singleton();
        OM_NodeDomainMod *domain = OM_NodeDomainMod::om_local_domain();
        OM_NodeContainer *dom_ctrl = domain->om_loc_domain_ctrl();
        DataPlacement *dp = om->om_dataplace_mod();
        ClusterMap *cm = om->om_clusmap_mod();

        // commit the DLT we got from config DB
        dp->commitDlt();

        // broadcast DLT to SMs only (so they know the diff when we update the DLT)
        dom_ctrl->om_bcast_dlt(dp->getCommitedDlt(), true, false, false);

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
    } catch(std::exception& e) {
        LOGERROR << "Orch Manager encountered exception while "
                    << "processing FSM DACT_UpdDlt :: " << e.what();
    }
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
    
    try {
        if (evt.svc_type == fpi::FDSP_STOR_MGR) {
            fds_assert(src.dlt_up == false);
            src.dlt_up = true;
        } else if (evt.svc_type == fpi::FDSP_DATA_MGR) {
            fds_assert(src.dmt_up == false);
            src.dmt_up = true;
        }
        
        LOGDEBUG << "GRD_DltDmtUp dlt up: " << src.dlt_up << " dmt up: " << src.dmt_up;

        if (src.dlt_up && src.dmt_up) {
            b_ret = true;
        } 
    
    } catch(std::exception& e) {
        LOGERROR << "Orch Manager encountered exception while "
                    << "processing FSM GRD_DltDmtUp :: " << e.what();
    }
        
    return b_ret;
}

/**
 * DACT_ShutAm
 * ------------
 * Send prepare for shutdown command to AMs
 * We are sending PrepareForShutdown msg to AMs first, AMs will drain all in-flight IO and respond
 * to OM; after that OM will send PrepareForShutdown to DMs and SMs.
 * TODO(Anna) We are sending to AMs first to decrease the chance that data becomes inconsistent
 * in SMs and DMs due to shutdown, but there is no guarantee. Once we implement consistency and
 * error handling/reconciling data and metadata in DMs and Sms, OM should just send
 * PrepareForShutdown msg to all services at once.
 */
template <class Evt, class Fsm, class SrcST, class TgtST>
void
NodeDomainFSM::DACT_ShutAm::operator()(Evt const &evt, Fsm &fsm, SrcST &src, TgtST &dst)
{
    LOGDEBUG << "Will send shutdown msg to all AMs";
    try {
        OM_NodeDomainMod *domain = OM_NodeDomainMod::om_local_domain();
        OM_NodeContainer *dom_ctrl = domain->om_loc_domain_ctrl();

        // broadcast shutdown message to all AMs
        dst.am_acks_to_wait = dom_ctrl->om_bcast_shutdown_msg(fpi::FDSP_ACCESS_MGR);
        LOGDEBUG << "Will wait for acks from " << dst.am_acks_to_wait << " AMs";
        if (dst.am_acks_to_wait == 0) {
            // do dummy ack event so we progress to next state
            fsm.process_event(ShutAckEvt(fpi::FDSP_ACCESS_MGR, ERR_OK));
        }
    } catch(std::exception& e) {
        LOGERROR << "Orch Manager encountered exception while "
                 << "processing FSM DACT_ShutAm :: " << e.what();
    }
}

/**
 * GRD_AmShut
 * ------------
 * Checks if AMs acked to "Prepare for shutdown" message
 */
template <class Evt, class Fsm, class SrcST, class TgtST>
bool
NodeDomainFSM::GRD_AmShut::operator()(Evt const &evt, Fsm &fsm, SrcST &src, TgtST &dst)
{
    fds_bool_t b_ret = false;

    // if timeout or couldn't send request over SL
    // handle error for now by printing error msg to log
    // TODO(Anna) will need to implement proper error handling
    if ((evt.error == ERR_SVC_REQUEST_TIMEOUT) ||
        (evt.error == ERR_SVC_REQUEST_INVOCATION)) {
        LOGWARN << "Coudn't reach AM service for Prepare for Shutdown"
                << "; should be ok if service is actually down."
                << " Treating as a success.";
    } else if (evt.error != ERR_OK) {
        // this is a more serious error, need to handle,
        // but for now printing to log and ignoring
        LOGERROR << "AM returned error to Prepare for shutdown: "
                 << evt.error << ". We continue with shutting down anyway";
    }

    try {
        if (evt.svc_type == fpi::FDSP_ACCESS_MGR) {
            if (src.am_acks_to_wait > 0) {
                src.am_acks_to_wait--;
            }
            if (src.am_acks_to_wait == 0) {
                b_ret = true;
            }
        } else {
            LOGERROR << "Received ack from unexpected service type "
                     << evt.svc_type << ", ignoring for now, but check "
                     << " why that happened";
        }
        
        LOGDEBUG << "AM acks to wait: " << src.am_acks_to_wait
                 << " GRD will return " << b_ret;
    } catch(std::exception& e) {
        LOGERROR << "Orch Manager encountered exception while "
                    << "processing FSM GRD_ShutAm :: " << e.what();
    }
        
    return b_ret;
}

/**
 * DACT_ShutDmSm
 * ------------
 * Send shutdown command to all DMs and SMs
 */
template <class Evt, class Fsm, class SrcST, class TgtST>
void
NodeDomainFSM::DACT_ShutDmSm::operator()(Evt const &evt, Fsm &fsm, SrcST &src, TgtST &dst)
{
    LOGDEBUG << "Will send shutdown msg to all DMs and SMs";
    try {
        OM_NodeDomainMod *domain = OM_NodeDomainMod::om_local_domain();
        OM_NodeContainer *dom_ctrl = domain->om_loc_domain_ctrl();

        // broadcast shutdown message to all DMs and SMs
        dst.dm_acks_to_wait = dom_ctrl->om_bcast_shutdown_msg(fpi::FDSP_DATA_MGR);
        LOGDEBUG << "Will wait for acks from " << dst.dm_acks_to_wait << " DMs";
        dst.sm_acks_to_wait = dom_ctrl->om_bcast_shutdown_msg(fpi::FDSP_STOR_MGR);
        LOGDEBUG << "Will wait for acks from " << dst.sm_acks_to_wait << " SMs";
        if (dst.dm_acks_to_wait == 0 && dst.sm_acks_to_wait == 0) {
            // do dummy acknowledge event so we progress to next state
            fsm.process_event(ShutAckEvt(fpi::FDSP_INVALID_SVC, ERR_OK));
        }
    } catch(std::exception& e) {
        LOGERROR << "Orch Manager encountered exception while "
                 << "processing FSM DACT_ShutDmSm :: " << e.what();
    }
}

/**
 * GRD_DmSmShut
 * ------------
 * Checks if DMs and SMs acknowledged to "Prepare for shutdown" message
 */
template <class Evt, class Fsm, class SrcST, class TgtST>
bool
NodeDomainFSM::GRD_DmSmShut::operator()(Evt const &evt, Fsm &fsm, SrcST &src, TgtST &dst)
{
    fds_bool_t b_ret = false;

    // if timeout or couldn't send request over SL
    // handle error for now by printing error msg to log
    // TODO(Anna) will need to implement proper error handling
    if ((evt.error == ERR_SVC_REQUEST_TIMEOUT) ||
        (evt.error == ERR_SVC_REQUEST_INVOCATION)) {
        LOGWARN << "Couldn't reach service type " << evt.svc_type
                << " for prepare for shutdown "
                << "; should be ok if service is actually down."
                << " BUT cannot assume shutdown is graceful! "
                << "For now treating as a success.";
    } else if (evt.error != ERR_OK) {
        // this is a more serious error, need to handle,
        // but for now printing to log and ignoring
        LOGERROR << "Service type " << evt.svc_type
                 << "returned error to Prepare for shutdown: "
                 << evt.error << ". We continue with shutting down anyway";
    }

    try {
        if (evt.svc_type == fpi::FDSP_STOR_MGR) {
            fds_verify(src.sm_acks_to_wait > 0);
            src.sm_acks_to_wait--;
        } else if (evt.svc_type == fpi::FDSP_DATA_MGR) {
            fds_verify(src.dm_acks_to_wait > 0);
            src.dm_acks_to_wait--;
        } else if (evt.svc_type == fpi::FDSP_INVALID_SVC) {
            fds_verify((src.sm_acks_to_wait == 0) &&
                       (src.dm_acks_to_wait == 0));
        }
        
        LOGDEBUG << "SM acknowledges to wait: " << src.sm_acks_to_wait
                 << ", DM acknowledges to wait: " << src.dm_acks_to_wait;

        if ((src.sm_acks_to_wait == 0) &&
            (src.dm_acks_to_wait == 0)) {
            b_ret = true;
        }
    
    } catch(std::exception& e) {
        LOGERROR << "Orch Manager encountered exception while "
                    << "processing FSM GRD_ShutDmSm :: " << e.what();
    }

    return b_ret;
}

/**
 * GRD_DeactSvc
 * ------------
 * Checks if PMs acknowledged to "deactivate services" message
 */
template <class Evt, class Fsm, class SrcST, class TgtST>
bool
NodeDomainFSM::GRD_DeactSvc::operator()(Evt const &evt, Fsm &fsm, SrcST &src, TgtST &dst)
{
    fds_bool_t b_ret = false;

    // if timeout or couldn't send request over SL
    // handle error for now by printing error msg to log
    // TODO(Anna) will need to implement proper error handling
    if ((evt.error == ERR_SVC_REQUEST_TIMEOUT) ||
        (evt.error == ERR_SVC_REQUEST_INVOCATION)) {
        LOGWARN << "Couldn't reach PM to deactivate services. "
                << "Should be ok if the node is down, but cannot assume shutdown "
                << " is graceful! For now treating as success";
    } else if (evt.error != ERR_OK) {
        // this is a more serious error, need to handle,
        // but for now printing to log and ignoring
        LOGERROR << "PM returned error to deactivate services: "
                 << evt.error << ". We continue with shutting down anyway";
    }

    try {
        if (src.acks_to_wait > 0) {
            src.acks_to_wait--;
        }

        LOGDEBUG << "PM acks for deactivate services to wait: " << src.acks_to_wait;

        if (src.acks_to_wait == 0) {
            b_ret = true;
        }
    
    } catch(std::exception& e) {
        LOGERROR << "Orch Manager encountered exception while "
                    << "processing FSM GRD_DeactSvc :: " << e.what();
    }

    return b_ret;
}

/**
 * DACT_DeactSvc
 * ------------
 * Send stop services msg to all PMs
 */
template <class Evt, class Fsm, class SrcST, class TgtST>
void
NodeDomainFSM::DACT_DeactSvc::operator()(Evt const &evt, Fsm &fsm, SrcST &src, TgtST &dst)
{
    // At this point we are ready to send msg to PMs to kill the services
    LOGDEBUG << "Will send deactivate services msg to all PMs";
    try {
        OM_NodeDomainMod *domain = OM_NodeDomainMod::om_local_domain();
        OM_NodeContainer *dom_ctrl = domain->om_loc_domain_ctrl();

        // broadcast stop services to all PMs
        // all "false" params mean stop all services that are running on node
        fds_uint32_t count = dom_ctrl->om_cond_bcast_stop_services(false, false, false);
        LOGDEBUG <<"--Error count is" << count;
        if (count < 1) {
            // ok if we don't have any PMs, just finish shutdown process
            dst.acks_to_wait = 1;
            fsm.process_event(DeactAckEvt(ERR_OK));
        } else {
            dst.acks_to_wait = count;
        }

    } catch(std::exception& e) {
        LOGERROR << "Orch Manager encountered exception while "
                 << "processing FSM DACT_Shutdown :: " << e.what();
    }
}

/**
 * DACT_Shutdown
 * ------------
 * Finished domain shutdown process; log a message
 */
template <class Evt, class Fsm, class SrcST, class TgtST>
void
NodeDomainFSM::DACT_Shutdown::operator()(Evt const &evt, Fsm &fsm, SrcST &src, TgtST &dst)
{
    LOGCRITICAL << "Domain shut down. OM will reject all requests from services";
}

/**
 * DACT_SvcActive
 * ------------
 * We got all SMs and DMs that we were waiting for up.
 * DLT and DMT is already committed, so we cancel the timer and broadcast DLTs and DMTs
 */
template <class Evt, class Fsm, class SrcST, class TgtST>
void
NodeDomainFSM::DACT_SvcActive::operator()(Evt const &evt, Fsm &fsm, SrcST &src, TgtST &dst)
{
    LOGDEBUG << "All services re-activated on domain activate";

    try {
        OM_NodeContainer *local = OM_NodeDomainMod::om_loc_domain_ctrl();
        OM_Module *om = OM_Module::om_singleton();
        DataPlacement *dp = om->om_dataplace_mod();
        VolumePlacement* vp = om->om_volplace_mod();

        // cancel timeout timer
        src.waitTimer->cancel(src.waitTimerTask);

        // Broadcast DLT to SMs. We did not do this on SM register, so that
        // SMs can resync with each other, better do it when all SMs that are in DLT
        // are up (or all that could get up).
        local->om_bcast_dlt(dp->getCommitedDlt(), true, false, false);

        // broadcast DMT to DMs so they can start resync
        local->om_bcast_dmt(fpi::FDSP_DATA_MGR, vp->getCommittedDMT());

    } catch(std::exception& e) {
        LOGERROR << "Orch Manager encountered exception while "
                 << "processing FSM DACT_SvcActive :: " << e.what();
    }
}

//--------------------------------------------------------------------
// OM Node Domain
//--------------------------------------------------------------------
OM_NodeDomainMod::OM_NodeDomainMod(char const *const name)
        : Module(name),
          fsm_lock("OM_NodeDomainMod fsm lock"),
          configDB(nullptr)
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

// om_local_domain_down
// ------------------
//
// Domain finished shutdown process and in down state. This is
// not an opposite of "up" state; if shutdown domain process started,
// domain is not in 'up' or 'down' state, when shutdown domain process
// finishes, domain goes in 'down' state; from this state, domain can
// be re-activated again
fds_bool_t
OM_NodeDomainMod::om_local_domain_down()
{
    fds_mutex::scoped_lock l(om_local_domain()->fsm_lock);
    return om_local_domain()->domain_fsm->is_flag_active<LocalDomainDown>();
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
void OM_NodeDomainMod::local_domain_event(ShutdownEvt const &evt) {
    fds_mutex::scoped_lock l(fsm_lock);
    domain_fsm->process_event(evt);
}
void OM_NodeDomainMod::local_domain_event(ShutAckEvt const &evt) {
    fds_mutex::scoped_lock l(fsm_lock);
    domain_fsm->process_event(evt);
}
void OM_NodeDomainMod::local_domain_event(DeactAckEvt const &evt) {
    fds_mutex::scoped_lock l(fsm_lock);
    domain_fsm->process_event(evt);
}

// om_load_state
// ------------------
//
Error
OM_NodeDomainMod::om_load_state(kvstore::ConfigDB* _configDB)
{
    TRACEFUNC;
    Error err(ERR_OK);
    OM_Module *om = OM_Module::om_singleton();
    DataPlacement *dp = om->om_dataplace_mod();
    VolumePlacement* vp = om->om_volplace_mod();

    LOGNOTIFY << "Loading persistent state to local domain";
    if (_configDB) {
        configDB = _configDB;  // cache config db in local domain
    }

    std::vector<fpi::SvcInfo> svcinfos;
    
    // get SMs that were up before and persistent in config db
    // if no SMs were up, even if platforms were running, we
    // don't need to wait for the platforms/DMs/AMs to come up
    // since we cannot admit any volumes anyway. So, we will go
    // directly to domain up state if no SMs were running...
    NodeUuidSet sm_services, deployed_sm_services;
    NodeUuidSet dm_services, deployed_dm_services;
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
                    LOGDEBUG << "Found SM on node "
                             << std::hex << (*cit).uuid_get_val() << " (SM "
                             << services.sm.uuid_get_val() << std::dec << ")";
                }
                if (services.dm.uuid_get_val() != 0) {
                    dm_services.insert(services.dm);
                    LOGDEBUG << "Found DM on node "
                             << std::hex << (*cit).uuid_get_val() << " (DM "
                             << services.dm.uuid_get_val() << std::dec << ")";
                }
            }
        }

        /*
         * Update Service Map, then broadcast it!
         */
        if ( configDB->getSvcMap( svcinfos ) ) {

            if ( svcinfos.size() > 0 ) {
                LOGDEBUG << "Updating Service Map from persist store.";
                /*
                 * service map updates will only take effect if the service
                 * does not already exists or if the incarnation is newer then
                 * then the existing service.
                 */
                
                MODULEPROVIDER()->getSvcMgr()->updateSvcMap(svcinfos);
            } else {
                LOGNORMAL << "No persisted Service Map found.";
            }

            // build the list of sm and dm services that we found that
            // are not in 'discovered' state
            for ( const auto svc : svcinfos ) {
                if ( svc.svc_status != fpi::SVC_STATUS_DISCOVERED ) {
                    NodeUuid svcUuid;
                    if ( isStorageMgrSvc( svc ) ) {
                        // SM in not 'discovered' state
                        svcUuid.uuid_set_type(svc.svc_id.svc_uuid.svc_uuid,
                                              fpi::FDSP_STOR_MGR);
                        if (sm_services.count(svcUuid)) {
                            deployed_sm_services.insert(svcUuid);
                            LOGDEBUG << "SM service "
                                     << std::hex << svcUuid.uuid_get_val() << std::dec
                                     << " was deployed";
                        }
                    } else if ( isDataMgrSvc( svc ) ) {
                        // DM in not 'discovered' state
                        svcUuid.uuid_set_type(svc.svc_id.svc_uuid.svc_uuid,
                                              fpi::FDSP_DATA_MGR);
                        if (dm_services.count(svcUuid)) {
                            deployed_dm_services.insert(svcUuid);
                            LOGDEBUG << "DM service "
                                     << std::hex << svcUuid.uuid_get_val() << std::dec
                                     << " was deployed";
                        }
                    }
                }
            }
        }
	
        // load DLT (and save as not committed) from config DB and
        // check if DLT matches the set of persisted nodes
        err = dp->loadDltsFromConfigDB(sm_services, deployed_sm_services);
        if (!err.ok()) {
            LOGERROR << "Persistent state mismatch: " << err
                     << std::endl << "OM will stay DOWN! Kill OM, cleanup "
                     << "persistent state and restart cluster again";
            return err;
        }

        // Load DMT
        err = vp->loadDmtsFromConfigDB(dm_services, deployed_dm_services);
        if (!err.ok()) {
            LOGERROR << "Persistent state mismatch: " << err
                     << std::endl << "OM will stay DOWN! Kill OM, cleanup "
                     << "persistent state and restart cluster again";
            return err;
        }
        fds_verify((sm_services.size() == 0 && dm_services.size() == 0) ||
                   (sm_services.size() > 0 && dm_services.size() > 0));
    }

    if ( ( sm_services.size() > 0 ) ||
         ( dm_services.size() > 0 ) )
    {
        std::vector<fpi::SvcInfo> pmSvcs;
        std::vector<fpi::SvcInfo> amSvcs;
        std::vector<fpi::SvcInfo> smSvcs;
        std::vector<fpi::SvcInfo> dmSvcs;
        
        if ( isAnyNonePlatformSvcActive( &pmSvcs, &amSvcs, &smSvcs, &dmSvcs ) )
        {
            LOGDEBUG << "OM Restart, Found ( returned ) " 
                     << pmSvcs.size() << " PMs. "
                     << amSvcs.size() << " AMs. "
                     << dmSvcs.size() << " DMs. "
                     << smSvcs.size() << " SMs.";
    
            /*
             * TODO need to determine if any persisted services are already
             * running. It would be nice if we could "ping" the PMs to
             * get its state and any services that its currently
             * monitoring.
             */
            
            LOGNOTIFY << "OM Restarted, while domains still active.";
            
            OM_Module *om = OM_Module::om_singleton();
            DataPlacement *dp = om->om_dataplace_mod();
            VolumePlacement* vp = om->om_volplace_mod();
            dp->commitDlt( true );
            LOGNOTIFY << "OM deployed DLT with "
                      << deployed_sm_services.size() << " nodes";

            vp->commitDMT( true );
                        
            spoofRegisterSvcs( pmSvcs );
            spoofRegisterSvcs( smSvcs );
            spoofRegisterSvcs( dmSvcs );
            spoofRegisterSvcs( amSvcs );
            
            om_load_volumes();
            local_domain_event( NoPersistEvt( ) );
        }
        else
        { 
            LOGNOTIFY << "Will wait for " << deployed_sm_services.size()
                      << " SMs and " << deployed_dm_services.size()
                      << " DMs to come up within next few minutes";
            local_domain_event( WaitNdsEvt( deployed_sm_services, 
                                            deployed_dm_services ) );
        }
    } 
    else
    {
        LOGNOTIFY << "We didn't persist any SMs or DMs or we couldn't load "
                  << "persistent state, so OM will come up in a moment.";
        local_domain_event( NoPersistEvt( ) );
    }

    return err;
}

Error
OM_NodeDomainMod::om_startup_domain()
{
    Error err(ERR_OK);
    OM_Module *om = OM_Module::om_singleton();
    DataPlacement *dp = om->om_dataplace_mod();
    VolumePlacement* vp = om->om_volplace_mod();
    ClusterMap *cm = om->om_clusmap_mod();
    NodeUuidSet sm_services;
    NodeUuidSet dm_services;

    // if domain is already UP, nothing to do
    if (om_local_domain_up()) {
        LOGWARN << "Domain is already active, not going to activate";
        return ERR_DUPLICATE;
    } else if (!om_local_domain_down()) {
        LOGWARN << "Domain is either in the process of coming up or down; "
                << " Try again soon";
        return ERR_NOT_READY;
    }

    LOGNOTIFY << "Starting up domain, will allow processing node add/remove"
              << " and other commands for the domain";

    // get SMs and DMs from cluster map
    for (ClusterMap::const_sm_iterator sm_cit = cm->cbegin_sm();
         sm_cit != cm->cend_sm();
         ++sm_cit) {
        sm_services.insert(sm_cit->first);
        LOGDEBUG << "Found SM with UUID "
                 << std::hex << (sm_cit->first).uuid_get_val() << std::dec;
    }
    for (ClusterMap::const_dm_iterator dm_cit = cm->cbegin_dm();
         dm_cit != cm->cend_dm();
         ++dm_cit) {
        dm_services.insert(dm_cit->first);
        LOGDEBUG << "Found DM with UUID "
                 << std::hex << (dm_cit->first).uuid_get_val() << std::dec;
    }

    // check that committed DLT matches the SM services in cluster map
    err = dp->validateDltOnDomainActivate(sm_services);
    if (!err.ok()) {
        LOGERROR << "Mismatch between cluster map and DLT " << err
                 << " local domain will remain in down state."
                 << " Need to implement recovery from this state";
        return err;
    }

    // check that committed DMT matches the DM services in cluster map
    err = vp->validateDmtOnDomainActivate(dm_services);
    if (!err.ok()) {
        LOGERROR << "Mismatch between cluster map and DMT " << err
                 << " local domain will remain in down state."
                 << " Need to implement recovery from this state";
        return err;
    }

    if ((sm_services.size() > 0) ||
        (dm_services.size() > 0)){
        LOGNOTIFY << "Will wait for " << sm_services.size()
                  << " SMs and " << dm_services.size()
                  << " DMs to come up within next few minutes";
        local_domain_event(WaitNdsEvt(sm_services, dm_services));
    } else {
        LOGNOTIFY << "We didn't persist any SMs and any DMs or we couldn't load "
                  << "persistent state, so Domain will come up in a moment.";
        local_domain_event(NoPersistEvt());
    }

    // once domain state machine is in correct waiting state,
    // activate all known services on all known nodes (platforms)
    RsArray pmNodes;
    fds_uint32_t pmCount = om_locDomain->om_pm_nodes()->rs_container_snapshot(&pmNodes);
    for (RsContainer::const_iterator it = pmNodes.cbegin();
         it != pmNodes.cend();
         ++it) {
        NodeAgent::pointer cur = agt_cast_ptr<NodeAgent>(*it);
        if ((cur != NULL) &&
            (om_locDomain->om_pm_nodes()->rs_get_resource(cur->get_uuid()))) {
            // above check is because apparently we can have NULL pointer in RsArray
            om_activate_known_services(cur->get_uuid());
        }
    }

    return err;
}


Error
OM_NodeDomainMod::om_load_volumes()
{
    TRACEFUNC;
    Error err(ERR_OK);

    // load volumes for this domain
    // so far we are assuming this is domain with id 0
    int my_domainId = 0;

    std::vector<VolumeDesc> vecVolumes;
    std::vector<VolumeDesc>::const_iterator volumeIter;
    configDB->getVolumes(vecVolumes, my_domainId);
    if (vecVolumes.empty()) {
        LOGDEBUG << "no volumes found for domain "
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
            LOGDEBUG << "not loading deleted volume : "
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
    TRACEFUNC;
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

// om_register_service
// ----------------
//
Error
OM_NodeDomainMod::om_register_service(boost::shared_ptr<fpi::SvcInfo>& svcInfo)
{
    TRACEFUNC;
    Error err;

    try
    {
        /*
         * TODO(OM team): This registration should be handled in synchronized 
         * manner (single thread handling is better) to avoid race conditions.
         */

        LOGNOTIFY << "Registering service: " 
                  << fds::logDetailedString( *svcInfo );

        /* Convert new registration request to existing registration request */
        fpi::FDSP_RegisterNodeTypePtr reg_node_req;
        reg_node_req.reset( new FdspNodeReg() );

        fromSvcInfoToFDSP_RegisterNodeTypePtr( svcInfo, reg_node_req );

        /* Do the registration */
        NodeUuid node_uuid(static_cast<uint64_t>(reg_node_req->service_uuid.uuid));
        err = om_reg_node_info(node_uuid, reg_node_req);

        if ( err.ok() )
        {
            /*
             * FS-1587 Tinius
             */
            if ( isPlatformSvc( *svcInfo ) )
            {
                if ( isKnownPM( *svcInfo ) )
                {
                    LOGDEBUG << "Found well known platform service UUID ( "
                             << std::hex 
                             << svcInfo->svc_id.svc_uuid.svc_uuid 
                             << std::dec
                             << " ), telling the platformd which services to start";

                     /*
                      * delay the start of the scheduled thread.
                      */
                    NodeUuid pmUuid;
                    pmUuid.uuid_set_type( ( svcInfo->svc_id).svc_uuid.svc_uuid, 
                                            fpi::FDSP_PLATFORM );
                    auto timer = MODULEPROVIDER()->getTimer();
                    auto task = boost::shared_ptr<FdsTimerTask>(
                        new FdsTimerFunctionTask(
                            *timer,
                            [this, pmUuid] () {
                            /* Immediately post to threadpool so we don't hold up timer thread */
                            MODULEPROVIDER()->proc_thrpool()->schedule(
                                &OM_NodeDomainMod::om_activate_known_services,
                                this,
                                pmUuid);
                        }));
                    /* schedule the task to be run on timer thread after 3 seconds */
                    timer->schedule(task, std::chrono::seconds(3));
                }
                else
                {
                    LOGDEBUG << "Platform Manager Service UUID ( "
                             << std::hex 
                             << svcInfo->svc_id.svc_uuid.svc_uuid 
                             << std::dec
                             << " ) is a new node.";
                    
                    svcInfo->svc_status = fpi::SVC_STATUS_ACTIVE;
                }
                auto curTime         = std::chrono::system_clock::now().time_since_epoch();
                double timeInMinutes = std::chrono::duration<double,std::ratio<60>>(curTime).count();

                gl_orch_mgr->omMonitor->updateKnownPMsMap(svcInfo->svc_id.svc_uuid, timeInMinutes );
            } 
            else if ( isStorageMgrSvc( *svcInfo ) || isDataMgrSvc( *svcInfo ) ) 
            {    
                if ( !isKnownService( *svcInfo ) ) {
                    LOGDEBUG << "SM or DM service ("
                             << std::hex 
                             << svcInfo->svc_id.svc_uuid.svc_uuid 
                             << std::dec
                             << ") is a new service; set state = DISCOVERED";
                    svcInfo->svc_status = fpi::SVC_STATUS_DISCOVERED;
                } 
                else 
                {
                    /*
                     * currently service always reports it's active; but if OM 
                     * knows that it is in discovered state, then we should 
                     * leave it in discovered state
                     */
                    fpi::ServiceStatus status = 
                        configDB->getStateSvcMap( svcInfo->svc_id.svc_uuid.svc_uuid );
                    if ( status == fpi::SVC_STATUS_DISCOVERED ) 
                    {
                        svcInfo->svc_status = fpi::SVC_STATUS_DISCOVERED;
                    }
                }
            }
            /*
             * FS-1587 Tinius
             */
        }
        else
        {
            /* 
             * We updated the service map before, so undo these changes by 
             * setting service status to invalid
             */
            svcInfo->svc_status = fpi::SVC_STATUS_INVALID;
        }

        /*
         * Update the service layer service map up front for PM so that any subsequent
         * communication with that service will work.
         */
        if (svcInfo->svc_type == fpi::FDSP_PLATFORM) {
            MODULEPROVIDER()->getSvcMgr()->updateSvcMap({*svcInfo});
            configDB->updateSvcMap(*svcInfo);
            om_locDomain->om_bcast_svcmap();

        } else {
            // SvcMap updates and broadcast for AM/DM/SM will happen at the end of setUpNewNode
            // Once the scheduling delay is removed, it probably makes sense to allow
            // updates to occur here as previously done
            addRegisteringSvc(svcInfo);
        }
    }
    catch(const Exception& e)
    {
        LOGERROR << "Orch Manager encountered exception while "
                 << "Registering service " << e.what();
        err = Error(ERR_SVC_REQUEST_FAILED);
        fds::util::print_stacktrace( );
    }

    return err;
}

void OM_NodeDomainMod::om_activate_known_services( const NodeUuid& node_uuid)
{

    NodeServices services;
    if ( configDB->getNodeServices( node_uuid, services ) )
    {
      fds_bool_t startAM = false;
      fds_bool_t startDM = false;
      fds_bool_t startSM = false;

      if ( services.am.uuid_get_type() == fpi::FDSP_ACCESS_MGR )
      {
          startAM = true;
      }

      if ( services.dm.uuid_get_type() == fpi::FDSP_DATA_MGR )
      {
          startDM = true;
      }

      if ( services.sm.uuid_get_type() == fpi::FDSP_STOR_MGR )
      {
          startSM = true;
      }

      if ( startAM || startDM || startSM )
      {
          OM_NodeContainer *local = OM_NodeDomainMod::om_loc_domain_ctrl();

          fpi::SvcUuid svcUuid;
          svcUuid.svc_uuid = node_uuid.uuid_get_val();
          std::vector<fpi::SvcInfo> svcInfoList;

          fds::getServicesToStart(startSM,
                                  startDM,
                                  startAM,
                                  gl_orch_mgr->getConfigDB(),
                                  node_uuid,
                                  svcInfoList);

          if (svcInfoList.size() == 0) {
              LOGWARN <<"No services found to start for node:"
                         << std::hex << node_uuid << std::dec;
          }
          else
          {
              bool domainRestart = true;
              bool startNode     = true;
              local->om_start_service( svcUuid, svcInfoList, domainRestart, startNode );
          }
      }
    }
}
   
void OM_NodeDomainMod::spoofRegisterSvcs( const std::vector<fpi::SvcInfo> svcs )
{    
    LOGDEBUG << "OM Restart, Register ( spoof ) services " << svcs.size();
    std::vector<fpi::SvcInfo> spoofed;
    for ( auto svc : svcs )
    {
        LOGDEBUG << "OM Restart, Register Service ( spoof ) " 
                 << fds::logDetailedString( svc );
        Error error( ERR_OK );
        
        fpi::FDSP_RegisterNodeTypePtr reg_node_req;
        reg_node_req.reset( new FdspNodeReg() );
        fromSvcInfoToFDSP_RegisterNodeTypePtr( svc, reg_node_req );
        
        NodeUuid node_uuid( static_cast<uint64_t>( reg_node_req->service_uuid.uuid ) );
        
        switch ( svc.svc_type )
        {
            case fpi::FDSP_DATA_MGR:
            case fpi::FDSP_STOR_MGR:
                error = om_handle_restart( node_uuid, reg_node_req );
                
                if ( error.ok() )
                {
                    NodeList addNodes, rmNodes;                        
                    OM_Module *om = OM_Module::om_singleton();
                    OM_NodeContainer* local = OM_NodeDomainMod::om_loc_domain_ctrl();
                    ClusterMap *cm = om->om_clusmap_mod();
                    
                    if ( svc.svc_type == fpi::FDSP_STOR_MGR )
                    {
                        addNodes.clear();
                        rmNodes.clear();
                        
                        OM_SmContainer::pointer smNodes = local->om_sm_nodes();
                        smNodes->om_splice_nodes_pend( &addNodes, &rmNodes );
                        cm->updateMap( fpi::FDSP_STOR_MGR, addNodes, rmNodes );
                        // we want to remove service from pending services
                        // only if it is not in discovered state
                        if ( svc.svc_status != fpi::SVC_STATUS_DISCOVERED ) {
                            cm->resetPendingAddedService(fpi::FDSP_STOR_MGR, node_uuid);
                        }
                    }
                    else if ( svc.svc_type == fpi::FDSP_DATA_MGR )
                    {
                        addNodes.clear();
                        rmNodes.clear();
                       
                        OM_DmContainer::pointer dmNodes = local->om_dm_nodes();
                        dmNodes->om_splice_nodes_pend( &addNodes, &rmNodes );
                        cm->updateMap( fpi::FDSP_DATA_MGR, addNodes, rmNodes );
                        // we want to remove service from pending services
                        // only if it is not in discovered state
                        if ( svc.svc_status != fpi::SVC_STATUS_DISCOVERED ) {
                            cm->resetPendingAddedService(fpi::FDSP_DATA_MGR, node_uuid);
                        }
                    }
                }
                break;
            case fpi::FDSP_PLATFORM:
                // TODO how do we set the node state?
                error = om_handle_restart( node_uuid, reg_node_req );
                break;    
            default:
                // skip any other service types
                break;
        }
        
        if ( error.ok() )
        {
            LOGDEBUG << "OM Restart, Successful Registered ( spoof ) Service: "
                     << fds::logDetailedString( svc );
            svc.incarnationNo = util::getTimeStampSeconds();
            svc.svc_status = fpi::SVC_STATUS_ACTIVE;
            spoofed.push_back( svc );
            configDB->updateSvcMap( svc );
        }
        else 
        {
            LOGWARN << "OM Restart, Failed to Register ( spoof ) Service: "
                    << fds::logDetailedString( svc );
        }
    }
    
    if ( spoofed.size() > 0  )
    {    
        LOGDEBUG << "OM Restart, updating and broadcasting service map ( spoof )";
        MODULEPROVIDER()->getSvcMgr()->updateSvcMap( spoofed );
        om_locDomain->om_bcast_svcmap();
    }
}
    
bool OM_NodeDomainMod::isAnyNonePlatformSvcActive( 
    std::vector<fpi::SvcInfo>* pmSvcs,
    std::vector<fpi::SvcInfo>* amSvcs,
    std::vector<fpi::SvcInfo>* smSvcs,
    std::vector<fpi::SvcInfo>* dmSvcs )
{
    std::vector<fpi::SvcInfo> svcs;
    configDB->getSvcMap( svcs );
    
    std::vector<fpi::SvcInfo> entries;
    MODULEPROVIDER()->getSvcMgr()->getSvcMap( entries );
    LOGDEBUG << "SERVICE MAP SIZE:: " << entries.size();
    
    LOGDEBUG << "OM Restart, Found " << svcs.size() << " persisted services.";
    if ( svcs.size() > 0 )
    {
        for ( const auto svc : svcs )
        {
            if ( svc.svc_status == fpi::SVC_STATUS_ACTIVE )
            {
                if ( isPlatformSvc( svc ) )
                {
                    pmSvcs->push_back( svc );
                }
                else if ( isStorageMgrSvc( svc ) )
                {
                    smSvcs->push_back( svc );
                }
                else if ( isDataMgrSvc( svc ) )
                {
                    dmSvcs->push_back( svc );                
                }
                else if ( isAccessMgrSvc( svc ) )
                {
                    amSvcs->push_back( svc );                
                }
            }
        }
    }
    
    /**
     * ignore any PMs that are running. They are expected.
     */
    return ( ( amSvcs->size() > 0 ) ||  
             ( smSvcs->size() > 0 ) || 
             ( dmSvcs->size() > 0 ) ) 
            ? true 
            : false;
}

bool OM_NodeDomainMod::isAccessMgrSvc( fpi::SvcInfo svcInfo )
{
    return svcInfo.svc_type == fpi::FDSP_ACCESS_MGR;
}

bool OM_NodeDomainMod::isDataMgrSvc( fpi::SvcInfo svcInfo )
{
    return svcInfo.svc_type == fpi::FDSP_DATA_MGR;
}

bool OM_NodeDomainMod::isStorageMgrSvc( fpi::SvcInfo svcInfo )
{
    return svcInfo.svc_type == fpi::FDSP_STOR_MGR;
}

bool OM_NodeDomainMod::isPlatformSvc( fpi::SvcInfo svcInfo )
{
    return svcInfo.svc_type == fpi::FDSP_PLATFORM;
}

bool OM_NodeDomainMod::isKnownPM(fpi::SvcInfo svcInfo)
{
    bool bRetCode = false;

    int64_t registerUUID = svcInfo.svc_id.svc_uuid.svc_uuid;
    std::vector<fpi::SvcInfo> configDBSvcs;

    configDB->getSvcMap( configDBSvcs );
    if ( configDBSvcs.size() > 0 )
    {
        if ( isPlatformSvc( svcInfo ) )
        {
            for ( auto dbSvc : configDBSvcs )
            {
                int64_t knownUUID = dbSvc.svc_id.svc_uuid.svc_uuid;

                if ( knownUUID == registerUUID )
                {
                    bRetCode = true;
                    break;
                }
            }
        }
    }

    return bRetCode;
}

bool OM_NodeDomainMod::isKnownService(fpi::SvcInfo svcInfo)
{
    bool bRetCode = false;

    int64_t serviceUUID = svcInfo.svc_id.svc_uuid.svc_uuid;
    std::vector<fpi::SvcInfo> configDBSvcs;

    configDB->getSvcMap( configDBSvcs );
    if ( configDBSvcs.size() > 0 )
    {
        for ( auto dbSvc : configDBSvcs )
        {
            int64_t knownUUID = dbSvc.svc_id.svc_uuid.svc_uuid;
            if ( knownUUID == serviceUUID )
            {
                bRetCode = true;
                break;
            }
        }
    }

    return bRetCode;
}

void OM_NodeDomainMod::addRegisteringSvc(SvcInfoPtr infoPtr)
{
    SCOPEDWRITE(svcRegVecLock);
    registeringSvcs.push_back(infoPtr);
    LOGDEBUG << "Added svc:" << std::hex << infoPtr->svc_id.svc_uuid.svc_uuid
             << std::dec << " to tracking vector(size:"
             << registeringSvcs.size() << ")";
}

Error
OM_NodeDomainMod::getRegisteringSvc(SvcInfoPtr& infoPtr, int64_t uuid)
{
    Error err(ERR_OK);

    SCOPEDREAD(svcRegVecLock);

    std::vector<SvcInfoPtr>::iterator iter;
    iter = std::find_if(registeringSvcs.begin(), registeringSvcs.end(),
                        [uuid](SvcInfoPtr info)->bool
                        {
                            return uuid == info->svc_id.svc_uuid.svc_uuid;
                        });
    if (iter == registeringSvcs.end()) {
        return Error(ERR_NOT_FOUND);
    }

    infoPtr = *iter;

    return err;
}

void OM_NodeDomainMod::removeRegisteredSvc(int64_t uuid)
{
    SCOPEDWRITE(svcRegVecLock);

    // Repeating the logic from above is safest, since we cannot
    // guarantee the validity of an iterator from the above get
    // function, if other threads came in and modified the vector
    // before we have had a chance to enter the remove
    std::vector<SvcInfoPtr>::iterator iter;
    iter = std::find_if(registeringSvcs.begin(), registeringSvcs.end(),
                        [uuid](SvcInfoPtr info)->bool
                        {
                            return uuid == info->svc_id.svc_uuid.svc_uuid;
                        });

    if (iter != registeringSvcs.end()) {
        registeringSvcs.erase(iter);

        LOGDEBUG <<"Erased registered svc:"
                 << std::hex << uuid
                 << std::dec << " from tracking vector(size:"
                 << registeringSvcs.size() << ")";

    } else {
        LOGERROR << "Error in erasing registered svc from tracking vector";
    }
}

void OM_NodeDomainMod::fromSvcInfoToFDSP_RegisterNodeTypePtr( 
    fpi::SvcInfo svc, 
    fpi::FDSP_RegisterNodeTypePtr& reg_node_req )
{
    TRACEFUNC; 
    
    reg_node_req->control_port = svc.svc_port;
    reg_node_req->data_port = svc.svc_port;
    reg_node_req->ip_lo_addr = fds::net::ipString2Addr( svc.ip );  
    reg_node_req->node_name = svc.name;   
    reg_node_req->node_type = svc.svc_type;
   
    fds::assign( reg_node_req->service_uuid, svc.svc_id );
    
    NodeUuid node_uuid;
    node_uuid.uuid_set_type( reg_node_req->service_uuid.uuid, 
                             fpi::FDSP_PLATFORM);
    reg_node_req->node_uuid.uuid = 
        static_cast<int64_t>( node_uuid.uuid_get_val() );

    fds_assert( reg_node_req->node_type != fpi::FDSP_PLATFORM ||
                reg_node_req->service_uuid == reg_node_req->node_uuid);

    if ( reg_node_req->node_type == fpi::FDSP_PLATFORM ) 
    {
        fpi::FDSP_AnnounceDiskCapability& diskInfo = reg_node_req->disk_info;

        util::Properties props( &svc.props );
        diskInfo.node_iops_max = props.getInt( "node_iops_max" );
        diskInfo.node_iops_min = props.getInt( "node_iops_min" );
        diskInfo.disk_capacity = props.getDouble( "disk_capacity" );
        diskInfo.ssd_capacity = props.getDouble( "ssd_capacity" );
        diskInfo.disk_type = props.getInt( "disk_type" );
    }
}

void OM_NodeDomainMod::fromSvcInfoToFDSP_RegisterNodeTypePtr( 
    boost::shared_ptr<fpi::SvcInfo>& svcInfo, 
    fpi::FDSP_RegisterNodeTypePtr& reg_node_req )
{
    TRACEFUNC; 
    
    reg_node_req->control_port = svcInfo->svc_port;
    reg_node_req->data_port = svcInfo->svc_port;
    reg_node_req->ip_lo_addr = fds::net::ipString2Addr(svcInfo->ip);  
    reg_node_req->node_name = svcInfo->name;   
    reg_node_req->node_type = svcInfo->svc_type;
   
    fds::assign(reg_node_req->service_uuid, svcInfo->svc_id);
    
    NodeUuid node_uuid;
    node_uuid.uuid_set_type( reg_node_req->service_uuid.uuid, 
                             fpi::FDSP_PLATFORM);
    reg_node_req->node_uuid.uuid = 
        static_cast<int64_t>( node_uuid.uuid_get_val() );

    fds_assert( reg_node_req->node_type != fpi::FDSP_PLATFORM ||
                reg_node_req->service_uuid == reg_node_req->node_uuid);

    if ( reg_node_req->node_type == fpi::FDSP_PLATFORM ) 
    {
        fpi::FDSP_AnnounceDiskCapability& diskInfo = reg_node_req->disk_info;

        util::Properties props( &svcInfo->props );
        diskInfo.node_iops_max = props.getInt( "node_iops_max" );
        diskInfo.node_iops_min = props.getInt( "node_iops_min" );
        diskInfo.disk_capacity = props.getDouble( "disk_capacity" );
        diskInfo.ssd_capacity = props.getDouble( "ssd_capacity" );
        diskInfo.disk_type = props.getInt( "disk_type" );
    }
}

Error
OM_NodeDomainMod::om_handle_restart( const NodeUuid& uuid,
                                     const FdspNodeRegPtr msg )
{
    TRACEFUNC; 
        
    LOGDEBUG << "OM Restart, Platform UUID:: "
             << std::hex << ( msg->node_uuid ).uuid << std::dec 
             << " Node Name:: " << msg->node_name;
    
    Error error( ERR_OK );
    NodeAgent::pointer nodeAgent;
    OM_PmContainer::pointer pmNodes;

    pmNodes = om_locDomain->om_pm_nodes();
    fds_assert( pmNodes != NULL );
         
    if ( ( msg->node_type == fpi::FDSP_STOR_MGR ) ||
         ( msg->node_type == fpi::FDSP_DATA_MGR ) ) 
    {
        // for this to be successful, PM must have been registered ( spoof ).
        if ( !pmNodes->check_new_service( ( msg->node_uuid ).uuid, msg->node_type ) ) 
        {
            LOGERROR << "OM Restart, cannot register ( spoof ) service " 
                     << msg->node_name
                     << " on platform with uuid " 
                     << std::hex << ( msg->node_uuid ).uuid << std::dec 
                     << " ( PM is not active. )"; 
            
            return Error( ERR_NODE_NOT_ACTIVE );
        }
    }
        
    error = om_locDomain->dc_register_node( uuid, msg, &nodeAgent );
    if ( error == ERR_DUPLICATE || error.ok() ) 
    {
        nodeAgent->set_node_state( fpi::FDS_Node_Up );
        // ANNA -- I don't want to mess with platform service state, so
        // calling this method for SM and DM only. The register service method
        // set correct discovered/active state for these services based on
        // whether this is known service or restarting service. We are
        // going to set node state based on service state in svc map
        if (msg->node_type == fpi::FDSP_STOR_MGR) {
            OM_SmAgent::pointer smAgent = om_sm_agent(nodeAgent->get_uuid());
            smAgent->set_state_from_svcmap();
        } else if (msg->node_type == fpi::FDSP_DATA_MGR) {
            OM_DmAgent::pointer dmAgent = om_dm_agent(nodeAgent->get_uuid());
            dmAgent->set_state_from_svcmap();
        }
        
        if ( msg->node_type == fpi::FDSP_STOR_MGR )
        {
            /*
             *  Activate and account node capacity only when SM ( spoof )
             *  registers with OM.
             */
            auto pm = 
                OM_PmAgent::agt_cast_ptr( 
                    pmNodes->agent_info(NodeUuid(msg->node_uuid.uuid)));
            if ( pm != NULL ) 
            {
                om_locDomain->om_update_capacity( pm, true );
            } 
            else 
            {
                LOGWARN << "Cannot find platform agent for node UUID ( "
                        << std::hex << msg->node_uuid.uuid << std::dec << " )";
            }
        }
            
        LOGNOTIFY << "OM Restart, spoof registration for"
             << " Platform UUID:: "
             << std::hex << ( msg->node_uuid ).uuid << std::dec 
             << " Service UUID:: "
             << std::hex << ( msg->service_uuid ).uuid << std::dec
             << " Type:: " << msg->node_type;
        error = ERR_OK;
    }
    
    return error;
}

// om_reg_node_info
// ----------------
//
Error
OM_NodeDomainMod::om_reg_node_info(const NodeUuid&      uuid,
                                   const FdspNodeRegPtr msg)
{
    TRACEFUNC;
    NodeAgent::pointer      newNode;
    OM_PmContainer::pointer pmNodes;
    bool fPrevRegistered = false;

    pmNodes = om_locDomain->om_pm_nodes();
    fds_assert(pmNodes != NULL);

    LOGNORMAL << "OM received register node for platform uuid " << std::hex
        << msg->node_uuid.uuid << ", service uuid " << msg->service_uuid.uuid
        << std::dec << ", type " << msg->node_type << ", ip "
        << net::ipAddr2String(msg->ip_lo_addr) << ", control port "
        << msg->control_port;

    if ((msg->node_type == fpi::FDSP_STOR_MGR) ||
        (msg->node_type == fpi::FDSP_DATA_MGR)) {
        // we must have a node (platform) that runs any service
        // registered with OM and node must be in active state
        if (!pmNodes->check_new_service((msg->node_uuid).uuid, msg->node_type)) {
            bool fAllowReRegistration = MODULEPROVIDER()->get_fds_config()->get<bool>
                    ("fds.feature_toggle.om.allow_service_reregistration", true);
            if (pmNodes->hasRegistered(msg) && fAllowReRegistration) {
                fPrevRegistered = true;
                LOGDEBUG << "re registration : " << msg->service_uuid.uuid;
            } else {
                LOGERROR << "Error: cannot register service " << msg->node_name
                         << " on platform with uuid " 
                         << std::hex << (msg->node_uuid).uuid << std::dec;
                return Error(ERR_NODE_NOT_ACTIVE);
            }
        }
    }
        
    Error err = om_locDomain->dc_register_node(uuid, msg, &newNode);
    if (err == ERR_DUPLICATE) {
        fPrevRegistered = true;
        LOGNOTIFY << "Service already exists; probably is re-registering "
                  << " after domain startup (re-activate after shutdown), "
                  << "uuid " << std::hex << (msg->node_uuid).uuid << std::dec;
        err = ERR_OK;  // this is ok, still want to continue re-registration
    }

    /**
     * Note this is a temporary hack to return the node registration call 
     * immediately and wait for 3 seconds before broadcast...
     */
    
    if (err.ok() && (msg->node_type != fpi::FDSP_PLATFORM)) {
        auto timer = MODULEPROVIDER()->getTimer();
        auto task = boost::shared_ptr<FdsTimerTask>(
            new FdsTimerFunctionTask(
                *timer,
                [this, uuid, msg, newNode, fPrevRegistered] () {
                /* Immediately post to threadpool so we don't hold up timer thread */
                MODULEPROVIDER()->proc_thrpool()->schedule(
                    &OM_NodeDomainMod::setupNewNode,
                    this, 
                    uuid, 
                    msg, 
                    newNode, 
                    fPrevRegistered );
                }));
        /* schedule the task to be run on timer thread after 3 seconds */
        if ( timer->schedule( task, std::chrono::seconds( 3 ) ) )
        {
            LOGNORMAL << "Successfully scheduled 'setupNewNode' for uuid " 
                      << std::hex << uuid << std::dec;
        }
        else
        {
            LOGERROR << "Failed to schedule 'setupNewNode' for "
                     << std::hex << uuid << std::dec;
            err = Error( ERR_TIMER_TASK_NOT_SCHEDULED );    
        }
    }

    return err;
}

void OM_NodeDomainMod::setupNewNode(const NodeUuid&      uuid,
                                    const FdspNodeRegPtr msg,
                                    NodeAgent::pointer   newNode,
                                    bool fPrevRegistered) {

    LOGNORMAL << "Scheduled task 'setupNewNode' started, uuid "
              << std::hex << uuid << std::dec;

    Error err(ERR_OK);
    OM_PmContainer::pointer pmNodes;

    pmNodes = om_locDomain->om_pm_nodes();
    fds_assert(pmNodes != NULL);

    fds_verify(newNode != NULL);

    // tell parent PM Agent about its new service
    newNode->set_node_state(fpi::FDS_Node_Up);
    // ANNA -- I don't want to mess with platform service state, so
    // calling this method for SM and DM only. The register service method
    // set correct discovered/active state for these services based on
    // whether this is known service or restarting service. We are
    // going to set node state based on service state in svc map
    if (msg->node_type == fpi::FDSP_STOR_MGR) {
        OM_SmAgent::pointer smAgent = om_sm_agent(newNode->get_uuid());
        smAgent->set_state_from_svcmap();
    } else if (msg->node_type == fpi::FDSP_DATA_MGR) {
        OM_DmAgent::pointer dmAgent = om_dm_agent(newNode->get_uuid());
        dmAgent->set_state_from_svcmap();
    }

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
    // Vy: we could get duplicate if the agent already registered by platform lib.
    // fds_verify(err.ok());

        if ( fpi::FDSP_CONSOLE == msg->node_type || 
             fpi::FDSP_TEST_APP == msg->node_type ) {
            return;
        }

    // Let this new node know about existing node list.
    // TODO(Andrew): this should change into dissemination of the cur cluster map.
    //
    if (msg->node_type == fpi::FDSP_STOR_MGR) {
        // Activate and account node capacity only when SM registers with OM.
        //
        auto pm = OM_PmAgent::agt_cast_ptr(pmNodes->\
                                           agent_info(NodeUuid(msg->node_uuid.uuid)));
        if (pm != NULL) {
            om_locDomain->om_update_capacity(pm, true);
        } else {
            LOGERROR << "Cannot find platform agent for node UUID ( "
                     << std::hex << msg->node_uuid.uuid << std::dec << " )";
        }
    } else if (msg->node_type == fpi::FDSP_DATA_MGR) {
        om_locDomain->om_bcast_stream_reg_list(newNode);
    }

    // Let this new node know about existing DLT if this is not SM or AM node
    // DLT deploy state machine will take care of SMs
    // AMs would have done the getDLT() call after it finished registered.
    // TODO(Andrew): this should change into dissemination of the cur cluster map.
    if ((msg->node_type != fpi::FDSP_STOR_MGR) &&
            (msg->node_type != fpi::FDSP_ACCESS_MGR)) {
        OM_Module *om = OM_Module::om_singleton();
        DataPlacement *dp = om->om_dataplace_mod();
        OM_SmAgent::agt_cast_ptr(newNode)->om_send_dlt(dp->getCommitedDlt());
    }

    // AM & SM services query for a DMT on startup, and DM node will get DMT
    // as part of a state machine; so not broadcasting DMT to any service!

    // send QOS related info to this node
    om_locDomain->om_send_me_qosinfo(newNode);

    /**
     * Starting with AM, we're going to move into a pull model by utilizing
     * the getDLT() and getDMT() methods, instead of waiting for the OM to
     * broadcast here.
     */
    if (om_local_domain_up()) {
        if (msg->node_type == fpi::FDSP_STOR_MGR) {
            om_dlt_update_cluster();
            om_locDomain->om_bcast_vol_list(newNode);
        } else if (msg->node_type == fpi::FDSP_DATA_MGR) {
            // Send the DMT to DMs.
            om_dmt_update_cluster();
            if (fPrevRegistered) {
                om_locDomain->om_bcast_vol_list(newNode);
                LOGDEBUG << "bcasting vol as domain is up : " << msg->node_type;
            }
        }
    } else {
        local_domain_event(RegNodeEvt(uuid, msg->node_type));

        // on domain re-activate, ok so send volume list right away
        // on restarting from persistent state, none of the volumes will
        // be loaded yet at this point, so broadcast will be NOOP
        // Do not broadcast volumes to AMs!!! An AM will get volume info when
        // it receives an IO for that volume
        if (msg->node_type != fpi::FDSP_ACCESS_MGR) {
            om_locDomain->om_bcast_vol_list(newNode);
            LOGDEBUG << "bcasting vol as domain is NOT up :" << msg->node_type;
        }
    }

    LOGNORMAL << "Scheduled task 'setupNewNode' finished, uuid " 
              << std::hex << uuid << std::dec;

    /*
     * Update the service layer service map up front so that any subsequent
     * communication with that service will work. We have already done this
     * for the PM at the end of om_register_service, so only do updates for other svcs
    */
    if (msg->node_type != fpi::FDSP_PLATFORM) {

        SvcInfoPtr infoPtr;
        Error err = getRegisteringSvc(infoPtr, uuid.uuid_get_val());

        if (err == ERR_OK) {
            LOGNOTIFY <<"Update and broadcast svcMap for svc:"
                      << std::hex
                      << infoPtr->svc_id.svc_uuid.svc_uuid
                      << std::dec;

            MODULEPROVIDER()->getSvcMgr()->updateSvcMap({*infoPtr});
            configDB->updateSvcMap(*infoPtr);
            om_locDomain->om_bcast_svcmap();

            // Now erase the svc from the the local tracking vector
            removeRegisteredSvc(infoPtr->svc_id.svc_uuid.svc_uuid);

        } else {
            LOGERROR << "Could not broadcast svcMap for service:"
                     << std::hex << uuid.uuid_get_val()
                     << std::dec << " , not found";
        }
    }
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
    TRACEFUNC;
    Error err(ERR_OK);

    OM_PmContainer::pointer pmNodes = om_locDomain->om_pm_nodes();
    // make sure that platform agents do not hold references to this node
    // and unregister service resources
    NodeUuid uuid;
    if (remove_sm) {
        uuid = pmNodes->
            handle_unregister_service(node_uuid, node_name, fpi::FDSP_STOR_MGR);
        if (uuid.uuid_get_val() != 0) {
            // dc_unregister_service requires node name and checks if UUID matches service
            // name, however handle_unregister_service returns service UUID only if there is
            // one service with the same name, so ok if we get name first
            OM_SmAgent::pointer smAgent = om_sm_agent(uuid);

            // remove this SM's capacity from total capacity
            auto pm = OM_PmAgent::agt_cast_ptr(pmNodes->agent_info(node_uuid));
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

            // send shutdown message to SM
            if (err.ok()) {
                err = smAgent->om_send_shutdown();
            } else {
                LOGERROR << "Failed to unregister SM node " << node_name << ":"
                         << std::dec << node_uuid.uuid_get_val() << std::hex << " " << err
                         << ". Not sending shutdown message to SM";
            }
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
            // send shutdown message to DM
            if (err.ok()) {
                err = dmAgent->om_send_shutdown();
            } else {
                LOGERROR << "Failed to unregister DM node " << node_name << ":"
                         << std::dec << node_uuid.uuid_get_val() << std::hex << " " << err
                         << ". Not sending shutdown message to DM";
            }
        }
        om_dmt_update_cluster();
    }
    if (err.ok() && remove_am) {
        uuid = pmNodes->
            handle_unregister_service(node_uuid, node_name, fpi::FDSP_ACCESS_MGR);
        if (uuid.uuid_get_val() != 0) {
            OM_AmAgent::pointer amAgent = om_am_agent(uuid);
            err = om_locDomain->dc_unregister_node(uuid, amAgent->get_node_name());
            LOGDEBUG << "Unregistered AM service for node " << node_name
                     << ":" << std::dec << node_uuid.uuid_get_val() << std::hex
                     << " result " << err.GetErrstr();

            // send shutdown msg to AM
            if (err.ok()) {
                err = amAgent->om_send_shutdown();
            } else {
                LOGERROR << "Failed to unregister AM node " << node_name << ":"
                         << std::dec << node_uuid.uuid_get_val() << std::hex << " " << err
                         << ". Not sending shutdown message to AM";
            }
        }
    }

    return err;
}

// om_shutdown_domain
// ------------------------
//
Error
OM_NodeDomainMod::om_shutdown_domain()
{
    Error err(ERR_OK);
    if (!om_local_domain_up()) {
        LOGWARN << "Domain is not up yet.. not going to shutdown, try again soon";
        return ERR_NOT_READY;
    }

    LOGNOTIFY << "Shutting down domain, will stop processing node add/remove"
              << " and other commands for the domain";
    local_domain_event(ShutdownEvt());

    return err;
}

void
OM_NodeDomainMod::om_dmt_update_cluster() {
    OM_Module *om = OM_Module::om_singleton();
    OM_DMTMod *dmtMod = om->om_dmt_mod();

    dmtMod->dmt_deploy_event(DmtDeployEvt());
    // in case there are no volume acknowledge to wait
    dmtMod->dmt_deploy_event(DmtVolAckEvt(NodeUuid()));
}
void
OM_NodeDomainMod::om_dmt_waiting_timeout() {
    OM_Module *om = OM_Module::om_singleton();
    OM_DMTMod *dmtMod = om->om_dmt_mod();

    dmtMod->dmt_deploy_event(DmtTimeoutEvt());
    // in case there are no volume acknowledge to wait
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
}

void
OM_NodeDomainMod::om_service_down(const Error& error,
                                  const NodeUuid& svcUuid,
                                  fpi::FDSP_MgrIdType svcType) {
    if (svcType == fpi::FDSP_STOR_MGR) {
        // this is SM -- notify DLT state machine
        om_dlt_update_cluster();
    } else if (svcType == fpi::FDSP_DATA_MGR) {
        // this is DM -- notify DMT state machine
        om_dmt_update_cluster();
    }
}

// Called when OM receives notification that the re-balance is
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

    if (migrError.ok()) {
        // Set node's state to 'node_up'
        agent->set_node_state(FDS_ProtocolInterface::FDS_Node_Up);

        // for now we shouldn't move to new dlt version until
        // we are done with current cluster update, so
        // expect to see migration done response for current dlt version
        fds_uint64_t cur_dlt_ver = dp->getLatestDltVersion();
        fds_verify(cur_dlt_ver == dlt_version);

        // update node's dlt version so we don't send this dlt again
        agent->set_node_dlt_version(dlt_version);

        // 'rebal ok' event, once all nodes sent migration done
        // notification, the state machine will commit the DLT
        // to other nodes.
        ClusterMap* cm = om->om_clusmap_mod();
        dltMod->dlt_deploy_event(DltRebalOkEvt(uuid));
    } else {
        LOGNOTIFY << "Received migration error " << migrError
                  << " will notify DLT state machine";
        dltMod->dlt_deploy_event(DltErrorFoundEvt(uuid, migrError));
    }

    return err;
}


//
// Called when OM received push meta response from DM service
//
Error
OM_NodeDomainMod::om_recv_pull_meta_resp(const NodeUuid& uuid,
                                         const Error& respError) {
    Error err(ERR_OK);
    OM_Module *om = OM_Module::om_singleton();
    OM_DMTMod *dmtMod = om->om_dmt_mod();

#ifdef TEST_DELTA_RSYNC
    LOGNOTIFY << "TEST: Will sleep before processing push meta ack";
    sleep(45);
    LOGNOTIFY << "TEST: Finished sleeping, will process push meta ack";
#endif

    if (respError.ok()) {
        dmtMod->dmt_deploy_event(DmtPushMetaAckEvt(uuid));
    } else {
        dmtMod->dmt_deploy_event(DmtErrorFoundEvt(uuid, respError));
    }
    return err;
}

//
// Called when OM receives response for DMT commit from a node
//
Error
OM_NodeDomainMod::om_recv_dmt_commit_resp(FdspNodeType node_type,
                                          const NodeUuid& uuid,
                                          fds_uint32_t dmt_version,
                                          const Error& respError) {
    Error err(ERR_OK);
    OM_Module *om = OM_Module::om_singleton();
    OM_DMTMod *dmtMod = om->om_dmt_mod();
    OM_NodeContainer *local = om_loc_domain_ctrl();
    AgentContainer::pointer agent_container = local->dc_container_frm_msg(node_type);
    NodeAgent::pointer agent = agent_container->agent_info(uuid);
    if (agent == NULL) {
        LOGERROR << "OM: Received DMT commit ack from unknown node: uuid "
                 << std::hex << uuid.uuid_get_val() << std::dec;
        return ERR_NOT_FOUND;
    }

    // if this is Service Layer error and not DM, we should not stop migration,
    // node is probably down... If this is AM, and it is still up, IO through that
    // AM will start getting DMT mismatch errors; We will need to solve this better
    // when implementing error handling
    // In current implementation, we will stop
    // migration if DM is down, because this could be DM that is source
    // for migration.
    if (respError.ok() ||
        ((respError == ERR_SVC_REQUEST_TIMEOUT ||
          respError == ERR_SVC_REQUEST_INVOCATION) && (node_type != fpi::FDSP_DATA_MGR))) {
        dmtMod->dmt_deploy_event(DmtCommitAckEvt(dmt_version, node_type));
    } else {
        dmtMod->dmt_deploy_event(DmtErrorFoundEvt(uuid, respError));
    }

    // in case dmt is in error mode, also send recover ack, since OM sends
    // dmt commit for previously commited DMT as part of recovery
    dmtMod->dmt_deploy_event(DmtRecoveryEvt(false, uuid, respError));

    return err;
}

//
// Called when OM receives response for DMT close from DM
//
Error
OM_NodeDomainMod::om_recv_dmt_close_resp(const NodeUuid& uuid,
                                         fds_uint64_t dmt_version,
                                         const Error& respError) {
    Error err(ERR_OK);
    OM_Module *om = OM_Module::om_singleton();
    OM_DMTMod *dmtMod = om->om_dmt_mod();

    // tell state machine that we received acknowledge for close
    if (respError.ok()) {
        dmtMod->dmt_deploy_event(DmtCloseOkEvt(dmt_version));
    } else {
        dmtMod->dmt_deploy_event(DmtErrorFoundEvt(uuid, respError));
    }
    return err;
}


//
// Called when OM receives response for DLT commit from a node
//
Error
OM_NodeDomainMod::om_recv_dlt_commit_resp(FdspNodeType node_type,
                                          const NodeUuid& uuid,
                                          fds_uint64_t dlt_version,
                                          const Error& respError) {
    Error err(ERR_OK);
    OM_Module *om = OM_Module::om_singleton();
    OM_DLTMod *dltMod = om->om_dlt_mod();
    DataPlacement *dp = om->om_dataplace_mod();
    OM_NodeContainer *local = om_loc_domain_ctrl();
    AgentContainer::pointer agent_container = local->dc_container_frm_msg(node_type);
    NodeAgent::pointer agent = agent_container->agent_info(uuid);
    if (agent == NULL) {
        LOGERROR << "OM: Received DLT commit acknowledge from unknown node: uuid "
                 << std::hex << uuid.uuid_get_val() << std::dec;
        return ERR_NOT_FOUND;
    }

    // for now we shouldn't move to new dlt version until
    // we are done with current cluster update, so
    // expect to see dlt commit response for current dlt version
    fds_uint64_t cur_dlt_ver = dp->getLatestDltVersion();
    if (cur_dlt_ver > (dlt_version+1)) {
        LOGWARN << "Received DLT commit for unexpected version:" << dlt_version;
        return err;
    }

    // if this is timeout and not SM, we should not stop migration,
    // node is probably down... In current implementation, we will stop
    // migration if SM is down, because this could be SM that is source
    // for migration.
    if (respError.ok() ||
        ((respError == ERR_SVC_REQUEST_TIMEOUT ||
          respError == ERR_SVC_REQUEST_INVOCATION) && (node_type != fpi::FDSP_STOR_MGR))) {
        // set node's confirmed dlt version to this version
        agent->set_node_dlt_version(dlt_version);

        // commit event, will transition to next state when
        // when all 'up' nodes acknowledged this dlt commit
        dltMod->dlt_deploy_event(DltCommitOkEvt(dlt_version, uuid));
    } else {
        LOGERROR << "Received " << respError << " with DLT commit; handling..";
        dltMod->dlt_deploy_event(DltErrorFoundEvt(uuid, respError));
    }

    // in case dlt is in error mode, also send recover acknowledge, since OM sends
    // dlt commit for previously committed DLT as part of recovery
    dltMod->dlt_deploy_event(DltRecoverAckEvt(false, uuid, 0, respError));

    return err;
}

//
// Called when OM receives response for DLT close from a node
//
Error
OM_NodeDomainMod::om_recv_dlt_close_resp(const NodeUuid& uuid,
                                         fds_uint64_t dlt_version,
                                         const Error& respError) {
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

    // tell state machine that we received ack for close
    // ignore errors here, we are going to complete DLT deployment
    // if we are in this stage.
    dltMod->dlt_deploy_event(DltCloseOkEvt(dlt_version));
    return err;
}

} // namespace fds
