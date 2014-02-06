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
    FAR_MIGSVC_COPY_TOKEN = 1000,
    FAR_MIGSVC_MIGRATION_COMPLETE,
    FAR_MIG_TCS_DATA_READ_DONE,
    /* Migration RPC */
    FAR_MIG_COPY_TOKEN_RPC,
    FAR_MIG_COPY_TOKEN_COMPLETE_RPC,
    FAR_MIG_PUSH_TOKEN_OBJECTS_RPC,

    /* Object store request range [2000-3000)*/
    FAR_OBJSTOR_TOKEN_OBJECTS_WRITTEN = 2000,
    FAR_OBJSTOR_TOKEN_OBJECTS_READ,
    /* Last request */
    FAR_MAX
};

/* Forward declarations */
class FdsActor;
class FdsRequestQueueActor;

class FdsActorRequest {
public:
    FdsActorRequest()
    : FdsActorRequest(FAR_MAX, nullptr)
    {
    }

    FdsActorRequest(FdsActorRequestType type, boost::shared_ptr<void> payload) {
        this->type = type;
        owner_ = nullptr;
        prev_owner_ = nullptr;
        this->payload = payload;
    }

    template <class PayloadT>
    boost::shared_ptr<PayloadT> get_payload() {
        return boost::static_pointer_cast<PayloadT>(payload);
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
