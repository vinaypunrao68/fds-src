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

class TokenCopyReceiver : public FdsRequestQueueActor {
public:
    TokenCopyReceiver(fds_threadpoolPtr threadpool);
    virtual ~TokenCopyReceiver();

    /* Overrides from FdsRequestQueueActor */
    virtual Error handle_actor_request(FdsActorRequestPtr req) override;

    /* State machine */
    struct StateMachine_ : public msm::front::state_machine_def<StateMachine_> {

        // To improve performance -- if behaviors will never throw
        // an exception, below shows how to disable exception handling
        typedef int no_exception_thrown;

        template <class Event, class FSM>
        void on_entry(Event const& , FSM&)
        {
            //FDS_PLOG(get_log()) << "TokenCopyReceiver SM";
        }
        template <class Event, class FSM>
        void on_exit(Event const&, FSM&)
        {
            //FDS_PLOG(get_log()) << "TokenCopyReceiver SM";
        }

        /* Events */
        typedef FDS_ProtocolInterface::PushTokenObjectsReq DataReceivedEvt;
        struct DataWrittenEvt {};

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

        struct Receiving : public msm::front::state<>
        {
            template <class Event, class FSM>
            void on_entry(Event const& , FSM&)
            {
                //FDS_PLOG(get_log()) << "Receiving State";
            }
            template <class Event, class FSM>
            void on_exit(Event const&, FSM&)
            {
                //FDS_PLOG(get_log()) << "Receiving State";
            }
        };

        struct Writing : public msm::front::state<>
        {
            typedef mpl::vector<DataReceivedEvt> deferred_events;

            template <class Event, class FSM>
            void on_entry(Event const& , FSM&)
            {
                //FDS_PLOG(get_log()) << "Writing State";
            }
            template <class Event, class FSM>
            void on_exit(Event const&, FSM&)
            {
                //FDS_PLOG(get_log()) << "Writing State";
            }
        };



        /* Actions */
        template <class EventT>
        void issue_copy_token(const EventT &e) {
            //FDS_PLOG(get_log()) << "Action";
            // TODO: Send a copy token to endpoint
        }

        template <class EventT>
        void write(const EventT &e) {
            //FDS_PLOG(get_log()) << "Action";
            // TODO: Do a write a request to ObjectStore manager
        }

        /* Guards */
        template <class EventT>
        bool more_to_receive(const EventT &e) {
            //FDS_PLOG(get_log()) << "Guard";
            // TODO: Check if there is more receive
            return false;
        }

        /* transition table */
        struct transition_table : mpl::vector<
        //              Start       Event               Next            Action                              Guard
        a_row <         Init,       front::none,        Receiving,      &StateMachine_::issue_copy_token    /*none*/          >, // NOLINT
        a_row <         Receiving,  DataReceivedEvt,    Writing,        &StateMachine_::write               /*none*/          >, // NOLINT
        g_row <         Writing,    DataWrittenEvt,     Receiving,      /*none*/                            &StateMachine_::more_to_receive > // NOLINT
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
