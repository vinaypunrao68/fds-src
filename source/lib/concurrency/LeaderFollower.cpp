/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

#include "concurrency/LeaderFollower.h"

#include <thread>

namespace fds
{

void LeaderFollower::follow()
{
    thread_local bool has_led {false};
    {
        std::unique_lock<std::mutex> l(leader_lock_);
        if (being_led_) {
            // Wait till it's our turn to lead
            leader_notify_.wait(l, [this] {return !this->being_led_;});
        } else {
            // Hmm, no one was waiting, if we still have followers to create
            // let's create one now
            if (0 < num_followers_) {
                auto t = std::thread(&LeaderFollower::follow, this);
                t.detach();
                --num_followers_;
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
