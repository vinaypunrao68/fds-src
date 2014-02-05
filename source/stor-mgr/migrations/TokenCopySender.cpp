/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <boost/msm/front/state_machine_def.hpp>
#include <boost/msm/front/functor_row.hpp>
#include <boost/msm/front/euml/common.hpp>
// for And_ operator
#include <boost/msm/front/euml/operator.hpp>
#include <TokenCopySender.h>

namespace fds {

using namespace FDS_ProtocolInterface;  // NOLINT

namespace msm = boost::msm;
namespace mpl = boost::mpl;
using namespace msm::front;  // NOLINT
typedef msm::front::none msm_none;

/* State machine */
struct TokenCopySenderFSM_
        : public msm::front::state_machine_def<TokenCopySenderFSM_> {
    // To improve performance --- if no message queue needed
    // message queue not needed if no action will itself generate
    // a new event
    typedef int no_message_queue;

    // To improve performance -- if behaviors will never throw
    // an exception, below shows how to disable exception handling
    typedef int no_exception_thrown;

    template <class Event, class FSM>
    void on_entry(Event const& , FSM&)
    {
        // FDS_PLOG(get_log()) << "TokenCopySender SM";
    }
    template <class Event, class FSM>
    void on_exit(Event const&, FSM&)
    {
        // FDS_PLOG(get_log()) << "TokenCopySender SM";
    }

    /* The list of state machine states */

    struct Init : public msm::front::state<>
    {
        template <class Event, class FSM>
        void on_entry(Event const& , FSM&)
        {
            // FDS_PLOG(get_log()) << "Init State";
        }
        template <class Event, class FSM>
        void on_exit(Event const&, FSM&)
        {
            // FDS_PLOG(get_log()) << "Init State";
        }
    };

    struct Reading : public msm::front::state<>
    {
        template <class Event, class FSM>
        void on_entry(Event const& , FSM&)
        {
            // FDS_PLOG(get_log()) << "Reading State";
        }
        template <class Event, class FSM>
        void on_exit(Event const&, FSM&)
        {
            // FDS_PLOG(get_log()) << "Reading State";
        }
    };

    struct Sending : public msm::front::state<>
    {
        template <class Event, class FSM>
        void on_entry(Event const& , FSM&)
        {
            // FDS_PLOG(get_log()) << "Sending State";
        }
        template <class Event, class FSM>
        void on_exit(Event const&, FSM&)
        {
            // FDS_PLOG(get_log()) << "Sending State";
        }
    };

    /* Events */
    typedef FDS_ProtocolInterface::FDSP_CopyTokenReq DataReadEvt;
    struct DataSentEvt {};

    /* Actions */
    struct read
    {
        template <class EVT, class FSM, class SourceState, class TargetState>
        void operator()(const EVT& evt, FSM& fsm, SourceState&, TargetState&)
        {
        }
    };
    struct send
    {
        template <class EVT, class FSM, class SourceState, class TargetState>
        void operator()(const EVT& evt, FSM& fsm, SourceState&, TargetState&)
        {
        }
    };

    /* Guards */
    struct more_to_read
    {
        template <class EVT, class FSM, class SourceState, class TargetState>
        bool operator()(const EVT& evt, FSM& fsm, SourceState&, TargetState&)
        {
        }
    };

    /* transition table */
    struct transition_table : mpl::vector< // NOLINT
    //    Start       Event             Next            Action                      Guard
    Row < Init,       msm_none,         Reading,        read,                       msm_none    >, // NOLINT
    Row < Reading,    DataReadEvt,      Sending,        send,                       msm_none    >, // NOLINT
    Row < Sending,    DataSentEvt,      Reading,        read,                       more_to_read > // NOLINT
    >
    {
    };

    /* Replaces the default no-transition response */
    template <class FSM, class Event>
    void no_transition(Event const& e, FSM&, int state)
    {
        fds_verify(!"Unexpected event");
    }

    /* the initial state of the player SM. Must be defined */
    typedef Init initial_state;
};  /* struct TokenCopySenderFSM_ */

TokenCopySender::TokenCopySender(fds_threadpoolPtr threadpool, fds_logPtr log)
    : FdsRequestQueueActor(threadpool),
      log_(log)
{
}

TokenCopySender::~TokenCopySender() {
}

Error TokenCopySender::handle_actor_request(FdsActorRequestPtr req)
{
    Error err = ERR_OK;
    FDSP_CopyTokenReqPtr copy_tok_req;

    switch (req->type) {
    case FAR_MIG_COPY_TOKEN:
        copy_tok_req = boost::static_pointer_cast<FDSP_CopyTokenReq>(req->payload);
        sm_->process_event(*copy_tok_req);
        break;
    case FAR_OBJSTOR_TOKEN_OBJECTS_READ:
        // TODO(rao): Implement this
    default:
        err = ERR_FAR_INVALID_REQUEST;
        break;
    }
    return err;
}
} /* namespace fds */
