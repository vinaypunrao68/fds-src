/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <string>
#include <vector>
#include <boost/msm/front/state_machine_def.hpp>
#include <boost/msm/front/functor_row.hpp>
#include <OmVolume.h>
#include <OmResources.h>
#include <OmVolumePlacement.h>
#include <orchMgr.h>
#include <OmDmtDeploy.h>
#include <orch-mgr/om-service.h>
#include <util/type.h>
#include <util/stringutils.h>
#include <unistd.h>
#define STATELOG(...) GLOGDEBUG << "[evt:" << fds::util::type(evt) \
    << " src:" << fds::util::type(src)                             \
    << " dst:" << fds::util::type(dst)                             \
    << " @:" << fds::util::type(*this)                             \
    << "]";
//    << " cur:" << current_state_name(fsm)

#define STATE_ENTER_EXIT(...)                                                                     \
template <class Event, class FSM> void on_entry(Event const & evt, FSM & fsm) {                   \
    GLOGDEBUG << "[enter : " << fds::util::type(*this) << " evt:" << fds::util::type(evt) << "]"; \
}                                                                                                 \
template <class Event, class FSM> void on_exit(Event const &evt, FSM &fsm) {                      \
    GLOGDEBUG << "[exit  : " << fds::util::type(*this) << " evt:" << fds::util::type(evt) << "]"; \
}

#define DEFINE_GUARD(name)                                        \
    struct name                                                   \
    {                                                             \
        template <class Evt, class Fsm, class SrcST, class TgtST> \
        bool operator()(Evt const &, Fsm &, SrcST &, TgtST &);    \
    }

#define DEFINE_ACTION(name)                                       \
    struct name                                                   \
    {                                                             \
        template <class Evt, class Fsm, class SrcST, class TgtST> \
        void operator()(Evt const &, Fsm &, SrcST &, TgtST &);    \
    }


namespace fds {

// --------------------------------------------------------------------------------------
// Volume state machine
// --------------------------------------------------------------------------------------
namespace msm = boost::msm;
namespace mpl = boost::mpl;
namespace msf = msm::front;

    // Refer: http://www.boost.org/doc/libs/1_46_0/libs/msm/doc/HTML/ch06s03.html#internals-state-id
    // NOTE: the order MUST be :
    //   'Start' column top -> bottom followed by
    //   'Next'  column top -> bottom
static char const * state_names[] = {
    "Inactive", "CrtPend", "Active", "Waiting" , "DetachPend", "DelPend", "DelSent" , "DelNotPend", "DelDone" //NOLINT
};

/*
const char* current_state_name(FSM_Volume& fsm) {
    const int* states = fsm.current_state();
    int numStates = sizeof(states) / sizeof(int); //NOLINT
    if ( 0 == numStates ) {
        return "unknown";
    }
    return state_names[states[0]];
}
*/

/**
* Flags -- allow info about a property of the current state
*/
struct VolumeInactive {};
struct VolumeDelPending {};
struct VolumeDelCheck {};

/**
 * OM Volume State Machine structure
 */
struct VolumeFSM: public msm::front::state_machine_def<VolumeFSM>, public HasLogger
{
    template <class Event, class FSM> void on_entry(Event const &, FSM &);
    template <class Event, class FSM> void on_exit(Event const &, FSM &);

    /**
     * OM Volume states
     */
    struct VST_Inactive: public msm::front::state<>
    {
        typedef mpl::vector1<VolumeInactive> flag_list;

        /**
         * We defer VolOpEvt (operations on this volume) in this state.
         * TODO(xxx) if we fail to add a volume, we should add the functionality
         * of getting all these events from the deferred queue and sending error
         * response for them
         *
         * Note that we don't need to defer "VolCrtOkEvt" in this state, because
         * even if vol create acks start arriving while we are still sending crt
         * msgs in VACT_NotifCrt, VolCrtOkEvt will not be executed until we
         * leave VACT_NotifCrt method -- because state machine events are processed
         * under lock.
         */
        typedef mpl::vector<VolOpEvt> deferred_events;

        template <class Evt, class Fsm, class State>
        void operator()(Evt const &, Fsm &, State &) {}

        STATE_ENTER_EXIT();
    };

    struct VST_CrtPend: public msm::front::state<>
    {
        typedef mpl::vector<VolOpEvt> deferred_events;

        VST_CrtPend() : acks_to_wait(0) {}

        template <class Evt, class Fsm, class State>
        void operator()(Evt const &, Fsm &, State &) {}

        STATE_ENTER_EXIT();
        int acks_to_wait;
    };

    struct VST_Active: public msm::front::state<>
    {
        template <class Evt, class Fsm, class State>
        void operator()(Evt const &, Fsm &, State &) {}

        STATE_ENTER_EXIT();
    };

    struct VST_Waiting: public msm::front::state<>
    {
        typedef mpl::vector<VolOpEvt> deferred_events;

        template <class Evt, class Fsm, class State>
        void operator()(Evt const &, Fsm &, State &) {}

        STATE_ENTER_EXIT();

        om_vol_notify_t wait_for_type;
    };

    struct VST_DelSent: public msm::front::state<>
    {
        typedef mpl::vector1<VolumeDelCheck> flag_list;

        VST_DelSent() : del_chk_ack_wait(0) {}

        template <class Evt, class Fsm, class State>
        void operator()(Evt const &, Fsm &, State &) {}

        STATE_ENTER_EXIT();
        fds_uint32_t del_chk_ack_wait;
    };

    struct VST_DelPend: public msm::front::state<>
    {
        typedef mpl::vector1<VolumeDelPending> flag_list;

        template <class Evt, class Fsm, class State>
        void operator()(Evt const &, Fsm &, State &) {}

        STATE_ENTER_EXIT();
    };

    struct VST_DetachPend: public msm::front::state<>
    {
        typedef mpl::vector1<VolumeDelPending> flag_list;

        template <class Evt, class Fsm, class State>
        void operator()(Evt const &, Fsm &, State &) {}

        STATE_ENTER_EXIT();
        int detach_ack_wait = 0;
    };

    struct VST_DelNotPend: public msm::front::state<>
    {
        typedef mpl::vector1<VolumeDelPending> flag_list;
        VST_DelNotPend() : del_notify_ack_wait(0) {}

        template <class Evt, class Fsm, class State>
        void operator()(Evt const &, Fsm &, State &) {}

        STATE_ENTER_EXIT();
        int del_notify_ack_wait;
    };

    struct VST_DelDone: public msm::front::state<>
    {
        typedef mpl::vector1<VolumeDelPending> flag_list;

        template <class Evt, class Fsm, class State>
        void operator()(Evt const &, Fsm &, State &) {}

        STATE_ENTER_EXIT();
    };

    /**
     * Initial state
     */
    typedef VST_Inactive initial_state;

    /**
     * Transition actions
     */
    DEFINE_ACTION(VACT_NotifCrt);
    DEFINE_ACTION(VACT_CrtDone);
    DEFINE_ACTION(VACT_VolOp);
    DEFINE_ACTION(VACT_OpResp);
    DEFINE_ACTION(VACT_DelChk);
    DEFINE_ACTION(VACT_Detach);
    DEFINE_ACTION(VACT_QueueDel);
    DEFINE_ACTION(VACT_DelNotify);
    DEFINE_ACTION(VACT_DelDone);

    /**
     * Guard conditions
     */
    DEFINE_GUARD(GRD_NotifCrt);
    DEFINE_GUARD(GRD_VolCrt);
    DEFINE_GUARD(GRD_OpResp);
    DEFINE_GUARD(GRD_VolOp);
    DEFINE_GUARD(GRD_DelSent);
    DEFINE_GUARD(GRD_QueueDel);
    DEFINE_GUARD(GRD_Detach);
    DEFINE_GUARD(GRD_DelDone);

