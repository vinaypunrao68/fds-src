/*
 * Copyright 2013 Formation Data Systems, Inc.
 *
 * Example of simple state machine of on/off switch
 *
 *  States: ON, OFF
 *  Transitions: 
 *    OFF -> turn_on -> ON
 *    ON -> turn-off -> OFF
 * 
 *  OFF ---- turn_on ------> ON
 *  ^                        |
 *  |                        |
 *   ------- turn_off --------
 * 
 * More complex but still simple example:
 * http://www.boost.org/doc/libs/1_49_0/libs/msm/doc/HTML/examples/SimpleTutorial.cpp
 */

#include <string>
#include <vector>
#include <fds_process.h>
#include <state_machine.hpp>

// events
struct event_turn_on {};
struct event_turn_off {};

struct switch_ : public msm::front::state_machine_def<switch_>
{
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
        std::cout << "entering: switch_" << std::endl;
    }
    template <class Event, class FSM>
    void on_exit(Event const&, FSM&)
    {
        std::cout << "leaving: switch_" << std::endl;
    }

    /* The list of state machine states */
    struct StateOff: public msm::front::state<>
    {
        template <class Event, class FSM>
        void on_entry(Event const&, FSM&)
        {
            std::cout << "entering: StateOff" << std::endl;
        }
        template <class Event, class FSM>
        void on_exit(Event const&, FSM&)
        {
            std::cout << "leaving: StateOff" << std::endl;
        }
    };
    struct StateOn: public msm::front::state<>
    {
        template <class Event, class FSM>
        void on_entry(Event const&, FSM&)
        {
            std::cout << "entering: StateOn" << std::endl;
        }
        template <class Event, class FSM>
        void on_exit(Event const&, FSM&)
        {
            std::cout << "leaving: StateOn" << std::endl;
        }
    };

    /* define the initial state */
    typedef StateOff initial_state;

    /* transition actions */
    void turn_on(event_turn_on const &) {
        std::cout << "switch::turn_on" << std::endl;
    }
    void turn_off(event_turn_off const &) {
        std::cout << "switch::turn_off" << std::endl;
    }

    /* transition table */
    struct transition_table : mpl::vector<
        //      Start      Event           Next        Action
        a_row < StateOff,  event_turn_on,  StateOn,  &switch_::turn_on  >,
        a_row < StateOn,   event_turn_off, StateOff, &switch_::turn_off >
        >
    {
    };
};


int main(int argc, char *argv[]) {
    fds::Module *smVec[] = {
        nullptr
    };

    msm::back::state_machine<switch_> s;
    s.start();
    s.process_event(event_turn_on());
    s.process_event(event_turn_off());
    s.stop();

    return 0;
}
