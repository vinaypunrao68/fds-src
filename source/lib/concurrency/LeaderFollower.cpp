/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

#include "concurrency/LeaderFollower.h"

#include <thread>

namespace fds
{

void LeaderFollower::follow()
{
    // This indicates whether we have already entered the event loop or not
    thread_local bool has_led {false};
    {
        std::unique_lock<std::mutex> l(leader_lock_);
        if (being_led_) {
            // Wait till it's our turn to lead
            leader_notify_.wait(l, [this] {return !this->being_led_;});
        } else {
            // Hmm, no one was waiting, if we still have followers to create
            // let's create one now
            if (0 < followers_to_create_) {
                auto t = std::thread(&LeaderFollower::follow, this);
                t.detach();
                --followers_to_create_;
            }
        }
        being_led_ = true;
    }
    if (reentrant_lead_ || !has_led) {
        // To support libraries that have their own event loop
        has_led = true;
        lead();
    }
}


}  // namespace fds
