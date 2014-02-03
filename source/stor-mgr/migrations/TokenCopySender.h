/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#ifndef TOKENCOPYSENDER_H_
#define TOKENCOPYSENDER_H_

#include <string>
#include <state_machine.hpp>

#include <fds_assert.h>
#include <concurrency/fds_actor.h>
#include <fdsp/FDSP_types.h>

namespace fds {

class TokenCopySender : public FdsRequestQueueActor {
public:
    TokenCopySender(fds_threadpoolPtr threadpool);
    virtual ~TokenCopySender();

    /* Overrides from FdsRequestQueueActor */
    virtual Error handle_actor_request(FdsActorRequestPtr req) override;

    /* State machine */
    struct StateMachine_ : public msm::front::state_machine_def<StateMachine_> {
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
            //FDS_PLOG(get_log()) << "TokenCopySender SM";
        }
        template <class Event, class FSM>
        void on_exit(Event const&, FSM&)
        {
            //FDS_PLOG(get_log()) << "TokenCopySender SM";
        }

        /* The list of state machine states */

        struct Init : public msm::front::state<>
        {
            template <class Event, class FSM>
            void on_entry(Event const& , FSM&)
            {
                //FDS_PLOG(get_log()) << "Init State";
            }
            template <class Event, class FSM>
            void on_exit(Event const&, FSM&)
            {
                //FDS_PLOG(get_log()) << "Init State";
            }
        };

        struct Reading : public msm::front::state<>
        {
            template <class Event, class FSM>
            void on_entry(Event const& , FSM&)
            {
                //FDS_PLOG(get_log()) << "Reading State";
            }
            template <class Event, class FSM>
            void on_exit(Event const&, FSM&)
            {
                //FDS_PLOG(get_log()) << "Reading State";
            }
        };

        struct Sending : public msm::front::state<>
        {
            template <class Event, class FSM>
            void on_entry(Event const& , FSM&)
            {
                //FDS_PLOG(get_log()) << "Sending State";
            }
            template <class Event, class FSM>
            void on_exit(Event const&, FSM&)
            {
                //FDS_PLOG(get_log()) << "Sending State";
            }
        };

        /* Events */
        typedef FDS_ProtocolInterface::FDSP_CopyTokenReq DataReadEvt;
        struct DataSentEvt {};

        /* Actions */
        template <class EventT>
        void read(const EventT &e) {
            //FDS_PLOG(get_log()) << "Action";
        }

        template <class EventT>
        void send(const EventT &e) {
            //FDS_PLOG(get_log()) << "Action";
        }

        /* Guards */
        template <class EventT>
        bool more_to_read(const EventT &e) {
            //FDS_PLOG(get_log()) << "Guard";
            return false;
        }

        /* transition table */
        struct transition_table : mpl::vector<
        //              Start       Event               Next            Action                      Guard
        a_row <         Init,       front::none,        Reading,        &StateMachine_::read                                 >, // NOLINT
        a_row <         Reading,    DataReadEvt,        Sending,        &StateMachine_::send                                 >, // NOLINT
        row <           Sending,    DataSentEvt,        Reading,        &StateMachine_::read,       &StateMachine_::more_to_read > // NOLINT
        >
        {
        };

        /* Replaces the default no-transition response */
        template <class FSM,class Event>
        void no_transition(Event const& e, FSM&,int state)
        {
            fds_verify(!"Unexpected event");
        }

        /* the initial state of the player SM. Must be defined */
        typedef Init initial_state;

    };  /* struct StateMachine_ */

    /* back-end */
    typedef msm::back::state_machine<StateMachine_> StateMachine;


private:
    StateMachine sm_;
};

} /* namespace fds */

#endif /* TOKENCOPYSENDER_H_ */
