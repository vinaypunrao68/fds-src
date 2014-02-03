/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#ifndef SOURCE_UTIL_ACTOR_REQUEST_H_
#define SOURCE_UTIL_ACTOR_REQUEST_H_

#include <queue>
#include <boost/shared_ptr.hpp>

namespace fds {

/* Types.  These types are divided into ranges.  DO NOT change the order */
enum FdsActorRequestType {
    /* Migration request range [1000-2000) */
    FAR_COPY_TOKEN = 1000,
    FAR_COPY_TOKEN_COMPLETE,

    /* Last request */
    FAR_MAX
};

/* Forward declarations */
class FdsActor;
class FdsRequestQueueActor;

class FdsActorRequest {
public:
    FdsActorRequest() {
        type = FAR_MAX;
        owner_ = nullptr;
        prev_owner_ = nullptr;
    }

public:
    /* Request payload */
    boost::shared_ptr<void> payload;
    /* Request type */
    FdsActorRequestType type;

protected:
    /* Current owner of the request */
    FdsActor *owner_;
    /* Previous owner */
    FdsActor *prev_owner_;

    friend class FdsRequestQueueActor;
};

typedef boost::shared_ptr<FdsActorRequest> FdsActorRequestPtr;
typedef std::queue<FdsActorRequestPtr> FdsActorRequestQueue;

}  // namespace fds

#endif  // SOURCE_UTIL_ACTOR_REQUEST_H_
