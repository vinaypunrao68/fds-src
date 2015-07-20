/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

#ifndef SOURCE_INCLUDE_CONCURRENCY_LEADERFOLLOWER_H_
#define SOURCE_INCLUDE_CONCURRENCY_LEADERFOLLOWER_H_

#include <condition_variable>
#include <mutex>

namespace fds
{

/***
 * A LeaderFollower is a design for Asynchronous I/O demultiplexing/dispatch
 *
 * The main idea is that at any one time there is:
 *  - A single "leader" thread.
 *  - 0 >= "follower" threads.
 *  - 0 >= "processing" threads.
 *
 *  The leader thread is the one waiting for an I/O event (e.g. on a set of
 *  sockets under an epoll loop). When an I/O event is received, the leader
 *  promotes a new leader from the set of followers, then processes the request
 *  while becoming a "processing" thread. The new leader is now waiting for the
 *  next event.
 *
 *  When a thread finishes processing an I/O event, it either becomes a
 *  follower if there is already a leader or is immediately promoted to the
 *  leader him/her self. 
 */
struct LeaderFollower {
    LeaderFollower(size_t const num_followers, bool const reentrant_lead)
        : followers_to_create_(num_followers),
          reentrant_lead_(reentrant_lead)
    {}
    LeaderFollower(LeaderFollower const& rhs) = delete;
    LeaderFollower& operator=(LeaderFollower const& rhs) = delete;
    virtual ~LeaderFollower() = default;

    /** Complete work, try and follow */
    void follow();

    void process()
    {
        {
            std::unique_lock<std::mutex> l(leader_lock_);
            being_led_ = false;
        }
        leader_notify_.notify_one();
    }

 protected:

    virtual void lead() = 0;

 private:
    size_t                  followers_to_create_      {0};
    bool                    being_led_          {false};
    bool                    reentrant_lead_     {true};

    std::mutex              leader_lock_;
    std::condition_variable leader_notify_;
};

}  // namespace fds

#endif  // SOURCE_INCLUDE_CONCURRENCY_LEADERFOLLOWER_H_