    /**
     * Transition table for OM Volume life cycle
     */
    struct transition_table : mpl::vector<
        // +---------------------,---------------,----------------,----------------,---------------  , //NOLINT
        // | Start               | Event         | Next           | Action         | Guard           , //NOLINT
        // +---------------------,---------------,----------------,----------------,---------------  , //NOLINT
        msf::Row< VST_Inactive   , VolCreateEvt  , VST_CrtPend    , VACT_NotifCrt  , GRD_NotifCrt>   , //NOLINT
        msf::Row< VST_CrtPend    , VolCrtOkEvt   , VST_Active     , VACT_CrtDone   , GRD_VolCrt  >   , //NOLINT
        msf::Row< VST_Active     , VolOpEvt      , VST_Waiting    , VACT_VolOp     , GRD_VolOp   >   , //NOLINT
        msf::Row< VST_Waiting    , VolOpRespEvt  , VST_Active     , VACT_OpResp    , GRD_OpResp  >   , //NOLINT
        // +---------------------,---------------,----------------,----------------,---------------  , //NOLINT
        msf::Row< VST_Inactive   , VolDeleteEvt  , VST_DelDone    , VACT_DelDone   , msf::none   >   , //NOLINT
        msf::Row< VST_Active     , VolDeleteEvt  , VST_DetachPend , VACT_Detach    , GRD_Detach  >   , //NOLINT
        msf::Row< VST_DetachPend , VolOpRespEvt  , VST_DelPend    , VACT_QueueDel  , GRD_QueueDel>   , //NOLINT
        msf::Row< VST_DelPend    , ResumeDelEvt  , VST_DelSent    , VACT_DelChk    , msf::none   >   , //NOLINT
        msf::Row< VST_DelSent    , DelRespEvt    , VST_DelNotPend , VACT_DelNotify , GRD_DelSent >   , //NOLINT
        msf::Row< VST_DelNotPend , VolOpRespEvt  , VST_DelDone    , VACT_DelDone   , GRD_DelDone >     //NOLINT
        // +---------------------,---------------,----------------,----------------,---------------  , //NOLINT
        >{};  // NOLINT


template <class Event, class FSM> void no_transition(Event const &, FSM &, int);
};

template <class Event, class Fsm>
void VolumeFSM::on_entry(Event const &evt, Fsm &fsm)
{
    GLOGDEBUG << "VolumeFSM on entry";
}

template <class Event, class Fsm>
void VolumeFSM::on_exit(Event const &evt, Fsm &fsm)
{
    GLOGDEBUG << "VolumeFSM on exit";
}

template <class Event, class Fsm>
void VolumeFSM::no_transition(Event const &evt, Fsm &fsm, int state)
{
    GLOGDEBUG << " no transition from state : " << state_names[state]
              << " on evt:" << fds::util::type(evt);
            // << " cur:" << current_state_name(fsm);
}

/**
 * GRD_NotifCrt
 * ------------
 * returns true if volumes from the same DMT column are not in the middle
 * of rebalancing their meta
 */
template <class Evt, class Fsm, class SrcST, class TgtST>
bool VolumeFSM::GRD_NotifCrt::operator()(Evt const &evt, Fsm &fsm, SrcST &src, TgtST &dst)
{
    STATELOG();
    fds_bool_t is_rebal;
    OM_Module *om = OM_Module::om_singleton();
    VolumePlacement* vp = om->om_volplace_mod();
    VolumeInfo *vol = evt.vol_ptr;
    fds_verify(vol != NULL);

    is_rebal = vp->isRebalancing((vol->vol_get_properties())->volUUID);
    if (is_rebal) {
        GLOGNOTIFY << "Volume " << vol->vol_get_name() << ":" << std::hex
                  << (vol->vol_get_properties())->volUUID << std::dec
                  << " is in a DMT column that is rebalancing now! "
                  << "Will delay volume creation until rebalancing is finished";
    }

    return !is_rebal;
}

/**
 * VACT_NotifCrt
 * -------------
 * Notify services about creation of this volume
 */
template <class Evt, class Fsm, class SrcST, class TgtST>
void
VolumeFSM::VACT_NotifCrt::operator()(Evt const &evt, Fsm &fsm, SrcST &src, TgtST &dst)
{
    STATELOG();
    OM_NodeContainer    *local = OM_NodeDomainMod::om_loc_domain_ctrl();
    VolumeInfo *vol = evt.vol_ptr;
    fds_verify(vol != NULL);
    GLOGDEBUG << "VolumeFSM VACT_NotifCrt for volume " << vol->vol_get_name();
    VolumeDesc* volDesc= vol->vol_get_properties();
    if (fpi::ResourceState::Created != volDesc->state) {
        volDesc->state = fpi::ResourceState::Loading;
    }

    // TODO(prem): store state even for volume.
    if (volDesc->isSnapshot()) {
        gl_orch_mgr->getConfigDB()->setSnapshotState(volDesc->getSrcVolumeId(),
                                                     volDesc->volUUID,
                                                     volDesc->state);
    } else {
        gl_orch_mgr->getConfigDB()->setVolumeState(volDesc->volUUID, volDesc->state);
    }
    // we will wait for *all* SMs and DMs to return volume create ack
    // because otherwise we may allow volume and start IO before an SM or DM
    // sets up QoS queue/etc to handle IO from that volume.
    // TODO(anna) once we handle node failures, we need to make sure that
    // volume FSM does not get stuck if node fails in the process of volume
    // create and don't wait indefinitely for acks to arrive.
    dst.acks_to_wait = local->om_bcast_vol_create(vol);
}

/**
 * GRD_VolCrt
 * ------------
 * returns true if we received quorum number of acks for volume create.
 */
template <class Evt, class Fsm, class SrcST, class TgtST>
bool VolumeFSM::GRD_VolCrt::operator()(Evt const &evt, Fsm &fsm, SrcST &src, TgtST &dst)
{
    STATELOG();
    fds_bool_t ret = false;
    GLOGDEBUG << "VolumeFSM GRD_VolCrt";
    if (evt.got_ack) {
        // if we are waiting for acks and got actual ack, acks_to_wait
        // should be positive, otherwise we would have got out of this
        // state before, this is unexpected!
        fds_verify(src.acks_to_wait != 0);
        src.acks_to_wait--;
    }

    ret = (src.acks_to_wait == 0);

    GLOGDEBUG << "GRD_VolCrt: acks to wait " << src.acks_to_wait << " return " << ret;
    return ret;
}

/**
 * VACT_CrtDone
 * ------------
 * Action when we received quorum number of acks for Volume Create
 */
template <class Evt, class Fsm, class SrcST, class TgtST>
void VolumeFSM::VACT_CrtDone::operator()(Evt const &evt, Fsm &fsm, SrcST &src, TgtST &dst)
{
    STATELOG();
    VolumeInfo* vol = evt.vol_ptr;
    VolumeDesc* volDesc = vol->vol_get_properties();
    volDesc->state = fpi::ResourceState::Active;
    GLOGDEBUG << "VolumeFSM VACT_CrtDone for " << volDesc->name;

    // TODO(prem): store state even for volume.
    if (volDesc->isSnapshot()) {
        gl_orch_mgr->getConfigDB()->setSnapshotState(volDesc->getSrcVolumeId(),
                                                 volDesc->volUUID,
                                                 volDesc->state);
    } else {
        gl_orch_mgr->getConfigDB()->setVolumeState(volDesc->volUUID, volDesc->state);
    }

    // nothing to do here -- unless we make cli async and we need to reply
    // TODO(anna) should we respond to create bucket from AM, or we still
    // going to attach the volume, so notify volume attach will be a response?
}

/**
 * GRD_VolOp
 * ------------
 * We only allow volume detach, and modify operations to reach Waiting state
 */
template <class Evt, class Fsm, class SrcST, class TgtST>
bool VolumeFSM::GRD_VolOp::operator()(Evt const &evt, Fsm &fsm, SrcST &src, TgtST &dst)
{
    STATELOG();
    bool ret = false;
    if (evt.op_type == FDS_ProtocolInterface::FDSP_MSG_MODIFY_VOL) {
        ret = true;
    }
    GLOGDEBUG << "VolumeFSM GRD_VolOp operation type " << evt.op_type
             << " guard result: " << ret;
    return ret;
}

/**
 * VACT_VolOp
 * ------------
 * Operation on volume: detach, modify
 * When we receive detach, modify messages for a
 * volume, we just queue them by using deferred events in state machine
 * We are handling each one of them in this method.
 */
template <class Evt, class Fsm, class SrcST, class TgtST>
void VolumeFSM::VACT_VolOp::operator()(Evt const &evt, Fsm &fsm, SrcST &src, TgtST &dst)
{
    STATELOG();
    Error err(ERR_OK);
    VolumeInfo* vol = evt.vol_ptr;
    switch (evt.op_type) {
        case FDS_ProtocolInterface::FDSP_MSG_MODIFY_VOL:
            GLOGDEBUG << "VACT_VolOp:: modify volume";
            dst.wait_for_type = om_notify_vol_mod;
            err = vol->vol_modify(evt.vdesc_ptr);
            break;
        default:
            fds_verify(false);  // guard should have caught this!
    };

    /* if error happened before we sent msg to remote node, finish op now*/
    if (!err.ok()) {
        // err happened before we sent msg to remote node, so finish op now
        fsm.process_event(VolOpRespEvt(vol->rs_get_uuid(), vol, dst.wait_for_type, err));
    }
}

/**
 * GRD_OpResp
 * ------------
 * We received a responses for a current operation (detach, modify)
 * Returns true if the quorum number of responses is received, otherwise
 * returns false
 */
template <class Evt, class Fsm, class SrcST, class TgtST>
bool VolumeFSM::GRD_OpResp::operator()(Evt const &evt, Fsm &fsm, SrcST &src, TgtST &dst)
{
    STATELOG();
    GLOGDEBUG << "VolumeFSM GRD_OpResp wait_for_type " << src.wait_for_type
              << " result: " << evt.op_err.GetErrstr();
    // check if we received the number of responses we need
    if (src.wait_for_type != evt.resp_type)
        return false;

    return true;
}

/**
 * VACT_OpResp
 * ------------
 * We received quorum number of responses for one of the following operations
 * on volume: detach, modify. Send response for this operation back
 * to requesting source.
 */
template <class Evt, class Fsm, class SrcST, class TgtST>
void VolumeFSM::VACT_OpResp::operator()(Evt const &evt, Fsm &fsm, SrcST &src, TgtST &dst)
{
    STATELOG();
    GLOGDEBUG << "VolumeFSM VACT_OpResp";
    // send reponse back to node that initiated the operation
}


/**
 * VACT_DelChk
 * ------------
 * Broadcasts notify vol delete to DMs only with check_only flag on
 * so that DMs check if there are any objects in this bucket
 */
template <class Evt, class Fsm, class SrcST, class TgtST>
void
VolumeFSM::VACT_DelChk::operator()(Evt const &evt, Fsm &fsm, SrcST &src, TgtST &dst)
{
    STATELOG();
    OM_NodeContainer* local = OM_NodeDomainMod::om_loc_domain_ctrl();
    VolumeInfo* vol = evt.vol_ptr;
    GLOGDEBUG << "VACT_DelChk for volume " << vol->vol_get_name();

    dst.del_chk_ack_wait = local->om_bcast_vol_delete(vol, true);
    if (dst.del_chk_ack_wait == 0) {
        GLOGWARN << " no acks to wait ... triggerring delete";
        fsm.process_event(DelRespEvt(vol, Error(ERR_OK)));
    }
}


template <class Evt, class Fsm, class SrcST, class TgtST>
bool VolumeFSM::GRD_QueueDel::operator()(Evt const &evt, Fsm &fsm, SrcST &src, TgtST &dst)
{
    STATELOG();
    // check if we have detached all the nodes
    fds_verify(src.detach_ack_wait > 0);
    src.detach_ack_wait--;
    GLOGDEBUG << " GRD_Detach: acks to wait " << src.detach_ack_wait
              << " detach done: " << (src.detach_ack_wait == 0);

    return (src.detach_ack_wait == 0);
}

/**
 * GRD_Detach
 * ------------
 * Make sure that we are not rebalancing before we delete
 * message -- in that case returns true.
 */
template <class Evt, class Fsm, class SrcST, class TgtST>
bool
VolumeFSM::GRD_Detach::operator()(Evt const &evt, Fsm &fsm, SrcST &src, TgtST &dst)
{
    STATELOG();
    // to return true, we should not be rebalancing this volume,
    // otherwise we return false, and continue with volume deletion
    // when rebalancing is finished
    OM_Module *om = OM_Module::om_singleton();
    VolumePlacement* vp = om->om_volplace_mod();
    VolumeInfo* vol = evt.vol_ptr;
    fds_verify(vol);
    if (vp->isRebalancing((vol->vol_get_properties())->volUUID)) {
        GLOGNOTIFY << "Volume " << vol->vol_get_name() << ":" << std::hex
                   << (vol->vol_get_properties())->volUUID << std::dec
                   << " is in a DMT column that is rebalancing now! "
                   << "Will delay volume deletion until rebalancing is finished";
        return false;
    }
    return true;
}

/**
 * GRD_DelSent
 * ------------
 * Guard to start deleting the volume. Returns true if all DMs said there are
 * no objects in this volume (reponses to delete vol notifications). Otherwise,
 * returns false and we stay in the current state.
 */
template <class Evt, class Fsm, class SrcST, class TgtST>
bool VolumeFSM::GRD_DelSent::operator()(Evt const &evt, Fsm &fsm, SrcST &src, TgtST &dst)
{
    STATELOG();
    bool ret = false;
    VolumeInfo* vol = evt.vol_ptr;
    GLOGDEBUG << "GRD_DelSent volume " << vol->vol_get_name()
             << "result : " << evt.chk_err.GetErrstr();

    if (evt.recvd_ack && (src.del_chk_ack_wait > 0)) {
        src.del_chk_ack_wait--;
    }

    // if we got an error -- go back to active state and send response
    // that delete volume failed, all subsequent delete notify from other DMs
    // will be ignored
    if (!evt.chk_err.ok()) {
        src.del_chk_ack_wait = 0;
    } else if (src.del_chk_ack_wait == 0) {
        ret = true;

        // to return true, we should not be rebalancing this volume,
        // otherwise we return false, and continue with volume deletion
        // when rebalancing is finished
        OM_Module *om = OM_Module::om_singleton();
        VolumePlacement* vp = om->om_volplace_mod();
        fds_verify(vol);
        if (vp->isRebalancing((vol->vol_get_properties())->volUUID)) {
            GLOGNOTIFY << "Volume " << vol->vol_get_name() << ":" << std::hex
                      << (vol->vol_get_properties())->volUUID << std::dec
                      << " is in a DMT column that is rebalancing now! "
                      << "Will delay volume deletion until rebalancing is finished";
            ret = false;
        }
    }

    GLOGDEBUG << "GRD_DelSent: acks to wait " << src.del_chk_ack_wait << " return " << ret;
    if (ret) {
        uint64_t TIMEWAIT = 10;  // 10 secs
        GLOGWARN << "waiting for [" << TIMEWAIT << "] seconds";
        usleep(TIMEWAIT*1000*1000);
    }
    return ret;
}

/**
 * VACT_Detach
 * -------------
 * We are here because this is the first step in volume deletion
 */
template <class Evt, class Fsm, class SrcST, class TgtST>
void
VolumeFSM::VACT_Detach::operator()(Evt const &evt, Fsm &fsm, SrcST &src, TgtST &dst)
{
    STATELOG();
    VolumeInfo* vol = evt.vol_ptr;
    GLOGDEBUG << "VolumeFSM VACT_Detach";
    VolumeDesc* volDesc = vol->vol_get_properties();

    // start delete volume process
    dst.detach_ack_wait = vol->vol_start_delete();
    if (dst.detach_ack_wait <= 0) {
        // no attached ams ..
        // dummy am
        dst.detach_ack_wait = 1;
        fsm.process_event(VolOpRespEvt(vol->rs_get_uuid(), vol,
                                       om_notify_vol_detach,
                                       Error(ERR_OK)));
    }
}

/**
 * We are here because the am's have been detached
 * No queue the delete request for future deletion
 */
template <class Evt, class Fsm, class SrcST, class TgtST>
void
VolumeFSM::VACT_QueueDel::operator()(Evt const &evt, Fsm &fsm, SrcST &src, TgtST &dst)
{
    STATELOG();
    VolumeInfo* vol = evt.vol_ptr;
    VolumeDesc* volDesc = vol->vol_get_properties();
    volDesc->state = fpi::ResourceState::MarkedForDeletion;

    // TODO(prem): store state even for volume.
    if (volDesc->isSnapshot()) {
        gl_orch_mgr->getConfigDB()->setSnapshotState(volDesc->getSrcVolumeId(),
                                                 volDesc->volUUID,
                                                 volDesc->state);
    } else {
        gl_orch_mgr->getConfigDB()->setVolumeState(volDesc->volUUID, volDesc->state);
    }

    // schedule the volume for deletion
    gl_orch_mgr->deleteScheduler.scheduleVolume(volDesc->volUUID);
}


/**
 * VACT_DelNotify
 * -------------
 * Broadcasts delete volume notification to all DMs/SMs
 */
template <class Evt, class Fsm, class SrcST, class TgtST>
void
VolumeFSM::VACT_DelNotify::operator()(Evt const &evt, Fsm &fsm, SrcST &src, TgtST &dst)
{
    STATELOG();
    VolumeInfo *vol = evt.vol_ptr;
    // broadcast delete volume notification to all DMs/SMs
    OM_NodeContainer    *local = OM_NodeDomainMod::om_loc_domain_ctrl();
    dst.del_notify_ack_wait = local->om_bcast_vol_delete(vol, false) + 1;
    fsm.process_event(VolOpRespEvt(vol->rs_get_uuid(), vol, om_notify_vol_rm, Error(ERR_OK)));
}

/**
 * GRD_DelDone
 * ------------
 * Guards so that we receive the quorum number of reponses from DM/SMs for
 * delete volume message -- in that case returns true.
 */
template <class Evt, class Fsm, class SrcST, class TgtST>
bool VolumeFSM::GRD_DelDone::operator()(Evt const &evt, Fsm &fsm, SrcST &src, TgtST &dst)
{
    STATELOG();
    fds_verify(src.del_notify_ack_wait > 0);
    src.del_notify_ack_wait--;
    bool ret = (src.del_notify_ack_wait == 0);

    GLOGDEBUG << "VolumeFSM GRD_DelDone: acks to wait " << src.del_notify_ack_wait
             << " return " << ret;
    return ret;
}

/**
 * VACT_DelDone
 * -------------
 * Send response for delete volume msg to the requesting node
 */
template <class Evt, class Fsm, class SrcST, class TgtST>
void VolumeFSM::VACT_DelDone::operator()(Evt const &evt, Fsm &fsm, SrcST &src, TgtST &dst)
{
    STATELOG();
    OM_NodeContainer *local = OM_NodeDomainMod::om_loc_domain_ctrl();
    VolumeContainer::pointer volumes = local->om_vol_mgr();
    VolumeInfo::pointer vol = VolumeInfo::vol_cast_ptr(volumes->rs_get_resource(evt.vol_uuid));
    VolumeDesc* volDesc = vol->vol_get_properties();
    volDesc->state = fpi::ResourceState::Deleted;

    // TODO(prem): store state even for volume.
    if (volDesc->isSnapshot()) {
        gl_orch_mgr->getConfigDB()->setSnapshotState(volDesc->getSrcVolumeId(),
                                                 volDesc->volUUID,
                                                 volDesc->state);
    } else {
        gl_orch_mgr->getConfigDB()->setVolumeState(volDesc->volUUID, volDesc->state);
    }

    GLOGDEBUG << "VolumeFSM VACT_DelDone";
    // TODO(anna) Send response to delete volume msg

    // do final cleanup
    volumes->om_cleanup_vol(evt.vol_uuid);
    // DO NOT WRITE ANY CODE IN THIS METHOD BELOW THIS LINE
}


// --------------------------------------------------------------------------------------
// Volume Info
// --------------------------------------------------------------------------------------
VolumeInfo::VolumeInfo(const ResourceUUID &uuid)
        : Resource(uuid), vol_properties(NULL),
          fsm_lock("VolumeInfo fsm lock")  {
    volume_fsm = new FSM_Volume();
}

VolumeInfo::~VolumeInfo()
{
    delete volume_fsm;
    delete vol_properties;
}

// vol_mk_description
// ------------------
//
void
VolumeInfo::vol_mk_description(const fpi::FDSP_VolumeDescType &info)
{
    vol_properties = new VolumeDesc(info, fds_volid_t(rs_uuid.uuid_get_val()));
    setName(info.vol_name);
    vol_name.assign(rs_name);

    volume_fsm->start();
}

// setDescription
// ---------------------
//
void
VolumeInfo::setDescription(const VolumeDesc &desc)
{
    if (vol_properties == NULL) {
        vol_properties = new VolumeDesc(desc);
    } else {
        delete vol_properties;
        vol_properties = new VolumeDesc(desc);
    }
    setName(desc.name);
    vol_name = desc.name;
    volUUID = desc.volUUID;
}

void VolumeInfo::initSnapshotVolInfo(VolumeInfo::pointer vol, const fpi::Snapshot& snapshot) {
    if (vol->vol_properties != NULL) {
        delete vol->vol_properties;
    }
    vol->vol_properties = new VolumeDesc(*vol_properties);
    vol->setName(snapshot.snapshotName);
    vol->vol_name = snapshot.snapshotName;
    vol->volUUID =  snapshot.snapshotId;
    vol->vol_properties->name = snapshot.snapshotName;
    vol->vol_properties->srcVolumeId = snapshot.volumeId;
    vol->vol_properties->lookupVolumeId = snapshot.volumeId;
    vol->vol_properties->qosQueueId = getUuidFromVolumeName(std::to_string(snapshot.volumeId));

    vol->vol_properties->volUUID = snapshot.snapshotId;
    vol->vol_properties->fSnapshot = true;
    vol->vol_properties->timelineTime = snapshot.timelineTime;

    vol->volume_fsm->start();
}

// vol_fmt_desc_pkt
// ----------------
//
void
VolumeInfo::vol_fmt_desc_pkt(fpi::FDSP_VolumeDescType *pkt) const
{
    VolumeDesc *pVol;

    pVol               = vol_properties;
    pkt->vol_name      = pVol->name;
    pkt->volUUID       = pVol->volUUID.get();
    pkt->tennantId     = pVol->tennantId;
    pkt->localDomainId = pVol->localDomainId;

    pkt->maxObjSizeInBytes = pVol->maxObjSizeInBytes;
    pkt->capacity      = pVol->capacity;
    pkt->volType       = pVol->volType;

    pkt->volPolicyId   = pVol->volPolicyId;
    pkt->iops_throttle = pVol->iops_throttle;
    pkt->iops_assured  = pVol->iops_assured;
    pkt->rel_prio      = pVol->relativePrio;

    pkt->mediaPolicy   = pVol->mediaPolicy;
    pkt->fSnapshot   = pVol->fSnapshot;
    pkt->srcVolumeId = pVol->srcVolumeId.get();
    pkt->contCommitlogRetention = pVol->contCommitlogRetention;
    pkt->timelineTime = pVol->timelineTime;
    pkt->state        = pVol->getState();
}

// vol_fmt_message
// ---------------
//
void
VolumeInfo::vol_fmt_message(om_vol_msg_t *out)
{
    switch (out->vol_msg_code) {
        case fpi::FDSP_MSG_GET_BUCKET_STATS_RSP: {
            VolumeDesc               *desc = vol_properties;
            fpi::FDSP_BucketStatType *stat = out->u.vol_stats;

            fds_verify(stat != NULL);
            stat->vol_name = vol_name;
            stat->assured  = desc->iops_assured;
            stat->throttle = desc->iops_throttle;
            stat->rel_prio = desc->relativePrio;
            break;
        }
        case fpi::FDSP_MSG_DELETE_VOL:
        case fpi::FDSP_MSG_MODIFY_VOL:
        case fpi::FDSP_MSG_CREATE_VOL: {
            /* TODO(Andrew): Remove usage of deleted struct fields.
               This code was dead (compiled, but unused) to begin with.
            FdspNotVolPtr notif = *out->u.vol_notif;

            vol_fmt_desc_pkt(&notif->vol_desc);
            notif->vol_name  = vol_get_name();
            if (out->vol_msg_code == fpi::FDSP_MSG_CREATE_VOL) {
                notif->type = fpi::FDSP_NOTIFY_ADD_VOL;
            } else if (out->vol_msg_code == fpi::FDSP_MSG_MODIFY_VOL) {
                notif->type = fpi::FDSP_NOTIFY_MOD_VOL;
            } else {
                notif->type = fpi::FDSP_NOTIFY_RM_VOL;
            }
            break;
            */
        }
        case fpi::FDSP_MSG_DETACH_VOL_CTRL: {
            /* TODO(Andrew): Remove usage of deleted struct fields.
               This code was dead (compiled, but unused) to begin with.

            vol_fmt_desc_pkt(&attach->vol_desc);
            attach->vol_name = vol_get_name();
            break;
            */
        }
        default: {
            fds_panic("Unknown volume request code");
            break;
        }
    }
}


// vol_am_agent
// ------------
//
NodeAgent::pointer
VolumeInfo::vol_am_agent(const NodeUuid &am_uuid)
{
    OM_NodeContainer  *local = OM_NodeDomainMod::om_loc_domain_ctrl();

    return local->om_am_agent(am_uuid);
}

Error
VolumeInfo::vol_modify(const boost::shared_ptr<VolumeDesc>& vdesc_ptr)
{
    Error err(ERR_OK);
    OM_NodeContainer    *local = OM_NodeDomainMod::om_loc_domain_ctrl();
    FdsAdminCtrl        *admin = local->om_get_admin_ctrl();

    // Check if this volume can go through admission control with modified policy info
    //
    err = admin->checkVolModify(vol_get_properties(), vdesc_ptr.get());
    if (!err.ok()) {
        LOGERROR << "Modify volume " << vdesc_ptr->name
                 << " -- cannot admit new policy info, keeping the old policy";
        return err;
    }
    // We admitted modified policy.
    setDescription(*vdesc_ptr);
    // store it in config db..
    if (!gl_orch_mgr->getConfigDB()->addVolume(*vdesc_ptr)) {
        LOGWARN << "unable to store volume info in to config db "
                << "[" << vdesc_ptr->name << ":" <<vdesc_ptr->volUUID << "]";
    }
    local->om_bcast_vol_modify(this);
    return err;
}

fds_uint32_t
VolumeInfo::vol_start_delete()
{
    fds_uint32_t count = 0;
    OM_NodeContainer    *local = OM_NodeDomainMod::om_loc_domain_ctrl();
    FdsAdminCtrl        *admin = local->om_get_admin_ctrl();

    LOGNOTIFY << "Start delete volume process for volume " << vol_name;

    // tell admission control we are deleting this volume
    admin->updateAdminControlParams(vol_get_properties());

    // send detach msg to all AMs that have this volume attached
    // note, that if this is a block volume, there shouldn't be any
    // AMs that have this volume attached, because we will not start
    // delete process in that case
    count = local->om_bcast_vol_detach(this);

    return count;
}

char const *const
VolumeInfo::vol_current_state()
{
    return state_names[volume_fsm->current_state()[0]];
}

void VolumeInfo::vol_event(VolCreateEvt const &evt) {
    fds_mutex::scoped_lock l(fsm_lock);
    volume_fsm->process_event(evt);
}
void VolumeInfo::vol_event(VolCrtOkEvt const &evt) {
    fds_mutex::scoped_lock l(fsm_lock);
    volume_fsm->process_event(evt);
}
void VolumeInfo::vol_event(VolOpEvt const &evt) {
    fds_mutex::scoped_lock l(fsm_lock);
    volume_fsm->process_event(evt);
}
void VolumeInfo::vol_event(VolOpRespEvt const &evt) {
    fds_mutex::scoped_lock l(fsm_lock);
    volume_fsm->process_event(evt);
}
void VolumeInfo::vol_event(VolDeleteEvt const &evt) {
    fds_mutex::scoped_lock l(fsm_lock);
    volume_fsm->process_event(evt);
}
void VolumeInfo::vol_event(ResumeDelEvt const &evt) {
    fds_mutex::scoped_lock l(fsm_lock);
    volume_fsm->process_event(evt);
}
void VolumeInfo::vol_event(DelRespEvt const &evt) {
    fds_mutex::scoped_lock l(fsm_lock);
    volume_fsm->process_event(evt);
}
void VolumeInfo::vol_event(DelNotifEvt const &evt) {
    fds_mutex::scoped_lock l(fsm_lock);
    volume_fsm->process_event(evt);
}
fds_bool_t VolumeInfo::isVolumeInactive() {
    fds_mutex::scoped_lock l(fsm_lock);
    return volume_fsm->is_flag_active<VolumeInactive>();
}
fds_bool_t VolumeInfo::isCheckDelete() {
    fds_mutex::scoped_lock l(fsm_lock);
    return volume_fsm->is_flag_active<VolumeDelCheck>();
}
fds_bool_t VolumeInfo::isDeletePending() {
    fds_mutex::scoped_lock l(fsm_lock);
    return volume_fsm->is_flag_active<VolumeDelPending>();
}

void VolumeInfo::vol_populate_fdsp_descriptor(fpi::CtrlNotifyVolAdd &fdsp_voladd) {
	VolumeDesc *desc = vol_get_properties();
	fds_assert(desc);

	// Counterpart found in VolumeDesc::VolumeDesc(fpi::FDSP_VolumeDescType)
	fdsp_voladd.vol_desc.vol_name = desc->name;
	fdsp_voladd.vol_desc.tennantId = desc->tennantId;
	fdsp_voladd.vol_desc.localDomainId = desc->localDomainId;
	fdsp_voladd.vol_desc.volUUID = desc->volUUID.v;
	fdsp_voladd.vol_desc.volType = desc->volType;
	fdsp_voladd.vol_desc.maxObjSizeInBytes = desc->maxObjSizeInBytes;
	fdsp_voladd.vol_desc.capacity = desc->capacity;
	fdsp_voladd.vol_desc.volPolicyId = desc->volPolicyId;
	fdsp_voladd.vol_desc.placementPolicy = desc->placementPolicy;
	fdsp_voladd.vol_desc.mediaPolicy = desc->mediaPolicy;
	fdsp_voladd.vol_desc.iops_assured = desc->iops_assured;
	fdsp_voladd.vol_desc.iops_throttle = desc->iops_throttle;
	fdsp_voladd.vol_desc.rel_prio = desc->relativePrio;
	fdsp_voladd.vol_desc.fSnapshot = desc->fSnapshot;
	fdsp_voladd.vol_desc.state = desc->state;
	fdsp_voladd.vol_desc.contCommitlogRetention = desc->contCommitlogRetention;
	fdsp_voladd.vol_desc.srcVolumeId = desc->srcVolumeId.v;
	fdsp_voladd.vol_desc.timelineTime = desc->timelineTime;
	fdsp_voladd.vol_desc.createTime = desc->createTime;
	fdsp_voladd.vol_desc.state = desc->state;
}

// --------------------------------------------------------------------------------------
// Volume Container
// --------------------------------------------------------------------------------------
VolumeContainer::~VolumeContainer() {}
VolumeContainer::VolumeContainer() : RsContainer() {}

// om_create_vol
// -------------
//
Error
VolumeContainer::om_create_vol(const FdspMsgHdrPtr &hdr,
                               const FdspCrtVolPtr &creat_msg)
{
    Error err(ERR_OK);
    OM_NodeContainer    *local = OM_NodeDomainMod::om_loc_domain_ctrl();
    OM_Module *om = OM_Module::om_singleton();
    VolumePlacement* vp = om->om_volplace_mod();
    VolPolicyMgr        *v_pol = OrchMgr::om_policy_mgr();
    FdsAdminCtrl        *admin = local->om_get_admin_ctrl();
    std::string         &vname = (creat_msg->vol_info).vol_name;
    VolumeInfo::pointer  vol;

    // check for a volume with the same name
    if (ERR_OK == getVolumeStatus(vname)) {
        vol = get_volume(vname);
        LOGWARN << "an active volume already exists with the same name : "
                << vname << ":" << vol->rs_get_uuid();
            return ERR_DUPLICATE;
    }

    auto uuid = gl_orch_mgr->getConfigDB()->getNewVolumeId();
    if (invalid_vol_id == uuid) {
        LOGWARN << "unable to generate a vol id";
        return ERR_INVALID_VOL_ID;
    }
    vol = VolumeInfo::vol_cast_ptr(rs_get_resource(uuid.get()));
    if (vol != NULL) {
        LOGWARN << "volume uuid [" << uuid << "] already exists : " << vname;
        return Error(ERR_DUPLICATE);
    }

    vol = VolumeInfo::vol_cast_ptr(rs_alloc_new(uuid.get()));
    vol->vol_mk_description(creat_msg->vol_info);

    err = v_pol->fillVolumeDescPolicy(vol->vol_get_properties());
    if (err == ERR_CAT_ENTRY_NOT_FOUND) {
        // TODO(xxx): policy not in the catalog.  Should we return error or use the
        // default policy?
        //
        LOGWARN << "Create volume " << vname
                << " with unknown policy "
                << creat_msg->vol_info.volPolicyId;
    } else if (!err.ok()) {
        // TODO(xxx): for now ignoring error.
        //
        LOGERROR << "Create volume " << vname
                 << ": failed to get policy info";
        return err;
    }

    err = admin->volAdminControl(vol->vol_get_properties());
    if (!err.ok()) {
        // TODO(Vy): delete the volume here.
        LOGERROR << "Unable to create volume " << vname
                 << " error: " << err.GetErrstr();
        rs_free_resource(vol);
        return err;
    }

    // register before b-casting vol crt, in case we start recevings acks
    // before vol_event for create vol returns
    if (vol->isStateActive()) {
        vol->setState(fpi::ResourceState::Created);
    } else {
        vol->setState(fpi::ResourceState::Loading);
    }
    err = rs_register(vol);
    if (!err.ok()) {
        LOGERROR << "unable to register vol as resource : " << vname;
        rs_free_resource(vol);
        return err;
    }


    const VolumeDesc& volumeDesc=*(vol->vol_get_properties());
    // store it in config db..
    if (!gl_orch_mgr->getConfigDB()->addVolume(volumeDesc)) {
        LOGWARN << "unable to store volume info in to config db "
                << "[" << volumeDesc.name << ":" <<volumeDesc.volUUID << "]";
    }


    // this event will broadcast vol create msg to other nodes and wait for acks
    vol->vol_event(VolCreateEvt(vol.get()));

    // in case there was no one to notify, check if we can proceed to
    // active state right away (otherwise guard will stop us)
    vol->vol_event(VolCrtOkEvt(false, vol.get()));

    return err;
}

// om_snap_vol
// -------------
//
Error
VolumeContainer::om_snap_vol(const FdspMsgHdrPtr &hdr,
                               const FdspCrtVolPtr &snap_msg)
{
    Error err(ERR_OK);
    OM_NodeContainer    *local = OM_NodeDomainMod::om_loc_domain_ctrl();
    FdsAdminCtrl        *admin = local->om_get_admin_ctrl();
    std::string         &vname = snap_msg->vol_name;
    VolumeInfo::pointer  vol;

    vol = get_volume(vname);
    if (vol == NULL) {
        LOGWARN << "Received SnapVol for non-existing volume " << vname;
        return Error(ERR_NOT_FOUND);
    }

    fds_uint32_t count = local->om_bcast_vol_snap(vol);

    return err;
}

// om_delete_vol
// -------------
//
Error
VolumeContainer::om_delete_vol(const FdspMsgHdrPtr &hdr,
                               const FdspDelVolPtr &del_msg)
{
    Error err(ERR_OK);
    std::string         &vname = del_msg->vol_name;
    VolumeInfo::pointer  vol;

    vol = get_volume(vname);
    if (vol == NULL) {
        LOGWARN << "Received DeleteVol for non-existing volume " << vname;
        return Error(ERR_NOT_FOUND);
    }

    // start volume delete process
    vol->vol_event(VolDeleteEvt(vol->rs_get_uuid(), vol.get()));

    return err;
}

Error VolumeContainer::om_delete_vol(fds_volid_t volId) {
    Error err(ERR_OK);
    ResourceUUID         uuid(volId.get());
    VolumeInfo::pointer  vol;

    vol = VolumeInfo::vol_cast_ptr(rs_get_resource(uuid));
    if (vol == NULL) {
        LOGWARN << "Received DeleteVol for non-existing volume " << volId;
        return Error(ERR_NOT_FOUND);
    }

    // start volume delete process
    vol->vol_event(VolDeleteEvt(uuid, vol.get()));

    return err;
}

// om_cleanup_vol
// -------------
//
void
VolumeContainer::om_cleanup_vol(const ResourceUUID& vol_uuid)
{
    VolumeInfo::pointer  vol = VolumeInfo::vol_cast_ptr(rs_get_resource(vol_uuid));
    fds_verify(vol != NULL);
    VolumeDesc* volDesc = vol->vol_get_properties();
    // remove the volume from configDB
    if (volDesc->isSnapshot()) {
        gl_orch_mgr->getConfigDB()->deleteSnapshot(volDesc->getSrcVolumeId(),
                                                   volDesc->volUUID);
    }
    // Nothing to do for volume in config db ..
    // as it should have been marked as Deleted already..
    // We need to retail this info for future.

    rs_unregister(vol);
    rs_free_resource(vol);
}


// get_volume
// ---------
//
VolumeInfo::pointer
VolumeContainer::get_volume(const std::string& vol_name)
{
    return VolumeInfo::vol_cast_ptr(rs_get_resource(vol_name.c_str()));
}

//
// Called by DMT state machine when rebalance is finished, can
// assume that rebalance is off for all volumes. Will call
// VolCreateEvt for all volumes in inactive state, and
// DelRespEvt for all volumes in 'delete check' state
//
void VolumeContainer::continueCreateDeleteVolumes() {
    for (fds_uint32_t i = 0; i < rs_cur_idx; i++) {
        VolumeInfo::pointer vol = VolumeInfo::vol_cast_ptr(rs_array[i]);
        if (rs_array[i] != NULL) {
            if (vol->isVolumeInactive()) {
                vol->vol_event(VolCreateEvt(vol.get()));
                // in case there was no one to notify, check if we can proceed to
                // active state right away (otherwise guard will stop us)
                vol->vol_event(VolCrtOkEvt(false, vol.get()));
            } else if (vol->isCheckDelete()) {
                // check if we can continue with delete process (all del check
                // acks received, but we were waiting for rebalance to finish)
                vol->vol_event(DelRespEvt(vol.get(), ERR_OK, false));
            }
        }
    }
}

Error VolumeContainer::getVolumeStatus(const std::string& volumeName) {
    VolumeInfo::pointer  vol = get_volume(volumeName);
    if (vol == NULL
        || vol->isDeletePending()
        || vol->isStateOffline()
        || vol->isStateDeleted()
        || vol->isStateMarkedForDeletion()) {
        return ERR_NOT_FOUND;
    }

    return ERR_OK;
}

// om_modify_vol
// -------------
//
Error
VolumeContainer::om_modify_vol(const FdspModVolPtr &mod_msg)
{
    Error err(ERR_OK);
    OM_NodeContainer    *local = OM_NodeDomainMod::om_loc_domain_ctrl();
    VolPolicyMgr        *v_pol = OrchMgr::om_policy_mgr();
    FdsAdminCtrl        *admin = local->om_get_admin_ctrl();
    std::string         &vname = (mod_msg->vol_desc).vol_name;

    VolumeInfo::pointer  vol = get_volume(vname);
    if (vol == NULL) {
        LOGWARN << "Received ModifyVol for non-existing volume " << vname;
        return Error(ERR_NOT_FOUND);
    } else if (vol->isDeletePending()) {
        LOGWARN << "Received ModifyVol for volume " << vname
                << " for which delete is pending";
        return Error(ERR_NOT_FOUND);
    }

    boost::shared_ptr<VolumeDesc> new_desc(new VolumeDesc(*(vol->vol_get_properties())));

    // We will not modify capacity for now; just policy id or min/max and priority.
    //
    if ((mod_msg->vol_desc.volPolicyId != 0) &&
        (mod_msg->vol_desc.volPolicyId != mod_msg->vol_desc.volPolicyId)) {
        // Change policy id and its description from the catalog.
        //
        new_desc->volPolicyId = mod_msg->vol_desc.volPolicyId;
        err = v_pol->fillVolumeDescPolicy(new_desc.get());
        if (!err.ok()) {
            const char *msg = (err == ERR_CAT_ENTRY_NOT_FOUND) ?
                    " - requested unknown policy id " :
                    " - failed to get policy info id ";

            // Policy not in the catalog, revert to old policy id and return.
            LOGERROR << "Modify volume " << vname << msg
                     << mod_msg->vol_desc.volPolicyId
                     << " keeping original policy "
                     << vol->vol_get_properties()->volPolicyId;
            return err;
        }
    } else if (mod_msg->vol_desc.volPolicyId != 0) {
        LOGNOTIFY << "Modify volume " << vname
                  << " policy id unchanged, not changing QoS Policy";
    } else {
        // Don't modify policy id, just min/max ips and priority.
        //
        new_desc->iops_assured = mod_msg->vol_desc.iops_assured;
        new_desc->iops_throttle = mod_msg->vol_desc.iops_throttle;
        new_desc->relativePrio = mod_msg->vol_desc.rel_prio;
        LOGNOTIFY << "Modify volume " << vname
                  << " - keeps policy id " << vol->vol_get_properties()->volPolicyId
                  << " with new assured iops " << new_desc->iops_assured
                  << " throttle iops " << new_desc->iops_throttle
                  << " priority " << new_desc->relativePrio;
    }
    if (mod_msg->vol_desc.mediaPolicy != fpi::FDSP_MEDIA_POLICY_UNSET) {
        new_desc->mediaPolicy = mod_msg->vol_desc.mediaPolicy;
        LOGNOTIFY << "Modify volume " << vname
                  << " also set media policy to " << new_desc->mediaPolicy;
    }

    vol->vol_event(VolOpEvt(vol.get(),
                            FDS_ProtocolInterface::FDSP_MSG_MODIFY_VOL,
                            new_desc));
    return err;
}

// om_get_volume_descriptor
// --------------
//
void
VolumeContainer::om_get_volume_descriptor(const boost::shared_ptr<fpi::AsyncHdr>     &hdr,
                                          const std::string& vol_name)
{
    OM_NodeContainer    *local = OM_NodeDomainMod::om_loc_domain_ctrl();
    std::string         vname = vol_name;
    NodeUuid             n_uid(hdr->msg_src_uuid.svc_uuid);
    VolumeInfo::pointer  vol;
    OM_AmAgent::pointer  am;

    LOGNOTIFY << "Received getVolumeDescriptor request " << vname
              << " from " << n_uid;

    am = local->om_am_agent(n_uid);
    if (am == NULL) {
        LOGNOTIFY << "OM does not know about node " << hdr->msg_src_uuid.svc_uuid;
    }
    vol = get_volume(vol_name);
    if (vol == NULL || vol->isDeletePending()) {
        if (vol) {
            LOGNORMAL << "delete pending on bucket " << vname;
        } else {
            LOGNOTIFY << "invalid bucket " << vname;
        }
        if (am != NULL) {
            am->om_send_vol_cmd(NULL, &vname, fpi::CtrlNotifyVolAddTypeId);
        }
    } else {
        // TODO(bszmyd): Thu 14 May 2015 06:49:26 AM MDT
        // Should make this a response a la SvcLayer and not an RPC message
        // Respond
        if (am != NULL) {
            am->om_send_vol_cmd(vol, fpi::CtrlNotifyVolAddTypeId);
        }
    }
}

void
VolumeContainer::om_notify_vol_resp(om_vol_notify_t type, NodeUuid from_svc, Error resp_err,
                                    const std::string& vol_name,
                                    const ResourceUUID& vol_uuid)
{
    VolumeInfo::pointer vol = VolumeInfo::vol_cast_ptr(rs_get_resource(vol_uuid));
    OM_Module *om = OM_Module::om_singleton();
    OM_DMTMod *dmtMod = om->om_dmt_mod();
    if (vol == NULL) {
        // we probably deleted a volume, just print a warning
        LOGWARN << "Received volume notify response for volume "
                << vol_name << " but volume does not exist anymore; "
                << "it was probably just deleted";
        return;
    }

    switch (type) {
        case om_notify_vol_add:
            if (resp_err.ok()) {
                vol->vol_event(VolCrtOkEvt(true, vol.get()));
                dmtMod->dmt_deploy_event(DmtVolAckEvt(from_svc));
            } else {
                // TODO(anna) send response to volume create here with error
                LOGERROR << "Received volume create response with error ("
                         << resp_err.GetErrstr() << ")"
                         << ", will revert volume create for " << vol_name;

                // start remove volume process (here we don't need to check
                // with DMs if there are any objects, since volume was never
                // attached to any AMs at this point)
                vol->vol_event(VolDeleteEvt(vol->rs_get_uuid(), vol.get()));
            }
            break;
        case om_notify_vol_mod:
        case om_notify_vol_detach:
        case om_notify_vol_rm:
            vol->vol_event(VolOpRespEvt(vol_uuid, vol.get(), type, resp_err));
            break;
        case om_notify_vol_rm_chk:
            vol->vol_event(DelRespEvt(vol.get(), resp_err));
            break;
        default:
            fds_verify(false);  // if there is a new vol notify type, add handling
    };
}

void
VolumeContainer::om_notify_vol_resp(om_vol_notify_t type,
                                    FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& fdsp_msg,
                                    const std::string& vol_name,
                                    const ResourceUUID& vol_uuid)
{
    VolumeInfo::pointer vol = VolumeInfo::vol_cast_ptr(rs_get_resource(vol_uuid));
    OM_Module *om = OM_Module::om_singleton();
    OM_DMTMod *dmtMod = om->om_dmt_mod();
    if (vol == NULL) {
        // we probably deleted a volume, just print a warning
        LOGWARN << "Received volume notify response for volume "
                << vol_name << " but volume does not exist anymore; "
                << "it was probably just deleted";
        return;
    }
    Error resp_err(fdsp_msg->err_code);
    LOGDEBUG << "received notification : " << type;
    switch (type) {
        case om_notify_vol_add:
            if (resp_err.ok()) {
                NodeUuid from_svc((fdsp_msg->src_service_uuid).uuid);
                vol->vol_event(VolCrtOkEvt(true, vol.get()));
                dmtMod->dmt_deploy_event(DmtVolAckEvt(from_svc));
            } else {
                // TODO(anna) send response to volume create here with error
                LOGERROR << "Received volume create response with error ("
                         << resp_err.GetErrstr() << ")"
                         << ", will revert volume create for " << vol_name;

                // start remove volume process (here we don't need to check
                // with DMs if there are any objects, since volume was never
                // attached to any AMs at this point)
                vol->vol_event(VolDeleteEvt(vol->rs_get_uuid(), vol.get()));
            }
            break;
        case om_notify_vol_mod:
        case om_notify_vol_detach:
        case om_notify_vol_rm:
            vol->vol_event(VolOpRespEvt(vol_uuid, vol.get(), type, resp_err));
            break;
        case om_notify_vol_rm_chk:
            vol->vol_event(DelRespEvt(vol.get(), resp_err));
            break;
        default:
            fds_verify(false);  // if there is a new vol notify type, add handling
    };
}


void VolumeContainer::om_vol_cmd_resp(VolumeInfo::pointer volinfo,
                                      fpi::FDSPMsgTypeId cmd_type,
                                      const Error & resp_err,
                                      NodeUuid from_svc)
{
    fds_uint64_t vol_uuid = volinfo->vol_get_properties()->volUUID.get();
    VolumeInfo::pointer vol = VolumeInfo::vol_cast_ptr(rs_get_resource(vol_uuid));
    OM_Module *om = OM_Module::om_singleton();
    OM_DMTMod *dmtMod = om->om_dmt_mod();
    if (vol == NULL) {
        // we probably deleted a volume, just print a warning
        LOGWARN << "Received volume notify response for volume "
                << volinfo->vol_get_properties()->name << " but volume does not exist anymore; "
                << "it was probably just deleted";
        return;
    }


    //  The following is ugly
    om_vol_notify_t type = om_notify_vol_max;

    if (from_svc.uuid_get_type() == fpi::FDSP_ACCESS_MGR) {
        switch (cmd_type) {
            case fpi::CtrlNotifyVolModTypeId:
                type = om_notify_vol_mod; break;
            case fpi::CtrlNotifyVolRemoveTypeId:
                type = om_notify_vol_detach; break;
            default: break;
        }
    }

    if (from_svc.uuid_get_type() != fpi::FDSP_ACCESS_MGR) {
        switch (cmd_type) {
            case fpi::CtrlNotifyVolAddTypeId:
                type = om_notify_vol_add; break;
            case fpi::CtrlNotifyVolModTypeId:
                type = om_notify_vol_mod; break;
            case fpi::CtrlNotifyVolRemoveTypeId:
                if (vol->isCheckDelete()) {
                    type = om_notify_vol_rm_chk;
                } else {
                    type = om_notify_vol_rm;
                }
                break;
            default: break;
        }
    }

    LOGDEBUG << "rcvd cmd resp [cmd:" << cmd_type <<"]"
             <<"  [type:" << type << "]";

    switch (type) {
        case om_notify_vol_add:
            // TODO(gurpreet) In addition to checking for timeout error
            // code we should check for node_state as well, to make sure
            // that the timeout error seen here is indeed because of the
            // service being down.
            if (resp_err.ok() ||
                resp_err == ERR_SVC_REQUEST_TIMEOUT ||
                resp_err == ERR_SVC_REQUEST_INVOCATION ||
                resp_err == ERR_VOL_DUPLICATE ||
                resp_err == ERR_DUPLICATE ) {
                vol->vol_event(VolCrtOkEvt(true, vol.get()));
                dmtMod->dmt_deploy_event(DmtVolAckEvt(from_svc));
            } else {
                // TODO(anna) send response to volume create here with error
                LOGERROR << "Received volume create response with error ("
                         << resp_err.GetErrstr() << ")"
                     << ", will revert volume create for " << volinfo->vol_get_properties()->name;

                // start remove volume process (here we don't need to check
                // with DMs if there are any objects, since volume was never
                // attached to any AMs at this point)
                vol->vol_event(VolDeleteEvt(vol_uuid, vol.get()));
            }
            break;
        case om_notify_vol_mod:
        case om_notify_vol_detach:
        case om_notify_vol_rm:
            vol->vol_event(VolOpRespEvt(vol_uuid, vol.get(), type, resp_err));
            break;
        case om_notify_vol_rm_chk:
            vol->vol_event(DelRespEvt(vol.get(), resp_err));
            break;
        default:
            break;
      }
}


bool VolumeContainer::addVolume(const VolumeDesc& volumeDesc) {
    VolPolicyMgr        *v_pol = OrchMgr::om_policy_mgr();
    OM_NodeContainer    *local = OM_NodeDomainMod::om_loc_domain_ctrl();
    FdsAdminCtrl        *admin = local->om_get_admin_ctrl();
    VolumeInfo::pointer  vol;
    ResourceUUID         uuid = volumeDesc.volUUID.get();
    Error  err(ERR_OK);

    vol = VolumeInfo::vol_cast_ptr(rs_get_resource(uuid));
    if (vol != NULL) {
        LOGWARN << "volume already exists : " << volumeDesc.name <<":" << uuid;
        return false;
    }

    // check for a volume with the same name
    if (ERR_OK == getVolumeStatus(volumeDesc.name)) {
        vol = get_volume(volumeDesc.name);
        LOGWARN << "an active volume already exists with the same name : "
                << volumeDesc.name << ":" << vol->rs_get_uuid();
            return false;
    }

    vol = VolumeInfo::vol_cast_ptr(rs_alloc_new(uuid));
    vol->setDescription(volumeDesc);
    err = admin->volAdminControl(vol->vol_get_properties());
    if (!err.ok()) {
        // TODO(Vy): delete the volume here.
        LOGERROR << "Unable to create volume " << " error: " << err.GetErrstr();
        rs_free_resource(vol);
        return false;
    }

    // register before b-casting vol crt, in case we start recevings acks
    // before vol_event for create vol returns
    if (!rs_register(vol)) {
        LOGERROR << "unable to register vol as resource : " << volumeDesc.name;
        rs_free_resource(vol);
        return false;
    }


    // store it in config db..
    if (!gl_orch_mgr->getConfigDB()->addVolume(volumeDesc)) {
        LOGWARN << "unable to store volume info in to config db "
                << "[" << volumeDesc.name << ":" <<volumeDesc.volUUID << "]";
    }

    if (vol->isStateActive()) {
        // if it was previously active then it means that 
        // this is an old volume
        vol->setState(fpi::ResourceState::Created);
    } else {
        vol->setState(fpi::ResourceState::Loading);
    }

    // this event will broadcast vol create msg to other nodes and wait for acks
    vol->vol_event(VolCreateEvt(vol.get()));

    // in case there was no one to notify, check if we can proceed to
    // active state right away (otherwise guard will stop us)
    vol->vol_event(VolCrtOkEvt(false, vol.get()));

    return true;
}

Error VolumeContainer::addSnapshot(const fpi::Snapshot& snapshot) {
    Error err(ERR_OK);
    OM_NodeContainer *local  = OM_NodeDomainMod::om_loc_domain_ctrl();
    OM_Module *om            = OM_Module::om_singleton();
    FdsAdminCtrl *admin      = local->om_get_admin_ctrl();

    VolumeInfo::pointer  vol, parentVol;

    vol = VolumeInfo::vol_cast_ptr(rs_get_resource(snapshot.snapshotId));
    if (vol != NULL) {
        LOGWARN << "Trying to add a snapshot with conflicting id:" << snapshot.snapshotId
                << " name:" << snapshot.snapshotName;
        return Error(ERR_DUPLICATE);
    }

    parentVol = VolumeInfo::vol_cast_ptr(rs_get_resource(snapshot.volumeId));
    if (parentVol == NULL) {
        LOGWARN << "Trying to create a snapshot for non existent volume:" << snapshot.volumeId
                << " name:" << snapshot.snapshotName;
        return Error(ERR_NOT_FOUND);
    }

    // check for a volume with the same name
    if (ERR_OK == getVolumeStatus(snapshot.snapshotName)) {
        vol = get_volume(snapshot.snapshotName);
        LOGWARN << "an active volume already exists with the same name : "
                << snapshot.snapshotName << ":" << vol->rs_get_uuid();
            return ERR_DUPLICATE;
    }


    vol = VolumeInfo::vol_cast_ptr(rs_alloc_new(snapshot.snapshotId));

    parentVol->initSnapshotVolInfo(vol, snapshot);

    err = admin->volAdminControl(vol->vol_get_properties());
    if (!err.ok()) {
        // TODO(Vy): delete the volume here.
        LOGERROR << "Unable to add snapshot " << snapshot.snapshotName
                 << " error: " << err.GetErrstr();
        rs_free_resource(vol);
        return err;
    }

    vol->setState(fpi::ResourceState::Loading);

    // store it in the configDB
    gl_orch_mgr->getConfigDB()->createSnapshot(const_cast<fpi::Snapshot&>(snapshot));

    if (!rs_register(vol)) {
        LOGERROR << "unable to register snapshot as resource : " << snapshot.snapshotName;
        rs_free_resource(vol);
        return ERR_DUPLICATE;
    }

    vol->vol_event(VolCreateEvt(vol.get()));

    // in case there was no one to notify, check if we can proceed to
    // active state right away (otherwise guard will stop us)
    vol->vol_event(VolCrtOkEvt(false, vol.get()));

    return err;
}

bool VolumeContainer::createSystemVolume(int32_t tenantId) {
    std::string name;
    if (tenantId >= 0) {
        name = util::strformat("SYSTEM_VOLUME_%d",tenantId);
    } else {
        name = "SYSTEM_VOLUME";
        tenantId = 0;
    }
    fds_bool_t fReturn = true;
    if (!gl_orch_mgr->getConfigDB()->volumeExists(name)) {
        VolumeDesc volume(name, fds_volid_t(1));
        
        volume.volUUID = gl_orch_mgr->getConfigDB()->getNewVolumeId();
        volume.createTime = util::getTimeStampMillis();
        volume.replicaCnt = 4;
        volume.maxObjSizeInBytes = 2 * MB;
        volume.contCommitlogRetention = 0;
        volume.tennantId = tenantId;
        volume.capacity  = 1*GB;
        fReturn = addVolume(volume);
        if (!fReturn) {
            LOGERROR << "unable to add system volume "
                     << "[" << volume.volUUID << ":" << volume.name << "]";
        }
    } else {
        LOGDEBUG << "system volume already exists : " << name;
    }
    return fReturn;
}

}  // namespace fds
